#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#define LAPIS_VERSION_MAJOR 0
#define LAPIS_VERSION_MINOR 7

namespace lapis {

	using coord_t = double;
	using cell_t = int64_t;
	using rowcol_t = int32_t;
	constexpr coord_t LAPIS_EPSILON = 0.0001;
	using metric_t = float;
	using intensity_t = uint32_t;
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it
}

#endif