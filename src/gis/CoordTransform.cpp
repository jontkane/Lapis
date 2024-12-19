#include"gis_pch.hpp"
#include"CoordTransform.hpp"


namespace lapis{
	CoordTransform::CoordTransform(const CoordRef& src, const CoordRef& dst) : _src(src), _dst(dst) {
		_conv = LinearUnitConverter(src.getZUnits(), dst.getZUnits());
		_needZConv = !src.isConsistentZUnits(dst);
		_needXYConv = !src.isConsistentHoriz(dst);
		if (_needXYConv) {
			_tr = projCrsToCrsWrapper(src.getSharedPtr(), dst.getSharedPtr());
		}
	}

	PJ* CoordTransform::getPtr() {
		return _tr.get();
	}

	const PJ * CoordTransform::getPtr() const {
		return _tr.get();
	}

	const SharedPJ& CoordTransform::getSharedPtr() const
	{
		return _tr;
	}

	CoordXY CoordTransform::transformSingleXY(coord_t x, coord_t y) const
	{
		proj_trans_generic(_tr.get(), PJ_FWD,
			&x, 0, 1,
			&y, 0, 1,
			nullptr, 0, 0,
			nullptr, 0, 0);
		return { x,y };
	}
	const CoordRef& CoordTransform::src() const
	{
		return _src;
	}
	const CoordRef& CoordTransform::dst() const
	{
		return _dst;
	}
}
