#pragma once
#ifndef LP_SMOOTHCSM_H
#define LP_SMOOTHCSM_H

#include"CsmPostProcessor.hpp"

namespace lapis {
	class SmoothCsm : public virtual CsmPostProcessor {
	public:
		SmoothCsm(int smoothWindow);

		virtual Raster<csm_t> postProcess(const Raster<csm_t>& csm);

		void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) override;

	protected:
		void _smoothSingleValue(const Raster<csm_t>& original, Raster<csm_t>& out, cell_t cell);

	private:
		int _smoothLookDist;
	};
}

#endif