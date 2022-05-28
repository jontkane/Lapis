#include"lapis_pch.hpp"
#include"LapisUtils.hpp"

namespace fs = std::filesystem;

namespace lapis {





	bool extentSorter(const Extent& lhs, const Extent& rhs) {
		if (lhs.ymax() != rhs.ymax()) {
			return lhs.ymax() < rhs.ymax();
		}
		if (lhs.xmax() != rhs.xmax()) {
			return lhs.xmax() < rhs.xmax();
		}
		if (lhs.ymin() != rhs.ymin()) {
			return lhs.ymin() < rhs.ymin();
		}
		return lhs.xmin() < rhs.ymin();
	}
	Raster<int> getNLazRaster(const Alignment& a, const fileToLasExtent& exts)
	{
		Raster<int> nLaz = Raster<int>{ a };
		for (auto& file : exts) {
			QuadExtent q{ file.second,a.crs() };
			//this is an overestimate of the number of cells, but the vast majority of the time, it should be a very small one, very unlikely to actually cross any cell borders
			std::vector<cell_t> cells = a.cellsFromExtent(q.outerExtent(), SnapType::out);
			for (cell_t cell : cells) {
				nLaz[cell].value()++;
				nLaz[cell].has_value() = true;
			}
		}
		return nLaz;
	}
	std::string insertZeroes(int value, int maxvalue)
	{
		int nDigits = (int)(std::log10(maxvalue) + 1);
		int thisDigitCount = std::max(1, (int)(std::log10(value) + 1));
		return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
	}
}