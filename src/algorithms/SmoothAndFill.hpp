#pragma once
#ifndef LP_SMOOTHANDFILL_H
#define LP_SMOOTHANDFILL_H

#include"CsmPostProcessor.hpp"
#include"SmoothCsm.hpp"
#include"FillCsm.hpp"

namespace lapis {
	class SmoothAndFill : public SmoothCsm, public FillCsm {

	public:
		SmoothAndFill(int smoothWindow, int neighborsNeeded, coord_t lookDistCsmXYUnits);

		Raster<csm_t> postProcess(const Raster<csm_t>& csm) override;
		void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) override;


	private:
		int _smooth;
		int _neighborsNeeded;
		coord_t _lookDist;
	};
}

#endif