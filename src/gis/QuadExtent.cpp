#include"gis_pch.hpp"
#include"QuadExtent.hpp"


namespace lapis {

	coord_t dotProduct(const CoordXY& lhs, const CoordXY& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	QuadExtent::QuadExtent() : _coords(4, CoordRef("")) {}

	QuadExtent::QuadExtent(const Extent& e) : _coords(4, e.crs()), _outerExtent(e) {
		_copyFromExtent(e);
	}
	QuadExtent::QuadExtent(const Extent& e, const CoordRef& crs) : _coords(4, e.crs()) {
		_copyFromExtent(e);
		_coords.transform(crs);
		_initOuter();
	}
	QuadExtent::QuadExtent(const Extent& e, const CoordTransform& transform) : _coords(4, e.crs())
	{
		_copyFromExtent(e);
		_coords.transformWithoutChecking(transform);
		_initOuter();
	}
	void QuadExtent::transform(const CoordRef& crs) {
		_coords.transform(crs);
	}

	bool QuadExtent::overlaps(const Extent& other) const {
		QuadExtent otherquad{ other,_coords.crs };
		return overlaps(otherquad);
	}

	bool QuadExtent::overlaps(QuadExtent other) const {

		other.transform(_coords.crs);

		//this is the separating axis theorem
		//https://stackoverflow.com/questions/753140/how-do-i-determine-if-two-convex-polygons-intersect#:~:text=To%20be%20able%20to%20decide,polygons%20forms%20such%20a%20line.

		//checking if any of the edges of *this are separating lines
		for (int lineidx = 0; lineidx < 4; ++lineidx) {
			CoordXY vectorline{ _coords[nextIndex(lineidx)].x - _coords[lineidx].x, _coords[nextIndex(lineidx)].y - _coords[lineidx].y};
			if (vectorline.x == 0. && vectorline.y == 0.) { //two identical coordinates is possible but obviously you can't get a valid line out of them
				continue;
			}

			coord_t maxthisproj = std::numeric_limits<coord_t>::lowest();
			coord_t maxotherproj = std::numeric_limits<coord_t>::lowest();
			coord_t minthisproj = std::numeric_limits<coord_t>::max();
			coord_t minotherproj = std::numeric_limits<coord_t>::max();

			//scalar projections of the other polygon
			for (int pointidx = 0; pointidx < 4; ++pointidx) {
				CoordXY otherpoint{ other._coords[pointidx].x - _coords[lineidx].x,other._coords[pointidx].y - _coords[lineidx].y };
				CoordXY thispoint{ _coords[pointidx].x - _coords[lineidx].x,_coords[pointidx].y - _coords[lineidx].y };

				//this is the dot product
				coord_t otherproj = dotProduct(otherpoint, vectorline);
				maxotherproj = std::max(maxotherproj, otherproj);
				minotherproj = std::min(minotherproj, otherproj);

				coord_t thisproj = dotProduct(thispoint, vectorline);
				maxthisproj = std::max(maxthisproj, thisproj);
				minthisproj = std::min(minthisproj, thisproj);
			}

			//if the projected scalars have no overlap in their ranges, we've found that the polygons do not overlap
			if (minotherproj > maxthisproj || minthisproj > maxotherproj) {
				return false;
			}
		}

		return true;
	}

	Extent QuadExtent::outerExtent() const
	{
		return _outerExtent;
	}

	const CoordXYVector& QuadExtent::coords() const
	{
		return _coords;
	}

	void QuadExtent::_copyFromExtent(const Extent& e) {
		_coords[0].x = e.xmin();
		_coords[0].y = e.ymin();

		_coords[1].x = e.xmin();
		_coords[1].y = e.ymax();

		_coords[2].x = e.xmax();
		_coords[2].y = e.ymax();

		_coords[3].x = e.xmax();
		_coords[3].y = e.ymin();
	}

	void QuadExtent::_initOuter()
	{
		coord_t xmin = std::numeric_limits<coord_t>::max();
		coord_t ymin = std::numeric_limits<coord_t>::max();
		coord_t xmax = std::numeric_limits<coord_t>::lowest();
		coord_t ymax = std::numeric_limits<coord_t>::lowest();

		for (int i = 0; i < 4; ++i) {
			xmin = std::min(xmin, _coords[i].x);
			xmax = std::max(xmax, _coords[i].x);
			ymin = std::min(ymin, _coords[i].y);
			ymax = std::max(ymax, _coords[i].y);
		}
		_outerExtent = Extent(xmin, xmax, ymin, ymax, _coords.crs);
	}

	const CoordRef& QuadExtent::crs() const {
		return _coords.crs;
	}

}