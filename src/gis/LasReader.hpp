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
		uint8_t returnNumber;

		LasPoint() : x(0), y(0), z(0), intensity(0), returnNumber(0) {}
		LasPoint(coord_t x, coord_t y, coord_t z, int intensity, uint8_t returnNumber) : x(x), y(y), z(z), intensity(intensity), returnNumber(returnNumber) {}
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
		LasReader(const Extent& e);
		const std::string& filename();
		virtual ~LasReader() = default;

		//Adds a filter which will be applied to every requested point. Points that fail the filter will be skipped.
		//Filters will be sorted by their priority member, hopefully resulting in checks which are fast to check and fail more often being checked first
		void addFilter(const std::shared_ptr<LasFilter>& filter);

		//This function will return up to n points in the LAS file
		//It will never return more than n points, but it may return less
		//This happens if fewer than n points remain in the file, or if any of the next n points fail to pass a filter
		//The capacity of the returned vector may be as high as n but will never exceed it
		LidarPointVector getPoints(size_t n);

	private:
		std::vector<std::shared_ptr<LasFilter>> _filters;
		std::string _filename;
	};
}

#endif