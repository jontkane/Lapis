#pragma once
#ifndef LP_MAXPOINT_H
#define LP_MAXPOINT_H

#include"CsmAlgorithm.hpp"

namespace lapis {

	class MaxPointCsmMaker : public CsmMaker {
	public:
		MaxPointCsmMaker(const Alignment& a, coord_t footprintRadius);
		void addPoints(const std::span<LasPoint>& points) override;
		std::shared_ptr<Raster<csm_t>> currentCsm() override;
	private:
		coord_t _footprintRadius;
		std::shared_ptr<Raster<csm_t>> _csm;
	};

	//this is the simplest algorithm: each cell is assigned the height of the highest point that falls within it
	//You can get a little fancy by providing a footprint size and modeling las returns as circles instead of points
	class MaxPoint : public CsmAlgorithm {
	public:
		MaxPoint(coord_t footprintDiameter);

		std::unique_ptr<CsmMaker> getCsmMaker(const Alignment& a) override;

		csm_t combineCells(csm_t a, csm_t b) override;

		void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) override;

		//just for testing
		coord_t footprintRadius();

	private:
		coord_t _footprintRadius;
	};
}

#endif