#pragma once
#ifndef LP_TAOIDALGORITHM_H
#define LP_TAOIDALGORITHM_H

#include"algo_pch.hpp"


namespace lapis {

	class TaoIdAlgorithm {
	public:
		virtual std::vector<cell_t> identifyTaos(const Raster<csm_t>& csm) = 0;
	};
}

#endif