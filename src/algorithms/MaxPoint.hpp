#pragma once
#ifndef LP_MAXPOINT_H
#define LP_MAXPOINT_H

#include"CsmAlgorithm.hpp"

namespace lapis {

	//this is the simplest algorithm: each cell is assigned the height of the highest point that falls within it
	//You can get a little fancy by providing a footprint size and modeling las returns as circles instead of points
	class MaxPoint : public CsmAlgorithm {
	public:
		MaxPoint(coord_t footprintDiameter);

		Raster<csm_t> createCsm(const LidarPointVector& points, const Alignment& a) override;

		csm_t combineCells(csm_t a, csm_t b) override;

		void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) override;

		//just for testing
		coord_t footprintRadius();

	private:
		coord_t _footprintRadius;
	};
}

#endif