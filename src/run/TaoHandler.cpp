#include"run_pch.hpp"
#include"TaoHandler.hpp"
#include"..\parameters\RunParameters.hpp"
#include"LapisController.hpp"

namespace lapis {
	size_t TaoHandler::handlerRegisteredIndex = LapisController::registerHandler(new TaoHandler(&RunParameters::singleton()));
	void TaoHandler::reset()
	{
		*this = TaoHandler(_getter);
	}

	void TaoHandler::_writeHighPointsAsXYZArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm,
		const Extent& unbufferedExtent, cell_t tile) const {

		std::filesystem::create_directories(taoTempDir());

		std::filesystem::path fileName = getFullTempFilename(taoTempDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "tmp");
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
		std::filesystem::path fileName = getFullTempFilename(taoTempDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "tmp");
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
		fs::path filename = getFullTileFilename(taoDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "shp");

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

		Raster<taoid_t> segments;
		std::string name = getFullTileFilename(taoTempDir(), _segmentsBasename, OutputUnitLabel::Unitless, tile).string();
		if (!std::filesystem::exists(name)) {
			return;
		}
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
		writeRasterLogErrors(getFullTileFilename(taoDir(), _segmentsBasename, OutputUnitLabel::Unitless, tile), segments);
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

		std::vector<cell_t> highPoints = _getter->taoIdAlgorithm()->identifyTaos(bufferedCsm);
		GenerateIdByTile idGenerator{ _getter->layout()->ncell(),tile };
		Raster<taoid_t> segments = _getter->taoSegAlgorithm()->segment(bufferedCsm, highPoints, idGenerator);

		_updateMap(segments, highPoints, unbufferedExtent, tile);

		Raster<csm_t> maxHeight{ (Alignment)segments };
		std::unordered_map<taoid_t, csm_t> heightByID;
		for (cell_t c : highPoints) {
			heightByID.emplace(segments[c].value(), bufferedCsm[c].value());
		}
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			if (segments[cell].has_value()) {
				maxHeight[cell].has_value() = true;
				maxHeight[cell].value() = heightByID[segments[cell].value()];
			}
		}

		segments = cropRaster(segments, unbufferedExtent, SnapType::out);
		maxHeight = cropRaster(maxHeight, unbufferedExtent, SnapType::out);

		_writeHighPointsAsXYZArray(highPoints, bufferedCsm, unbufferedExtent, tile);
		writeRasterLogErrors(getFullTileFilename(taoTempDir(), _segmentsBasename, OutputUnitLabel::Unitless, tile), segments);
		writeRasterLogErrors(getFullTileFilename(taoDir(), _maxHeightBasename, OutputUnitLabel::Default, tile), maxHeight);
	}
	void TaoHandler::cleanup()
	{

		if (!_getter->doTaos()) {
			return;
		}

		cell_t sofar = 0;
		std::vector<std::thread> threads;
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads.push_back(std::thread(
				[&sofar, this]() {
					while (true) {
						cell_t thisidx;
						{
							std::lock_guard lock(_getter->globalMutex());
							if (sofar >= _getter->layout()->ncell()) {
								break;
							}
							thisidx = sofar;
							++sofar;
						}
						_fixTaoIdsThread(thisidx);
					}
				}
			));
		}
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads[i].join();
		}

		std::filesystem::remove_all(taoTempDir());
		deleteTempDirIfEmpty();
	}
	std::filesystem::path TaoHandler::taoDir() const
	{
		return parentDir() / "TreeApproximateObjects";
	}
	std::filesystem::path TaoHandler::taoTempDir() const
	{
		return tempDir() / "TAO";
	}
}