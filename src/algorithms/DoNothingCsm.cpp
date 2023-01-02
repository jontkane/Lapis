#include"algo_pch.hpp"
#include"DoNothingCsm.hpp"

namespace lapis {
	Raster<csm_t> DoNothingCsm::postProcess(const Raster<csm_t>& csm)
	{
		return csm;
	}
}