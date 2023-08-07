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
			throw InvalidRasterFileException("Error reading " + filename + " as an alignment");
		}
		alignmentInitFromGDALRaster(wgd, getGeoTrans(wgd, filename));
		checkValidAlignment();
	}

	Extent Alignment::alignExtent(const Extent& e, const SnapType snap) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in alignExtent");
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
			throw CRSMismatchException("CRS mismatch in cellsFromExtent");
		}

		std::vector<cell_t> out{};
		if (!overlaps(e)) {
			return out;
		}
		Extent alignE = alignExtent(e, snap);
		alignE = cropExtent(alignE, *this);
		if (alignE.xmin() == alignE.xmax() || alignE.ymin() == alignE.ymax()) {
			return out;
		}

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

	bool Alignment::isSameAlignment(const Alignment& a) const {
		bool consistent = true;
		consistent = consistent && ((Extent)(*this) == (Extent)a);
		consistent = consistent && (_nrow == a.nrow());
		consistent = consistent && (_ncol == a.ncol());
		consistent = consistent && _crs.isConsistentHoriz(a.crs());
		return consistent;
	}

	Alignment::RowColExtent Alignment::rowColExtent(const Extent& e, SnapType snap) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in rowColExtent");
		}
		if (!overlaps(e)) {
			throw OutsideExtentException("Outside extent in rowColExtent");
		}
		auto snapE = alignExtent(e, snap);
		snapE = cropExtent(snapE, *this);

		//debuffering the extent slightly to correct for floating point imprecision
		snapE = Extent(snapE.xmin() + xres() / 10., snapE.xmax() - xres() / 10., snapE.ymin() + yres() / 10., snapE.ymax() - yres() / 10.);

		auto rc = RowColExtent();
		if (snapE.ymax() >= _ymax) {
			rc.minrow = 0;
		}
		else {
			rc.minrow = rowFromY(snapE.ymax());
		}
		if (snapE.ymin() <= _ymin) {
			rc.maxrow = _nrow;
		}
		else {
			rc.maxrow = rowFromY(snapE.ymin());
		}
		if (snapE.xmin() <= _xmin) {
			rc.mincol = 0;
		}
		else {
			rc.mincol = colFromX(snapE.xmin());
		}
		if (snapE.xmax() >= _xmax) {
			rc.maxcol = _ncol;
		}
		else {
			rc.maxcol = colFromX(snapE.xmax());
		}
		if (rc.maxrow < rc.minrow) {
			rc.maxrow = rc.minrow;
		}
		if (rc.maxcol < rc.mincol) {
			rc.maxcol = rc.mincol;
		}
		return rc;
	}

	void Alignment::checkValidAlignment() {
		checkValidExtent();
		if (_nrow < 0 || _ncol < 0 || _xres <= 0 || _yres <= 0) {
			throw InvalidAlignmentException("Ncol, nrow, xres, or yres less than 0");
		}
		if ((_xmax - _xmin) / _ncol - _xres > LAPIS_EPSILON) {
			throw InvalidAlignmentException("Extent, ncol, and xres not consistent");
		}
		if ((_ymax - _ymin) / _nrow - _yres > LAPIS_EPSILON) {
			throw InvalidAlignmentException("Extent, nrow, and yres not consistent");
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

	Alignment cropAlignment(const Alignment& a, const Extent& e, SnapType snap) {
		if (!a.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in cropAlignment");
		}
		if (!a.overlaps(e)) {
			throw OutsideExtentException("Outside extent in cropAlignment");
		}
		Extent cropE = a.alignExtent(e, snap);
		cropE = cropExtent(cropE, a);
		rowcol_t nrow = (rowcol_t)std::round((cropE.ymax() - cropE.ymin()) / a.yres());
		rowcol_t ncol = (rowcol_t)std::round((cropE.xmax() - cropE.xmin()) / a.xres());
		return Alignment(cropE.xmin(), cropE.ymin(), nrow, ncol, a.xres(), a.yres(), a.crs());
	}

	Alignment extendAlignment(const Alignment& a, const Extent& e, SnapType snap) {
		if (!a.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException();
		}
		if (e.xmin() >= a.xmin() && e.xmax() <= a.xmax() && e.ymin() >= a.ymin() && e.ymax() <= a.ymax()) {
			return a;
		}
		Extent extE = extendExtent(e, a);
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
		return Alignment(extE.xmin(),extE.ymin(), a.nrow() + postrows + prerows, a.ncol() + postcols + precols,a.xres(),a.yres(),a.crs());
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

	Alignment Alignment::transformAlignment(const CoordRef& crs) const {

		if (_crs.isConsistentHoriz(crs)) {
			Alignment out = *this;
			out.defineCRS(crs);
			return out;
		}

		CoordTransform crt{ _crs,crs };

		//the strategy here is to transform the four corners of the extent, taking mins/maxes, and setting xres and yres to whatever they need to be to preserve nrow/ncol
		//if xres and yres are equal in *this, and come out to within an epsilon of equal, they'll be set to their average to preserve square pixels
		CoordXY lowerLeft = crt.transformSingleXY(xmin(), ymin());
		CoordXY lowerRight = crt.transformSingleXY(xmax(), ymin());
		CoordXY upperLeft = crt.transformSingleXY(xmin(), ymax());
		CoordXY upperRight = crt.transformSingleXY(xmax(), ymax());

		coord_t newxmin = std::min(lowerLeft.x, upperLeft.x);
		coord_t newxmax = std::max(lowerRight.x, upperRight.x);
		coord_t newymin = std::min(lowerLeft.y, lowerRight.y);
		coord_t newymax = std::max(upperLeft.y, upperRight.y);

		//these can happen if you switch from a CRS where the ymax is in the north to one where the ymax is in the south
		//(or vice versa, or similarly for east/west)
		if (newymin > newymax)
			std::swap(newymin, newymax);
		if (newxmin > newxmax)
			std::swap(newxmin, newxmax);
			
		coord_t newxres = ncol() ? (newxmax - newxmin) / ncol() : 1;
		coord_t newyres = nrow() ? (newymax - newymin) / nrow() : 1;

		if (xres() == yres() && std::abs(newxres - newyres) < LAPIS_EPSILON) {
			newxres = newyres = (newxres + newyres) / 2.;
		}

		return Alignment(newxmin, newymin, nrow(), ncol(), newxres, newyres, crs);
	}

	bool Alignment::consistentAlignment(const Alignment& a) const {

		auto closeEnough = [](coord_t a, coord_t b) {
			return std::abs(a - b) < LAPIS_EPSILON;
		};

		bool consistent = true;

		consistent = consistent && crs().isConsistentHoriz(a.crs());

		consistent = consistent && closeEnough(xres(), a.xres());
		consistent = consistent && closeEnough(yres(), a.yres());

		//float imprecisions can cause one alignment to have an origin near 0 and the other to have an origin near its cellsize without meaningful difference
		consistent = consistent && (closeEnough(xOrigin(), a.xOrigin()) || closeEnough(xres() - xOrigin(), a.xOrigin()));
		consistent = consistent && (closeEnough(yOrigin(), a.yOrigin()) || closeEnough(yres() - yOrigin(), a.yOrigin()));

		return consistent;
	}
}
