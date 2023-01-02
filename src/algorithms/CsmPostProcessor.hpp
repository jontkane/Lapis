#pragma once
#ifndef LP_CSMPOSTPROCESSOR_H
#define LP_CSMPOSTPROCESSOR_H

#include"algo_pch.hpp"

namespace lapis {

	template<class T>
	class Raster;

	class CsmPostProcessor {
	public:
		virtual Raster<csm_t> postProcess(const Raster<csm_t>& csm) = 0;
	};
}

#endif