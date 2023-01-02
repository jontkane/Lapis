#pragma once
#ifndef LP_FILLCSM_H
#define LP_FILLCSM_H

#include"CsmPostProcessor.hpp"

namespace lapis {
	class FillCsm : public virtual CsmPostProcessor {
	public:
		FillCsm(int neighborsNeeded, coord_t lookDistCsmXYUnits);

		virtual Raster<csm_t> postProcess(const Raster<csm_t>& csm);

	protected:
		void _fillSingleValue(const Raster<csm_t>& original, Raster<csm_t>& output, cell_t cell);

	private:
		int _maxMisses;
		rowcol_t _cardinalFillDist;
		rowcol_t _diagonalFillDist;
	};
}

#endif