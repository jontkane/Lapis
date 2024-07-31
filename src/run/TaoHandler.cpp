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

	bool TaoHandler::doThisProduct()
	{
		return _getter->doTaos();
	}

	std::string TaoHandler::name()
	{
		return "TAOs";
	}

	void TaoHandler::_writeHighPointsAsArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& bufferedCsm, const Raster<taoid_t>& bufferedSegments, 
		const Extent& unbufferedExtent, cell_t tile) const {

		std::filesystem::create_directories(taoTempDir());

		std::filesystem::path fileName = getFullTempFilename(taoTempDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "tmp");
		std::ofstream ofs{ fileName,std::ios::binary };

		if (!ofs) {
			LapisLogger::getLogger().logWarning("Error writing to " + fileName.string());
			return;
		}

		auto writeBytes = [&](auto x) {
			ofs.write((const char*)&x, sizeof(x));
		};

		LinearUnitConverter converter{ bufferedSegments.crs().getXYLinearUnits(), bufferedSegments.crs().getZUnits() };
		const coord_t cellArea = converter(bufferedSegments.xres()) * converter(bufferedSegments.yres());

		std::unordered_map<taoid_t, coord_t> areas;
		for (cell_t cell = 0; cell < bufferedSegments.ncell(); ++cell) {
			if (bufferedSegments[cell].has_value()) {
				areas.try_emplace(bufferedSegments[cell].value(), 0);
				areas[bufferedSegments[cell].value()] += cellArea;
			}
		}

		for (cell_t cell : highPoints) {
			coord_t x = bufferedCsm.xFromCellUnsafe(cell);
			coord_t y = bufferedCsm.yFromCellUnsafe(cell);
			if (!unbufferedExtent.contains(x, y)) {
				continue;
			}
			writeBytes(x);
			writeBytes(y);
			writeBytes(bufferedCsm[cell].value());
			writeBytes(areas[bufferedSegments[cell].value()]);
		}
	}
	std::vector<TaoHandler::TaoInfo> TaoHandler::_readHighPointsFromArray(cell_t tile) const {
		std::filesystem::path fileName = getFullTempFilename(taoTempDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "tmp");
		std::ifstream ifs{ fileName,std::ios::binary };
		if (!ifs) {
			LapisLogger::getLogger().logWarning("Error reading from " + fileName.string());
			return std::vector<TaoInfo>();
		}

		coord_t x, y;
		csm_t height;
		coord_t area;
		std::vector<TaoInfo> out;
		out.reserve(std::filesystem::file_size(fileName) / sizeof(TaoInfo));

		auto readBytes = [&](auto* x) {
			ifs.read((char*)x, sizeof(*x));
		};

		while (ifs) {
			readBytes(&x);
			readBytes(&y);
			readBytes(&height);
			readBytes(&area);
			if (ifs.eof()) {
				break;
			}
			out.emplace_back(x, y, height, area);
		}
		return out;
	}
	void TaoHandler::_writeHighPointsAsShp(const Raster<taoid_t>& segments, const std::vector<TaoInfo>& highPoints, cell_t tile) const {
		namespace fs = std::filesystem;

		gdalAllRegisterThreadSafe();
		fs::create_directories(taoDir());
		fs::path filename = getFullTileFilename(taoDir(), _taoBasename, OutputUnitLabel::Unitless, tile, "shp");

		UniqueGdalDataset outshp = gdalCreateWrapper("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);

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

		for (const TaoInfo& highPoint : highPoints) {
			if (!segments.contains(highPoint.x, highPoint.y)) {
				continue;
			}

			taoid_t id = segments.atXYUnsafe(highPoint.x, highPoint.y).value();

			UniqueOGRFeature feature = createFeatureWrapper(layer);
			feature->SetField("ID", (int64_t)id);
			feature->SetField("X", highPoint.x);
			feature->SetField("Y", highPoint.y);
			feature->SetField("Height", highPoint.height);
			feature->SetField("Area", highPoint.area);

			feature->SetField("Radius", std::sqrt(highPoint.area / M_PI));

			OGRPoint point;
			point.setX(highPoint.x);
			point.setY(highPoint.y);
			feature->SetGeometry(&point);

			layer->CreateFeature(feature.get());
		}
	}
	void TaoHandler::_updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx)
	{
		{
			std::scoped_lock<std::mutex> lock(_getter->globalMutex());
			idMap.tileToLocalNames.try_emplace(tileidx, TaoIdMap::IDToCoord());
		}
		for (cell_t cellInTile : highPoints) {
			coord_t x = segments.xFromCellUnsafe(cellInTile);
			coord_t y = segments.yFromCellUnsafe(cellInTile);
			cell_t cellFullAlign = RunParameters::singleton().csmAlign()->cellFromXYUnsafe(x, y);

			//in theory, you could identify areas that belong only to this tile, and aren't even in the buffers of any other tiles
			//this is easy in a normal case, but kind of annoying to account for edge cases
			//this could save a lot of memory here, if it ends up being necessary to do so

			if (unbufferedExtent.contains(x, y)) { //this tao id is id that other tiles will have to match
				std::scoped_lock<std::mutex> lock(_getter->globalMutex());
				idMap.cellToFinalName.try_emplace(cellFullAlign, segments[cellInTile].value());
			}
			else { //this tao id will have to change eventually
				idMap.tileToLocalNames[tileidx].try_emplace(segments[cellInTile].value(), cellFullAlign);
			}
		}
	}
	Raster<taoid_t> TaoHandler::_fixTaoIdsThread(cell_t tile) const
	{
		LapisLogger& log = LapisLogger::getLogger();

		Raster<taoid_t> segments;
		std::string name = getFullTileFilename(taoTempDir(), _segmentsBasename, OutputUnitLabel::Unitless, tile).string();
		if (!std::filesystem::exists(name)) {
			return segments;
		}
		try {
			segments = Raster<taoid_t>{ name };
		}
		catch (InvalidRasterFileException e) {
			log.logWarning("Error opening " + name);
			return segments;
		}

		const TaoIdMap::IDToCoord& localNameMap = idMap.tileToLocalNames.at(tile);
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			auto v = segments[cell];
			if (!v.has_value()) {
				continue;
			}
			if (!localNameMap.contains(v.value())) {
				continue;
			}
			if (!idMap.cellToFinalName.contains(localNameMap.at(v.value()))) { //edge of the acquisition
				continue;
			}
			v.value() = idMap.cellToFinalName.at(localNameMap.at(v.value()));
		}
		return segments;
	}
	TaoHandler::TaoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void TaoHandler::prepareForRun()
	{
		tryRemove(taoDir());
		tryRemove(taoTempDir());
	}
	void TaoHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
	}
	void TaoHandler::finishLasFile(const Extent& e, size_t index)
	{
	}
	void TaoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void TaoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		LapisLogger& log = LapisLogger::getLogger();
		if (!bufferedCsm.hasAnyValue()) {
			return;
		}

		Extent unbufferedExtent = _getter->layout()->extentFromCell(tile);

		log.beginVerboseBenchmarkTimer("Identifying TAOs");
		std::vector<cell_t> highPoints = _getter->taoIdAlgorithm()->identifyTaos(bufferedCsm);
		log.endVerboseBenchmarkTimer("Identifying TAOs");
		log.beginVerboseBenchmarkTimer("Segmenting TAOs");
		GenerateIdByTile idGenerator{ _getter->layout()->ncell(),tile };
		Raster<taoid_t> segments = _getter->taoSegAlgorithm()->segment(bufferedCsm, highPoints, idGenerator);
		log.endVerboseBenchmarkTimer("Segmenting TAOs");

		_updateMap(segments, highPoints, unbufferedExtent, tile);

		log.beginVerboseBenchmarkTimer("Calulating TAO max height");
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
		log.endVerboseBenchmarkTimer("Calculating TAO max height");

		_writeHighPointsAsArray(highPoints, bufferedCsm, segments, unbufferedExtent, tile);

		segments = cropRaster(segments, unbufferedExtent, SnapType::out);
		maxHeight = cropRaster(maxHeight, unbufferedExtent, SnapType::out);

		writeRasterLogErrors(getFullTileFilename(taoTempDir(), _segmentsBasename, OutputUnitLabel::Unitless, tile), segments);
		writeRasterLogErrors(getFullTileFilename(taoDir(), _maxHeightBasename, OutputUnitLabel::Default, tile), maxHeight);
	}
	void TaoHandler::cleanup()
	{
		LapisLogger::getLogger().setProgress("Assigning Unique TAO IDs");
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
						Raster<taoid_t> fixedSegments = _fixTaoIdsThread(thisidx);
						if (!fixedSegments.hasAnyValue()) {
							continue;
						}
						writeRasterLogErrors(getFullTileFilename(taoDir(), _segmentsBasename, OutputUnitLabel::Unitless, thisidx), fixedSegments);
						std::vector<TaoInfo> highPoints = _readHighPointsFromArray(thisidx);
						_writeHighPointsAsShp(fixedSegments, highPoints, thisidx);
					}
				}
			));
		}
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads[i].join();
		}

		tryRemove(taoTempDir());
		deleteTempDirIfEmpty();
	}
	void TaoHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Tree-Approximate Objects");
		pdf.writeTextBlockWithWrap("Tree-approximate objects (TAOs) represent Lapis' best guess at where individual trees are. The name TAO reflects "
			"the uncertainty inherent in this task.");

		_getter->taoIdAlgorithm()->describeInPdf(pdf, _getter);
		_getter->taoSegAlgorithm()->describeInPdf(pdf, _getter);

		pdf.writeSubsectionTitle("Products");
		pdf.writeTextBlockWithWrap("The output TAO data can be found in the TreeApproximateObjects directory. There are three kinds of products: the TAOs themselves, the segment "
			"rasters, and the max height rasters. To avoid unusably large filesizes, they are tiled. Their filenames indicate the row and olumn each tile belongs to. "
			"The location of each tile is available in TileLayout.shp, in the Layout directory.");

		pdf.writeSubsectionTitle("TAOs");
		std::stringstream ss;
		ss << "The TAOs themselves are stored in files with names like " << getFullTileFilename("", _taoBasename, OutputUnitLabel::Unitless, 0, "shp") << ". ";
		ss << "These are point vector files, whose locations represent the TAOs identified by the identification algorithm. ";
		ss << "There are six attributes in the attribute table. ID is a unique identifier for each TAO. X and Y are the coordinates of the TAO. ";
		ss << "Height is the height of the TAO, measured from the canopy surface model, in " << pdf.strToLower(_getter->unitPlural()) << ". ";
		ss << "Area is the area of the region assigned to each TAO by the segmentation algorithm, in square " << pdf.strToLower(_getter->unitPlural()) << ". ";
		ss << "Radius is the estimated crown radius of the TAO, derived directly from the area, as if the TAO was a circle, in " << pdf.strToLower(_getter->unitPlural()) << ".";
		pdf.writeTextBlockWithWrap(ss.str());

		pdf.writeSubsectionTitle("Segments");
		ss.str("");
		ss.clear();
		ss << "The files with names like " << getFullTileFilename("", _segmentsBasename, OutputUnitLabel::Unitless, 0) << " ";
		ss << "represent the division of the landscape into TAOs. The value of each pixel is 0 if that pixel does not belong to any TAO, ";
		ss << "or matches the ID of the TAO it belongs to, if any. The files have the same resolution as the canoyp surface model.";
		pdf.writeTextBlockWithWrap(ss.str());

		pdf.writeSubsectionTitle("Max Height");
		ss.str("");
		ss.clear();
		ss << "The files with names like " << getFullTileFilename("", _maxHeightBasename, OutputUnitLabel::Default, 0) << " ";
		ss << "are similar to the segments files, but instead of using the TAO's ID as their value, they use the TAO's height. ";
		ss << "These are thus similar in concept to a canopy surface model, but as if trees were a constant height, instead of having varying heights throughout their area.";
		pdf.writeTextBlockWithWrap(ss.str());

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