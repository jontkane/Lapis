#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#define LAPIS_VERSION_MAJOR 0
#define LAPIS_VERSION_MINOR 9

//if this is true, the program will catch errors and log them in the gui
//if false, the program will crash (and provide a more useful traceback)
#define LAPIS_HANDLE_ERRORS false

namespace lapis {
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it

    constexpr int MAX_CONCURRENT_IO = 100; //needs a maximum because it's backed by std::counting_semaphore, which has a templated maximum
}

#endif