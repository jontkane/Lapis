#include"gis_pch.hpp"
#include"Alignment.hpp"


namespace lapis {
	Alignment::Alignment(const Extent& e, const coord_t xorigin, const coord_t yorigin, const coord_t xres, const coord_t yres) :
		_xres(xres), _yres(yres) {
		
		_crs = CoordRef(e.crs());
		//getting the origins to be positive numbers less than the res
		coord_t xoriginmod = std::fmod(xorigin, xres);
		if (xoriginmod < 0) {
			xoriginmod += _xres;
		}
		coord_t yoriginmod = std::fmod(yorigin, yres);
		if (yoriginmod < 0) {
			yoriginmod += _yres;
		}

		//indexing the columns with the positive one closest to the Y-axis as 0, which one is the first to touch the extent?
		coord_t firstcol = std::floor((e.xmin() - xoriginmod) / _xres);
		_xmin = xoriginmod + firstcol * _xres;

		//same but for rows
		coord_t firstrow = std::floor((e.ymin() - yoriginmod) / _yres);
		_ymin = yoriginmod + firstrow * _yres;

		_ncol = (rowcol_t)std::ceil((e.xmax() - _xmin) / _xres);
		_nrow = (rowcol_t)std::ceil((e.ymax() - _ymin) / _yres);

		_xmax = _xmin + _ncol * _xres;
		_ymax = _ymin + _nrow * _yres;

		checkValidAlignment();
	}

	Alignment::Alignment(const std::string& filename) {
		GDALDatasetWrapper wgd = rasterGDALWrapper(filename);
		if (wgd.isNull()) {
			throw InvalidRasterFileException(filename);
		}
		alignmentInitFromGDALRaster(wgd, getGeoTrans(wgd, filename));
		checkValidAlignment();
	}

	Extent Alignment::alignExtent(const Extent& e, const SnapType snap) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}

		coord_t xori = xOrigin();
		coord_t yori = yOrigin();
		coord_t xmin = 0, xmax = 0, ymin = 0, ymax = 0;
		if (snap == SnapType::near) {
			xmin = snapExtent(e.xmin(), _xres, xori, round);
			xmax = snapExtent(e.xmax(), _xres, xori, round);
			ymin = snapExtent(e.ymin(), _yres, yori, round);
			ymax = snapExtent(e.ymax(), _yres, yori, round);
		}
		else if (snap == SnapType::out) {
			xmin = snapExtent(e.xmin(), _xres, xori, floor);
			xmax = snapExtent(e.xmax(), _xres, xori, ceil);
			ymin = snapExtent(e.ymin(), _yres, yori, floor);
			ymax = snapExtent(e.ymax(), _yres, yori, ceil);
		}
		else if (snap == SnapType::in) {
			xmin = snapExtent(e.xmin(), _xres, xori, ceil);
			xmax = snapExtent(e.xmax(), _xres, xori, floor);
			ymin = snapExtent(e.ymin(), _yres, yori, ceil);
			ymax = snapExtent(e.ymax(), _yres, yori, floor);
		}
		else if (snap == SnapType::ll) {
			xmin = snapExtent(e.xmin(), _xres, xori, floor);
			xmax = snapExtent(e.xmax(), _xres, xori, floor);
			ymin = snapExtent(e.ymin(), _yres, yori, floor);
			ymax = snapExtent(e.ymax(), _yres, yori, floor);
		}
		return Extent(xmin, xmax, ymin, ymax, _crs);
	}

	std::vector<cell_t> Alignment::cellsFromExtent(const Extent& e, const SnapType snap) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}

		std::vector<cell_t> out{};
		if (!overlaps(e)) {
			return out;
		}
		Extent alignE = alignExtent(e, snap);
		alignE = crop(alignE, *this);

		//Bringing the extent in slightly so you don't have to deal with the edge of the extent aligning with the edges of this object's cells.
		alignE = Extent(alignE.xmin() + 0.25 * _xres, alignE.xmax() - 0.25 * _xres,
			alignE.ymin() + 0.25 * _yres, alignE.ymax() - 0.25 * _yres);

		//The mins and maxes are *inclusive*
		rowcol_t mincol = colFromX(alignE.xmin());
		rowcol_t maxcol = colFromX(alignE.xmax());
		rowcol_t minrow = rowFromY(alignE.ymax());
		rowcol_t maxrow = rowFromY(alignE.ymin());
		out.reserve((size_t)(maxcol - mincol + 1) * (size_t)(maxrow - minrow + 1));
		for (rowcol_t r = minrow; r <= maxrow; ++r) {
			for (rowcol_t c = mincol; c <= maxcol; ++c) {
				out.push_back(cellFromRowCol(r, c));
			}
		}
		return out;
	}

	bool Alignment::consistentAlignment(const Alignment& a) const {
		bool consistent = true;
		consistent = consistent && ((Extent)(*this) == (Extent)a);
		consistent = consistent && (_nrow == a.nrow());
		consistent = consistent && (_ncol == a.ncol());
		consistent = consistent && _crs.isConsistentHoriz(a.crs());
		return consistent;
	}

	Alignment::RowColExtent Alignment::rowColExtent(const Extent& e, SnapType snap) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}
		if (!overlaps(e)) {
			throw OutsideExtentException();
		}
		auto snapE = alignExtent(e, snap);
		auto rc = RowColExtent();
		coord_t midx = (snapE.xmin() + snapE.xmax()) / 2;
		coord_t midy = (snapE.ymin() + snapE.ymax()) / 2;
		if (snapE.ymax() >= _ymax) {
			rc.minrow = 0;
		}
		else {
			rc.minrow = rowFromY(snapE.ymax());
		}
		if (!snapE.contains(midx, yFromRow(rc.minrow))) {
			rc.minrow += 1;
		}
		if (snapE.ymin() <= _ymin) {
			rc.maxrow = _nrow - 1;
		}
		else {
			rc.maxrow = rowFromY(snapE.ymin());
		}
		if (!snapE.contains(midx, yFromRow(rc.maxrow))) {
			rc.maxrow -= 1;
		}
		if (snapE.xmin() <= _xmin) {
			rc.mincol = 0;
		}
		else {
			rc.mincol = colFromX(snapE.xmin());
		}
		if (!snapE.contains(xFromCol(rc.mincol), midy)) {
			rc.mincol += 1;
		}
		if (snapE.xmax() >= _xmax) {
			rc.maxcol = _ncol - 1;
		}
		else {
			rc.maxcol = colFromX(snapE.xmax());
		}
		if (!snapE.contains(xFromCol(rc.maxcol), midy)) {
			rc.maxcol -= 1;
		}
		return rc;
	}

	void Alignment::checkValidAlignment() {
		checkValidExtent();
		if (_nrow < 0 || _ncol < 0 || _xres <= 0 || _yres <= 0) {
			throw InvalidAlignmentException("");
		}
		if ((_xmax - _xmin) / _ncol - _xres > LAPIS_EPSILON) {
			throw InvalidAlignmentException("");
		}
		if ((_ymax - _ymin) / _nrow - _yres > LAPIS_EPSILON) {
			throw InvalidAlignmentException("");
		}
	}

	void Alignment::alignmentInitFromGDALRaster(GDALDatasetWrapper& wgd, const std::array<double, 6>& geotrans) {
		//xmin, xres, xshear, ymax, yshear, yres
		extentInitFromGDALRaster(wgd, geotrans);
		_ncol = wgd->GetRasterXSize();
		_nrow = wgd->GetRasterYSize();
		_xres = std::abs(geotrans[1]);
		_yres = std::abs(geotrans[5]);
	}

	Alignment crop(const Alignment& a, const Extent& e, SnapType snap) {
		if (!a.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}
		if (!a.overlaps(e)) {
			throw OutsideExtentException();
		}
		Extent cropE = a.alignExtent(e, snap);
		cropE = crop(cropE, a);
		rowcol_t nrow = (rowcol_t)std::round((cropE.ymax() - cropE.ymin()) / a.yres());
		rowcol_t ncol = (rowcol_t)std::round((cropE.xmax() - cropE.xmin()) / a.xres());
		return Alignment(cropE, nrow, ncol);
	}

	Alignment extend(const Alignment& a, const Extent& e, SnapType snap) {
		if (!a.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}
		if (e.xmin() >= a.xmin() && e.xmax() <= a.xmax() && e.ymin() >= a.ymin() && e.ymax() <= a.ymax()) {
			return a;
		}
		Extent extE = extend(e, a);
		extE = a.alignExtent(extE, snap);
		rowcol_t precols = 0, postcols = 0, prerows = 0, postrows = 0;
		if (extE.xmin() < a.xmin()) {
			precols = (rowcol_t)round((a.xmin() - extE.xmin()) / a.xres());
		}
		if (extE.xmax() > a.xmax()) {
			postcols = (rowcol_t)round((extE.xmax() - a.xmax()) / a.xres());
		}
		if (extE.ymin() < a.ymin()) {
			postrows = (rowcol_t)round((a.ymin() - extE.ymin()) / a.yres());
		}
		if (extE.ymax() > a.ymax()) {
			prerows = (rowcol_t)round((extE.ymax() - a.ymax()) / a.yres());
		}
		return Alignment(extE, a.nrow() + postrows + prerows, a.ncol() + postcols + precols);
	}

	Alignment crop(const Alignment& a, const Extent& e) {
		return crop(a, e, SnapType::near);
	}

	Alignment extend(const Alignment& a, const Extent& e) {
		return extend(a, e, SnapType::near);
	}

	bool operator==(const Alignment& lhs, const Alignment& rhs) {
		bool equal = true;
		equal = equal && ((Extent)lhs == (Extent)rhs);
		equal = equal && (lhs.nrow() == rhs.nrow());
		equal = equal && (lhs.ncol() == rhs.ncol());
		//xres and yres being deducable from the extent, nrow, and ncol is one of the core guarantees of Alignment, so need to check it here
		return equal;
	}

	bool operator!=(const Alignment& lhs, const Alignment& rhs) {
		return !(lhs == rhs);
	}
}
