#pragma once
#ifndef lp_quadextent_h
#define lp_quadextent_h

#include"Extent.hpp"
#include"CoordVector.hpp"

namespace lapis {

	//this class is designed to hold any convex quadrilateral, and implement functions like contains and overlaps, just like a rectangular extent
	//because its purpose is to store re-projected extents, no general-purpose constructors are provided
	//in the slightly weird case where it's derived from an extent where xmin==xmax or ymin==ymax, this class may have slightly weird behavior
	class QuadExtent {
	public:

		QuadExtent();

		//just converts the extent into a quad extent--i.e., storing a rectangle as a generic quadrilateral
		explicit QuadExtent(const Extent& e);

		QuadExtent(const Extent& e, const CoordRef& crs);

		//applies the transform to the extent, without checking whether this transformation makes sense
		//use with caution, but allows you to use the same transform over and over without re-constructing
		QuadExtent(const Extent& e, const CoordTransform& transform);

		void transform(const CoordRef& crs);

		const CoordRef& crs() const;

		//returns true if this extent overlaps the other in their interiors
		//because of float imprecision, behavior is a bit unpredictable if the extents originally exactly touched
		//if possible, please compare extents that were originally in the same CRS
		//using Extent::overlaps, in their native CRS, not QuadExtent::overlaps
		bool overlaps(QuadExtent other) const;
		bool overlaps(const Extent& other) const;

		//returns true if the given point is inside or on the edge of this quadrilateral
		//note that, due to float imprecision, points on the edge of the orginal extent may no longer remain on the edge after reprojection
		//whenever possible, do comparisons in the native CRS of the data
		bool contains(coord_t x, coord_t y) const {
			//most points outside the quadrilateral will miss by so much that they're outside the bounding box
			if (!_outerExtent.contains(x, y)) {
				return false;
			}

			//finally, if the point falls in the tiny window between the inscribed and bounding rectangles, do the full check
			//solution 3 from:
			//http://paulbourke.net/geometry/polygonmesh/#insidepoly
			int prevsign = 0;
			for (int vertidx = 0; vertidx < 4; ++vertidx) {
				coord_t signindicator = (y - _coords[vertidx].y) * (_coords[nextIndex(vertidx)].x - _coords[vertidx].x)
					- (x - _coords[vertidx].x) * (_coords[nextIndex(vertidx)].y - _coords[vertidx].y);
				int sign = (signindicator < 0) - (signindicator > 0);
				if (sign == 0) { //edges count as inside
					return true;
				}
				if (prevsign == 0) {
					prevsign = sign;
				}
				if (sign != prevsign) { //flipping from the right to left side means it's outside
					return false;
				}
			}
			return true;
		}

		//returns the extent object corresponding to the minimal bounding orthogonal rectangle of this quadrilateral
		Extent outerExtent() const;

		//starts in the lower left corner and goes clockwise
		const CoordXYVector& coords() const;

private:
		CoordXYVector _coords;
		Extent _outerExtent;

		void _copyFromExtent(const Extent& e);
		void _initOuter();
		static int nextIndex(int i) {
			if (i == 3) {
				return 0;
			}
			return i + 1;
		}
	};

}

#endif