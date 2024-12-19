#pragma once
#ifndef lp_coordtransform_h
#define lp_coordtransform_h

#include"Coordinate.hpp"
#include"coordref.hpp"


namespace lapis {
	class CoordTransform {
	public:
		CoordTransform() : _tr(nullptr), _conv(), _needZConv(false), _needXYConv(false), _src(), _dst() {}
		CoordTransform(const CoordRef& src, const CoordRef& dst);

		PJ* getPtr();
		const PJ* getPtr() const;

		const SharedPJ& getSharedPtr() const;

		//T should be a class with member doubles x and y for transformXY, and member doubles x, y, and z for transformXYZ
		template<class T>
		void transformXY(std::vector<T>& points, size_t startIdx = 0) const;

		//This will reproject x/y and do unit conversion on z. It will never do unit conversion if either units are unknown
		template<class T>
		void transformXYZ(std::vector<T>& points, size_t startIdx = 0) const;

		CoordXY transformSingleXY(coord_t x, coord_t y) const;

		const CoordRef& src() const;
		const CoordRef& dst() const;

	private:
		SharedPJ _tr;
		LinearUnitConverter _conv;
		bool _needZConv;
		bool _needXYConv;
		CoordRef _src;
		CoordRef _dst;
	};

	template<class T>
	inline void CoordTransform::transformXY(std::vector<T>& points, size_t startIdx) const {
		if (!_tr) {
			return;
		}
		if (_needXYConv) {
			proj_trans_generic(_tr.get(), PJ_FWD,
				&(points[startIdx].x), sizeof(T), points.size() - startIdx,
				&(points[startIdx].y), sizeof(T), points.size() - startIdx,
				nullptr, 0, 0,
				nullptr, 0, 0);
		}
	}

	template<class T>
	inline void CoordTransform::transformXYZ(std::vector<T>& points, size_t startIdx) const {
		if (!points.size()) {
			return;
		}
		//the caching here kind of sucks; potentially a place to optimize if necessary
		transformXY(points, startIdx);
		_conv.convertManyInPlace(&points[startIdx].z, points.size() - startIdx, sizeof(T));
	}
}

#endif