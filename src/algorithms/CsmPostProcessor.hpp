#pragma once
#ifndef LP_CSMPOSTPROCESSOR_H
#define LP_CSMPOSTPROCESSOR_H

#include"algo_pch.hpp"

namespace lapis {
	class MetadataPdf;
	class CsmParameterGetter;

	class CsmPostProcessor {
	public:

		virtual ~CsmPostProcessor() = default;

		virtual Raster<csm_t> postProcess(const Raster<csm_t>& csm) = 0;

		virtual void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) = 0;
	};
}

#endif