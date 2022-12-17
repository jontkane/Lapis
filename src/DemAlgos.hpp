#pragma once
#ifndef LP_DEMALGOS_H
#define LP_DEMALGOS_H

#include"app_pch.hpp"
#include"LapisTypeDefs.hpp"

namespace lapis {
	
	class DemAlgorithm {
	public:
		
		//this function takes in a list of lidar points and does two things:
		//it normalizes the points to the ground according to whatever algorithm this class represents
		//it calculates a rasterized version of the ground model
		//the passed extent should contain all the points, and be in the same CRS as them
		//the returned raster will be at least as large as e, and have a resolution no coarser than 1 meter
		//the returned points will not contain any points outside of the min and max filters, and will not contain any points that couldn't be normalized
		virtual PointsAndDem normalizeToGround(const LidarPointVector& points, const Extent& e) = 0;
	};

	//this class represents the algorithm of using the vendor-provided ground models, and bilinearly extracting from them
	class VendorRaster : public DemAlgorithm {
	public:
		PointsAndDem normalizeToGround(const LidarPointVector& points, const Extent& e) override;
	};
}

#endif