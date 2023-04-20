#include"gis_pch.hpp"
#include"CoordTransform.hpp"


namespace lapis{
CoordTransform::CoordTransform(const CoordRef& src, const CoordRef& dst) {
	_conv = LinearUnitConverter(src.getZUnits(), dst.getZUnits());
	_needZConv = !src.isConsistentZUnits(dst);
	_needXYConv = !src.isConsistentHoriz(dst);
	if (_needXYConv) {
		ProjPJWrapper temp = ProjPJWrapper(proj_create_crs_to_crs_from_pj(
			ProjContextByThread::get(), src.getPtr(), dst.getPtr(), nullptr, nullptr));
		if (temp.ptr() != nullptr) {
			_tr = ProjPJWrapper(proj_normalize_for_visualization(ProjContextByThread::get(), temp.ptr()));
		}
	}
}

PJ * CoordTransform::getPtr() {
	return _tr.ptr();
}

const PJ * CoordTransform::getPtr() const {
	return _tr.ptr();
}

ProjPJWrapper& CoordTransform::getWrapper() {
	return _tr;
}

inline const ProjPJWrapper& CoordTransform::getWrapper() const {
	return _tr;
}
CoordXY CoordTransform::transformSingleXY(coord_t x, coord_t y)
{
	proj_trans_generic(_tr.ptr(), PJ_FWD,
		&x, 0, 1,
		&y, 0, 1,
		nullptr, 0, 0,
		nullptr, 0, 0);
	return { x,y };
}
}
