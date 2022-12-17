#include"app_pch.hpp"
#include"LapisController.hpp"
#include"DemAlgos.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

#define LAPIS_CHECK_ABORT_AND_DEALLOC \
if (data.needAbort) { \
	data.cleanAfterRun(); \
	_isRunning = false; \
	return; \
}
#define LAPIS_CHECK_ABORT \
if (LapisData::getDataObject().needAbort) { \
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
		LapisData& data = LapisData::getDataObject();
		log.reset();

		data.prepareForRun();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		writeParams();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		PointMetricCalculator::setInfo(data.canopyCutoff(), data.maxHt(), data.binSize(), data.strataBreaks());

		std::vector<std::unique_ptr<ProductHandler>> handlers;
		handlers.push_back(std::make_unique<PointMetricHandler>());
		handlers.push_back(std::make_unique<CsmHandler>());
		handlers.push_back(std::make_unique<TaoHandler>());
		handlers.push_back(std::make_unique<FineIntHandler>());
		handlers.push_back(std::make_unique<TopoHandler>());

		log.setProgress(RunProgress::lasFiles);
		uint64_t soFar = 0;
		std::vector<std::thread> threads;
		auto lasThreadFunc = [&]() {
			distributeWork(soFar, data.sortedLasList().size(), [&](size_t n) {this->lasThread(handlers, n); }, data.globalMutex());
		};
		for (int i = 0; i < data.nThread(); ++i) {
			threads.push_back(std::thread(lasThreadFunc));
		}
		for (int i = 0; i < data.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;


		log.setProgress(RunProgress::csmTiles);
		soFar = 0;
		threads.clear();
		auto tileThreadFunc = [&]() {
			distributeWork(soFar, data.layout()->ncell(), [&](cell_t tile) {this->tileThread(handlers, tile); }, data.globalMutex());
		};
		for (int i = 0; i < data.nThread(); ++i) {
			threads.push_back(std::thread(tileThreadFunc));
		}
		for (int i = 0; i < data.nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		cleanUp(handlers);
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		data.cleanAfterRun();

		log.setProgress(RunProgress::finished);
		_isRunning = false;
	}

	bool LapisController::isRunning() const
	{
		return _isRunning;
	}

	void LapisController::writeParams() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		auto& d = LapisData::getDataObject();

		fs::path paramDir = d.outFolder() / "RunParameters";
		fs::create_directories(paramDir);

		std::ofstream fullParams{ paramDir / "FullParameters.ini" };
		if (!fullParams) {
			log.logMessage("Could not open " + (paramDir / "FullParameters.ini").string() + " for writing");
		}
		else {
			d.writeOptions(fullParams, ParamCategory::data);
			d.writeOptions(fullParams, ParamCategory::process);
			d.writeOptions(fullParams, ParamCategory::computer);
			
		}

		std::ofstream runAndComp{ paramDir / "ProcessingAndComputerParameters.ini" };
		if (!runAndComp) {
			log.logMessage("Could not open " + (paramDir / "ProcessingAndComputerParameters.ini").string() + " for writing");
		}
		else {
			d.writeOptions(runAndComp, ParamCategory::process);
			d.writeOptions(runAndComp, ParamCategory::computer);
		}

		std::ofstream data{ paramDir / "DataParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "DataParameters.ini").string() + " for writing");
		}
		else {
			d.writeOptions(data, ParamCategory::data);
		}

		std::ofstream metric{ paramDir / "ProcessingParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "ProcessingParameters.ini").string() + " for writing");
		}
		else {
			d.writeOptions(metric, ParamCategory::process);
		}

		std::ofstream computer{ paramDir / "ComputerParameters.ini" };
		if (!data) {
			log.logMessage("Could not open " + (paramDir / "ComputerParameters.ini").string() + " for writing");
		}
		else {
			d.writeOptions(computer, ParamCategory::computer);
		}
	}

	void LapisController::lasThread(HandlerVector& handlers, size_t n) const
	{
		LapisData& data = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();

		LasReader lr;
		std::string filename = data.sortedLasList()[n].filename;
		try {
			lr = LasReader(filename);
		}
		catch (InvalidLasFileException e) {
			log.logMessage("Error opening " + filename);
		}
		LAPIS_CHECK_ABORT;

		if (!data.lasCrsOverride().isEmpty()) {
			lr.defineCRS(data.lasCrsOverride());
		}
		if (!data.lasUnitOverride().isUnknown()) {
			lr.setZUnits(data.lasUnitOverride());
		}
		for (auto& f : data.filters()) {
			lr.addFilter(f);
		}

		LidarPointVector points = lr.getPoints(lr.nPoints());
		LAPIS_CHECK_ABORT;

		Extent projectedExtent = QuadExtent(lr, data.metricAlign()->crs()).outerExtent();
		points.transform(projectedExtent.crs());
		for (cell_t tile : data.layout()->cellsFromExtent(projectedExtent, SnapType::out)) {
			auto v = data.layout()->atCellUnsafe(tile);
			v.has_value() = true;
			v.value() = true;
		}
		
		std::shared_ptr<DemAlgorithm> normalizer = data.demAlgorithm();
		PointsAndDem ground = normalizer->normalizeToGround(points, projectedExtent);
		points.clear();
		points.shrink_to_fit();
		LAPIS_CHECK_ABORT;

		for (size_t i = 0; i < handlers.size(); ++i) {
			handlers[i]->handlePoints(ground.points, ground.dem, projectedExtent, n);
			LAPIS_CHECK_ABORT;
		}
		LapisLogger::getLogger().incrementTask();
	}

	void LapisController::tileThread(HandlerVector& handlers, cell_t tile) const
	{
		for (size_t i = 0; i < handlers.size(); ++i) {
			handlers[i]->handleTile(tile);
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
		LapisData& data = LapisData::getDataObject();

		Raster<bool>& layout = *data.layout();

		fs::path layoutDir = data.outFolder() / "Layout";
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
			std::string name = nameFromLayout(cell);
			if (!layout.atCell(cell).has_value()) {
				continue;
			}
			OGRFeatureWrapper feature(layer);
			feature->SetField("Name", nameFromLayout(cell).c_str());
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