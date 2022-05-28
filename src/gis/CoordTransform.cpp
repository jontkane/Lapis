#include"gis_pch.hpp"
#include"CoordTransform.hpp"


namespace lapis{
CoordTransform::CoordTransform(const CoordRef& src, const CoordRef& dst) {
	_convFactor = src.getZUnits().convFactor / dst.getZUnits().convFactor;
	_needZConv = !src.isConsistentZUnits(dst);
	_needXYConv = !src.isConsistentHoriz(dst);
	if (_needXYConv) {
		_tr = ProjPJWrapper(proj_create_crs_to_crs_from_pj(ProjContextByThread::get(), src.getPtr(), dst.getPtr(), nullptr, nullptr));
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
}
