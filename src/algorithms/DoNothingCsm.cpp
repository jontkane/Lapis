#include"algo_pch.hpp"
#include"DoNothingCsm.hpp"
#include"..\utils\MetadataPdf.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
	Raster<csm_t> DoNothingCsm::postProcess(const Raster<csm_t>& csm)
	{
		return csm;
	}
	void DoNothingCsm::describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter)
	{
	}
}