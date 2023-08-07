#include"gis_pch.hpp"
#include"GisExceptions.hpp"
#include"Extent.hpp"

namespace lapis {
	Extent::Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax) : _xmin(xmin), _xmax(xmax), _ymin(ymin), _ymax(ymax) {
		//checkValidExtent();
	}

	//A full definition of the extent, including crs information

	Extent::Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax, const CoordRef& crs) :
		_xmin(xmin), _xmax(xmax), _ymin(ymin), _ymax(ymax), _crs(crs) {
		checkValidExtent();
	}

	Extent::Extent(const coord_t xmin, const coord_t xmax, const coord_t ymin, const coord_t ymax, CoordRef&& crs) :
		_xmin(xmin), _xmax(xmax), _ymin(ymin), _ymax(ymax), _crs(std::move(crs)) {
		checkValidExtent();
	}

	//This function will, in order, attempt to read the file as a raster, a vector, and a las point cloud
	//Extent and crs information will be read from the file if possible

	Extent::Extent(const std::string& filename) {

		bool init = false;

		std::string ext = filename.substr(filename.size() - 4, 4);

		if (ext == ".las" || ext == ".laz") {
			LasIO las{ filename }; //This may throw an InvalidLasFileException, which I'm okay with because nobody is naming a valid raster or vector file .las
			_setFromLasIO(las);

			init = true;
		}

		if (!init) {
			GDALDatasetWrapper wgd = rasterGDALWrapper(filename);
			if (!wgd.isNull()) {
				extentInitFromGDALRaster(wgd, getGeoTrans(wgd, filename));
				init = true;
			}
		}

		if (!init) {
			GDALDatasetWrapper wgd = vectorGDALWrapper(filename);
			if (!wgd.isNull()) {
				OGREnvelope envelope{};
				auto layer = wgd->GetLayer(0);
				auto errcode = layer->GetExtent(&envelope);
				if (errcode != OGRERR_NONE) {
					throw InvalidVectorFileException("Unable to open " + filename + " as an extent");
				}
				_xmin = envelope.MinX;
				_xmax = envelope.MaxX;
				_ymin = envelope.MinY;
				_ymax = envelope.MaxY;

				auto ogrspat = layer->GetSpatialRef();
				GDALStringWrapper wkt{};
				ogrspat->exportToWkt(&wkt);
				_crs = CoordRef(std::string(wkt.ptr));

				init = true;
			}
		}
		if (!init) {
			throw std::runtime_error(filename + " is not a valid GIS file");
		}
		checkValidExtent();
	}

	Extent::Extent(const LasIO& las)
	{
		_setFromLasIO(las);
	}

	//return the internal objects

	const CoordRef& Extent::crs() const {
		return _crs;
	}

	coord_t Extent::xmin() const {
		return _xmin;
	}

	coord_t Extent::xmax() const {
		return _xmax;
	}

	coord_t Extent::ymin() const {
		return _ymin;
	}

	coord_t Extent::ymax() const {
		return _ymax;
	}

	//returns true if this extent has an overlap with non-zero area with the other extent
	bool Extent::overlaps(const Extent& e) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in overlaps");
		}
		return overlapsUnsafe(e);
	}

	bool Extent::overlapsUnsafe(const Extent& e) const {
		//the 'left' extent is the one with the lower xmin
		coord_t rightxmin = std::max(_xmin, e._xmin);
		coord_t leftxmax = _xmin <= e._xmin ? _xmax : e._xmax;
		bool xoverlap = leftxmax > rightxmin;
		//the 'bottom' extent is the one with the lower ymin
		coord_t topymin = std::max(_ymin, e._ymin);
		coord_t bottymax = _ymin <= e._ymin ? _ymax : e._ymax;
		bool yoverlap = bottymax > topymin;
		return xoverlap && yoverlap;
	}

	//returns true if the extents touch at all, even at a single point

	bool Extent::touches(const Extent& e) const {
		if (!_crs.isConsistentHoriz(e.crs())) {
			throw CRSMismatchException("CRS mismatch in touches");
		}
		//the 'left' extent is the one with the lower xmin
		coord_t rightxmin = std::max<coord_t>(_xmin, e._xmin);
		coord_t leftxmax = _xmin <= e._xmin ? _xmax : e._xmax;
		bool xoverlap = leftxmax >= rightxmin;
		//the 'bottom' extent is the one with the lower ymin
		coord_t topymin = std::max<coord_t>(_ymin, e._ymin);
		coord_t bottymax = _ymin <= e._ymin ? _ymax : e._ymax;
		bool yoverlap = bottymax >= topymin;
		return xoverlap && yoverlap;
	}

	void Extent::setZUnits(LinearUnit zUnits) {
		_crs.setZUnits(zUnits);
	}

	void Extent::defineCRS(const CoordRef& crs) {
		_crs = crs;
	}

	void Extent::defineCRS(CoordRef&& crs) {
		_crs = std::move(crs);
	}

	void Extent::checkValidExtent() {
		if (_xmin > _xmax || _ymin > _ymax) {
			throw InvalidExtentException("Invalid extent");
		}
	}

	std::array<double, 6> Extent::getGeoTrans(GDALDatasetWrapper& wgd, const std::string& errormsg) {
		std::array<double, 6> gt{}; //xmin, xres, xshear, ymax, yshear, yres
		auto errcode = wgd->GetGeoTransform(gt.data());
		if (errcode != CE_None) {
			throw InvalidRasterFileException("GDAL error: " + errormsg);
		}
		return gt;
	}

	void Extent::extentInitFromGDALRaster(GDALDatasetWrapper& wgd, const std::array<double, 6>& geotrans) {
		//xmin, xres, xshear, ymax, yshear, yres
		int ncol = wgd->GetRasterXSize();
		int nrow = wgd->GetRasterYSize();
		_xmin = geotrans[0];
		_xmax = geotrans[0] + geotrans[1] * ncol;
		_ymax = geotrans[3];
		_ymin = geotrans[3] + geotrans[5] * nrow; //yres is a negative number so this is correct

		auto wkt = wgd->GetProjectionRef();
		_crs = CoordRef(wkt);
	}

	void Extent::_setFromLasIO(const LasIO& las)
	{
		_crs = CoordRef(las);
		_xmin = las.header.xmin;
		_xmax = las.header.xmax;
		_ymin = las.header.ymin;
		_ymax = las.header.ymax;
	}


	bool operator==(const Extent& lhs, const Extent& rhs) {
		bool same = true;
		auto near = [](coord_t x, coord_t y) {
			return std::abs(x - y) < LAPIS_EPSILON;
		};
		same = same && near(lhs.xmin(), rhs.xmin());
		same = same && near(lhs.xmax(), rhs.xmax());
		same = same && near(lhs.ymin(), rhs.ymin());
		same = same && near(lhs.ymax(), rhs.ymax());
		same = same && lhs.crs().isConsistentHoriz(rhs.crs());
		return same;
	}
	bool operator!=(const Extent& lhs, const Extent& rhs) {
		return !(lhs == rhs);
	}
	std::ostream& operator<<(std::ostream& os, const Extent& e) {
		os << "EXTENT: xmin: " << e.xmin() << " xmax: " << e.xmax() << " ymin: " << e.ymin() << " ymax: " << e.ymax();
		return os;
	}

	//This function creates a new extent object with only the overlapping area of the two given extents
	//If they do not touch, this will throw an exception
	Extent cropExtent(const Extent& base, const Extent& e) {
		if (!(base.crs().isConsistentHoriz(e.crs()))) {
			throw CRSMismatchException("CRS mismatch in cropExtent");
		}
		if (!base.touches(e)) {
			throw OutsideExtentException("Outside extent in cropExtent");
		}
		coord_t xmin = std::max(base.xmin(), e.xmin());
		coord_t xmax = std::min(base.xmax(), e.xmax());
		coord_t ymin = std::max(base.ymin(), e.ymin());
		coord_t ymax = std::min(base.ymax(), e.ymax());
		return Extent(xmin, xmax, ymin, ymax, base.crs());
	}

	//This function creates a new extent object which is the smallest extent to encompass both of the given extents
	Extent extendExtent(const Extent& base, const Extent& e) {
		if (!(base.crs().isConsistentHoriz(e.crs()))) {
			throw CRSMismatchException("CRS mismatch in extendExtent");
		}
		coord_t xmin = std::min(base.xmin(), e.xmin());
		coord_t xmax = std::max(base.xmax(), e.xmax());
		coord_t ymin = std::min(base.ymin(), e.ymin());
		coord_t ymax = std::max(base.ymax(), e.ymax());
		return Extent(xmin, xmax, ymin, ymax, base.crs());
	}
	Extent bufferExtent(const Extent& base, coord_t bufferSize)
	{
		return Extent{ base.xmin() - bufferSize,base.xmax() + bufferSize,base.ymin() - bufferSize,base.ymax() + bufferSize };
	}
}