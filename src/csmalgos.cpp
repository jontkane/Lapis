#include"app_pch.hpp"
#include"csmalgos.hpp"
#include"LapisGui.hpp"

namespace lapis {

	inline void smoothSingleValue(const Raster<csm_t>& original, Raster<csm_t>& output, int lookdist, cell_t cell) {
		csm_t numerator = 0;
		csm_t denominator = 0;
		rowcol_t row = original.rowFromCell(cell);
		rowcol_t col = original.colFromCell(cell);
		for (rowcol_t nudgeRow = std::max(0, row - lookdist); nudgeRow <= std::min(original.nrow() - 1, row + lookdist); ++nudgeRow) {
			for (rowcol_t nudgeCol = std::max(0, col - lookdist); nudgeCol <= std::min(original.ncol() - 1, col + lookdist); ++nudgeCol) {
				auto v = original.atRCUnsafe(nudgeRow, nudgeCol);
				if (v.has_value()) {
					numerator += v.value();
					denominator++;
				}
			}
		}
		if (denominator == 0) {
			return;
		}
		auto v = output[cell];
		v.has_value() = true;
		v.value() = numerator / denominator;
	}

	inline void fillSingleValue(const Raster<csm_t>& original, Raster<csm_t>& output, int neighborsNeeded, rowcol_t carddist, rowcol_t diagdist, cell_t cell) {
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
		int maxMisses = 8 - neighborsNeeded;
		int missesSoFar = 0;

		rowcol_t row = original.rowFromCell(cell);
		rowcol_t col = original.colFromCell(cell);

		for (size_t i = 0; i < directions.size(); ++i) {

			bool isDiagonal = i >= firstDiagonal;
			rowcol_t maxPixelDist = isDiagonal ? diagdist : carddist;
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
				if (missesSoFar > maxMisses) {
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

	Raster<csm_t> smoothAndFill(const Raster<csm_t>& r, int smoothWindow, int neighborsNeeded, coord_t maxDistOutXYUnits) {

		int lookdist = smoothWindow / 2;
		if (lookdist < 1) {
			return r;
		}

		//currently, you cannot have a CSM with different X and Y resolutions
		rowcol_t cardinalDist = (rowcol_t)(maxDistOutXYUnits / r.xres());
		rowcol_t diagonalDist = (rowcol_t)(maxDistOutXYUnits / r.xres() / std::sqrt(2.));

		Raster<csm_t> out{ (Alignment)r };
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			if (r[cell].has_value()) {
				smoothSingleValue(r, out, lookdist, cell);
			}
			else {
				fillSingleValue(r, out, neighborsNeeded, cardinalDist, diagonalDist, cell);
			}
		}

		return out;
	}

	//this data structure is taken from https://www.researchgate.net/publication/261191274_Hierarchical_Queues_general_description_and_implementation_in_MAMBA_Image_library
	class HierarchicalQueue {
	public:
		HierarchicalQueue(csm_t min, csm_t max, csm_t binSize);

		void push(csm_t height, cell_t cell);

		//pops the next element of the queue and returns it, or returns -1 if the queue is empty
		cell_t popAndReturn();

		size_t size();

	private:
		csm_t min, max, binSize;
		size_t currentQueue;
		std::vector<std::queue<cell_t>> queues;
		size_t _size;
	};

	size_t HierarchicalQueue::size() {
		return _size;
	}

	HierarchicalQueue::HierarchicalQueue(csm_t min, csm_t max, csm_t binSize) {
		this->min = min;
		this->max = max;
		this->binSize = binSize;
		currentQueue = 0;
		size_t nBins = (size_t)std::ceil((max - min) / binSize);
		queues = std::vector<std::queue<cell_t>>(nBins);
		_size = 0;
	}

	void HierarchicalQueue::push(csm_t height, cell_t cell) {
		size_t bin = (size_t)((max - height) / binSize);
		bin = std::max(bin, currentQueue);
		bin = std::min(bin, queues.size() - 1); //this is needed if the value is exactly equal to min
		queues[bin].push(cell);
		++_size;
	}

	cell_t HierarchicalQueue::popAndReturn() {
		if (_size==0) {
			return -1;
		}
		while (!queues[currentQueue].size()) {
			currentQueue++;
		}
		cell_t out = queues[currentQueue].front();
		queues[currentQueue].pop();
		--_size;
		return out;
	}

	Raster<taoid_t> watershedSegment(const Raster<csm_t>& csm, const std::vector<cell_t>& highPoints, cell_t thisTile, cell_t nTiles)
	{
		//this is modified from https://arxiv.org/pdf/1511.04463.pdf
		//algorithm 5 on page 15
		auto& d = LapisData::getDataObject();
		HierarchicalQueue open{ d.canopyCutoff(), d.maxHt(),d.binSize()};
		const taoid_t CANDIDATE = -2;
		const taoid_t QUEUED = -3;
		const taoid_t INTENTIONALLY_UNABELLED = 0;
		Raster<taoid_t> labels((Alignment)csm);
		for (cell_t cell = 0; cell < labels.ncell(); ++cell) {
			if (csm[cell].has_value()) {
				labels[cell].has_value() = true;
				if (csm[cell].value() >= d.canopyCutoff()) {
					labels[cell].value() = CANDIDATE;
				}
				else {
					labels[cell].value() = INTENTIONALLY_UNABELLED;
				}
			}
		}
		taoid_t label = (taoid_t)thisTile;
		if (label == 0) {
			label = (taoid_t)nTiles;
		}

		for (const cell_t& c : highPoints) {
			labels[c].value() = QUEUED;
			open.push(csm[c].value(), c);
		}

		while (open.size()) {
			cell_t c;
			c = open.popAndReturn();
			if (labels[c].has_value() && labels[c].value() == QUEUED) {
				labels[c].value() = label;
				label += (taoid_t)nTiles;
			}
			rowcol_t row = labels.rowFromCellUnsafe(c);
			rowcol_t col = labels.colFromCellUnsafe(c);

			struct rc { rowcol_t row, col; };
			std::vector<rc> neighbors = { {row + 1,col},{row - 1,col},{row,col + 1},{row,col - 1},
				{row + 1,col + 1},{row - 1,col - 1},{row + 1,col - 1},{row - 1,col + 1}
			};
			for (auto& thisRC : neighbors) {
				rowcol_t rowNudge = thisRC.row;
				rowcol_t colNudge = thisRC.col;
				if (rowNudge < 0 || colNudge < 0 || rowNudge >= csm.nrow() || colNudge >= csm.ncol()) {
					continue;
				}
				cell_t n = csm.cellFromRowColUnsafe(rowNudge, colNudge);
				if (!labels[n].has_value() || labels[n] != CANDIDATE) {
					continue;
				}
				labels[n].value() = labels[c].value();
				//the original algorithm had an optimization here using a regular queue but that was only an optimization
				//if the cells you started from were kind of arbitrary
				//by handpicking high points, it's unneccesary, and comes with a bit of overhead as well
				open.push(csm[n].value(), n);
			}
		}
		return labels;
	}

	Raster<csm_t> maxHeightBySegment(const Raster<taoid_t>& segments, const Raster<csm_t>& csm, std::vector<cell_t>& highPoints)
	{
		Raster<csm_t> maxHeight{ (Alignment)segments };
		std::unordered_map<taoid_t, csm_t> heightByID;
		for (cell_t c : highPoints) {
			heightByID.emplace(segments[c].value(), csm[c].value());
		}
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			if (segments[cell].has_value()) {
				maxHeight[cell].has_value() = true;
				maxHeight[cell].value() = heightByID[segments[cell].value()];
			}
		}
		return maxHeight;
	}

	std::vector<cell_t> identifyHighPoints(const Raster<csm_t>& csm, csm_t canopyCutoff) {
		
		std::vector<cell_t> out;

		for (rowcol_t row = 0; row < csm.nrow(); ++row) {
			for (rowcol_t col = 0; col < csm.ncol(); ++col) {
				auto center = csm.atRCUnsafe(row, col);
				if (!center.has_value()) {
					continue;
				}
				if (center.value() < canopyCutoff) {
					continue;
				}
				bool isHighPoint = true;

				struct rc { rowcol_t row, col; };
				std::vector<rc> neighbors = { {row + 1,col},{row - 1,col},{row,col + 1},{row,col - 1},
					{row + 1,col + 1},{row - 1,col - 1},{row + 1,col - 1},{row - 1,col + 1}
				};
				for (auto& thisRC : neighbors) {
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
					}
				}

				if (isHighPoint) {
					out.push_back(csm.cellFromRowColUnsafe(row,col));
				}
			}
		}

		return out;
	}

	std::vector<cell_t> identifyHighPointsWithMinDist(const Raster<csm_t>& csm, csm_t canopyCutoff, coord_t minDist) {

		std::vector<cell_t> candidates = identifyHighPoints(csm, canopyCutoff);
		if (minDist <= 0) {
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

		rowcol_t maxPixels = (rowcol_t)std::ceil(std::abs(minDist / csm.xres()));
		circle.reserve((size_t)maxPixels * maxPixels);

		coord_t minDistPixSq = (minDist / csm.xres()) * (minDist / csm.xres());
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
}