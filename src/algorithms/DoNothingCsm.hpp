#pragma once
#ifndef LP_DONOTHINGCSM_H
#define LP_DONOTHINGCSM_H

#include"CsmPostProcessor.hpp"

namespace lapis {

	//the simplest possible post-processing: doing absolutely nothing
	class DoNothingCsm : public CsmPostProcessor {
	public:
		Raster<csm_t> postProcess(const Raster<csm_t>& csm) override;

		void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) override;
	};
}

#endif