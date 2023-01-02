#pragma once
#ifndef LP_TAOSEGMENTALGORITHM_H
#define LP_TAOSEGMENTALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {
	
	class UniqueIdGenerator {
	public:
		virtual taoid_t nextId() = 0;
	};

	class TaoSegmentAlgorithm {
	public:
		virtual Raster<taoid_t> segment(const Raster<csm_t>& csm, const std::vector<cell_t>& taos, UniqueIdGenerator& idGenerator) = 0;
	};
}

#endif