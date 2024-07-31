#pragma once
#ifndef lapis_raster_h
#define lapis_raster_h

#include"gis_pch.hpp"
#include"Alignment.hpp"

namespace lapis {

	enum class ExtractMethod {
		near, bilinear
	};

	template<class T>
	using RastData = xtl::xoptional_vector<T>;

	template<class T>
	class Raster : public Alignment {
	public:

		Raster() : Alignment(), _data() {}
		virtual ~Raster() noexcept = default;

		//creates a raster from the given alignment, and fills it with missing values
		explicit Raster(const Alignment& a) : Alignment(a) {
			_data.resize(ncell());
		}

		//Constructs a raster from a GDAL-readable file.
		Raster(const std::string& filename, const int band = 1);

		//Constructs a raster from a GDAL-readable file, while only reading data that falls within the specified extent
		Raster(const std::string& filename, const Extent& e, SnapType snap, const int band = 1);

		//disallow copy and move constructors from rasters with different templates. This will call the Raster(const Alignment& a) signature, which is very confusing
		//by deleting them, that just won't even compile--do an explicit cast to Alignment if you want that behavior
		template<class S>
		Raster(const Raster<S>& r) = delete;
		template<class S>
		Raster(Raster<S>&& r) = delete;
		template<class S>
		Raster<T>& operator=(const Raster<S>& r) = delete;
		template<class S>
		Raster<T>& operator=(Raster<S>&& r) = delete;

		Raster(const Raster<T>& r) = default;
		Raster(Raster<T>&& r) noexcept {
			*this = std::move(r);
		}
		Raster<T>& operator=(const Raster<T>& r) = default;
		Raster<T>& operator=(Raster<T>&& r) noexcept {
			_data = std::move(r._data);
			_crs = std::move(r._crs);
			_xmin = r._xmin; r._xmin = 0;
			_xmax = r._xmax; r._xmax = 0;
			_ymin = r._ymin; r._ymin = 0;
			_ymax = r._ymax; r._ymax = 0;
			_xres = r._xres; r._xres = 1;
			_yres = r._yres; r._yres = 1;
			_ncol = r._ncol; r._ncol = 0;
			_nrow = r._nrow; r._nrow = 0;
			return *this;
		}

		//Methods to access values. As usual, operator[] does no bounds checking. The other three do check.
		auto atCell(const cell_t cell) {
			_checkCell(cell);
			return (*this)[cell];
		}
		auto atRC(const rowcol_t row, const rowcol_t col) {
			return (*this)[cellFromRowCol(row, col)];
		}
		auto atXY(const coord_t x, const coord_t y) {
			return (*this)[cellFromXY(x, y)];
		}

		const auto atCell(const cell_t cell) const {
			_checkCell(cell);
			return (*this)[cell];
		}
		const auto atRC(const rowcol_t row, const rowcol_t col) const {
			return (*this)[cellFromRowCol(row, col)];
		}
		const auto& atXY(const coord_t x, const coord_t y) const {
			return (*this)[cellFromXY(x, y)];
		}


		//Versions of atRC, and atXY that don't bother with bounds checking, and thus have minimal overhead
		//only use if you're completely confident that you don't need bounds checking
		auto atRCUnsafe(const rowcol_t row, const rowcol_t col) {
			return (*this)[cellFromRowColUnsafe(row, col)];
		}
		const auto atRCUnsafe(const rowcol_t row, const rowcol_t col) const {
			return (*this)[cellFromRowColUnsafe(row, col)];
		}
		auto atXYUnsafe(const coord_t x, const coord_t y) {
			return (*this)[cellFromRowColUnsafe(rowFromYUnsafe(y), colFromXUnsafe(x))];
		}
		const auto atXYUnsafe(const coord_t x, const coord_t y) const {
			return (*this)[cellFromRowColUnsafe(rowFromYUnsafe(y), colFromXUnsafe(x))];
		}
		const auto operator[](const cell_t cell) const {
			return _data[cell];
		}
		auto operator[](const cell_t cell) {
			return _data[cell];
		}
		auto atCellUnsafe(const cell_t cell) {
			return _data[cell];
		}
		const auto atCellUnsafe(const cell_t cell) const {
			return _data[cell];
		}

		//This function is similar to atXY, but returns nodata if the point is outside of the extent
		//In addition, you can specify an extraction method:
		//near uses nearest neighbor, and bilinear does a bilinear interpolation of the four nearest points
		//bilinear extraction will do its best as long as one of the four points has data, but will return nodata if the point is entirely outside of the extent
		//The interpolation is done using the math of the T type, so bilinear interpolations of integral types may produce unintuitive results
		xtl::xoptional<T> extract(const coord_t x, const coord_t y, const ExtractMethod method) const {
			if (!contains(x, y)) {
				return xtl::missing<T>();
			}
			if (method == ExtractMethod::near) {
				//we just checked the bounds a few lines up so unsafe is okay
				return atXYUnsafe(x, y);
			}
			if (method == ExtractMethod::bilinear) {
				rowcol_t colToLeft = (rowcol_t)std::floor((x - xmin() - xres() / 2) / xres());
				rowcol_t rowAbove = (rowcol_t)std::floor((ymax() - y - yres() / 2) / yres());

				//in this case, we explicitly want the behavior where we do the math even if the value is out of bounds
				coord_t xToLeft = xFromColUnsafe(colToLeft);
				coord_t xToRight = xToLeft + _xres;
				coord_t yAbove = yFromRowUnsafe(rowAbove);
				coord_t yBelow = yAbove - _yres;

				auto ll = extract(xToLeft, yBelow, ExtractMethod::near);
				auto lr = extract(xToRight, yBelow, ExtractMethod::near);
				auto ul = extract(xToLeft, yAbove, ExtractMethod::near);
				auto ur = extract(xToRight, yAbove, ExtractMethod::near);

				auto linearInterp = [](const coord_t lowcoord, const coord_t highcoord, const coord_t thiscoord,
					const xtl::xoptional<T> lowvalue, const xtl::xoptional<T> highvalue)
					->xtl::xoptional<T> {

					xtl::xoptional interp = xtl::missing<T>();
					if (lowvalue.has_value() && highvalue.has_value()) {
						interp = (highcoord - thiscoord) / (highcoord - lowcoord) * lowvalue + (thiscoord - lowcoord) / (highcoord - lowcoord) * highvalue;
					}
					else if (lowvalue.has_value() && !highvalue.has_value()) {
						interp = lowvalue;
					}
					else if (!lowvalue.has_value() && highvalue.has_value()) {
						interp = highvalue;
					}
					return interp;
				};

				//linear interp between 
				xtl::xoptional<T> directBelow = linearInterp(xToLeft, xToRight, x, ll, lr);
				xtl::xoptional<T> directAbove = linearInterp(xToLeft, xToRight, x, ul, ur);
				return linearInterp(yBelow, yAbove, y, directBelow, directAbove);
			}
			return xtl::missing<T>(); //shouldn't ever happen but makes the compiler happy
		}

		//Writes the Raster object to the harddrive. Missing values will be replaced by naValue. It's up to the user to make sure the driver and the file extension correspond.
		//You can specify the datatype of the file, or leave it as GDT_Unknown to choose the one that corresponds to the template of the raster object.
		void writeRaster(const std::string& file, const std::string driver = "GTiff", const T navalue = std::numeric_limits<T>::lowest(), GDALDataType gdt = GDT_Unknown);

		//This function produces a new raster, with alignment a, where the values are what you get by extracting at the cell centers of this
		Raster<T> resample(const Alignment& a, ExtractMethod method) const;
		//This is resample, but the alignment is the result of transformAlignment
		Raster<T> transformRaster(const CoordRef& crs, ExtractMethod method) const;

		//This function takes another raster whose alignment is consistent with this one (same cellsize, origin, crs)
		//For any overlapping cells where this is nodata, and the other raster isn't, replaces the value with the new value
		//VALUECOMBINER is a functor that can take two Ts and return a T. In cases where both rasters have a value, the functor will be used to set the value
		//the value in this is first, and the value in other is second, so you can distinguish them if needed
		template<typename VALUECOMBINER>
		void overlay(const Raster<T>& other, const VALUECOMBINER& combiner);

		//this is similar to overlay, but instead of using a functor to decide the new value, the value is taken from whichever raster the cell is further from the edge of
		//this is useful for combining two rasters that should be equal normally, but have edge effects
		void overlayInside(const Raster<T>& other);

		//returns true if has_value is true for any cell; false otherwise
		bool hasAnyValue() const;

		template<class S>
		void mask(const Raster<S>& m);

		//basic element-wise artihmetic
		template<class S>
		Raster<T>& operator+=(const Raster<S>& rhs);
		template<class S>
		Raster<T>& operator+=(const S rhs);
		template<class S>
		Raster<T>& operator-=(const Raster<S>& rhs);
		template<class S>
		Raster<T>& operator-=(const S rhs);
		template<class S>
		Raster<T>& operator*=(const Raster<S>& rhs);
		template<class S>
		Raster<T>& operator*=(const S rhs);
		template<class S>
		Raster<T>& operator/=(const Raster<S>& rhs);
		template<class S>
		Raster<T>& operator/=(const S rhs);

	private:
		RastData<T> _data;

		static GDALDataType GDT() {
			if (std::is_same<T, double>::value) {
				return GDT_Float64;
			}
			if (std::is_same<T, float>::value) {
				return GDT_Float32;
			}
			if (std::is_same<T, std::int16_t>::value) {
				return GDT_Int16;
			}
			if (std::is_same<T, std::int32_t>::value) {
				return GDT_Int32;
			}
			if (std::is_same<T, std::int64_t>::value) {
				return GDT_Int64;
			}
			if (std::is_same<T, std::uint16_t>::value) {
				return GDT_UInt16;
			}
			if (std::is_same<T, std::uint32_t>::value) {
				return GDT_UInt32;
			}
			if (std::is_same<T, std::uint64_t>::value) {
				return GDT_UInt64;
			}
			return GDT_Unknown;
		}
	};

	template<class T>
	inline bool operator==(const Raster<T>& lhs, const Raster<T>& rhs) {
		bool equal = true;
		equal = equal && ((Alignment)lhs == (Alignment)rhs);
		if (!equal) {
			return false;
		}
		for (cell_t cell = 0; cell < lhs.ncell(); ++cell) {
			equal = equal && (lhs[cell].has_value() == rhs[cell].has_value());
			if (lhs[cell].has_value() && rhs[cell].has_value()) {
				equal = equal && (lhs[cell].value() == rhs[cell].value());
			}
			if (!equal) {
				return false;
			}
		}
		return true;
	}
	template<class T>
	inline bool operator!=(const Raster<T>& lhs, const Raster<T>& rhs) {
		return !(lhs == rhs);
	}

	//Like the crop and extend functions for Alignment, but also copies in the data from the given raster to the corresponding x/y locations in the new raster
	template<class T>
	inline Raster<T> cropRaster(const Raster<T>& r, const Extent& e, const SnapType snap) {
		if (!r.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in cropRaster");
		}
		Extent snapE = r.alignExtent(e, snap);
		Alignment a = cropAlignment((Alignment)r, snapE, snap);
		if (snapE == (Extent)r) {
			return r;
		}
		Raster<T> out = Raster<T>(a);
		auto rc = r.rowColExtent(snapE, snap);

		for (rowcol_t row = 0; row < out.nrow(); ++row) {
			for (rowcol_t col = 0; col < out.ncol(); ++col) {
				auto vout = out.atRCUnsafe(row, col);
				auto vr = r.atRCUnsafe(row + rc.minrow, col + rc.mincol);
				vout.value() = vr.value();
				vout.has_value() = vr.has_value();
			}
		}
		return out;
	}

	template<class T>
	inline Raster<T> extendRaster(const Raster<T>& r, const Extent& e, const SnapType snap) {
		if (!r.crs().isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in extendRaster");
		}
		Extent snapE = r.alignExtent(e, snap);
		Alignment a = extendAlignment((Alignment)r, snapE, snap);
		if (snapE == (Extent)r) {
			return r;
		}
		Raster<T> out = Raster<T>(a);

		rowcol_t precols = 0, postcols = 0, prerows = 0, postrows = 0;
		if (snapE.xmin() < r.xmin()) {
			precols = (rowcol_t)round((r.xmin() - snapE.xmin()) / r.xres());
		}
		if (snapE.xmax() > r.xmax()) {
			postcols = (rowcol_t)round((snapE.xmax() - r.xmax()) / r.xres());
		}
		if (snapE.ymin() < r.ymin()) {
			postrows = (rowcol_t)round((r.ymin() - snapE.ymin()) / r.yres());
		}
		if (snapE.ymax() > r.ymax()) {
			prerows = (rowcol_t)round((snapE.ymax() - r.ymax()) / r.yres());
		}
		for (rowcol_t row = 0; row < out.nrow(); ++row) {
			for (rowcol_t col = 0; col < out.ncol(); ++col) {
				if (row < prerows || col < precols || row >= r.nrow() + prerows || col >= r.ncol() + precols) {
					continue; //the default fill for a raster constructed from an alignment is NA, so no need to modify the value
				}
				auto outv = out.atRCUnsafe(row, col);
				auto rv = r.atRCUnsafe(row - prerows, col - precols);
				outv.value() = rv.value();
				outv.has_value() = rv.has_value();
			}
		}
		return out;
	}

	template<class T, class S>
	inline Raster<T> operator+(Raster<T> lhs, const S rhs) {
		lhs += rhs;
		return lhs;
	}
	template<class T, class S>
	inline Raster<T> operator+(const S lhs, Raster<T> rhs) {
		for (cell_t cell = 0; cell < rhs.ncell(); ++cell) {
			rhs[cell].value() = lhs + rhs[cell].value();
		}
		return rhs;
	}
	template<class T, class S>
	inline auto operator+(const Raster<T>& lhs, const Raster<S>& rhs)->Raster<decltype(T() + S())> {
		if (!lhs.isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator+");
		}
		using outtype = decltype(T() + S());
		Raster<outtype> out{ (Alignment)lhs };
		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			out[cell].has_value() = lhs[cell].has_value() && rhs[cell].has_value();
			out[cell].value() = lhs[cell].value() + rhs[cell].value();
		}
		return out;
	}

	template<class T, class S> inline Raster<T> operator-(Raster<T> lhs, const S rhs) {
		lhs -= rhs;
		return lhs;
	}
	template<class T, class S>
	inline Raster<T> operator-(const S lhs, Raster<T> rhs) {
		for (cell_t cell = 0; cell < rhs.ncell(); ++cell) {
			rhs[cell].value() = lhs - rhs[cell].value();
		}
		return rhs;
	}
	template<class T, class S>
	inline auto operator-(const Raster<T>& lhs, const Raster<S>& rhs)->Raster<decltype(T() - S())>
	{
		if (!lhs.isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator-");
		}
		using outtype = decltype(T() - S());
		Raster<outtype> out{ (Alignment)lhs };
		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			out[cell].has_value() = lhs[cell].has_value() && rhs[cell].has_value();
			out[cell].value() = lhs[cell].value() - rhs[cell].value();
		}
		return out;
	}

	template<class T, class S>
	inline Raster<T> operator*(Raster<T> lhs, const S rhs) {
		lhs *= rhs; return lhs;
	}
	template<class T, class S>
	inline Raster<T> operator*(const S lhs, Raster<T> rhs) {
		for (cell_t cell = 0; cell < rhs.ncell(); ++cell) {
			rhs[cell].value() = lhs * rhs[cell].value();
		}
		return rhs;
	}
	template<class T, class S>
	inline auto operator*(const Raster<T>& lhs, const Raster<S>& rhs)->Raster<decltype(T()* S())> {
		if (!lhs.isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator*");
		}
		using outtype = decltype(T()* S());
		Raster<outtype> out{ (Alignment)lhs };
		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			out[cell].has_value() = lhs[cell].has_value() && rhs[cell].has_value();
			out[cell].value() = lhs[cell].value() * rhs[cell].value();
		}
		return out;
	}

	template<class T, class S>
	inline Raster<T> operator/(Raster<T> lhs, const S rhs) {
		lhs /= rhs;
		return lhs;
	}
	template<class T, class S>
	inline Raster<T> operator/(const S lhs, Raster<T> rhs) {
		for (cell_t cell = 0; cell < rhs.ncell(); ++cell) {
			if (rhs[cell].value() == 0) {
				rhs[cell].has_value() = false;
			}
			else {
				rhs[cell].value() = lhs / rhs[cell].value();
			}
		}
		return rhs;
	}
	template<class T, class S>
	inline auto operator/(const Raster<T>& lhs, const Raster<S>& rhs)->Raster<decltype(T() / S())> {
		if (!lhs.isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator/");
		}
		using outtype = decltype(T() / S());
		Raster<outtype> out{ (Alignment)lhs };
		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			if (rhs[cell].value() == 0) {
				out[cell].has_value() = false;
			}
			else {
				out[cell].has_value() = lhs[cell].has_value() && rhs[cell].has_value();
				out[cell].value() = lhs[cell].value() / rhs[cell].value();
			}
		}
		return out;
	}

	template<class T>
	inline std::ostream& operator<<(std::ostream& os, const Raster<T>& r) {
		os << "RASTER: " << typeid(T).name();
		os << (Alignment)r;
		return os;
	}


	template<class T>
	Raster<T>::Raster(const std::string& filename, const int band) {

		UniqueGdalDataset wgd = rasterGDALWrapper(filename);
		if (!wgd) {
			throw InvalidRasterFileException("Unable to opent " + filename + " as a raster");
		}
		alignmentInitFromGDALRaster(wgd, getGeoTrans(wgd, filename));
		checkValidAlignment();

		_data.resize(ncell());

		GDALRasterBand* rBand = wgd->GetRasterBand(band);
		double naValue = rBand->GetNoDataValue();
		rBand->RasterIO(GF_Read, 0, 0, _ncol, _nrow, _data.value().data(), _ncol, _nrow, GDT(), 0, 0);

		for (cell_t cell = 0; cell < ncell(); ++cell) {
			double asDouble = _data.value()[cell];
			if (asDouble != naValue && !(naValue < -2000000 && asDouble < -2000000) && !std::isnan(asDouble)) {
				_data.has_value()[cell] = true;
			}
		}
	}

	template<class T>
	Raster<T>::Raster(const std::string& filename, const Extent& e, SnapType snap, const int band) {

		Alignment a = Alignment(filename);
		Extent snapE = a.alignExtent(e, snap);
		snapE = cropExtent(snapE, a);
		auto rc = a.rowColExtent(snapE, snap);

		_crs = a.crs();
		_xmin = snapE.xmin();
		_xmax = snapE.xmax();
		_ymin = snapE.ymin();
		_ymax = snapE.ymax();
		_nrow = rc.maxrow - rc.minrow + 1;
		_ncol = rc.maxcol - rc.mincol + 1;
		_xres = a.xres();
		_yres = a.yres();
		checkValidAlignment();

		_data.resize(ncell());
		UniqueGdalDataset wgd = rasterGDALWrapper(filename);
		if (!wgd) {
			throw InvalidRasterFileException("Unable to open " + filename + " as a raster");
		}
		GDALRasterBand* rBand = wgd->GetRasterBand(band);
		T naValue = (T)(rBand->GetNoDataValue());
		rBand->RasterIO(GF_Read, rc.mincol, rc.minrow, _ncol, _nrow, _data.value().data(), _ncol, _nrow, GDT(), 0, 0);
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			double asDouble = _data.value()[cell];
			if (asDouble != naValue && !(naValue < -2000000 && asDouble < -2000000) && !std::isnan(asDouble)) {
				_data.has_value()[cell] = true;
			}
		}
	}

	template<class T>
	void Raster<T>::writeRaster(const std::string& file, const std::string driver, const T navalue, GDALDataType dataType) {
		if (dataType == GDT_Unknown) {
			dataType = GDT();
		}
		UniqueGdalDataset wgd = gdalCreateWrapper(driver,file,ncol(),nrow(),dataType);
		if (!wgd) {
			throw InvalidRasterFileException("Unable to open " + file + " as a raster");
		}
		std::array<double, 6> gt = { _xmin, _xres,0,_ymax,0,-(_yres) };
		wgd->SetGeoTransform(gt.data());
		wgd->SetProjection(_crs.getCompleteWKT().c_str());
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			if (!_data[cell].has_value()) {
				_data[cell].value() = navalue;
			}
		}
		auto band = wgd->GetRasterBand(1);
		band->SetNoDataValue((double)navalue);
		band->RasterIO(GF_Write, 0, 0, _ncol, _nrow, _data.value().data(), _ncol, _nrow, dataType, 0, 0);
	}

	template<class T>
	Raster<T> Raster<T>::resample(const Alignment& a, ExtractMethod method) const {
		Raster<T> out{ a };
		CoordTransform transform{ a.crs(),crs() };
		for (cell_t cell = 0; cell < out.ncell(); ++cell) {
			CoordXY xy = transform.transformSingleXY(out.xFromCellUnsafe(cell), out.yFromCellUnsafe(cell));
			auto v = this->extract(xy.x, xy.y, method);
			out[cell].has_value() = v.has_value();
			out[cell].value() = v.value();
		}
		return out;
	}
	template<class T>
	Raster<T> Raster<T>::transformRaster(const CoordRef& crs, ExtractMethod method) const {
		if (crs.isConsistentHoriz(this->crs())) {
			return *this;
		}
		return resample(this->transformAlignment(crs), method);
	}

	template<class T>
	template<typename VALUECOMBINER>
	void Raster<T>::overlay(const Raster<T>& other, const VALUECOMBINER& combiner) {
		if (!consistentAlignment(other)) {
			throw AlignmentMismatchException{ "Alignment mismatch in overlay" };
		}
		if (!overlaps(other)) {
			return;
		}

		RowColExtent rcExt = rowColExtent(other, SnapType::near); //snap type shouldn't matter with consistent alignments, but 'near' will correct for floating point issues

		for (rowcol_t row = rcExt.minrow; row <= rcExt.maxrow; ++row) {
			for (rowcol_t col = rcExt.mincol; col <= rcExt.maxcol; ++col) {
				const auto otherValue = other.atRCUnsafe(row - rcExt.minrow, col - rcExt.mincol);
				auto thisValue = atRCUnsafe(row, col);
				if (!otherValue.has_value()) {
					continue;
				}
				if (!thisValue.has_value()) {
					thisValue.has_value() = true;
					thisValue.value() = otherValue.value();
					continue;
				}
				thisValue.value() = combiner(thisValue.value(), otherValue.value());
			}
		}
	}

	template<class T>
	void Raster<T>::overlayInside(const Raster<T>& other) {
		if (!consistentAlignment(other)) {
			throw AlignmentMismatchException("Alignment mismatch in overlayInside");
		}
		if (!overlaps(other)) {
			return;
		}

		RowColExtent rcExt = rowColExtent(other, SnapType::near); //snap type doesn't matter with consistent alignments, but 'near' will correct for floating point issues

		auto distFromEdge = [](const Alignment& a, rowcol_t row, rowcol_t col) {
			rowcol_t dist = std::min(col, row);
			dist = std::min(dist, a.ncol() - 1 - col);
			dist = std::min(dist, a.nrow() - 1 - row);
			return dist;
		};

		for (rowcol_t row = rcExt.minrow; row <= rcExt.maxrow; ++row) {
			for (rowcol_t col = rcExt.mincol; col <= rcExt.maxcol; ++col) {
				const auto otherValue = other.atRCUnsafe(row - rcExt.minrow, col - rcExt.mincol);
				auto thisValue = atRCUnsafe(row, col);
				if (!otherValue.has_value()) {
					continue;
				}
				if (!thisValue.has_value()) {
					thisValue.has_value() = true;
					thisValue.value() = otherValue.value();
					continue;
				}
				
				rowcol_t thisDistFromEdge = distFromEdge(*this, row, col);
				rowcol_t otherDistFromEdge = distFromEdge(other, row - rcExt.minrow, col - rcExt.mincol);
				thisValue.value() = thisDistFromEdge <= otherDistFromEdge ? otherValue.value() : thisValue.value();
			}
		}
	}

	template<class T>
	bool Raster<T>::hasAnyValue() const {
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			if (atCellUnsafe(cell).has_value()) {
				return true;
			}
		}
		return false;
	}

	template<class T>
	template<class S>
	void Raster<T>::mask(const Raster<S>& m) {
		if (!isSameAlignment(m)) {
			throw AlignmentMismatchException("Alignment mismatch in mask");
		}
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			if (!m[cell].has_value()) {
				atCellUnsafe(cell).has_value() = false;
			}
		}
	}


	template<class T> template<class S>
	Raster<T>& Raster<T>::operator+=(const Raster<S>& rhs) {
		if (!isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator+=");
		}
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].has_value() = _data[cell].has_value() && rhs[cell].has_value();
			_data[cell].value() += rhs[cell].value();
		} return *this;
	}
	template<class T> template<class S>
	Raster<T>& Raster<T>::operator+=(const S rhs) {
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].value() += rhs;
		}
		return *this;
	}

	template<class T> template<class S>
	Raster<T>& Raster<T>::operator-=(const Raster<S>& rhs) {
		if (!isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator-=");
		}
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].has_value() = _data[cell].has_value() && rhs[cell].has_value();
			_data[cell].value() -= rhs[cell].value();
		}
		return *this;
	}
	template<class T> template<class S>
	Raster<T>& Raster<T>::operator-=(const S rhs) {
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].value() -= rhs;
		} return *this;
	}

	template<class T> template<class S>
	Raster<T>& Raster<T>::operator*=(const Raster<S>& rhs) {
		if (!isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator*=");
		}
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].has_value() = _data[cell].has_value() && rhs[cell].has_value();
			_data[cell].value() *= rhs[cell].value();
		} return *this;
	}
	template<class T> template<class S>
	Raster<T>& Raster<T>::operator*=(const S rhs) {
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			_data[cell].value() *= rhs;
		}
		return *this;
	}

	template<class T> template<class S>
	Raster<T>& Raster<T>::operator/=(const Raster<S>& rhs) {
		if (!isSameAlignment(rhs)) {
			throw AlignmentMismatchException("Alignment mismatch in operator/=");
		}
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			if (rhs[cell].value() == 0) {
				_data[cell].has_value() = false;
			}
			else {
				_data[cell].has_value() = _data[cell].has_value() && rhs[cell].has_value();
				_data[cell].value() /= rhs[cell].value();
			}
		}
		return *this;
	}
	template<class T> template<class S>
	Raster<T>& Raster<T>::operator/=(const S rhs) {
		for (cell_t cell = 0; cell < ncell(); ++cell) {
			if (rhs == 0) {
				_data[cell].has_value() = false;
			}
			else {
				_data[cell].value() /= rhs;
			}
		}
		return *this;
	}
}

#endif