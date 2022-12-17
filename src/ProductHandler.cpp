#include"app_pch.hpp"
#include"ProductHandler.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"
#include"csmalgos.hpp"

namespace lapis {

	template<class T>
	void writeRasterWithFullName(const std::filesystem::path& dir, const std::string& baseName, Raster<T>& r, OutputUnitLabel u) {
		namespace fs = std::filesystem;
		LapisData& data = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();

		fs::create_directories(dir);
		std::string unitString;
		using oul = OutputUnitLabel;
		if (u == oul::Default) {
			Unit outUnit = data.metricAlign()->crs().getZUnits();
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
		std::string runName = data.name().size() ? data.name() + "_" : "";
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
	Raster<T> getEmptyRasterFromTile(cell_t tile, const Alignment& a, coord_t minBufferMeters) {
		std::shared_ptr<Alignment> layout = LapisData::getDataObject().layout();
		std::shared_ptr<Alignment> metricAlign = LapisData::getDataObject().metricAlign();
		Extent thistile = layout->extentFromCell(tile);

		//The buffering ensures CSM metrics don't suffer from edge effects
		coord_t bufferDist = convertUnits(minBufferMeters, LinearUnitDefs::meter, layout->crs().getXYUnits());
		if (bufferDist > 0) {
			bufferDist = std::max(std::max(metricAlign->xres(), metricAlign->yres()), bufferDist);
		}
		Extent bufferExt = Extent(thistile.xmin() - bufferDist, thistile.xmax() + bufferDist, thistile.ymin() - bufferDist, thistile.ymax() + bufferDist);
		return Raster<T>(crop(a, bufferExt, SnapType::ll));
	}
	std::string insertZeroes(int value, int maxvalue)
	{
		size_t nDigits = (size_t)(std::log10(maxvalue) + 1);
		size_t thisDigitCount = std::max(1ull, (size_t)(std::log10(value) + 1));
		return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
	}
	std::string nameFromLayout(cell_t cell) {
		const Alignment& layout = *LapisData::getDataObject().layout();
		return "Col" + insertZeroes(layout.colFromCell(cell) + 1, layout.ncol()) +
			"_Row" + insertZeroes(layout.rowFromCell(cell) + 1,layout.nrow());
	}
	std::filesystem::path ProductHandler::parentDir()
	{
		return LapisData::getDataObject().outFolder();
	}
	std::filesystem::path ProductHandler::tempDir()
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
	void assignPointsToCalculators(const LidarPointVector& points)
	{
		LapisData& data = LapisData::getDataObject();
		shared_raster<PointMetricCalculator> allReturnPMC = data.allReturnPMC();
		shared_raster<PointMetricCalculator> firstReturnPMC = data.firstReturnPMC();

		for (auto& p : points) {
			cell_t cell = data.metricAlign()->cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ data.cellMutex(cell) };
			if constexpr (ALL_RETURNS) {
				allReturnPMC->atCellUnsafe(cell).value().addPoint(p);
			}
			if constexpr (FIRST_RETURNS) {
				if (p.returnNumber == 1) {
					firstReturnPMC->atCellUnsafe(cell).value().addPoint(p);
				}
			}
		}
	}
	void processPMCCell(cell_t cell, PointMetricCalculator& pmc,
		std::vector<PointMetricRaster>& metrics, std::vector<StratumMetricRasters>& stratumMetrics) {
		for (PointMetricRaster& v : metrics) {
			MetricFunc& f = v.fun;
			(pmc.*f)(v.rast, cell);
		}
		for (StratumMetricRasters& v : stratumMetrics) {
			StratumFunc& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(pmc.*f)(v.rasters[i], cell, i);
			}
		}
		pmc.cleanUp();
	}
	void writePointMetricRasters(const std::filesystem::path& dir, std::vector<PointMetricRaster>& metrics,
		std::vector<StratumMetricRasters>& stratumMetrics) {

		LapisLogger& log = LapisLogger::getLogger();

		for (PointMetricRaster& metric : metrics) {
			writeRasterWithFullName(dir, metric.name, metric.rast, metric.unit);
			log.incrementTask();
		}
		for (StratumMetricRasters& metric : stratumMetrics) {
			for (size_t i = 0; i < metric.stratumNames.size(); ++i) {
				writeRasterWithFullName(dir / "StratumMetrics", metric.baseName + metric.stratumNames[i], metric.rasters[i], metric.unit);
				log.incrementTask();
			}
		}
	}

	PointMetricHandler::PointMetricHandler()
	{}
	void PointMetricHandler::handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index)
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doPointMetrics()) {
			return;
		}

		//This structure is kind of ugly and inelegant, but it ensures that all returns and first returns can share a call to cellFromXY
		//without needing to pollute the loop with a bunch of if checks
		//Right now, with only two booleans, the combinatorics are bearable; if it increases, then it probably won't be
		if (data.doFirstReturnMetrics() && data.doAllReturnMetrics()) {
			assignPointsToCalculators<true, true>(points);
		}
		else if (data.doAllReturnMetrics()) {
			assignPointsToCalculators<true, false>(points);
		}
		else if (data.doFirstReturnMetrics()) {
			assignPointsToCalculators<false, true>(points);
		}

		std::vector<cell_t> cells = data.nLazRaster()->cellsFromExtent(e, SnapType::out);
		for (cell_t cell : cells) {
			std::scoped_lock lock{ data.cellMutex(cell) };
			shared_raster<int> nLaz = data.nLazRaster();
			nLaz->atCellUnsafe(cell).value()--;
			if (nLaz->atCellUnsafe(cell).value() != 0) {
				break;
			}
			if (data.doAllReturnMetrics())
				processPMCCell(cell, data.allReturnPMC()->atCellUnsafe(cell).value(),
					data.allReturnPointMetrics(), data.allReturnStratumMetrics());

			if (data.doFirstReturnMetrics())
				processPMCCell(cell, data.firstReturnPMC()->atCellUnsafe(cell).value(),
					data.firstReturnPointMetrics(), data.firstReturnStratumMetrics());
		}

	}
	void PointMetricHandler::handleTile(cell_t tile) {}
	void PointMetricHandler::cleanup() {
		LapisData& data = LapisData::getDataObject();
		namespace fs = std::filesystem;

		if (!data.doPointMetrics()) {
			return;
		}
		
		if (data.doAllReturnMetrics()) {
			fs::path allReturnsMetricDir = data.doFirstReturnMetrics() ? pointMetricDir() / "AllReturns" : pointMetricDir();

			writePointMetricRasters(allReturnsMetricDir,
				data.allReturnPointMetrics(),
				data.allReturnStratumMetrics());
		}
		if (data.doFirstReturnMetrics()) {
			fs::path firstReturnsMetricDir = data.doAllReturnMetrics() ? pointMetricDir() / "FirstReturns" : pointMetricDir();

			writePointMetricRasters(firstReturnsMetricDir,
				data.firstReturnPointMetrics(),
				data.firstReturnStratumMetrics());
		}

		data.allReturnPointMetrics().clear();
		data.allReturnPointMetrics().shrink_to_fit();
		data.allReturnStratumMetrics().clear();
		data.allReturnStratumMetrics().shrink_to_fit();

		data.firstReturnPointMetrics().clear();
		data.firstReturnPointMetrics().shrink_to_fit();
		data.firstReturnStratumMetrics().clear();
		data.firstReturnStratumMetrics().shrink_to_fit();
	}
	std::filesystem::path PointMetricHandler::pointMetricDir()
	{
		return parentDir() / "PointMetrics";
	}

	void CsmHandler::handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index)
	{
		LapisData& data = LapisData::getDataObject();

		if (!data.doCsm()) {
			return;
		}

		const coord_t footprintRadius = data.footprintDiameter() / 2;

		Raster<csm_t> csm{ crop(*data.csmAlign(),
			Extent(e.xmin() - footprintRadius,e.xmax() + footprintRadius,e.ymin() - footprintRadius,e.ymax() + footprintRadius)) };


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
	void CsmHandler::handleTile(cell_t tile) 
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doCsm()) {
			return;
		}

		std::shared_ptr<Alignment> layout = data.layout();
		std::shared_ptr<Alignment> csmAlign = data.csmAlign();

		Extent thistile = layout->extentFromCell(tile);

		Raster<csm_t> fullTile = getEmptyRasterFromTile<csm_t>(tile, *csmAlign, 30);
		
		Extent extentWithData;
		bool extentInit = false;

		auto& lasFiles = data.sortedLasList();
		for (size_t i = 0; i < lasFiles.size(); ++i) {
			Extent thisext = lasFiles[i].ext;

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			if (!thisext.overlaps(fullTile)) {
				continue;
			}
			thisext = crop(thisext, fullTile);
			Raster<csm_t> thisr;
			std::string filename = (tempDir() / (std::to_string(i) + ".tif")).string();
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
				extentWithData = extend(extentWithData, thisr);
			}


			//for the same reason as the above comment
			thisr.defineCRS(fullTile.crs());
			fullTile.overlay(thisr, [](csm_t a, csm_t b) {return std::max(a, b); });
		}

		if (!fullTile.hasAnyValue()) {
			return;
		}

		int neighborsNeeded = 6;
		fullTile = smoothAndFill(fullTile, data.smooth(), neighborsNeeded, {});

		for (CSMMetric& metric : data.csmMetrics()) {
			Raster<metric_t> tileMetric = aggregate(fullTile, crop(*data.metricAlign(), fullTile, SnapType::out), metric.f);
			metric.r.overlayInside(tileMetric);
		}
		
		Extent cropExt = layout->extentFromCell(tile); //unbuffered extent
		cropExt = crop(cropExt, extentWithData);
		fullTile = crop(fullTile, cropExt, SnapType::out);

		std::string outname = "CanopySurfaceModel_" + nameFromLayout(tile);
		writeRasterWithFullName(csmDir(), outname, fullTile, OutputUnitLabel::Default);
	}
	void CsmHandler::cleanup() 
	{
		LapisData& data = LapisData::getDataObject();

		if (!data.doCsm()) {
			return;
		}
		std::filesystem::remove_all(csmTempDir());
		deleteTempDirIfEmpty();

		for (CSMMetric& metric : data.csmMetrics()) {
			writeRasterWithFullName(csmMetricDir(), metric.name, metric.r, metric.unit);
		}
		data.csmMetrics().clear();
		data.csmMetrics().shrink_to_fit();
	}
	std::filesystem::path CsmHandler::csmTempDir()
	{
		return tempDir() / "CSM";
	}
	std::filesystem::path CsmHandler::csmDir()
	{
		return parentDir() / "CanopySurfaceModel";
	}
	std::filesystem::path CsmHandler::csmMetricDir()
	{
		return parentDir() / "CanopyMetrics";
	}

	void writeHighPointsAsXYZArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm, const Extent& unbufferedExtent, cell_t tile) {
		std::filesystem::path fileName = TaoHandler::taoTempDir() / (tile + ".tmp");
		std::ofstream ofs{ fileName,std::ios::binary };

		if (!ofs) {
			LapisLogger::getLogger().logMessage("Error writing to " + fileName.string());
			return;
		}

		for (cell_t cell : highPoints) {
			coord_t x = csm.xFromCellUnsafe(cell);
			coord_t y = csm.yFromCellUnsafe(cell);
			if (!unbufferedExtent.contains(x, y)) {
				continue;
			}

			ofs << x << y << csm[cell].value();
		}
	}
	std::vector<CoordXYZ> readHighPointsFromXYZArray(cell_t tile) {
		std::filesystem::path fileName = TaoHandler::taoTempDir() / (tile + ".tmp");
		std::ifstream ifs{ fileName,std::ios::binary };
		if (!ifs) {
			LapisLogger::getLogger().logMessage("Error reading from " + fileName.string());
			return std::vector<CoordXYZ>();
		}

		coord_t x, y;
		csm_t z;
		std::vector<CoordXYZ> out;
		while (ifs) {
			ifs >> x;
			ifs >> y;
			ifs >> z;
			out.emplace_back(x, y, z);
		}
		return out;
	}
	void writeHighPointsAsShp(const Raster<taoid_t>& segments, cell_t tile) {
		namespace fs = std::filesystem;

		GDALRegisterWrapper::allRegister();
		fs::create_directories(TaoHandler::taoDir());
		fs::path filename = (TaoHandler::taoDir() / ("TAOs_" + nameFromLayout(tile) + ".shp")).string();

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

		std::vector<CoordXYZ> highPoints = readHighPointsFromXYZArray(tile);


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
		LapisData& data = LapisData::getDataObject();
		
		auto doubleUnbuffer = [](coord_t bufferedValue, coord_t unbufferedValue) -> coord_t {
			return 2. * unbufferedValue - unbufferedValue;
		};
		//this is the extent where the TAOs are so far in that there's no chance of their ID not being final
		//(excluding extreme edge cases, which we're intentionally ignoring for computational reasons)
		Extent unchanging = Extent(
			doubleUnbuffer(segments.xmin(),unbufferedExtent.xmin()),
			doubleUnbuffer(segments.xmax(),unbufferedExtent.xmax()),
			doubleUnbuffer(segments.ymin(),unbufferedExtent.ymin()),
			doubleUnbuffer(segments.ymax(),unbufferedExtent.ymax())
			);


		{
			std::scoped_lock<std::mutex> lock(data.globalMutex());
			idMap.tileToLocalNames.emplace(tileidx, TaoIdMap::IDToCoord());
		}
		for (cell_t cell : highPoints) {
			coord_t x = segments.xFromCellUnsafe(cell);
			coord_t y = segments.yFromCellUnsafe(cell);
			if (unchanging.contains(x, y)) { //not an ID that will need changing
				continue;
			}
			if (unbufferedExtent.contains(x, y)) { //this tao id is id that other tiles will have to match
				std::scoped_lock<std::mutex> lock(data.globalMutex());
				idMap.coordsToFinalName.emplace(TaoIdMap::XY{ x,y }, segments[cell].value());
			}
			else { //this tao id will have to change eventually
				idMap.tileToLocalNames[tileidx].emplace(segments[cell].value(), TaoIdMap::XY{ x,y });
			}
		}
	}
	void TaoHandler::_fixTaoIdsThread(cell_t tile) const
	{
		LapisLogger& log = LapisLogger::getLogger();

		std::string tileName = nameFromLayout(tile);
		

		Raster<taoid_t> segments;
		std::string name = (taoTempDir() / ("Segments" + tileName + ".tif")).string();
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
		writeRasterWithFullName(taoDir(), "Segments" + tileName, segments, OutputUnitLabel::Unitless);
		writeHighPointsAsShp(segments, tile);
	}
	void TaoHandler::handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index)
	{
	}
	void TaoHandler::handleTile(cell_t tile) 
	{
		LapisData& data = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();
		if (!data.doTaos()) {
			return;
		}

		Alignment& layout = *data.layout();

		Extent unbufferedExtent = layout.extentFromCell(tile);
		Raster<csm_t> bufferedCsm = getEmptyRasterFromTile<csm_t>(tile, *data.csmAlign(), 30.);
		
		rowcol_t row = layout.rowFromCellUnsafe(tile);
		rowcol_t col = layout.colFromCellUnsafe(tile);
		for (rowcol_t nudgedRow : {row - 1, row, row + 1}) {
			for (rowcol_t nudgedCol : {col - 1, col, col + 1}) {
				if (nudgedRow < 0 || nudgedCol < 0 || nudgedRow >= layout.nrow() || nudgedCol >= layout.ncol()) {
					continue;
				}
				cell_t nudgedCell = layout.cellFromRowColUnsafe(nudgedRow, nudgedCol);
				Extent nudgedExtent = crop(bufferedCsm, layout.extentFromCell(nudgedCell));

				std::filesystem::path fileName = CsmHandler::csmDir() / ("CanopySurfaceModel_" + nameFromLayout(nudgedCell));

				Raster<csm_t> nudgedCsm;
				try {
					nudgedCsm = Raster<csm_t>(fileName.string(), nudgedExtent);
				}
				catch (InvalidRasterFileException e) {
					log.logMessage("Error opening " + fileName.string());
					continue;
				}

				bufferedCsm.overlay(nudgedCsm, [](csm_t a, csm_t b) {return a; });
			}
		}

		if (!bufferedCsm.hasAnyValue()) {
			return;
		}

		std::vector<cell_t> highPoints = identifyHighPointsWithMinDist(bufferedCsm, data.minTaoHt(), data.minTaoDist());
		Raster<taoid_t> segments = watershedSegment(bufferedCsm, highPoints, tile, layout.ncell());
		_updateMap(segments, highPoints, unbufferedExtent, tile);

		Raster<csm_t> maxHeight = maxHeightBySegment(segments, bufferedCsm, highPoints);

		segments = crop(segments, unbufferedExtent, SnapType::out);
		maxHeight = crop(maxHeight, unbufferedExtent, SnapType::out);

		std::string tileName = nameFromLayout(tile);

		writeHighPointsAsXYZArray(highPoints, bufferedCsm, unbufferedExtent, tile);
		writeTempRaster(taoTempDir(), "Segments_" + tileName, segments);
		writeRasterWithFullName(taoDir(), "MaxHeight_" + tileName, maxHeight, OutputUnitLabel::Default);
		segments = Raster<taoid_t>();
		maxHeight = Raster<csm_t>();
	}
	void TaoHandler::cleanup()
	{
		LapisData& data = LapisData::getDataObject();

		if (!data.doTaos()) {
			return;
		}

		uint64_t sofar = 0;
		auto threadfunc = [&]() {
			distributeWork(sofar, data.layout()->ncell(), [&](cell_t tile) {this->_fixTaoIdsThread(tile); }, data.globalMutex());
		};
		std::vector<std::thread> threads;
		for (int i = 0; i < data.nThread(); ++i) {
			threads.push_back(std::thread(threadfunc));
		}
		for (int i = 0; i < data.nThread(); ++i) {
			threads[i].join();
		}

		std::filesystem::remove_all(taoTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path TaoHandler::taoDir()
	{
		return parentDir() / "TreeApproximateObjects";
	}

	std::filesystem::path TaoHandler::taoTempDir()
	{
		return tempDir() / "TAO";
	}

	void FineIntHandler::handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index)
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator{ crop(*data.fineIntAlign(),e) };
		Raster<intensity_t> denominator{ (Alignment)numerator };

		coord_t cutoff = data.fineIntCanopyCutoff();
		for (auto& p : points) {
			if (p.z < cutoff) {
				continue;
			}
			cell_t cell = numerator.cellFromXYUnsafe(p.x, p.y);
			numerator[cell].value() += p.intensity;
			denominator[cell].value()++;
		}

		//there are many more points than cells, so rearranging works to be O(cells) instead of O(points) is generally worth it, even if it's a bit awkward
		for (cell_t cell = 0; cell < denominator.ncell(); ++cell) {
			if (denominator[cell].value() > 0) {
				denominator[cell].has_value() = true;
				numerator[cell].has_value() = true;
			}
		}

		writeTempRaster(fineIntTempDir(), std::to_string(index) + "_num", numerator);
		writeTempRaster(fineIntTempDir(), std::to_string(index) + "_denom", denominator);
	}
	void FineIntHandler::handleTile(cell_t tile)
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doFineInt()) {
			return;
		}

		Raster<intensity_t> numerator = getEmptyRasterFromTile<intensity_t>(tile, *data.fineIntAlign(), 0.);
		Raster<intensity_t> denominator{ (Alignment)numerator };

		Extent extentWithData;
		bool extentInit = false;

		auto& lasFiles = data.sortedLasList();
		for (size_t i = 0; i < lasFiles.size(); ++i) {
			Extent thisext = lasFiles[i].ext;

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			Raster<intensity_t> thisNumerator, thisDenominator;

			try {
				thisNumerator = Raster<intensity_t>{ (fineIntTempDir() / (std::to_string(i) + "_num.tif")).string(), thisext, SnapType::near };
				thisDenominator = Raster<intensity_t>{ (fineIntTempDir() / (std::to_string(i) + "_denom.tif")).string(), thisext, SnapType::near };
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
				extentWithData = extend(extentWithData, numerator);
			}
		}

		if (!denominator.hasAnyValue()) {
			return;
		}

		std::string tileName = nameFromLayout(tile);

		Raster<intensity_t> meanIntensity = numerator / denominator;
		meanIntensity = crop(meanIntensity, extentWithData);
		writeRasterWithFullName(fineIntDir(), "MeanCanopyIntensity_" + tileName, meanIntensity, OutputUnitLabel::Unitless);
	}
	void FineIntHandler::cleanup() {
		std::filesystem::remove_all(fineIntTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path FineIntHandler::fineIntDir()
	{
		return  parentDir() / "Intensity";
	}
	std::filesystem::path FineIntHandler::fineIntTempDir()
	{
		return tempDir() / "Intensity";
	}

	void TopoHandler::handlePoints(const LidarPointVector& points, const Raster<coord_t>& dem, const Extent& e, size_t index)
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doTopo()) {
			return;
		}

		Raster<coord_t> coarseSum = aggregate<coord_t, coord_t>(dem, *data.metricAlign(), &viewSum<coord_t>);
		data.elevNum()->overlay(coarseSum, [](coord_t a, coord_t b) {return a + b; });

		Raster<coord_t> coarseCount = aggregate<coord_t, coord_t>(dem, *data.metricAlign(), &viewCount<coord_t>);
		data.elevDenom()->overlay(coarseCount, [](coord_t a, coord_t b) {return a + b; });
	}
	void TopoHandler::handleTile(cell_t tile)
	{
	}
	void TopoHandler::cleanup()
	{
		LapisData& data = LapisData::getDataObject();
		if (!data.doTopo()) {
			return;
		}

		Raster<coord_t> elev = *data.elevNum() / *data.elevDenom();
		
		writeRasterWithFullName(topoDir(), "Elevation", elev, OutputUnitLabel::Default);

		for (auto& metric : data.topoMetrics()) {
			Raster<metric_t> r = focal(elev, 3, metric.metric);
			writeRasterWithFullName(topoDir(), metric.name, r, metric.unit);
		}
	}
	std::filesystem::path TopoHandler::topoDir()
	{
		return parentDir() / "Topography";
	}

}