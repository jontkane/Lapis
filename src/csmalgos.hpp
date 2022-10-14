#pragma once
#ifndef lp_csmalgos_h
#define lp_csmalgos_h

#include"app_pch.hpp"
#include"LapisTypedefs.hpp"

namespace lapis {

	struct TaoIdMap {
		struct XY { coord_t x, y; };
		struct XYHasher {
			std::size_t operator() (const XY& coord) const {
				return (std::hash<coord_t>()(coord.x) >> 1) ^ (std::hash<coord_t>()(coord.y));
			}
		};
		struct XYEqual {
			bool operator() (const XY& lhs, const XY& rhs) const {
				return lhs.x == rhs.x && lhs.y == rhs.y;
			}
		};
		using IDToCoord = std::unordered_map<taoid_t, XY>;
		std::unordered_map<cell_t, IDToCoord> tileToLocalNames;
		std::unordered_map<XY, taoid_t, XYHasher, XYEqual> coordsToFinalName;
	};

	//applies a smoothing and filling algorithm to the given CSM raster
	//r is the input csm
	//smoothWindow is the size of the window to use for smoothing, and should be odd. If this is less than 3, than no smoothing is actually performed
	//neighborsNeeded is the number of non-nodata neighbors a nodata pixel needed before it gets filled. Set this to 9 or greater if you don't want filling
	//preserveValues is a list of cells to not smooth; usually the output of identifyHighPoints.
	Raster<csm_t> smoothAndFill(const Raster<csm_t>& r, int smoothWindow, int neighborsNeeded, const std::vector<cell_t>& preserveValues);


	//identifies the high points/TAOs present in this CSM
	//the ids produced by this function will be guaranteed to have a remainder of thistile when divided by nTiles, in order to produce unique TAO IDs across multiple tiles.
	std::vector<cell_t> identifyHighPoints(const Raster<csm_t>& csm, csm_t canopyCutoff);

	//returns a segmented and labeled raster
	//Theres's a few constraints here:
	//gp.canopyCutoff must be positive
	//highPoints should legitimately contain all local maxima of the csm (creating such a vector is the purpose of the identifyHighPoints function)
	//thisTile should be >=0 and <nTiles
	//the labels of the output raster will all have the property that their value, mod nTiles, is congruent to thisTile
	//this can be used to guarantee unique IDs across all tiles
	Raster<taoid_t> watershedSegment(const Raster<csm_t>& csm, const std::vector<cell_t>& highPoints, cell_t thisTile, cell_t nTiles);

	Raster<csm_t> maxHeightBySegment(const Raster<taoid_t>& segments, const Raster<csm_t>& csm, std::vector<cell_t>& highPoints);

}

#endif