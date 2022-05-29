#pragma once
#ifndef lp_lasreader_h
#define lp_lasreader_h


#include"lasfilter.hpp"
#include"LasExtent.hpp"
#include"CoordVector.hpp"
#include"Raster.hpp"


namespace lapis {

	class LasFilter;

	//This is a struct designed for storing the important parts of a lidar point
	//to avoid storing stuff like GPS time for every single point once it ceases to be useful
	struct LasPoint {
		coord_t x, y, z;
		int intensity;

		LasPoint() : x(0), y(0), z(0), intensity(0) {}
		LasPoint(coord_t x, coord_t y, coord_t z, int intensity) : x(x), y(y), z(z), intensity(intensity) {}
	};

	using LidarPointVector = CoordVector3D<LasPoint>;

	class LasReader : public CurrentLasPoint {
	public:
		LasReader(const std::string& file);
		LasReader() = default;
		LasReader(const LasReader&) = delete;
		LasReader(LasReader&&) = default;
		LasReader& operator=(const LasReader&) = delete;
		LasReader& operator=(LasReader&&) = default;
		virtual ~LasReader() = default;

		//Adds a filter which will be applied to every requested point. Points that fail the filter will be skipped.
		//Filters will be sorted by their priority member, hopefully resulting in checks which are fast to check and fail more often being checked first
		template<class T>
		void addFilter(const std::shared_ptr<T>& filter);

		//This function will return up to n points from the LAS file.
		//n is the maximum number of points to receive; various circumstances may cause the function to return fewer
		//Memory will always be allocated for n points or for the number of points remaining in the file, whichever is lower
		//However, fewer points than that may be returned if the number of points remaining that pass the filters is lower than the requested number of points
		//After applying most filters, if any DTMs have been assigned to this reader, the points will be normalized to the ground
		//Un-normalized points will be removed from the list and then filters that require a normalized height will be applied
		LidarPointVector getPoints(size_t n);

		//This function sets a filter, applied after height normalization, to exclude points outside of a given height range
		void setHeightLimits(coord_t minht, coord_t maxht, const Unit& units);

		//This function adds a raster to the list of DTMs for this las
		//Extent checking is done to ensure that as little data as possible is read
		//The DTMs will be used in a priority order with the finest resolutions first
		void addDEM(const std::string& file, const CoordRef& crsOverride = CoordRef(), const Unit& unitOverride = linearUnitDefs::unkLinear);

	private:
		std::vector<std::shared_ptr<LasFilter>> _filters;
		std::vector<Raster<coord_t>> _dtms;
		coord_t _minht, _maxht;

		LidarPointVector _tmp;

		size_t _normalize(Raster<coord_t>& r, LidarPointVector& points, std::vector<bool>& alreadyNormalized);

		LidarPointVector& _getTransformedPoints(LidarPointVector& points, const CoordRef& crs);
	};

	template<class T>
	void LasReader::addFilter(const std::shared_ptr<T>& filter) {
		_filters.push_back(std::dynamic_pointer_cast<LasFilter, T>(filter));
		std::sort(_filters.begin(), _filters.end(), [](const auto& a, const auto& b)->bool {return a.get()->priority > b.get()->priority; });
	}
}

#endif