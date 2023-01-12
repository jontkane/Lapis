#include"algo_pch.hpp"
#include"AlreadyNormalized.hpp"

namespace lapis {
	Raster<coord_t> AlreadyNormalized::normalizeToGround(LidarPointVector& points, const Extent& e)
	{
		Raster<coord_t> out = Raster<coord_t>(Alignment(e, 1, 1)); //the simplest and smallest raster that fulfills the contract of the function
		out[0].has_value() = true;
		out[0].value() = 0;

		auto doNothing = [](LasPoint&)->bool { return true; };

		_normalizeByFunction(points, doNothing);

		return out;
	}
}