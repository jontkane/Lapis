#include"run_pch.hpp"
#include"LapisController.hpp"
#include"AllHandlers.hpp"
#include"..\parameters\RunParameters.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

#define LAPIS_CHECK_ABORT_AND_DEALLOC \
if (_needAbort) { \
	RunParameters::singleton().cleanAfterRun(); \
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
	}

	void LapisController::processFullArea()
	{
		_isRunning = true;
		LapisLogger& log = LapisLogger::getLogger();
		RunParameters& rp = RunParameters::singleton();
		log.reset();

		if (!rp.prepareForRun()) {
			sendAbortSignal();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		writeParams();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		std::vector<std::unique_ptr<ProductHandler>> handlers;
		handlers.push_back(std::make_unique<PointMetricHandler>(&rp));
		handlers.push_back(std::make_unique<CsmHandler>(&rp));
		handlers.push_back(std::make_unique<TaoHandler>(&rp));
		handlers.push_back(std::make_unique<FineIntHandler>(&rp));
		handlers.push_back(std::make_unique<TopoHandler>(&rp));
		
		rp.demAlgorithm()->setMinMax(rp.minHt(), rp.maxHt());

		log.setProgress(RunProgress::lasFiles, (int)rp.lasExtents().size());
		uint64_t soFar = 0;
		std::vector<std::thread> threads;
		auto lasThreadFunc = [&]() {
			_distributeWork(soFar, rp.lasExtents().size(), [&](size_t n) {this->lasThread(handlers, n); }, rp.globalMutex());
		};
		for (int i = 0; i < rp.nThread(); ++i) {
			threads.push_back(std::thread(lasThreadFunc));
		}
		for (int i = 0; i < rp.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;


		log.setProgress(RunProgress::csmTiles, (int)rp.layout()->ncell());
		soFar = 0;
		threads.clear();
		auto tileThreadFunc = [&]() {
			_distributeWork(soFar, rp.layout()->ncell(), [&](cell_t tile) {this->tileThread(handlers, tile); }, rp.globalMutex());
		};
		for (int i = 0; i < rp.nThread(); ++i) {
			threads.push_back(std::thread(tileThreadFunc));
		}
		for (int i = 0; i < rp.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		log.setProgress(RunProgress::cleanUp);
		cleanUp(handlers);
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		rp.cleanAfterRun();

		log.setProgress(RunProgress::finished);
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

	void LapisController::lasThread(HandlerVector& handlers, size_t n) const
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
		for (cell_t tile : rp.layout()->cellsFromExtent(projectedExtent, SnapType::out)) {
			auto v = rp.layout()->atCellUnsafe(tile);
			v.has_value() = true;
			v.value() = true;
		}
		
		auto ground = rp.demAlgorithm()->normalizeToGround(points, projectedExtent);
		points.clear();
		points.shrink_to_fit();
		LAPIS_CHECK_ABORT;

		for (size_t i = 0; i < handlers.size(); ++i) {
			handlers[i]->handlePoints(ground.points, projectedExtent, n);
			handlers[i]->handleDem(ground.dem, n);
			LAPIS_CHECK_ABORT;
		}
		LapisLogger::getLogger().incrementTask();
	}

	void LapisController::tileThread(HandlerVector& handlers, cell_t tile) const
	{
		Raster<csm_t> bufferedCsm;

		for (size_t i = 0; i < handlers.size(); ++i) {
			CsmHandler* csmHandler = dynamic_cast<CsmHandler*>(handlers[i].get());
			if (csmHandler) {
				bufferedCsm = csmHandler->getBufferedCsm(tile);
				break;
			}
		}
		if (!bufferedCsm.hasAnyValue()) {
			return;
		}

		for (size_t i = 0; i < handlers.size(); ++i) {
			handlers[i]->handleCsmTile(bufferedCsm, tile);
			LAPIS_CHECK_ABORT;
		}

		LapisLogger::getLogger().incrementTask();
	}

	void LapisController::cleanUp(HandlerVector& handlers) const
	{
		writeLayout();
		for (size_t i = 0; i < handlers.size(); ++i) {
			LAPIS_CHECK_ABORT;
			handlers[i]->cleanup();
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