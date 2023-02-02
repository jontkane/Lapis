#include"algo_pch.hpp"
#include"WatershedSegment.hpp"
#include"..\utils\MetadataPdf.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
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
		if (_size == 0) {
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
	WatershedSegment::WatershedSegment(coord_t canopyCutoff, coord_t maxHt, coord_t binSize)
		: _canopyCutoff(canopyCutoff), _maxHt(maxHt), _binSize(binSize)
	{
	}
	Raster<taoid_t> WatershedSegment::segment(const Raster<csm_t>& csm, const std::vector<cell_t>& taos, UniqueIdGenerator& idGenerator)
	{
		//this is modified from https://arxiv.org/pdf/1511.04463.pdf
		//algorithm 5 on page 15
		HierarchicalQueue open{ _canopyCutoff, _maxHt, _binSize };
		const taoid_t CANDIDATE = -2;
		const taoid_t QUEUED = -3;
		const taoid_t INTENTIONALLY_UNABELLED = 0;
		Raster<taoid_t> labels((Alignment)csm);
		for (cell_t cell = 0; cell < labels.ncell(); ++cell) {
			if (csm[cell].has_value()) {
				labels[cell].has_value() = true;
				if (csm[cell].value() >= _canopyCutoff) {
					labels[cell].value() = CANDIDATE;
				}
				else {
					labels[cell].value() = INTENTIONALLY_UNABELLED;
				}
			}
		}

		for (const cell_t& c : taos) {
			labels[c].value() = QUEUED;
			open.push(csm[c].value(), c);
		}

		while (open.size()) {
			cell_t c;
			c = open.popAndReturn();
			if (labels[c].has_value() && labels[c].value() == QUEUED) {
				labels[c].value() = idGenerator.nextId();
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
	void WatershedSegment::describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter)
	{
		pdf.writeSubsectionTitle("Watershed Segmentation Algorithm");
		pdf.writeTextBlockWithWrap(
			"A tree segmentation algorithm takes the TAOs identified by the identification algorithm and assigns regions of the acquisition to each one. "
			"The watershed segmentation algorithm is a canopy surface model-based algorithm; it is performed entirely on the CSM, without reference "
			"to the original point data. It gets its name from hydrology, where it is used to segment landscapes into watersheds. "
			"The algorithm turns the canopy upside-down and treats it as if it were terrain; each TAO is assigned to the 'watershed' it would "
			"belong to if this imaginary terrain were filled with water."
		);
	}
}