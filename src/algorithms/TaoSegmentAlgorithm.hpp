#pragma once
#ifndef LP_TAOSEGMENTALGORITHM_H
#define LP_TAOSEGMENTALGORITHM_H

#include"algo_pch.hpp"
#include"TaoAlgoCommonStuff.hpp"

namespace lapis {

	class MetadataPdf;
	class TaoParameterGetter;
	struct IDedTao;

	class TaoSegmentAlgorithm {
	public:

		virtual ~TaoSegmentAlgorithm() = default;

        //the raster output should NA out segments associated with taos outside the unbuffered extent
        //the vector output should have an "ID" attribute.
		//usually this will be produced by the function rasterToMultiPolyonForTaos, with nullptr as the attributes parameter
		//X, Y, and Area will be supplied by other parts of the code,
		//but if the algorithm wants to for some reason, it's okay to add additional attributes other than those
		virtual SegmentResults segment(const Raster<csm_t>& bufferedCsm, const std::vector<IDedTao>& taos, const Extent& unbufferedExtent) = 0;

		virtual const std::string& name() const = 0;

		virtual void describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter) = 0;

		virtual bool producesRaster() const = 0;
        virtual bool producesVector() const = 0;
	};
}

#endif