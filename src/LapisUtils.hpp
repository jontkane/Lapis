#pragma once
#ifndef lp_lapisutils_h
#define lp_lapisutils_h

#include"app_pch.hpp"

namespace lapis {

	struct LasFileExtent {
		std::string filename;
		LasExtent ext;
	};
	struct DemFileAlignment {
		std::string filename;
		Alignment align;
	};

	bool operator<(const DemFileAlignment& a, const DemFileAlignment& b);

	bool operator<(const LasFileExtent& a, const LasFileExtent& b);


	//sorts extents by north to south, then east to west. If the LAS files are well-tiled, this will result in a pattern where
	//the exposed surface area of already-processed files is minimized, since that's a big source of memory usage
	bool extentSorter(const Extent& lhs, const Extent& rhs);

	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread();

	void ImGuiHelpMarker(const char* desc);

	template<typename WORKERFUNC>
	inline void distributeWork(uint64_t& sofar, uint64_t max, const WORKERFUNC& func, std::mutex& mut) {
		while (true) {
			cell_t thisidx;
			{
				std::lock_guard lock(mut);
				if (sofar >= max) {
					break;
				}
				thisidx = sofar;
				++sofar;
			}
			func(thisidx);
		}
	}
}

#endif