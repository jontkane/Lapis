#pragma once
#ifndef LP_HIGHPOINTS_H
#define LP_HIGHPOINTS_H

#include"TaoIdAlgorithm.hpp"

namespace lapis {

	class HighPoints : public TaoIdAlgorithm {

	public:
		HighPoints(coord_t minHtCsmZUnits, coord_t minDistCsmXYUnits);

		std::vector<cell_t> identifyTaos(const Raster<csm_t>& csm);

		//for testing
		coord_t minHt();
		coord_t minDist();

	private:

		std::vector<cell_t> _taoCandidates(const Raster<csm_t>& csm);

		coord_t _minHt;
		coord_t _minDist;
	};
}

#endif