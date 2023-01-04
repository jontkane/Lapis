#pragma once
#ifndef LP_DEMALGORITHM_H
#define LP_DEMALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {
	
	class DemAlgorithm {
	public:

		virtual ~DemAlgorithm() = default;

		struct PointsAndDem {
			LidarPointVector points;
			Raster<coord_t> dem;
		};
		
		//this function takes in a list of lidar points and does two things:
		//it normalizes the points to the ground according to whatever algorithm this class represents
		//it calculates a rasterized version of the ground model
		//the passed extent should contain all the points, and be in the same CRS as them
		//the returned raster will be at least as large as e, and have a resolution no coarser than 1 meter
		//the returned points will not contain any points outside of the min and max filters, and will not contain any points that couldn't be normalized
		virtual PointsAndDem normalizeToGround(const LidarPointVector& points, const Extent& e) = 0;

		void setMinMax(coord_t minHt, coord_t maxHt) {
			_minHt = minHt;
			_maxHt = maxHt;
		}

	protected:
		coord_t _minHt = 0;
		coord_t _maxHt = 300;
	};
}

#endif