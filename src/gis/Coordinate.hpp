#pragma once
#ifndef LP_COORDINATE_H
#define LP_COORDINATE_H

#include"..\LapisTypeDefs.hpp"

namespace lapis {
	struct CoordXY {
		coord_t x, y;
		CoordXY() : x(0), y(0) {}
		CoordXY(coord_t x, coord_t y) : x(x), y(y) {}
	};
	struct CoordXYZ {
		coord_t x, y, z;
		CoordXYZ() : x(0), y(0), z(0) {}
		CoordXYZ(coord_t x, coord_t y, coord_t z) : x(x), y(y), z(z) {}
	};
}

#endif