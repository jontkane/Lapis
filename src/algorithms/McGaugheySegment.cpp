#include"McGaugheySegment.hpp"
#include"..\utils\MetadataPdf.hpp"

namespace lapis {
	McGaugheySegment::McGaugheySegment(int nVertices, csm_t slopeChangeMultiplier, csm_t heightCutoffMultiplier, coord_t maxDistMultiplier, McGaugheySmoothType smoothType)
        : _nVertices(nVertices), 
		_slopeChangeMultiplier(slopeChangeMultiplier),
		_heightCutoffMultiplier(heightCutoffMultiplier),
		_maxDistMultiplier(maxDistMultiplier),
		_smoothType(smoothType)
	{
	}
	SegmentResults McGaugheySegment::segment(const Raster<csm_t>& bufferedCsm, const std::vector<IDedTao>& taos, const Extent& unbufferedExtent)
    {
        VectorDataset<MultiPolygon> polygons{ bufferedCsm.crs() };
        polygons.addNumericField<taoid_t>("ID");
        for (IDedTao tao : taos) {
            coord_t x = bufferedCsm.xFromCellUnsafe(tao.location);
            coord_t y = bufferedCsm.yFromCellUnsafe(tao.location);
            if (!unbufferedExtent.contains(x, y)) {
                continue;
            }
            try {
                Polygon poly = _processOneTao(x, y, bufferedCsm);
                MultiPolygon mp;
                mp.addPolygon(poly);
                polygons.addGeometry(mp);
                polygons.back().setNumericField<taoid_t>("ID", tao.id);
            }
            catch (...) {} //the possible exceptions shouldn't ever occur, but if they do, we just skip this tao

        }

		SegmentResults results;
        results.vector = std::move(polygons);
		return results;
    }
    const std::string& McGaugheySegment::name() const
    {
        static const std::string name = "McGaughey";
        return name;
    }
	void McGaugheySegment::describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter)
	{
        pdf.writeSubsectionTitle("McGaughey Segmentation Algorithm");

		std::stringstream text;
		text << "The McGaughey segmentation algorithm is from FUSION; specifically, the TreeSeg program. "
			"It generates polygons surrounding each TAO location, attempting to outline the shape of the tree canopy. "
			"It does this by casting" << _nVertices << " rays out from the TAO location, at evenly spaced angles. "
			"Each ray continues on until it meets a condition that indicates the edge of the tree's canopy, at which point that location is recorded as a vertex of the polygon. "
			"The following conditions are used to determine the edge of the canopy:\n"
			"1. The ray has traveled a distance greater than " << _maxDistMultiplier << " times the height of the TAO.\n"
			"2. The ray has encountered a location with a height less than " << _heightCutoffMultiplier << " times the height of the TAO.\n"
			"3. The ray has encountered a local minimum in height.\n"
			"4. The ray has encountered a steep drop-off in height, defined as a location where the height differential between the current and next location exceeds "
			<< _slopeChangeMultiplier << " times the height differential between the previous and current location.\n"
			"5. The ray encounters a cell with no data (i.e., a location with no lidar returns).\n\n";
		if (_smoothType == McGaugheySmoothType::fusion) {
			text << "The polygon is then smoothed the same way as in FUSION, which is fairly complicated:\n"
				"First, large outward 'spikes' are identified. These are vertices whose distance from the TAO location exceeds both of their neighbors by at least 25%. "
				"They are smoothed by setting their distance from the TAO location to be the average of the distances of their two neighbors.\n"
				"Then, large inward 'spikes' are identified. These are vertices whose distance from the TAO location is less than both of their neighbors by at least 25%. "
				"They are smoothed the same way.\n"
				"Finally, a final pass is done, which smooths all outward 'spikes', large or small. These are all vertices whose distance exceeds both of their neighbors, by any amount. "
				"They are smoothed the same way.";
		}
		else if (_smoothType == McGaugheySmoothType::simple) {
			text << "The polygon is then smoothed by averaging the distances of each vertex with its two neighbors.";
		}

		pdf.writeTextBlockWithWrap(text.str());
	}
	bool McGaugheySegment::producesRaster() const
	{
		return false;
	}
	bool McGaugheySegment::producesVector() const
	{
		return true;
	}
    Polygon McGaugheySegment::_processOneTao(coord_t x, coord_t y, const Raster<csm_t>& csm) const
    {
        if (!csm.contains(x, y)) {
            throw std::runtime_error("Tao location is outside of the raster bounds.");
        }
        auto v = csm.atXYUnsafe(x, y);
        if (!v.has_value()) {
            throw std::runtime_error("Tao location has no value in the raster.");
        }
        csm_t height = v.value();

        double angleSpacing = 2. * M_PI / _nVertices;
        static double sqrtTwo = std::sqrt(2.);
        int maxPoints = 1023;

        const csm_t heightCutoff = height * _heightCutoffMultiplier;
        const coord_t maxDist = height * _maxDistMultiplier;
        const int nPoints = std::min(maxPoints, (int)(maxDist * sqrtTwo / csm.xres()));
        const coord_t pointSpacing = maxDist / nPoints;

        std::vector<coord_t> vertexDistances;
        vertexDistances.reserve(_nVertices);

		for (int vertexIdx = 0; vertexIdx < _nVertices; ++vertexIdx) {

			const coord_t thisSin = std::sin(angleSpacing * vertexIdx);
			const coord_t thisCos = std::cos(angleSpacing * vertexIdx);

			auto getPoint = [&](int spacingCount) {
				return CoordXY{ x + spacingCount * thisCos * pointSpacing ,y + spacingCount * thisSin * pointSpacing };
				};


			int currentSpacing = 1;
			CoordXY prevPoint{ x,y };
			xtl::xoptional<csm_t> prevHeight = height;
			CoordXY currentPoint = getPoint(currentSpacing);
			xtl::xoptional<csm_t> currentHeight = csm.extract(currentPoint.x, currentPoint.y, ExtractMethod::bilinear);
			CoordXY nextPoint = getPoint(currentSpacing + 1);
			xtl::xoptional<csm_t> nextHeight = csm.extract(nextPoint.x, nextPoint.y, ExtractMethod::bilinear).value();

			bool boundaryFound = false;
			if (!currentHeight.has_value()) {
				boundaryFound = true;
			}

			while (!boundaryFound) {
				//we've hit the maximum allowed distance
				if (currentSpacing >= nPoints) {
					boundaryFound = true;
				}

				//we've hit the edge of the acquistion
				if (!nextHeight.has_value()) {
					boundaryFound = true;
				}

				//we've hit a local minimum
				if (currentHeight.value() < prevHeight.value() && currentHeight.value() <= nextHeight.value()) {
					boundaryFound = true;
				}

				//we've gone too far below the high point
				if (currentHeight.value() < heightCutoff && prevHeight.value() < heightCutoff && nextHeight.value() < heightCutoff) {
					boundaryFound = true;
				}

				//we've hit a steep drop-off
				csm_t prevDiff = prevHeight.value() - currentHeight.value();
				csm_t nextDiff = currentHeight.value() - nextHeight.value();
				if (prevDiff > 0 && nextDiff > 0) {
					if (nextDiff > prevDiff * _slopeChangeMultiplier) {
						boundaryFound = true;
					}
				}

				if (!boundaryFound) {
					currentSpacing++;
					prevPoint = currentPoint;
					prevHeight = currentHeight;
					currentPoint = nextPoint;
					currentHeight = nextHeight;
					nextPoint = getPoint(currentSpacing + 1);
					nextHeight = csm.extract(nextPoint.x, nextPoint.y, ExtractMethod::bilinear).value();
				}

			}
			vertexDistances.push_back(currentSpacing * pointSpacing);
		}

		switch (_smoothType) {
		case McGaugheySmoothType::fusion:
			_fusionSmooth(vertexDistances);
            break;
        case McGaugheySmoothType::simple:
			_simpleSmooth(vertexDistances);
            break;
        case McGaugheySmoothType::none:
		default:
			break;
		}

		std::vector<CoordXY> ring;
		ring.reserve(_nVertices);
		for (size_t vertex = 0; vertex < _nVertices; ++vertex) {
			const coord_t thisSin = std::sin(angleSpacing * vertex);
			const coord_t thisCos = std::cos(angleSpacing * vertex);
			ring.push_back(CoordXY{ x + vertexDistances[vertex] * thisCos ,y + vertexDistances[vertex] * thisSin });
		}

		//get it clockwise
		std::reverse(ring.begin(), ring.end());
		return lapis::Polygon(ring);
    }
	void McGaugheySegment::_fusionSmooth(std::vector<coord_t>& vertexDistances) const
	{
		//this algorithm is copied as closely as possible from the FUSION source code,
		//even where it doesn't really make sense

		//a helper function to deal with the fact that we'll routinely go one past the edge of the vector's size
		auto getVertexDistance = [&](size_t vertex) {
			return vertexDistances[vertex % vertexDistances.size()];
			};

		std::vector<coord_t> smoothedVertexDistances = vertexDistances;

		auto smoothVertex = [&](size_t vertex) {
			smoothedVertexDistances[vertex] = (getVertexDistance(vertex - 1) + getVertexDistance(vertex + 1)) / 2.;
			};

		constexpr coord_t spikeMultiplier = 0.25;

		//technically, the outward and inward spikes could be detected in a single pass by taking the absolute value of the difference
		//but the fact that the smoothing happens in two stages presumably changes the values, so doing this would change the algorithm

		//outward spikes
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				(getVertexDistance(i) - getVertexDistance(i - 1)) >= (getVertexDistance(i) * spikeMultiplier) ||
				(getVertexDistance(i) - getVertexDistance(i + 1)) >= (getVertexDistance(i) * spikeMultiplier)) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;

		//inward spikes
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				(getVertexDistance(i - 1) - getVertexDistance(i)) >= (getVertexDistance(i) * spikeMultiplier) ||
				(getVertexDistance(i + 1) - getVertexDistance(i)) >= (getVertexDistance(i) * spikeMultiplier)) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;

		//smooth all outward spikes...again? without a check this time to ensure they're extra-spikey
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				getVertexDistance(i - 1) < getVertexDistance(i)
				&& getVertexDistance(i + 1) < getVertexDistance(i)
				) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;
	}
	void McGaugheySegment::_simpleSmooth(std::vector<coord_t>& vertexDistances) const
	{
        //assign each vertex distance to a simple three-way average of the previous, current, and next vertex distances

		//a helper function to deal with the fact that we'll routinely go one past the edge of the vector's size
		auto getVertexDistance = [&](size_t vertex) {
			return vertexDistances[vertex % vertexDistances.size()];
			};

        std::vector<coord_t> smoothedVertexDistances = vertexDistances;
		for (size_t i = 0; i < _nVertices; ++i) {
			smoothedVertexDistances[i] = (getVertexDistance(i - 1) + getVertexDistance(i) + getVertexDistance(i + 1)) / 3.;
		}

        vertexDistances = smoothedVertexDistances;
	}
}