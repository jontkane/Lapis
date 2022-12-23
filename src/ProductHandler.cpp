#include"app_pch.hpp"
#include"ProductHandler.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisLogger.hpp"

namespace lapis {

	template<class T>
	void writeRasterWithFullName(SharedParameterGetter* getter,
		const std::filesystem::path& dir, const std::string& baseName,
		Raster<T>& r, OutputUnitLabel u) {

		namespace fs = std::filesystem;
		LapisLogger& log = LapisLogger::getLogger();

		fs::create_directories(dir);
		std::string unitString;
		using oul = OutputUnitLabel;
		if (u == oul::Default) {
			Unit outUnit = getter->outUnits();
			std::regex meterregex{ ".*met.*",std::regex::icase };
			std::regex footregex{ ".*f(o|e)(o|e)t.*",std::regex::icase };
			if (std::regex_match(outUnit.name, meterregex)) {
				unitString = "_Meters";
			}
			else if (std::regex_match(outUnit.name, footregex)) {
				unitString = "_Feet";
			}
		}
		else if (u == oul::Percent) {
			unitString = "_Percent";
		}
		else if (u == oul::Radian) {
			unitString = "_Radians";
		}
		std::string runName = getter->name().size() ? getter->name() + "_" : "";
		fs::path fullPath = dir / (runName + baseName + unitString + ".tif");

		try{
			r.writeRaster(fullPath.string());
		}
		catch (InvalidRasterFileException e) {
			LapisLogger::getLogger().logMessage("Error writing " + baseName);
		}
	}
	template<class T>
	void writeTempRaster(const std::filesystem::path& dir, const std::string& name, Raster<T>& r) {
		std::filesystem::create_directories(dir);
		std::string filename = (dir / std::filesystem::path(name + ".tif")).string();
		try {
			r.writeRaster(filename);
		}
		catch (...) {
			LapisLogger::getLogger().logMessage("Issue writing " + name);
		}
	}
	template<class T>
	Raster<T> getEmptyRasterFromTile(SharedParameterGetter* getter, cell_t tile,
		const Alignment& a, coord_t minBufferMeters) {

		std::shared_ptr<Alignment> layout = getter->layout();
		std::shared_ptr<Alignment> metricAlign = getter->metricAlign();
		Extent thistile = layout->extentFromCell(tile);

		//The buffering ensures CSM metrics don't suffer from edge effects
		coord_t bufferDist = convertUnits(minBufferMeters, LinearUnitDefs::meter, layout->crs().getXYUnits());
		if (bufferDist > 0) {
			bufferDist = std::max(std::max(metricAlign->xres(), metricAlign->yres()), bufferDist);
		}
		Extent bufferExt = Extent(thistile.xmin() - bufferDist, thistile.xmax() + bufferDist, thistile.ymin() - bufferDist, thistile.ymax() + bufferDist);
		return Raster<T>(cropAlignment(a, bufferExt, SnapType::ll));
	}

	ProductHandler::ProductHandler(ParamGetter* p) : _sharedGetter(p)
	{
		if (p == nullptr) {
			throw std::invalid_argument("ParamGetter is null");
		}
		std::filesystem::remove_all(tempDir());
	}
	std::filesystem::path ProductHandler::parentDir() const
	{
		return _sharedGetter->outFolder();
	}
	std::filesystem::path ProductHandler::tempDir() const
	{
		return parentDir() / "Temp";
	}
	void ProductHandler::deleteTempDirIfEmpty() const
	{
		namespace fs = std::filesystem;
		if (fs::is_empty(tempDir())) {
			fs::remove(tempDir());
		}
	}

	template<bool ALL_RETURNS, bool FIRST_RETURNS>
	void PointMetricHandler::_assignPointsToCalculators(const LidarPointVector& points)
	{
		for (const LasPoint& p : points) {
			cell_t cell = _getter->metricAlign()->cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ _getter->cellMutex(cell) };
			if constexpr (ALL_RETURNS) {
				_allReturnPMC->atCellUnsafe(cell).value().addPoint(p);
			}
			if constexpr (FIRST_RETURNS) {
				if (p.returnNumber == 1) {
					_firstReturnPMC->atCellUnsafe(cell).value().addPoint(p);
				}
			}
		}
	}
	void PointMetricHandler::_processPMCCell(cell_t cell, PointMetricCalculator& pmc, ReturnType r) {

		for (PointMetricRasters& v : _pointMetrics) {
			MetricFunc& f = v.fun;
			if (_getter->doAllReturnMetrics()) {
				(pmc.*f)(v.rasters.get(r), cell);
			}
		}
		for (StratumMetricRasters& v : _stratumMetrics) {
			StratumFunc& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(pmc.*f)(v.rasters[i].get(r), cell, i);
			}
		}

		pmc.cleanUp();
	}
	void PointMetricHandler::_writePointMetricRasters(const std::filesystem::path& dir, ReturnType r) {
		for (PointMetricRasters& metric : _pointMetrics) {
			writeRasterWithFullName(_getter, dir, metric.name, metric.rasters.get(r), metric.unit);
		}
		for (StratumMetricRasters& metric : _stratumMetrics) {
			for (size_t i = 0; i < metric.rasters.size(); ++i) {
				writeRasterWithFullName(_getter, dir / "StratumMetrics", metric.baseName + _getter->strataNames()[i],
					metric.rasters[i].get(r), metric.unit);
			}
		}
	}
	PointMetricHandler::PointMetricHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(pointMetricDir());

		if (!_getter->doPointMetrics()) {
			return;
		}

		using pmc = PointMetricCalculator;
		using oul = OutputUnitLabel;

		pmc::setInfo(_getter->canopyCutoff(), _getter->maxHt(), _getter->binSize(), _getter->strataBreaks());

		_nLaz = Raster<int>(*_getter->metricAlign());
		for (const Extent& e : _getter->lasExtents()) {
			for (cell_t cell : _nLaz.cellsFromExtent(e, SnapType::out)) {
				_nLaz[cell].has_value() = true;
				_nLaz[cell].value()++;
			}
		}

		if (_getter->doAllReturnMetrics()) {
			_allReturnPMC = std::make_unique<Raster<pmc>>(*_getter->metricAlign());
		}
		if (_getter->doFirstReturnMetrics()) {
			_firstReturnPMC = std::make_unique<Raster<pmc>>(*_getter->metricAlign());
		}

		auto addPointMetric = [&](const std::string& name, MetricFunc f, oul u) {
			_pointMetrics.emplace_back(_getter, name, f, u);
		};

		addPointMetric("Mean_CanopyHeight", &pmc::meanCanopy, oul::Default);
		addPointMetric("StdDev_CanopyHeight", &pmc::stdDevCanopy, oul::Default);
		addPointMetric("25thPercentile_CanopyHeight", &pmc::p25Canopy, oul::Default);
		addPointMetric("50thPercentile_CanopyHeight", &pmc::p50Canopy, oul::Default);
		addPointMetric("75thPercentile_CanopyHeight", &pmc::p75Canopy, oul::Default);
		addPointMetric("95thPercentile_CanopyHeight", &pmc::p95Canopy, oul::Default);
		addPointMetric("TotalReturnCount", &pmc::returnCount, oul::Unitless);
		addPointMetric("CanopyCover", &pmc::canopyCover, oul::Percent);
		if (_getter->doAdvancedPointMetrics()) {
			addPointMetric("CoverAboveMean", &pmc::coverAboveMean, oul::Percent);
			addPointMetric("CanopyReliefRatio", &pmc::canopyReliefRatio, oul::Unitless);
			addPointMetric("CanopySkewness", &pmc::skewnessCanopy, oul::Unitless);
			addPointMetric("CanopyKurtosis", &pmc::kurtosisCanopy, oul::Unitless);
			addPointMetric("05thPercentile_CanopyHeight", &pmc::p05Canopy, oul::Default);
			addPointMetric("10thPercentile_CanopyHeight", &pmc::p10Canopy, oul::Default);
			addPointMetric("15thPercentile_CanopyHeight", &pmc::p15Canopy, oul::Default);
			addPointMetric("20thPercentile_CanopyHeight", &pmc::p20Canopy, oul::Default);
			addPointMetric("30thPercentile_CanopyHeight", &pmc::p30Canopy, oul::Default);
			addPointMetric("35thPercentile_CanopyHeight", &pmc::p35Canopy, oul::Default);
			addPointMetric("40thPercentile_CanopyHeight", &pmc::p40Canopy, oul::Default);
			addPointMetric("45thPercentile_CanopyHeight", &pmc::p45Canopy, oul::Default);
			addPointMetric("55thPercentile_CanopyHeight", &pmc::p55Canopy, oul::Default);
			addPointMetric("60thPercentile_CanopyHeight", &pmc::p60Canopy, oul::Default);
			addPointMetric("65thPercentile_CanopyHeight", &pmc::p65Canopy, oul::Default);
			addPointMetric("70thPercentile_CanopyHeight", &pmc::p70Canopy, oul::Default);
			addPointMetric("80thPercentile_CanopyHeight", &pmc::p80Canopy, oul::Default);
			addPointMetric("85thPercentile_CanopyHeight", &pmc::p85Canopy, oul::Default);
			addPointMetric("90thPercentile_CanopyHeight", &pmc::p90Canopy, oul::Default);
			addPointMetric("99thPercentile_CanopyHeight", &pmc::p99Canopy, oul::Default);
		}

		if (_getter->doStratumMetrics()) {
			if (_getter->strataBreaks().size()) {
				_stratumMetrics.emplace_back(_getter, "StratumCover_", &pmc::stratumCover, oul::Percent);
				_stratumMetrics.emplace_back(_getter, "StratumReturnProportion_", &pmc::stratumPercent, oul::Percent);
			}
		}
	}
	void PointMetricHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doPointMetrics()) {
			return;
		}

		//This structure is kind of ugly and inelegant, but it ensures that all returns and first returns can share a call to cellFromXY
		//without needing to pollute the loop with a bunch of if checks
		//Right now, with only two booleans, the combinatorics are bearable; if it increases, then it probably won't be
		if (_getter->doFirstReturnMetrics() && _getter->doAllReturnMetrics()) {
			_assignPointsToCalculators<true, true>(points);
		}
		else if (_getter->doAllReturnMetrics()) {
			_assignPointsToCalculators<true, false>(points);
		}
		else if (_getter->doFirstReturnMetrics()) {
			_assignPointsToCalculators<false, true>(points);
		}

		std::vector<cell_t> cells = _nLaz.cellsFromExtent(e, SnapType::out);
		for (cell_t cell : cells) {
			std::scoped_lock lock{ _getter->cellMutex(cell) };
			_nLaz.atCellUnsafe(cell).value()--;
			if (_nLaz.atCellUnsafe(cell).value() != 0) {
				continue;
			}
			if (_getter->doAllReturnMetrics())
				_processPMCCell(cell, _allReturnPMC->atCellUnsafe(cell).value(), ReturnType::ALL);
			if (_getter->doFirstReturnMetrics())
				_processPMCCell(cell, _firstReturnPMC->atCellUnsafe(cell).value(),ReturnType::FIRST);
		}

	}
	void PointMetricHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void PointMetricHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) {}
	void PointMetricHandler::cleanup() {
		namespace fs = std::filesystem;

		if (!_getter->doPointMetrics()) {
			return;
		}
		
		if (_getter->doAllReturnMetrics()) {
			fs::path allReturnsMetricDir = _getter->doFirstReturnMetrics() ? pointMetricDir() / "AllReturns" : pointMetricDir();
			_writePointMetricRasters(allReturnsMetricDir, ReturnType::ALL);
		}
		if (_getter->doFirstReturnMetrics()) {
			fs::path firstReturnsMetricDir = _getter->doAllReturnMetrics() ? pointMetricDir() / "FirstReturns" : pointMetricDir();
			_writePointMetricRasters(firstReturnsMetricDir,ReturnType::FIRST);
		}

		_pointMetrics.clear();
		_pointMetrics.shrink_to_fit();

		_stratumMetrics.clear();
		_stratumMetrics.shrink_to_fit();
	}
	std::filesystem::path ProductHandler::pointMetricDir() const
	{
		return parentDir() / "PointMetrics";
	}

	CsmHandler::CsmHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(csmDir());
		std::filesystem::remove_all(csmTempDir());

		if (!_getter->doCsmMetrics()) {
			return;
		}
		using oul = OutputUnitLabel;

		auto addMetric = [&](CsmMetricFunc fun, const std::string& name, oul unit) {
			_csmMetrics.emplace_back(_getter, name, fun, unit);
		};

		addMetric(&viewMax<csm_t>, "MaxCSM", oul::Default);
		addMetric(&viewMean<csm_t>, "MeanCSM", oul::Default);
		addMetric(&viewStdDev<csm_t>, "StdDevCSM", oul::Default);
		addMetric(&viewRumple<csm_t>, "RumpleCSM", oul::Unitless);
	}
	void CsmHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doCsm()) {
			return;
		}

		const coord_t footprintRadius = _getter->footprintDiameter() / 2;

		Raster<csm_t> csm{ cropAlignment(*_getter->csmAlign(),
			Extent(e.xmin() - footprintRadius,e.xmax() + footprintRadius,e.ymin() - footprintRadius,e.ymax() + footprintRadius),
			SnapType::out) };


		const coord_t diagonal = footprintRadius / std::sqrt(2);
		const coord_t epsilon = -0.000001; //the purpose of this value is to avoid ties caused by the footprint algorithm messing with the high points TAO algorithm
		struct XYEpsilon {
			coord_t x, y, epsilon;
		};
		std::vector<XYEpsilon> circle;
		if (footprintRadius == 0) {
			circle = { {0.,0.,0.} };
		}
		else {
			circle = { {0,0,0},
				{footprintRadius,0,epsilon},
				{-footprintRadius,0,epsilon},
				{0,footprintRadius,epsilon},
				{0,-footprintRadius,epsilon},
				{diagonal,diagonal,2 * epsilon},
				{diagonal,-diagonal,2 * epsilon},
				{-diagonal,diagonal,2 * epsilon},
				{-diagonal,-diagonal,2 * epsilon} };
		}

		//there are generally far more points than cells, so it's worth it to do some goofy stuff to reduce the amount of work in the core loop
		//in particular, this escapes the need to handle has_value() inside the point loop
		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			csm[cell].value() = std::numeric_limits<csm_t>::lowest();
		}

		for (const LasPoint& p : points) {
			for (const XYEpsilon& direction : circle) {
				coord_t x = p.x + direction.x;
				coord_t y = p.y + direction.y;
				csm_t z = p.z + direction.epsilon;
				cell_t cell = csm.cellFromXYUnsafe(x, y);

				csm[cell].value() = std::max(csm[cell].value(), z);
			}
		}

		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			if (csm[cell].value() > std::numeric_limits<csm_t>::lowest()) {
				csm[cell].has_value() = true;
			}
		}

		try {
			writeTempRaster(csmTempDir(),
				std::to_string(index), csm);
		}
		catch (InvalidRasterFileException e) {
			LapisLogger::getLogger().logMessage("Error Writing Temporary CSM File");
			throw InvalidRasterFileException("Error Writing Temporary CSM File");
		}
	}
	void CsmHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void CsmHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		for (CSMMetricRaster& metric : _csmMetrics) {
			Raster<metric_t> tileMetric = aggregate(bufferedCsm, cropAlignment(*_getter->metricAlign(), bufferedCsm, SnapType::out), metric.fun);
			metric.raster.overlayInside(tileMetric);
		}
		
		Extent cropExt = _getter->layout()->extentFromCell(tile); //unbuffered extent
		Raster<csm_t> unbuffered = cropRaster(bufferedCsm, cropExt, SnapType::out);

		std::string outname = "CanopySurfaceModel_" + nameFromLayout(*_getter->layout(), tile);
		writeRasterWithFullName(_getter, csmDir(), outname, unbuffered, OutputUnitLabel::Default);
	}
	void CsmHandler::cleanup() 
	{
		if (!_getter->doCsm()) {
			return;
		}
		std::filesystem::remove_all(csmTempDir());
		deleteTempDirIfEmpty();

		for (CSMMetricRaster& metric : _csmMetrics) {
			writeRasterWithFullName(_getter, csmMetricDir(), metric.name, metric.raster, metric.unit);
		}
		_csmMetrics.clear();
		_csmMetrics.shrink_to_fit();
	}
	Raster<csm_t> CsmHandler::getBufferedCsm(cell_t tile) const
	{
		if (!_getter->doCsm()) {
			return Raster<csm_t>();
		}

		std::shared_ptr<Alignment> layout = _getter->layout();
		std::shared_ptr<Alignment> csmAlign = _getter->csmAlign();

		Extent thistile = layout->extentFromCell(tile);

		Raster<csm_t> bufferedCsm = getEmptyRasterFromTile<csm_t>(_getter, tile, *csmAlign, 30);

		Extent extentWithData;
		bool extentInit = false;

		const std::vector<Extent>& lasExtents = _getter->lasExtents();
		for (size_t i = 0; i < lasExtents.size(); ++i) {
			Extent thisext = lasExtents[i]; //intentional copy

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			if (!thisext.overlaps(bufferedCsm)) {
				continue;
			}
			thisext = cropExtent(thisext, bufferedCsm);
			Raster<csm_t> thisr;
			std::string filename = (csmTempDir() / (std::to_string(i) + ".tif")).string();
			try {
				thisr = Raster<csm_t>{ filename,thisext, SnapType::out };
			}
			catch (InvalidRasterFileException e) {
				LapisLogger::getLogger().logMessage("Unable to open " + filename);
				continue;
			}

			if (!extentInit) {
				extentWithData = thisr;
				extentInit = true;
			}
			else {
				extentWithData = extendExtent(extentWithData, thisr);
			}


			//for the same reason as the above comment
			thisr.defineCRS(bufferedCsm.crs());
			bufferedCsm.overlay(thisr, [](csm_t a, csm_t b) {return std::max(a, b); });
		}

		if (!bufferedCsm.hasAnyValue()) {
			return Raster<csm_t>();
		}

		bufferedCsm = cropRaster(bufferedCsm, extentWithData, SnapType::out);

		int neighborsNeeded = 6;
		bufferedCsm = smoothAndFill(bufferedCsm, _getter->smooth(), neighborsNeeded, {});

		return bufferedCsm;
	}
	std::filesystem::path ProductHandler::csmTempDir() const
	{
		return tempDir() / "CSM";
	}
	std::filesystem::path ProductHandler::csmDir() const
	{
		return parentDir() / "CanopySurfaceModel";
	}
	std::filesystem::path ProductHandler::csmMetricDir() const
	{
		return parentDir() / "CanopyMetrics";
	}

	void TaoHandler::_writeHighPointsAsXYZArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm,
		const Extent& unbufferedExtent, cell_t tile) const {

		std::filesystem::create_directories(taoTempDir());

		std::filesystem::path fileName = taoTempDir() / (std::to_string(tile) + ".tmp");
		std::ofstream ofs{ fileName,std::ios::binary };

		if (!ofs) {
			LapisLogger::getLogger().logMessage("Error writing to " + fileName.string());
			return;
		}

		auto writeBytes = [&](auto x) {
			ofs.write((const char*)&x, sizeof(x));
		};

		for (cell_t cell : highPoints) {
			coord_t x = csm.xFromCellUnsafe(cell);
			coord_t y = csm.yFromCellUnsafe(cell);
			if (!unbufferedExtent.contains(x, y)) {
				continue;
			}
			writeBytes(x);
			writeBytes(y);
			writeBytes(csm[cell].value());
		}
	}
	std::vector<CoordXYZ> TaoHandler::_readHighPointsFromXYZArray(cell_t tile) const {
		std::filesystem::path fileName = taoTempDir() / (std::to_string(tile) + ".tmp");
		std::ifstream ifs{ fileName,std::ios::binary };
		if (!ifs) {
			LapisLogger::getLogger().logMessage("Error reading from " + fileName.string());
			return std::vector<CoordXYZ>();
		}

		coord_t x, y;
		csm_t z;
		std::vector<CoordXYZ> out;

		auto readBytes = [&](auto* x) {
			ifs.read((char*)x, sizeof(*x));
		};

		while (ifs) {
			readBytes(&x);
			readBytes(&y);
			readBytes(&z);
			if (ifs.eof()) {
				break;
			}
			out.emplace_back(x, y, z);
		}
		return out;
	}
	void TaoHandler::_writeHighPointsAsShp(const Raster<taoid_t>& segments, cell_t tile) const {
		namespace fs = std::filesystem;

		GDALRegisterWrapper::allRegister();
		fs::create_directories(taoDir());
		fs::path filename = (taoDir() / (_getter->name() + "_TAOs_" + nameFromLayout(*_getter->layout(), tile) + ".shp"));

		GDALDatasetWrapper outshp("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(segments.crs().getCleanEPSG().getCompleteWKT().c_str());

		OGRLayer* layer;
		layer = outshp->CreateLayer("point_out", &crs, wkbPoint, nullptr);


		OGRFieldDefn idField("ID", OFTInteger64);
		layer->CreateField(&idField);
		OGRFieldDefn xField("X", OFTReal);
		layer->CreateField(&xField);
		OGRFieldDefn yField("Y", OFTReal);
		layer->CreateField(&yField);
		OGRFieldDefn heightField("Height", OFTReal);
		layer->CreateField(&heightField);
		OGRFieldDefn areaField("Area", OFTReal);
		layer->CreateField(&areaField);

		OGRFieldDefn radiusField("Radius", OFTReal);
		layer->CreateField(&radiusField);

		const coord_t cellArea = convertUnits(segments.xres(), segments.crs().getXYUnits(), segments.crs().getZUnits())
			* convertUnits(segments.yres(), segments.crs().getXYUnits(), segments.crs().getZUnits());

		std::unordered_map<taoid_t, coord_t> areas;
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			if (segments[cell].has_value()) {
				areas.try_emplace(segments[cell].value(), 0);
				areas[segments[cell].value()] += cellArea;
			}
		}

		std::vector<CoordXYZ> highPoints = _readHighPointsFromXYZArray(tile);


		for (const CoordXYZ& highPoint : highPoints) {
			if (!segments.contains(highPoint.x, highPoint.y)) {
				continue;
			}

			taoid_t id = segments.atXYUnsafe(highPoint.x, highPoint.y).value();

			OGRFeatureWrapper feature(layer);
			feature->SetField("ID", (int64_t)id);
			feature->SetField("X", highPoint.x);
			feature->SetField("Y", highPoint.y);
			feature->SetField("Height", highPoint.z);
			feature->SetField("Area", areas[id]);

			feature->SetField("Radius", std::sqrt(areas[id] / M_PI));

			OGRPoint point;
			point.setX(highPoint.x);
			point.setY(highPoint.y);
			feature->SetGeometry(&point);

			layer->CreateFeature(feature.ptr);
		}
	}
	void TaoHandler::_updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx)
	{
		{
			std::scoped_lock<std::mutex> lock(_getter->globalMutex());
			idMap.tileToLocalNames.try_emplace(tileidx, TaoIdMap::IDToCoord());
		}
		for (cell_t cell : highPoints) {
			coord_t x = segments.xFromCellUnsafe(cell);
			coord_t y = segments.yFromCellUnsafe(cell);

			//in theory, you could identify areas that belong only to this tile, and aren't even in the buffers of any other tiles
			//this is easy in a normal case, but kind of annoying to account for edge cases
			//this could save a lot of memory here, if it ends up being necessary to do so

			if (unbufferedExtent.contains(x, y)) { //this tao id is id that other tiles will have to match
				std::scoped_lock<std::mutex> lock(_getter->globalMutex());
				idMap.coordsToFinalName.try_emplace(TaoIdMap::XY{ x,y }, segments[cell].value());
			}
			else { //this tao id will have to change eventually
				idMap.tileToLocalNames[tileidx].try_emplace(segments[cell].value(), TaoIdMap::XY{ x,y });
			}
		}
	}
	void TaoHandler::_fixTaoIdsThread(cell_t tile) const
	{
		LapisLogger& log = LapisLogger::getLogger();

		std::string tileName = nameFromLayout(*_getter->layout(), tile);
		

		Raster<taoid_t> segments;
		std::string name = (taoTempDir() / ("Segments_" + tileName + ".tif")).string();
		try {
			segments = Raster<taoid_t>{ name };
		}
		catch (InvalidRasterFileException e) {
			log.logMessage("Error opening " + name);
			return;
		}

		auto& localNameMap = idMap.tileToLocalNames.at(tile);
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			auto v = segments[cell];
			if (!v.has_value()) {
				continue;
			}
			if (!localNameMap.contains(v.value())) {
				continue;
			}
			if (!idMap.coordsToFinalName.contains(localNameMap.at(v.value()))) { //edge of the acquisition
				continue;
			}
			v.value() = idMap.coordsToFinalName.at(localNameMap.at(v.value()));
		}
		writeRasterWithFullName(_getter, taoDir(), "Segments_" + tileName, segments, OutputUnitLabel::Unitless);
		_writeHighPointsAsShp(segments, tile);
	}
	TaoHandler::TaoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(taoDir());
		std::filesystem::remove_all(taoTempDir());
	}
	void TaoHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
	}
	void TaoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void TaoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		LapisLogger& log = LapisLogger::getLogger();
		if (!_getter->doTaos()) {
			return;
		}
		if (!bufferedCsm.hasAnyValue()) {
			return;
		}

		Extent unbufferedExtent = _getter->layout()->extentFromCell(tile);

		std::vector<cell_t> highPoints = identifyHighPointsWithMinDist(bufferedCsm, _getter->minTaoHt(), _getter->minTaoDist());
		Raster<taoid_t> segments = watershedSegment(bufferedCsm, highPoints, tile, _getter->layout()->ncell());
		_updateMap(segments, highPoints, unbufferedExtent, tile);

		Raster<csm_t> maxHeight = maxHeightBySegment(segments, bufferedCsm, highPoints);

		segments = cropRaster(segments, unbufferedExtent, SnapType::out);
		maxHeight = cropRaster(maxHeight, unbufferedExtent, SnapType::out);

		std::string tileName = nameFromLayout(*_getter->layout(), tile);

		_writeHighPointsAsXYZArray(highPoints, bufferedCsm, unbufferedExtent, tile);
		writeTempRaster(taoTempDir(), "Segments_" + tileName, segments);
		writeRasterWithFullName(_getter, taoDir(), "MaxHeight_" + tileName, maxHeight, OutputUnitLabel::Default);
		segments = Raster<taoid_t>();
		maxHeight = Raster<csm_t>();
	}
	void TaoHandler::cleanup()
	{

		if (!_getter->doTaos()) {
			return;
		}

		uint64_t sofar = 0;
		auto threadfunc = [&]() {
			distributeWork(sofar, _getter->layout()->ncell(), [&](cell_t tile) {this->_fixTaoIdsThread(tile); }, _getter->globalMutex());
		};
		std::vector<std::thread> threads;
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads.push_back(std::thread(threadfunc));
		}
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads[i].join();
		}

		std::filesystem::remove_all(taoTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path ProductHandler::taoDir() const
	{
		return parentDir() / "TreeApproximateObjects";
	}
	std::filesystem::path ProductHandler::taoTempDir() const
	{
		return tempDir() / "TAO";
	}

	FineIntHandler::FineIntHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(fineIntDir());
		std::filesystem::remove_all(fineIntTempDir());
	}
	void FineIntHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator{ cropAlignment(*_getter->fineIntAlign(),e, SnapType::out) };
		Raster<intensity_t> denominator{ (Alignment)numerator };

		coord_t cutoff = _getter->fineIntCanopyCutoff();
		for (const LasPoint& p : points) {
			if (p.z < cutoff) {
				continue;
			}
			cell_t cell = numerator.cellFromXYUnsafe(p.x, p.y);
			numerator[cell].value() += p.intensity;
			denominator[cell].value()++;
		}

		//there are many more points than cells, so rearranging work to be O(cells) instead of O(points) is generally worth it, even if it's a bit awkward
		for (cell_t cell = 0; cell < denominator.ncell(); ++cell) {
			if (denominator[cell].value() > 0) {
				denominator[cell].has_value() = true;
				numerator[cell].has_value() = true;
			}
		}

		writeTempRaster(fineIntTempDir(), std::to_string(index) + "_num", numerator);
		writeTempRaster(fineIntTempDir(), std::to_string(index) + "_denom", denominator);
	}
	void FineIntHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void FineIntHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{

		if (!_getter->doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator = getEmptyRasterFromTile<intensity_t>(_getter, tile, *_getter->fineIntAlign(), 0.);
		Raster<intensity_t> denominator{ (Alignment)numerator };

		Extent extentWithData;
		bool extentInit = false;

		const std::vector<Extent>& lasExtents = _getter->lasExtents();
		for (size_t i = 0; i < lasExtents.size(); ++i) {
			Extent thisext = lasExtents[i]; //intentional copy

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			Raster<intensity_t> thisNumerator, thisDenominator;

			if (!thisext.overlaps(numerator)) {
				continue;
			}
			thisext = cropExtent(thisext, numerator);

			try {
				thisNumerator = Raster<intensity_t>{ (fineIntTempDir() / (std::to_string(i) + "_num.tif")).string(), numerator, SnapType::near };
				thisDenominator = Raster<intensity_t>{ (fineIntTempDir() / (std::to_string(i) + "_denom.tif")).string(), numerator, SnapType::near };
			}
			catch (InvalidRasterFileException e) {
				LapisLogger::getLogger().logMessage("Issue opening temporary intensity file " + std::to_string(i));
				continue;
			}

			numerator.overlay(thisNumerator, [](intensity_t a, intensity_t b) {return a + b; });
			denominator.overlay(thisDenominator, [](intensity_t a, intensity_t b) {return a + b; });

			if (!extentInit) {
				extentInit = true;
				extentWithData = numerator;
			}
			else {
				extentWithData = extendExtent(extentWithData, numerator);
			}
		}

		if (!denominator.hasAnyValue()) {
			return;
		}

		std::string tileName = nameFromLayout(*_getter->layout(), tile);

		Raster<intensity_t> meanIntensity = numerator / denominator;
		meanIntensity = cropRaster(meanIntensity, extentWithData, SnapType::out);
		std::string baseName = _getter->fineIntCanopyCutoff() > std::numeric_limits<coord_t>::lowest() ? "MeanCanopyIntensity_" : "MeanIntensity_";
		writeRasterWithFullName(_getter, fineIntDir(), baseName + tileName, meanIntensity, OutputUnitLabel::Unitless);
	}
	void FineIntHandler::cleanup() {
		std::filesystem::remove_all(fineIntTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path ProductHandler::fineIntDir() const
	{
		return  parentDir() / "Intensity";
	}
	std::filesystem::path ProductHandler::fineIntTempDir() const
	{
		return tempDir() / "Intensity";
	}

	TopoHandler::TopoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;

		std::filesystem::remove_all(topoDir());

		if (!_getter->doTopo()) {
			return;
		}

		_elevNumerator = Raster<coord_t>(*_getter->metricAlign());
		_elevDenominator = Raster<coord_t>(*_getter->metricAlign());

		using oul = OutputUnitLabel;
		_topoMetrics.emplace_back("Slope", viewSlope<coord_t, metric_t>, oul::Radian);
		_topoMetrics.emplace_back("Aspect", viewAspect<coord_t, metric_t>, oul::Radian);

	}
	void TopoHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
	}
	void TopoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
		if (!_getter->doTopo()) {
			return;
		}

		Raster<coord_t> coarseSum = aggregate<coord_t, coord_t>(dem, *_getter->metricAlign(), &viewSum<coord_t>);
		_elevNumerator.overlay(coarseSum, [](coord_t a, coord_t b) {return a + b; });

		Raster<coord_t> coarseCount = aggregate<coord_t, coord_t>(dem, *_getter->metricAlign(), &viewCount<coord_t>);
		_elevDenominator.overlay(coarseCount, [](coord_t a, coord_t b) {return a + b; });
	}
	void TopoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
	}
	void TopoHandler::cleanup()
	{

		if (!_getter->doTopo()) {
			return;
		}

		Raster<coord_t> elev = _elevNumerator / _elevDenominator;
		
		writeRasterWithFullName(_getter, topoDir(), "Elevation", elev, OutputUnitLabel::Default);

		for (TopoMetric& metric : _topoMetrics) {
			Raster<metric_t> r = focal(elev, 3, metric.fun);
			writeRasterWithFullName(_getter, topoDir(), metric.name, r, metric.unit);
		}
	}
	std::filesystem::path ProductHandler::topoDir() const
	{
		return parentDir() / "Topography";
	}

	PointMetricHandler::PointMetricRasters::PointMetricRasters(ParamGetter* getter, const std::string& name, MetricFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit), rasters(getter)
	{
	}
	PointMetricHandler::StratumMetricRasters::StratumMetricRasters(ParamGetter* getter, const std::string& baseName, StratumFunc fun, OutputUnitLabel unit)
		: baseName(baseName), fun(fun), unit(unit)
	{
		for (size_t i = 0; i < getter->strataBreaks().size() + 1; ++i) {
			rasters.emplace_back(getter);
		}
	}
	PointMetricHandler::TwoRasters::TwoRasters(ParamGetter* getter)
	{
		if (getter->doAllReturnMetrics()) {
			all = Raster<metric_t>(*getter->metricAlign());
		}
		if (getter->doFirstReturnMetrics()) {
			first = Raster<metric_t>(*getter->metricAlign());
		}
	}
	Raster<metric_t>& PointMetricHandler::TwoRasters::get(ReturnType r)
	{
		if (r == ReturnType::ALL) {
			return all.value();
		}
		return first.value();
	}

	CsmHandler::CSMMetricRaster::CSMMetricRaster(ParamGetter* getter, const std::string& name, CsmMetricFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit), raster(*getter->metricAlign())
	{
	}
	TopoHandler::TopoMetric::TopoMetric(const std::string& name, TopoFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit)
	{
	}

}