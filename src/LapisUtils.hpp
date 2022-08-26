#pragma once
#ifndef lp_lapisutils_h
#define lp_lapisutils_h

#include"app_pch.hpp"
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

	std::string insertZeroes(int value, int maxvalue);


	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread();


}

#endif