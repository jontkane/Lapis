#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#define LAPIS_VERSION_MAJOR 0
#define LAPIS_VERSION_MINOR 9

#define LAPIS_HANDLE_ERRORS true

namespace lapis {
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it
}

#endif