#include"algo_pch.hpp"
#include"SmoothCsm.hpp"
#include"..\utils\MetadataPdf.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
	SmoothCsm::SmoothCsm(int smoothWindow)
	{
		_smoothLookDist = smoothWindow / 2; //int division is intentional
	}
	Raster<csm_t> SmoothCsm::postProcess(const Raster<csm_t>& csm)
	{
		if (_smoothLookDist < 1) {
			return csm;
		}

		Raster<csm_t> out{ (Alignment)csm };
		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			if (csm[cell].has_value()) {
				_smoothSingleValue(csm, out, cell);
			}
		}

		return out;
	}
	void SmoothCsm::describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter)
	{
		pdf.writeSubsectionTitle("Smoothing");
		std::stringstream desc;
		int smooth = _smoothLookDist * 2 + 1;
		desc << "After producing the initial CSM, it was smoothed by applying a "
			<< smooth << "x" << smooth << " mean filter smooth. ";
		desc << "This means that each pixel was assigned a value equal to the mean of the values in the "
			<< smooth << "x" << smooth << " window surrounding it. This process is designed to reduce "
			<< "noise in the CSM.";
		pdf.writeTextBlockWithWrap(desc.str());
	}
	void SmoothCsm::_smoothSingleValue(const Raster<csm_t>& original, Raster<csm_t>& out, cell_t cell)
	{
		csm_t numerator = 0;
		csm_t denominator = 0;
		rowcol_t row = original.rowFromCell(cell);
		rowcol_t col = original.colFromCell(cell);
		for (rowcol_t nudgeRow = std::max(0, row - _smoothLookDist); nudgeRow <= std::min(original.nrow() - 1, row + _smoothLookDist); ++nudgeRow) {
			for (rowcol_t nudgeCol = std::max(0, col - _smoothLookDist); nudgeCol <= std::min(original.ncol() - 1, col + _smoothLookDist); ++nudgeCol) {
				auto v = original.atRCUnsafe(nudgeRow, nudgeCol);
				if (v.has_value()) {
					numerator += v.value();
					denominator++;
				}
			}
		}
		if (denominator == 0) {
			return;
		}
		auto v = out[cell];
		v.has_value() = true;
		v.value() = numerator / denominator;
	}
}