#include"algo_pch.hpp"
#include"SmoothAndFill.hpp"

namespace lapis {
	SmoothAndFill::SmoothAndFill(int smoothWindow, int neighborsNeeded, coord_t lookDistCsmXYUnits)
		: SmoothCsm(smoothWindow), FillCsm(neighborsNeeded, lookDistCsmXYUnits)
	{
	}
	Raster<csm_t> SmoothAndFill::postProcess(const Raster<csm_t>& csm)
	{

		Raster<csm_t> out{ (Alignment)csm };
		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			if (csm[cell].has_value()) {
				_smoothSingleValue(csm, out, cell);
			}
			else {
				_fillSingleValue(csm, out, cell);
			}
		}

		return out;
	}
}