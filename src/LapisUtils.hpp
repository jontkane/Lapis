#pragma once
#ifndef lp_lapisutils_h
#define lp_lapisutils_h

#include"lapis_pch.hpp"
#include"Logger.hpp"
#include"gis/LasExtent.hpp"
#include"gis/Raster.hpp"

namespace lapis {

	struct LasFileExtent {
		std::string filename;
		Extent ext;
	};
	struct DemFileAlignment {
		std::string filename;
		Alignment align;
	};


	//sorts extents by north to south, then east to west. If the LAS files are well-tiled, this will result in a pattern where
	//the exposed surface area of already-processed files is minimized, since that's a big source of memory usage
	bool extentSorter(const Extent& lhs, const Extent& rhs);

	//returns a raster whose values are the number of extents that overlap that cell
	Raster<int> getNLazRaster(const Alignment& a, const fileToLasExtent& exts);

	std::string insertZeroes(int value, int maxvalue);
}

#endif