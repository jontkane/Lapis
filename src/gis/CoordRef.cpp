#include"gis_pch.hpp"
#include"CoordRef.hpp"
#include"GisExceptions.hpp"


//these functions aren't in the GDAL headers but are in gdal_i.lib and are linkable
extern "C" {
	char* GTIFGetOGISDefn(GTIF*, GTIFDefn*);
	void VSIFree(void* data);
}

namespace lapis {
	CoordRef::CoordRef(const std::string& s) {
		_crsFromString(s);
		_zUnits = _inferZUnits();
	}

	CoordRef::CoordRef(const std::string& s, LinearUnit zUnits) {
		_crsFromString(s);
		setZUnits(zUnits);
	}

	CoordRef::CoordRef(const char* s) 
	{
		if (s) {
			_crsFromString(std::string(s));
		}
	}

	CoordRef::CoordRef(const char* s, LinearUnit zUnits)
	{
		if (s) {
			_crsFromString(std::string(s));
		}
		setZUnits(zUnits);
	}

	CoordRef::CoordRef(const SharedPJ& pj)
	{
		_p = pj;
		_zUnits = _inferZUnits();
	}

	CoordRef::CoordRef(const LasIO& las)
	{
		_crsFromLasIO(las);
		_zUnits = _inferZUnits();
	}

	const std::string CoordRef::getPrettyWKT() const {
		if (isEmpty()) {
			return std::string("");
		}
		const char* wkt = proj_as_wkt(ProjContextByThread::get(), _p.get(), PJ_WKT2_2019_SIMPLIFIED, nullptr);
		std::string out{ wkt };
		return out;
	}

	const std::string CoordRef::getCompleteWKT() const
	{
		if (isEmpty()) {
			return std::string("");
		}

		//Multithreading was sometimes causing problems here despite the different proj contexts. Either a bug in proj or I'm missing something about how the functions are supposed to be used.
		//Either way, this function is called rarely enough that using a mutex to manage it doesn't really slow the code down.
		static std::mutex mut;
		std::scoped_lock lock{ mut };

		const char* wkt = proj_as_wkt(ProjContextByThread::get(), _p.get(), PJ_WKT2_2019, nullptr);
		std::string out{ wkt };
		return out;
	}

	const std::string CoordRef::getSingleLineWKT() const
	{
		if (isEmpty()) {
			return std::string("");
		}
		const char* wkt = proj_as_wkt(ProjContextByThread::get(), _p.get(), PJ_WKT1_ESRI, nullptr);
		std::string out{ wkt };
		return out;
	}

	const std::string CoordRef::getShortName() const
	{
		if (isEmpty()) {
			return "Unknown";
		}
		return proj_get_name(getPtr());
	}

	bool CoordRef::isEmpty() const {
		return _p.get() == nullptr;
	}

	bool CoordRef::isSame(const CoordRef& other) const {
		if (getPtr() == nullptr && other.getPtr() == nullptr) {
			return true;
		}
		return proj_is_equivalent_to_with_ctx(ProjContextByThread::get(), _p.get(), other.getPtr(), PJ_COMP_EQUIVALENT);
	}

	bool CoordRef::isConsistentHoriz(const CoordRef& other) const {
		if (isEmpty() || other.isEmpty()) {
			return true;
		}
		if (isSame(other)) {
			return true;
		}

		//compiler please I'm not even the one who defined it as an enum why is this warning coming from my file
#pragma warning (suppress : 26812)
		PJ_TYPE thistype = proj_get_type(_p.get());
		PJ_TYPE othertype = proj_get_type(other.getPtr());

		SharedPJ thishoriz;
		SharedPJ otherhoriz;
		if (thistype == PJ_TYPE_COMPOUND_CRS) {
			thishoriz = getSubCrs(_p, 0);
		}
		else {
			thishoriz = _p;
		}
		if (othertype == PJ_TYPE_COMPOUND_CRS) {
			otherhoriz = getSubCrs(other.getSharedPtr(), 0);
		}
		else {
			otherhoriz = other._p;
		}
		return proj_is_equivalent_to(thishoriz.get(), otherhoriz.get(), PJ_COMP_EQUIVALENT);
	}

	bool CoordRef::isConsistentZUnits(const CoordRef& other) const {
		return _zUnits.isConsistent(other.getZUnits());
	}

	bool CoordRef::isConsistent(const CoordRef& other) const {
		return isConsistentHoriz(other) && isConsistentZUnits(other);
	}

	bool CoordRef::isProjected() const {
		if (isEmpty()) {
			return true;
		}
		PJ_TYPE t = proj_get_type(_p.get());
		if (t == PJ_TYPE_PROJECTED_CRS) {
			return true;
		}
		if (t == PJ_TYPE_COMPOUND_CRS) {
			for (int i = 0; i <= 1; ++i) { //the horizontal CRS component will usually be index 0 but no guarantee if the WKT string is constructed oddly
				SharedPJ wp = getSubCrs(_p, 0);
				PJ_TYPE subtype = proj_get_type(wp.get());
				if (subtype == PJ_TYPE_PROJECTED_CRS) {
					return true;
				}
			}
		}
		return false;
	}

	std::optional<LinearUnit> CoordRef::getXYLinearUnits() const {
		if (isEmpty()) {
			return linearUnitPresets::unknownLinear;
		}
		if (!isProjected()) {
			return std::optional<LinearUnit>();
		}

		SharedPJ horiz = _p;

		PJ_TYPE t = proj_get_type(_p.get());

		if (t == PJ_TYPE_COMPOUND_CRS) {
			horiz = getSubCrs(_p, 0);
		}
		SharedPJ cs = makeSharedPJ(proj_crs_get_coordinate_system(ProjContextByThread::get(), horiz.get()));
		double convFactor = 0;
		const char* unitName = nullptr; //I'm pretty sure the string that's being put here will be cleaned up when cs is destroyed

		proj_cs_get_axis_info(ProjContextByThread::get(), cs.get(), 0,
			nullptr, nullptr, nullptr,
			&convFactor, &unitName,
			nullptr, nullptr);
		if (unitName == nullptr) {
			return linearUnitPresets::unknownLinear;
		}
		return LinearUnit(std::string(unitName), convFactor);
	}

	//returns the Z units of the CRS
	//If there's no vertical datum, it returns a Unit object which is unknown, but has the same convfactor as the horizontal units
	const LinearUnit& CoordRef::getZUnits() const {
		return _zUnits;
	}

	void CoordRef::setZUnits(const LinearUnit& zUnits) {
		_zUnits = zUnits;
	}

	bool CoordRef::hasVertDatum() const {
		return proj_get_type(_p.get()) == PJ_TYPE_COMPOUND_CRS;
	}

	std::string CoordRef::getEPSG() const {
		if (isEmpty()) {
			return "Unknown";
		}
		PJ_TYPE t = proj_get_type(_p.get());

		auto epsgFromCRS = [&](const SharedPJ& pj)->std::string {
			PJIdentifyWrapper matches{ pj };
			if (!proj_list_get_count(matches.getObjList())) {
				return "Unknown";
			}
			SharedPJ epsgcrs = makeSharedPJ(proj_list_get(ProjContextByThread::get(),matches.getObjList(),0));
			const char* code = proj_get_id_code(epsgcrs.get(), 0);
			if (code == nullptr) {
				return "Unknown";
			}
			std::string out{ code };
			return out;
		};

		if (t != PJ_TYPE_COMPOUND_CRS) {
			return epsgFromCRS(_p);
		}
		else {
			SharedPJ horiz = getSubCrs(_p, 0);
			std::string out = epsgFromCRS(horiz);
			SharedPJ vert = getSubCrs(_p, 1);
			out = out + '+' + epsgFromCRS(vert);
			return out;
		}

	}

	CoordRef CoordRef::getCleanEPSG() const
	{
		std::string epsg = getEPSG();
		std::regex horizunknown{ "Unknown.*" };
		std::regex vertunknown{ ".*Unknown" };
		//TODO: if the horizontal is Unknown, return *this
		//if the horizontal is known, clean it up and then read the units from the vertical
		if (std::regex_match(epsg, horizunknown)) {
			return *this;
		}
		else if (std::regex_match(epsg, vertunknown)) {
			SharedPJ horiz = getSubCrs(_p, 0);
			CoordRef out = CoordRef(CoordRef(horiz).getEPSG());
			LinearUnit vertUnits = getZUnits();
			out.setZUnits(vertUnits);
			return out;
		}
		else {
			LinearUnit vertUnits = getZUnits();
			CoordRef out = CoordRef(epsg);
			out.setZUnits(vertUnits);
			return out;
		}
	}

	PJ* CoordRef::getPtr() {
		return _p.get();
	}

	const PJ* CoordRef::getPtr() const {
		return _p.get();
	}

	SharedPJ& CoordRef::getSharedPtr() {
		return _p;
	}

	const SharedPJ& CoordRef::getSharedPtr() const {
		return _p;
	}

	void CoordRef::_crsFromString(const std::string& s) {
		if (!s.size()) {
			_p = SharedPJ();
			return;
		}
		std::ifstream ifs;
		ifs.open(s);
		if (!ifs) { //if it isn't an openable file, then we check in with proj
			if (s[0] == '+') { //only a proj-string should start with +
				_p = projCreateWrapper(s + " +type=crs"); //not all methods of producing a proj-string will have this, and it's important to make PROJ realize I'm trying to define a CRS
			}
			else if (std::isdigit(s[0])) { //likely an ill-formatted EPSG code
				_p = projCreateWrapper("EPSG:" + s);
			}
			else {
				_p = projCreateWrapper(s);
			}
		}
		ifs.close();

		if (s.size() < 4) { //not enough characters to be a filename with an extension
			throw UnableToDeduceCRSException("No extension in " + s);
		}
		std::string ext = s.substr(s.size() - 4, 4);

		if (ext == ".prj") {
			_crsFromPrj(s);
			return;
		}
		if (ext == ".las" || ext == ".laz") {
			LasIO l{ s };
			_crsFromLasIO(l);
			return;
		}

		_crsFromRaster(s);
		if (isEmpty()) {
			_crsFromVector(s);
		}

		if (isEmpty()) {
			throw UnableToDeduceCRSException("Unable to get a CRS from " + s);
		}
	}

	void CoordRef::_crsFromLasIO(const LasIO& las)
	{
		const LasVLRs& vlrs = las.vlrs;

		if (las.header.isWKT()) {
			_p = projCreateWrapper(vlrs.wkt);
			return;
		}

		if (!vlrs.gtifKeys.size()) {
			return; //las file doesn't have a defined CRS
		}

		tiffWrapper t{};
		TIFFMethod method;
		GTIFSetSimpleTagsMethods(&method);

		ST_SetKey(t.ptr, gtifKeysCode, ((int)vlrs.gtifKeys.size() * sizeof(gtifKey)) / sizeof(std::uint16_t), STT_SHORT, (void*)vlrs.gtifKeys.data());
		if (vlrs.gtifDoubleParams.size()) {
			ST_SetKey(t.ptr, gtifDoublesCode, (int)vlrs.gtifDoubleParams.size() / sizeof(double), STT_DOUBLE, (void*)vlrs.gtifDoubleParams.data());
		}
		if (vlrs.gtifAsciiParams.size()) {
			ST_SetKey(t.ptr, gtifAsciiCode, (int)vlrs.gtifAsciiParams.size(), STT_ASCII, (void*)vlrs.gtifAsciiParams.data());
		}

		gtifWrapper g{ t,&method };

		if (g.ptr == nullptr) {
			return; //we did our best
		}

		GTIFDefn defn;
		if (GTIFGetDefn(g.ptr, &defn)) {
			char* wkt = GTIFGetOGISDefn(g.ptr, &defn);
			if (wkt) {
				_p = projCreateWrapper(wkt);
				VSIFree(wkt);
			}
		}
	}

	void CoordRef::_crsFromPrj(const std::string& s)
	{
		UniqueCslString wgp = cslLoadWrapper(s);
		OGRSpatialReference osr{};
		osr.importFromESRI(wgp.get());
		if (osr.IsEmpty()) {
			throw UnableToDeduceCRSException("Failed to read CRS from " + s);
		}
		_p = sharedPJFromOSR(osr);
	}

	void CoordRef::_crsFromRaster(const std::string& s)
	{
		auto rast = rasterGDALWrapper(s);
		if (rast) {
			const char* r_crs = rast->GetProjectionRef();

			if (r_crs == nullptr) {
				_p = SharedPJ();
				return;
			}
			_p = projCreateWrapper(std::string(r_crs));
		}
	}

	void CoordRef::_crsFromVector(const std::string& s)
	{
		auto vect = vectorGDALWrapper(s);
		if (vect) {
			OGRLayer* layer = vect->GetLayer(0);
			OGRSpatialReference* osr = layer->GetSpatialRef();
			_p = sharedPJFromOSR(*osr);
		}
	}

	LinearUnit CoordRef::_inferZUnits() {
		if (isEmpty()) {
			return linearUnitPresets::unknownLinear;
		}
		PJ_TYPE t = proj_get_type(_p.get());
		if (t != PJ_TYPE_COMPOUND_CRS) {
			std::optional<LinearUnit> u = getXYLinearUnits();
			return u.value_or(linearUnitPresets::unknownLinear);
		}
		SharedPJ vert = getSubCrs(_p, 1);
		SharedPJ cs = makeSharedPJ(
			proj_crs_get_coordinate_system(ProjContextByThread::get(), vert.get())
		);
		double convFactor = 0;
		const char* unitName = nullptr; //I'm pretty sure the string that's being put here will be cleaned up when cs is destroyed
		proj_cs_get_axis_info(ProjContextByThread::get(), cs.get(), 0,
			nullptr, nullptr, nullptr,
			&convFactor, &unitName,
			nullptr, nullptr);
		return LinearUnit(std::string(unitName), convFactor);
	}

	bool CoordRefComparator::operator()(const CoordRef& a, const CoordRef& b) const
	{
		return a.isConsistent(b);
	}

	size_t CoordRefHasher::operator()(const CoordRef& a) const
	{
		return std::hash<std::string>()(a.getCompleteWKT() + a.getZUnits().name());
	}

}