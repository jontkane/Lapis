#include"algo_pch.hpp"
#include"HighPoints.hpp"
#include"..\utils\MetadataPdf.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
	HighPoints::HighPoints(coord_t minHtCsmZUnits, coord_t minDistCsmXYUnits)
		: _minHt(minHtCsmZUnits), _minDist(minDistCsmXYUnits)
	{
	}
	std::vector<cell_t> HighPoints::identifyTaos(const Raster<csm_t>& csm)
	{
		std::vector<cell_t> candidates = _taoCandidates(csm);

		if (_minDist <= 0) {
			return candidates;
		}

		struct CellValue {
			csm_t value;
			cell_t cell;
		};
		std::vector<CellValue> sortableValues;
		sortableValues.reserve(candidates.size());

		for (cell_t cell : candidates) {
			auto v = csm[cell];
			sortableValues.emplace_back(v.value(), cell);
		}
		std::sort(sortableValues.begin(), sortableValues.end(), [](auto& a, auto& b) {return a.value > b.value; });

		Raster<bool> masked{ (Alignment)csm }; //this raster's values will be true for pixels which don't qualify as high points because they're too close to another high points

		struct RelativePosition {
			rowcol_t x, y;
		};
		std::vector<RelativePosition> circle;

		rowcol_t maxPixels = (rowcol_t)std::ceil(std::abs(_minDist / csm.xres()));
		circle.reserve((size_t)maxPixels * maxPixels);

		coord_t minDistPixSq = (_minDist / csm.xres()) * (_minDist / csm.xres());
		for (rowcol_t r = -maxPixels; r <= maxPixels; ++r) {
			for (rowcol_t c = -maxPixels; c <= maxPixels; ++c) {
				if (r * r + c * c < minDistPixSq) {
					circle.emplace_back(c, r);
				}
			}
		}

		std::vector<cell_t> out;

		for (CellValue& candidate : sortableValues) {
			if (!masked[candidate.cell].value()) {
				out.push_back(candidate.cell);

				rowcol_t thisRow = masked.rowFromCellUnsafe(candidate.cell);
				rowcol_t thisCol = masked.colFromCellUnsafe(candidate.cell);
				for (RelativePosition& rp : circle) {
					rowcol_t row = thisRow + rp.y;
					rowcol_t col = thisCol + rp.x;
					if (row < 0 || row >= masked.nrow() || col < 0 || col >= masked.ncol()) {
						continue;
					}
					masked.atRCUnsafe(row, col).value() = true;
				}
			}
		}

		return out;
	}
	void HighPoints::describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter)
	{
		pdf.writeSubsectionTitle("High Points TAO Identification Algorithm");
		std::string candidate = _minDist > 0 ? "TAO candidate" : "TAO";
		std::stringstream ss;
		ss << "This run used the High Points algorithm to identify the locations of TAOs. "
			"This is a canopy surface model-based algorithm; it it calculated entirely on the CSM, without reference to the original lidar points. "
			"A pixel in the CSM is considered a " << candidate << " if it is higher than the eight pixels surrounding it. "
			<< "This algorithm works well with tree species, such as conifers, that have a well-defined top. ";
		if (_minDist > 0) {
			ss << "TAOs were required to be at least " <<
				pdf.numberWithUnits(getter->outUnits().convertOneToThis(_minDist,getter->metricAlign()->crs().getXYLinearUnits()),
					getter->unitSingular(), getter->unitPlural())
				<< " apart. If two candidates were closer than that, only the taller one was retained. "
				<< " This can correct for situations where a single tree has multiple TAO candidates.";
		}
		pdf.writeTextBlockWithWrap(ss.str());
	}
	coord_t HighPoints::minHt()
	{
		return _minHt;
	}
	coord_t HighPoints::minDist()
	{
		return _minDist;
	}
	std::vector<cell_t> HighPoints::_taoCandidates(const Raster<csm_t>& csm)
	{
		std::vector<cell_t> candidates;
		candidates.reserve(csm.ncell() / 10);

		for (rowcol_t row = 0; row < csm.nrow(); ++row) {
			for (rowcol_t col = 0; col < csm.ncol(); ++col) {
				auto center = csm.atRCUnsafe(row, col);
				if (!center.has_value()) {
					continue;
				}
				if (center.value() < _minHt) {
					continue;
				}
				bool isHighPoint = true;

				//this setup with priority is to ensure that areas with strictly equal height only get one candidate
				struct rc { rowcol_t row, col; };
				std::vector<rc> higherPriorityNeighbors = { {row + 1,col},{row,col + 1},{row + 1,col + 1},{row - 1,col + 1} };
				std::vector<rc> lowerPriorityNeighbors = { {row - 1,col},{row,col - 1},{row - 1,col - 1},{row + 1,col - 1} };

				for (auto& thisRC : higherPriorityNeighbors) {
					rowcol_t rowNudge = thisRC.row;
					rowcol_t colNudge = thisRC.col;
					if (rowNudge < 0 || colNudge < 0 || rowNudge >= csm.nrow() || colNudge >= csm.ncol()) {
						continue;
					}
					auto compare = csm.atRCUnsafe(rowNudge, colNudge);
					if (!compare.has_value()) {
						continue;
					}
					if (compare.value() > center.value()) {
						isHighPoint = false;
						break;
					}
				}
				if (isHighPoint) {
					for (auto& thisRC : lowerPriorityNeighbors) {
						rowcol_t rowNudge = thisRC.row;
						rowcol_t colNudge = thisRC.col;
						if (rowNudge < 0 || colNudge < 0 || rowNudge >= csm.nrow() || colNudge >= csm.ncol()) {
							continue;
						}
						auto compare = csm.atRCUnsafe(rowNudge, colNudge);
						if (!compare.has_value()) {
							continue;
						}
						if (compare.value() >= center.value()) {
							isHighPoint = false;
							break;
						}
					}
				}

				if (isHighPoint) {
					candidates.push_back(csm.cellFromRowColUnsafe(row, col));
				}
			}
		}
		return candidates;
	}
}