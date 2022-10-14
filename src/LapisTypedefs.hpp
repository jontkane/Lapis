#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#include"app_pch.hpp"

namespace lapis {
	using metric_t = coord_t;
	using intensity_t = uint32_t;
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it

	enum class OutputUnitLabel {
		Default, Radian, Percent, Unitless
	};
}

#endif