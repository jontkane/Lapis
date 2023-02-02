#pragma once
#ifndef LP_ALREADYNORMALIZED_H
#define LP_ALREADYNORMALIZED_H

#include"DemAlgorithm.hpp"

namespace lapis {

	//The most basic possible algorithm: none. The input data is already normalized to the ground
	class AlreadyNormalized : public DemAlgorithm {
	public:
		Raster<coord_t> normalizeToGround(LidarPointVector& points, const Extent& e) override;
		void describeInPdf(MetadataPdf& pdf) override;
	};
}

#endif