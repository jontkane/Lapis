#pragma once
#ifndef LP_WATERSHEDSEGMENT_H
#define LP_WATERSHEDSEGMENT_H

#include"TaoSegmentAlgorithm.hpp"

namespace lapis {
	class WatershedSegment : public TaoSegmentAlgorithm {
	public:
		WatershedSegment(coord_t canopyCutoff, coord_t maxHt, coord_t binSize);

		Raster<taoid_t> segment(const Raster<csm_t>& csm, const std::vector<cell_t>& taos, UniqueIdGenerator& idGenerator);

	private:
		coord_t _canopyCutoff;
		coord_t _maxHt;
		coord_t _binSize;
	};
}

#endif