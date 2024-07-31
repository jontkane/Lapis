#pragma once
#ifndef lapis_fixedextent_h
#define lapis_fixedextent_h

#include"..\LapisTypeDefs.hpp"
#include"lasio.hpp"
#include"CoordRef.hpp"

namespace lapis {

	//This class is just the sides of a rectangle and a coordinate system
	//No functions exist to modify these the bounds because many classes will inherit from it
	//Instead, functions to modify extent objects are all out-of-place
	class Extent {

	public:
		//the default constructor produced the extent (0,0,0,0) with no crs information
		Extent() : _xmin(0), _ymin(0), _xmax(0), _ymax(0), _crs() {}

		virtual ~Extent() noexcept = default; //just declaring the destructor virtual

		//This constructor manually sets the bounds but leaves the crs undefined
		Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax);

		//A full definition of the extent, including crs information
		Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax, const CoordRef& crs);
		Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax, CoordRef&& crs);

		//This function will, in order, attempt to read the file as a raster, a vector, and a las point cloud
		//Extent and crs information will be read from the file if possible
		Extent(const std::string& filename);

		//creates an extent from an already-opened las file to avoid double hitting the harddrive
		Extent(const LasIO& las);

		//return the internal objects
		const CoordRef& crs() const;
		coord_t xmin() const;
		coord_t xmax() const;
		coord_t ymin() const;
		coord_t ymax() const;

		coord_t xspan() const;
		coord_t yspan() const;

		//returns true if this extent has an overlap with non-zero area with the other extent
		bool overlaps(const Extent& e) const;
		//returns true if the extents touch at all, even at a single point
		bool touches(const Extent& e) const;

		//this version of the functions do no sanity checking on CRS. Not unsafe in the sense that they might cause a crash, but unsafe in the sense that they might
		//allow bugs to escape detection. However, they are much faster
		bool overlapsUnsafe(const Extent& e) const;

		//returns true if the specified point is contained in the extent (edge counts)
		//ensuring matching CRS is the responsibility of the caller
		bool contains(const coord_t x, const coord_t y) const {
			return (x >= _xmin) && (x <= _xmax) && (y >= _ymin) && (y <= _ymax);
		}
		//like contains, but the edge doesn't count
		bool strictContains(const coord_t x, const coord_t y) const {
			return (x > _xmin) && (x < _xmax) && (y > _ymin) && (y < _ymax);
		}

		/*eventually, some sort of version of contains that accounts for projections will go here*/

		//functions for modifying the crs
		void setZUnits(LinearUnit zUnits);
		void defineCRS(const CoordRef& crs);
		void defineCRS(CoordRef&& crs);

	protected:
		coord_t _xmin, _xmax, _ymin, _ymax;
		CoordRef _crs;

		void checkValidExtent();

		std::array<double, 6> getGeoTrans(UniqueGdalDataset& wgd, const std::string& errormsg);
		void extentInitFromGDALRaster(UniqueGdalDataset& wgd, const std::array<double, 6>& geotrans);

		void _setFromLasIO(const LasIO& las);
	};

	bool operator==(const Extent& lhs, const Extent& rhs);
	bool operator!=(const Extent& lhs, const Extent& rhs);
	std::ostream& operator<<(std::ostream& os, const Extent& e);

	//projectExtentOuter will go here once reprojection is architected

	//This function creates a new extent object with only the overlapping area of the two given extents
	//If they do not touch, this will throw an exception
	Extent cropExtent(const Extent& base, const Extent& e);
	//This function creates a new extent object which is the smallest extent to encompass both of the given extents
	Extent extendExtent(const Extent& base, const Extent& e);

	//This function creates a new extent in which the two mins are reduced by bufferSize,
	//And the two maxes are increased by the same amount
	Extent bufferExtent(const Extent& base, coord_t bufferSize);
}

#endif