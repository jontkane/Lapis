#include"app_pch.hpp"
#include"csmalgos.hpp"
#include"LapisGui.hpp"

namespace lapis {

	Raster<csm_t> smoothAndFill(const Raster<csm_t>& r, int smoothWindow, int neighborsNeeded, const std::vector<cell_t>& preserveValues) {

		int lookdist = smoothWindow / 2;
		if (lookdist < 1) {
			return r;
		}

		std::unordered_set<cell_t> preserveSet;
		for (auto& v : preserveValues) {
			preserveSet.insert(v);
		}

		struct Direction {
			rowcol_t x, y;
		};
		std::vector<Direction> cardinals = { {0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1} };

		Raster<csm_t> out{ (Alignment)r };
		for (rowcol_t row = 0; row < r.nrow(); ++row) {
			for (rowcol_t col = 0; col < r.ncol(); ++col) {
				cell_t cell = r.cellFromRowColUnsafe(row, col);
				if (preserveSet.contains(cell)) {
					out[cell].has_value() = r[cell].has_value();
					out[cell].value() = r[cell].value();
					continue;
				}

				csm_t numerator = 0;
				csm_t denominator = 0;
				for (rowcol_t nudgeRow = std::max(0, row - lookdist); nudgeRow <= std::min(r.nrow()-1, row + lookdist); ++nudgeRow) {
					for (rowcol_t nudgeCol = std::max(0, col - lookdist); nudgeCol <= std::min(r.ncol()-1, col + lookdist); ++nudgeCol) {
						auto v = r.atRCUnsafe(nudgeRow, nudgeCol);
						if (v.has_value()) {
							numerator += v.value();
							denominator++;
						}
					}
				}
				if (denominator == 0) {
					continue;
				}
				if (r[cell].has_value()) {
					auto v = out[cell];
					v.has_value() = true;
					v.value() = numerator / denominator;
				}
				else {
					//the algorithm here to distinguish legitimately absent data from holes is:
					//in all 8 cardinal directions, there needs to be either data or the edge of the raster
					//if this condition is met, the value is assigned to be the mean of all adjacent pixels that have data
					int datacount = 0;
					for (auto& d : cardinals) {
						bool founddata = false;
						for (int i : {1, 2}) {
							rowcol_t thisrow = row + d.y * i;
							rowcol_t thiscol = col + d.x * i;
							if (thisrow < 0 || thisrow >= r.nrow() ||
								thiscol < 0 || thiscol >= r.ncol() ||
								r.atRCUnsafe(thisrow, thiscol).has_value()) {
								founddata = true;
								break;
							}
						}
						if (founddata) {
							datacount++;
						}
					}

					if (datacount >= neighborsNeeded) {
						auto v = out[cell];
						v.has_value() = true;
						v.value() = numerator / denominator;
					}
				}

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

			/*for (rowcol_t rowNudge = std::max(0, row - 1); rowNudge <= std::min(csm.nrow() - 1, row + 1); ++rowNudge) {
				for (rowcol_t colNudge = std::max(0, col - 1); colNudge <= std::min(csm.ncol() - 1, col + 1); ++colNudge) {
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
			}*/
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
}