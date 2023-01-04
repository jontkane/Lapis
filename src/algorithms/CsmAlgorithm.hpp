#pragma once
#ifndef LP_CSMALGORITHM_H
#define LP_CSMALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {
	class CsmAlgorithm {
	public:

		virtual ~CsmAlgorithm() = default;

		//Takes a set of normalized, filtered units in the correct units, and the alignment you want the output CSM to be at
		//The points must all be contained in that alignment
		//The output CSM will be at least as large as a, but might be larger
		virtual Raster<csm_t> createCsm(const LidarPointVector& points, const Alignment& a) = 0;

		//A function appropriate to pass to overlay, to combine points of overlap between two previously-produced CSMs
		virtual csm_t combineCells(csm_t a, csm_t b) = 0;
	};
}

#endif