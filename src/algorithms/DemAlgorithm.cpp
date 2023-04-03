#include"algo_pch.hpp"
#include"DemAlgorithm.hpp"

namespace lapis {
	void DemAlgoApplier::normalizePointVector(LidarPointVector& points)
	{
		size_t nFiltered = 0;
		for (size_t i = 0; i < points.size(); ++i) {
			LasPoint& point = points[i];
			if (!normalizePoint(point)) {
				nFiltered++;
				continue;
			}

			if (point.z > _maxHt || point.z < _minHt) {
				nFiltered++;
				continue;
			}
			if (nFiltered > 0) {
				points[i - nFiltered] = point;
			}
		}
		points.resize(points.size() - nFiltered);
	}
}