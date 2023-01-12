#include"run_pch.hpp"
#include"LapisController.hpp"
#include"AllHandlers.hpp"
#include"..\parameters\RunParameters.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

#define LAPIS_CHECK_ABORT_AND_DEALLOC \
if (_needAbort) { \
	LapisLogger::getLogger().logMessage("Aborting"); \
	RunParameters::singleton().cleanAfterRun(); \
	LapisLogger::getLogger().setProgress("Aborted",0,false); \
	_isRunning = false; \
	return; \
}
#define LAPIS_CHECK_ABORT \
if (_needAbort) { \
	return; \
}

namespace lapis {

	LapisController::LapisController()
	{
		for (size_t i = 0; i < _handlers().size(); ++i) {
			_handlers()[i]->reset();
		}
	}
	void LapisController::processFullArea()
	{
		_isRunning = true;
		LapisLogger& log = LapisLogger::getLogger();
		RunParameters& rp = RunParameters::singleton();

		PJ* test_proj = proj_create(ProjContextByThread::get(), "EPSG:2927");
		if (test_proj == nullptr) {
			log.logMessage("proj.db not loaded");
			sendAbortSignal();
		}
		else {
			proj_destroy(test_proj);
#ifndef NDEBUG
			log.logMessage("proj.db successfully loaded");
#endif
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		if (!rp.prepareForRun()) {
			sendAbortSignal();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		writeParams();
		LAPIS_CHECK_ABORT_AND_DEALLOC;
		
		rp.demAlgorithm()->setMinMax(rp.minHt(), rp.maxHt());

		for (auto& p : _handlers()) {
			p->prepareForRun();
		}

		log.setProgress("Processing LAS Files", (int)rp.lasExtents().size());
		uint64_t soFar = 0;
		std::vector<std::thread> threads;
		auto lasThreadFunc = [&]() {
			_distributeWork(soFar, rp.lasExtents().size(), [&](size_t n) {this->lasThread(n); }, rp.globalMutex());
		};
		for (int i = 0; i < rp.nThread(); ++i) {
			threads.push_back(std::thread(lasThreadFunc));
		}
		for (int i = 0; i < rp.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;


		log.setProgress("Processing Tiles", (int)rp.layout()->ncell());
		soFar = 0;
		threads.clear();
		auto tileThreadFunc = [&]() {
			_distributeWork(soFar, rp.layout()->ncell(), [&](cell_t tile) {this->tileThread(tile); }, rp.globalMutex());
		};
		for (int i = 0; i < rp.nThread(); ++i) {
			threads.push_back(std::thread(tileThreadFunc));
		}
		for (int i = 0; i < rp.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		log.setProgress("Final Processing and Cleanup");
		cleanUp();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		rp.cleanAfterRun();

		log.setProgress("Done!",0,false);
		_isRunning = false;
	}

	bool LapisController::isRunning() const
	{
		return _isRunning;
	}

	void LapisController::sendAbortSignal()
	{
		_needAbort = true;
	}

	size_t LapisController::registerHandler(ProductHandler* handler)
	{
		_handlers().emplace_back(handler);
		return _handlers().size() - 1;
	}

	void LapisController::writeParams() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		RunParameters& rp = RunParameters::singleton();

		fs::path paramDir = rp.outFolder() / "RunParameters";
		fs::create_directories(paramDir);

		std::ofstream fullParams{ paramDir / "FullParameters.ini" };
		if (!fullParams) {
			log.logMessage("Could not open " + (paramDir / "FullParameters.ini").string() + " for writing");
		}
		else {
			rp.writeOptions(fullParams, ParamCategory::data);
			rp.writeOptions(fullParams, ParamCategory::process);
			rp.writeOptions(fullParams, ParamCategory::computer);
			
		}

		std::ofstream runAndComp{ paramDir / "ProcessingAndComputerParameters.ini" };
		if (!runAndComp) {
			log.logMessage("Could not open " + (paramDir / "ProcessingAndComputerParameters.ini").string() + " for writing");
		}
		else {
			rp.writeOptions(runAndComp, ParamCategory::process);
			rp.writeOptions(runAndComp, ParamCategory::computer);
		}

		std::ofstream data{ paramDir / "DataParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "DataParameters.ini").string() + " for writing");
		}
		else {
			rp.writeOptions(data, ParamCategory::data);
		}

		std::ofstream metric{ paramDir / "ProcessingParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "ProcessingParameters.ini").string() + " for writing");
		}
		else {
			rp.writeOptions(metric, ParamCategory::process);
		}

		std::ofstream computer{ paramDir / "ComputerParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "ComputerParameters.ini").string() + " for writing");
		}
		else {
			rp.writeOptions(computer, ParamCategory::computer);
		}
	}

	std::vector<std::unique_ptr<ProductHandler>>& LapisController::_handlers()
	{
		static std::vector<std::unique_ptr<ProductHandler>> handlers = std::vector<std::unique_ptr<ProductHandler>>();
		return handlers;
	}

	void LapisController::lasThread(size_t n)
	{
		RunParameters& rp = RunParameters::singleton();
		LapisLogger& log = LapisLogger::getLogger();

		LasReader lr;
		try {
			lr = rp.getLas(n);
		}
		catch (InvalidLasFileException e) {
			log.logMessage(e.what());
		}
		LAPIS_CHECK_ABORT;

		LidarPointVector points = lr.getPoints(lr.nPoints());
		LAPIS_CHECK_ABORT;

		Extent projectedExtent = QuadExtent(lr, rp.metricAlign()->crs()).outerExtent();
		points.transform(projectedExtent.crs());
		
		//also normalizes the point in-place
		Raster<coord_t> ground = rp.demAlgorithm()->normalizeToGround(points, projectedExtent);
		LAPIS_CHECK_ABORT;

		for (size_t i = 0; i < _handlers().size(); ++i) {
			_handlers()[i]->handlePoints(points, projectedExtent, n);
			_handlers()[i]->handleDem(ground, n);
			LAPIS_CHECK_ABORT;
		}
		LapisLogger::getLogger().incrementTask("Las File Finished");
	}

	void LapisController::tileThread(cell_t tile)
	{
		RunParameters& rp = RunParameters::singleton();

		CsmHandler* csmhandler = dynamic_cast<CsmHandler*>(_handlers()[CsmHandler::handlerRegisteredIndex].get());
		Raster<csm_t> bufferedCsm = csmhandler->getBufferedCsm(tile);

		if (!bufferedCsm.overlaps(*rp.layout())) { //indicates a filler value due to the buffered tile not having any data
			return;
		}
		CropView cv{ &bufferedCsm,rp.layout()->extentFromCell(tile),SnapType::out };
		if (!cv.hasAnyValue()) {
			return;
		}
		else {
			auto v = rp.layout()->atCellUnsafe(tile);
			v.has_value() = true;
			v.value() = true;
		}

		for (size_t i = 0; i < _handlers().size(); ++i) {
			_handlers()[i]->handleCsmTile(bufferedCsm, tile);
			LAPIS_CHECK_ABORT;
		}

		LapisLogger::getLogger().incrementTask("Tile Finished");
	}

	void LapisController::cleanUp()
	{
		writeLayout();
		for (size_t i = 0; i < _handlers().size(); ++i) {
			LAPIS_CHECK_ABORT;
			_handlers()[i]->cleanup();
		}
	}

	void LapisController::writeLayout() const {
		RunParameters& rp = RunParameters::singleton();

		Raster<bool>& layout = *rp.layout();

		fs::path layoutDir = rp.outFolder() / "Layout";
		fs::path filename = layoutDir / "TileLayout.shp";
		fs::create_directories(layoutDir);

		GDALRegisterWrapper::allRegister();

		GDALDatasetWrapper outshp("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(layout.crs().getCleanEPSG().getCompleteWKT().c_str());

		OGRLayer* layer;
		layer = outshp->CreateLayer("point_out", &crs, wkbPolygon, nullptr);

		OGRFieldDefn nameField("Name", OFTString);
		nameField.SetWidth(13);
		layer->CreateField(&nameField);

		OGRFieldDefn idField("ID", OFTInteger);
		layer->CreateField(&idField);

		OGRFieldDefn colField("Column", OFTInteger);
		layer->CreateField(&colField);

		OGRFieldDefn rowField("Row", OFTInteger);
		layer->CreateField(&rowField);
		for (cell_t cell = 0; cell < layout.ncell(); ++cell) {
			if (!layout.atCell(cell).has_value()) {
				continue;
			}
			OGRFeatureWrapper feature(layer);
			feature->SetField("Name", rp.layoutTileName(cell).c_str());
			feature->SetField("ID", cell);
			feature->SetField("Column", layout.colFromCell(cell)+1);
			feature->SetField("Row", layout.rowFromCell(cell)+1);
			
#pragma pack(push)
#pragma pack(1)
#pragma warning(push)
#pragma warning(disable: 26495)
			struct WkbRectangle {
				const uint8_t endianness = 1;
				const uint32_t type = 3;
				const uint32_t numRings = 1;
				const uint32_t numPoints = 5; //because the first point is repeated
				struct Point {
					double x, y;
				};

				Point points[5];
			};
#pragma warning(pop)
#pragma pack(pop)

			WkbRectangle tile;
			coord_t xCenter = layout.xFromCell(cell);
			coord_t yCenter = layout.yFromCell(cell);
			tile.points[0].x = tile.points[3].x = tile.points[4].x = xCenter - layout.xres() / 2;
			tile.points[1].x = tile.points[2].x = xCenter + layout.xres() / 2;
			tile.points[0].y = tile.points[1].y = tile.points[4].y = yCenter + layout.yres() / 2;
			tile.points[2].y = tile.points[3].y = yCenter - layout.yres() / 2;

			OGRGeometry* geom;
			size_t consumed = 0;
			OGRGeometryFactory::createFromWkb((const void*)&tile, &crs, &geom, sizeof(WkbRectangle), wkbVariantPostGIS1, consumed);

			feature->SetGeometry(geom);
			layer->CreateFeature(feature.ptr);
		}
	}

}