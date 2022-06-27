#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#include"gis/BaseDefs.hpp"

namespace lapis {
	using metric_t = coord_t;
	using intensity_t = uint32_t;
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it
}

#endif