#include"gis_pch.hpp"
#include"LasReader.hpp"

namespace lapis {
	LasReader::LasReader(const std::string& file) : _filters(), CurrentLasPoint(file),
	_dtms(), _minht(std::numeric_limits<coord_t>::lowest()),_maxht(std::numeric_limits<coord_t>::max()) {}

	LidarPointVector LasReader::getPoints(size_t n)
	{
		if (_currentPoint >= _nPoints) {
			return LidarPointVector();
		}
		size_t maxCount = std::min(n, _nPoints - _currentPoint);
		LidarPointVector points{ _crs };
		points.reserve(maxCount);

		while(_currentPoint < _nPoints && points.size() < n) {
			advance();
			bool filtered = false;

			for (auto& filter : _filters) {
				if (filter->isFiltered(*this)) {
					filtered = true;
					break;
				}
			}

			//These lines should do nothing if the las file is well-constructed.
			//However, if it isn't well constructed, a point off in the middle of nowhere could cause a crash if you don't check for it
			if (x() < xmin()) {
				continue;
			}
			if (x() > xmax()) {
				continue;
			}
			if (y() < ymin()) {
				continue;
			}
			if (y() > ymax()) {
				continue;
			}

			if (!filtered) {
				points.emplace_back(x(), y(), z(), intensity(), returnNumber());
			}
		}
		if (!_dtms.size()) {
			return points;
		}

		std::vector<bool> normalized = std::vector<bool>(points.size());
		size_t normcount = 0;
		for (auto& r : _dtms) {
			normcount += _normalize(r, points, normalized);
			if (normcount >= points.size()) {
				break;
			}
		}
		_tmp.clear();
		_tmp.shrink_to_fit();

		LidarPointVector out{ _crs };
		out.reserve(points.size());
		for (size_t i = 0; i < points.size(); ++i) {
			if (normalized[i] && points[i].z >= _minht && points[i].z <= _maxht) {
				out.emplace_back(points[i]);
			}
		}
		return out;
	}

	void LasReader::setHeightLimits(coord_t minht, coord_t maxht, const Unit& units)
	{
		_minht = convertUnits(minht,units, crs().getZUnits());
		_maxht = convertUnits(maxht, units, crs().getZUnits());
	}

	void LasReader::addDEM(const std::string& file, const CoordRef& crsOverride, const Unit& unitOverride) {

		Extent rastExt = Alignment(file); //this kind of goofy line forces the check that file is a raster file and not some other object
		bool hasoverlap = false;
		QuadExtent thisProjExt{ *this,rastExt.crs() };
		if (crs().isConsistentHoriz(rastExt.crs())) {
			hasoverlap = overlaps(rastExt);
		}
		else {
			hasoverlap = thisProjExt.overlaps(rastExt);
		}
		
		if (!hasoverlap) {
			return;
		}

		auto r = Raster<coord_t>(file, thisProjExt.outerExtent(), SnapType::out);
		_dtms.push_back(r);
		//_dtms.emplace_back(file, thisProjExt.outerExtent(), SnapType::out);
		if (!crsOverride.isEmpty()) {
			_dtms[_dtms.size() - 1].defineCRS(crsOverride);
		}
		if (!unitOverride.isUnknown()) {
			_dtms[_dtms.size() - 1].setZUnits(unitOverride);
		}
		std::sort(_dtms.begin(), _dtms.end(), [](const auto& lhs, const auto& rhs)->bool {return lhs.xres() < rhs.xres(); });
	}

	Raster<coord_t> LasReader::unifiedDEM(const Alignment& a)
	{
		Extent thisExt = QuadExtent(*this, a.crs()).outerExtent();
		Alignment thisAlign = extend(a, thisExt, SnapType::out);
		thisAlign = crop(a, thisExt, SnapType::out);
		Raster<coord_t> out{ thisAlign };

		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			coord_t x = out.xFromCell(cell);
			coord_t y = out.yFromCell(cell);
			for (auto& r : _dtms) {
				auto v = r.extract(x, y, ExtractMethod::bilinear);
				if (v.has_value()) {
					out[cell].has_value() = true;
					out[cell].value() = v.value();
					break;
				}
			}
		}

		return out;
	}

	size_t LasReader::_normalize(Raster<coord_t>& r, LidarPointVector& points, std::vector<bool>& alreadyNormalized)
	{

		LidarPointVector& usepoints = _getTransformedPoints(points, r.crs());

		size_t normcount = 0;

		for (size_t idx = 0; idx < points.size(); ++idx) {
			if (alreadyNormalized[idx]) {
				continue;
			}
			auto v = r.extract(usepoints[idx].x, usepoints[idx].y, ExtractMethod::bilinear);
			if (v.has_value()) {
				coord_t z = usepoints[idx].z - v.value();
				points.setZ(z, idx, r.crs().getZUnits());
				++normcount;
				alreadyNormalized[idx] = true;
			}
		}

		return normcount;
	}

	LidarPointVector& LasReader::_getTransformedPoints(LidarPointVector& points, const CoordRef& crs)
	{
		if (points.crs.isConsistent(crs)) {
			return points;
		}
		if (_tmp.size() && _tmp.crs.isConsistent(crs)) {
			return _tmp;
		}
		_tmp = points;
		_tmp.transform(crs);
		return _tmp;
	}

}