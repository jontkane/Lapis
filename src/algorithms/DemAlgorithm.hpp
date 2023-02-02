#pragma once
#ifndef LP_DEMALGORITHM_H
#define LP_DEMALGORITHM_H

#include"algo_pch.hpp"

namespace lapis {

	class MetadataPdf;
	
	class DemAlgorithm {
	public:

		virtual ~DemAlgorithm() = default;
		
		//this function takes in a list of lidar points and does two things:
		//it normalizes the points to the ground according to whatever algorithm this class represents
		//it calculates a rasterized version of the ground model
		//the passed extent should contain all the points, and be in the same CRS as them
		//the returned raster will be at least as large as e, and have a resolution no coarser than 1 meter
		//the points vector will be modified in-place to not contain any points outside of the min and max filters
		//and will not contain any points that couldn't be normalized
		virtual Raster<coord_t> normalizeToGround(LidarPointVector& points, const Extent& e) = 0;

		virtual void describeInPdf(MetadataPdf& pdf) = 0;

		void setMinMax(coord_t minHt, coord_t maxHt) {
			_minHt = minHt;
			_maxHt = maxHt;
		}

	protected:
		coord_t _minHt = 0;
		coord_t _maxHt = 300;

		//a helper function to take care of some shared logic
		//f should be a functor like:
		//bool operator()(LasPoint&) const;
		//It normalizes the z value of the point in place and returns true if the normalization was successfull
		//and false if it wasn't.
		template<class FUNC>
		void _normalizeByFunction(LidarPointVector& points, const FUNC& f);
	};
	template<class FUNC>
	inline void DemAlgorithm::_normalizeByFunction(LidarPointVector& points, const FUNC& f)
	{
		size_t nFiltered = 0;
		for (size_t i = 0; i < points.size(); ++i) {
			LasPoint& point = points[i];
			if (!f(point)) {
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

#endif