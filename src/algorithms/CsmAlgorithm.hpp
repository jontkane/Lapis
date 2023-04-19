#pragma once
#ifndef LP_CSMALGORITHM_H
#define LP_CSMALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {
	class MetadataPdf;
	class CsmParameterGetter;

	class CsmMaker {
	public:

		virtual ~CsmMaker() = default;

		virtual void addPoints(const std::span<LasPoint>& points) = 0;
		virtual std::shared_ptr<Raster<csm_t>> currentCsm() = 0;
	};

	class CsmAlgorithm {
	public:

		virtual ~CsmAlgorithm() = default;

		virtual std::unique_ptr<CsmMaker> getCsmMaker(const Alignment& a) = 0;

		//A function appropriate to pass to overlay, to combine points of overlap between two previously-produced CSMs
		virtual csm_t combineCells(csm_t a, csm_t b) = 0;

		virtual void describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter) = 0;
	};
}

#endif