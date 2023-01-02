#pragma once
#ifndef LP_ALREADYNORMALIZED_H
#define LP_ALREADYNORMALIZED_H

#include"DemAlgorithm.hpp"

namespace lapis {

	//The most basic possible algorithm: none. The input data is already normalized to the ground
	class AlreadyNormalized : public DemAlgorithm {
	public:
		PointsAndDem normalizeToGround(const LidarPointVector& points, const Extent& e);
	};
}

#endif