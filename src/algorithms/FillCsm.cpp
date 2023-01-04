#include"algo_pch.hpp"
#include"FillCsm.hpp"

namespace lapis {
	FillCsm::FillCsm(int neighborsNeeded, coord_t lookDistCsmXYUnits)
		: _maxMisses(8 - neighborsNeeded), _cardinalFillDist((rowcol_t)lookDistCsmXYUnits), _diagonalFillDist((rowcol_t)(lookDistCsmXYUnits / std::sqrt(2)))
	{
	}
	Raster<csm_t> FillCsm::postProcess(const Raster<csm_t>& csm)
	{
		Raster<csm_t> out{ (Alignment)csm };
		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			if (!csm[cell].has_value()) {
				_fillSingleValue(csm, out, cell);
			}
			else {
				out[cell].has_value() = true;
				out[cell].value() = csm[cell].value();
			}
		}

		return out;
	}
	void FillCsm::_fillSingleValue(const Raster<csm_t>& original, Raster<csm_t>& output, cell_t cell)
	{
		//the algorithm here to distinguish legitimately absent data from holes is:
		//in all 8 cardinal directions, look for data up to a certain distance away
		//if you find data or the edge of the raster in at least neighborsneeded directions,
		//then assign the value to be an inverse-distance-weighted mean of the closest value found in each direction

		struct Direction {
			rowcol_t rowDir, colDir;
		};
		static std::vector<Direction> directions = {
			{0,1},{0,-1},{1,0},{-1,0},
			{1,1},{1,-1},{-1,1},{-1,-1}
		};
		static size_t firstDiagonal = 4;
		static coord_t sqrtTwo = std::sqrt(2.);

		struct ValueDist {
			csm_t value;
			csm_t dist;
		};
		std::vector<ValueDist> foundValues;
		foundValues.reserve(8);

		int missesSoFar = 0;

		rowcol_t row = original.rowFromCell(cell);
		rowcol_t col = original.colFromCell(cell);

		for (size_t i = 0; i < directions.size(); ++i) {

			bool isDiagonal = i >= firstDiagonal;
			rowcol_t maxPixelDist = isDiagonal ? _diagonalFillDist : _cardinalFillDist;
			csm_t distMultiplier = isDiagonal ? sqrtTwo : 1.;

			bool missed = true;

			for (rowcol_t d = 1; d <= maxPixelDist; ++d) {
				rowcol_t thisRow = row + d * directions[i].rowDir;
				rowcol_t thisCol = col + d * directions[i].colDir;
				if (thisRow < 0 || thisCol < 0 || thisRow >= original.nrow() || thisCol >= original.ncol()) {
					//in this case, we found the edge of the raster
					//we don't have a value to contribute to the eventual mean, but we don't count it as a miss either
					missed = false;
					break;
				}
				auto v = original.atRCUnsafe(thisRow, thisCol);
				if (!v.has_value()) {
					continue;
				}
				missed = false;
				foundValues.emplace_back(v.value(), distMultiplier * d);
				break;
			}
			if (missed) {
				missesSoFar++;
				if (missesSoFar > _maxMisses) {
					return;
				}
			}
		}

		csm_t numerator = 0;
		csm_t denominator = 0;
		for (const ValueDist v : foundValues) {
			csm_t inverseDist = 1 / v.dist;
			numerator += inverseDist * v.value;
			denominator += inverseDist;
		}
		auto v = output[cell];
		v.has_value() = true;
		v.value() = numerator / denominator;
	}
}