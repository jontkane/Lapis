#pragma once
#ifndef LP_TAOIDALGORITHM_H
#define LP_TAOIDALGORITHM_H

#include"algo_pch.hpp"


namespace lapis {

	class MetadataPdf;
	class TaoParameterGetter;

	class TaoIdAlgorithm {
	public:

		virtual ~TaoIdAlgorithm() = default;

		virtual std::vector<cell_t> identifyTaos(const Raster<csm_t>& csm) = 0;

		virtual void describeInPdf(MetadataPdf& pdf, TaoParameterGetter*) = 0;
	};
}

#endif