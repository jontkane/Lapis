#include"algo_pch.hpp"
#include"AlreadyNormalized.hpp"

namespace lapis {
	DemAlgorithm::PointsAndDem AlreadyNormalized::normalizeToGround(const LidarPointVector& points, const Extent& e)
	{
		PointsAndDem out;
		out.dem = Raster<coord_t>(Alignment(e, 1, 1)); //the simplest and smallest raster that fulfills the contract of the function
		out.dem[0].has_value() = true;
		out.dem[0].value() = 0;

		out.points.reserve(points.size());
		for (const LasPoint& p : points) {
			if (p.z >= _minHt && p.z <= _maxHt) {
				out.points.push_back(p);
			}
		}

		return out;
	}
}