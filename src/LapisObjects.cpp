#include"app_pch.hpp"
#include"LapisObjects.hpp"

namespace lapis {

	LapisObjects::LapisObjects()
	{
		lasProcessingObjects = std::make_unique<LasProcessingObjects>();
		globalProcessingObjects = std::make_unique<GlobalProcessingObjects>();
	}
	LapisObjects::LapisObjects(const FullOptions& opt)
	{
		lasProcessingObjects = std::make_unique<LasProcessingObjects>();
		globalProcessingObjects = std::make_unique<GlobalProcessingObjects>();

		identifyLasFiles(opt);
		createOutAlignment(opt);
		sortLasFiles(opt);

		identifyDEMFiles(opt);
		setFilters(opt);
		setPointMetrics(opt);
		setTopoMetrics(opt);
		setCSMMetrics(opt);

		makeNLaz();

		finalParams(opt);
	}
	void LapisObjects::cleanUpAfterPointMetrics()
	{
		lasProcessingObjects.reset(nullptr);
	}
	void LapisObjects::identifyLasFiles(const FullOptions& opt)
	{
		std::vector<LasFileExtent> foundLaz = iterateOverFileSpecifiers<LasFileExtent>(opt.dataOptions.lasFileSpecifiers, &tryLasFile,
			globalProcessingObjects->log, lasProcessingObjects->lasCRSOverride, lasProcessingObjects->lasUnitOverride);
		std::unordered_set<std::string> unique;
		for (auto& v : foundLaz) {
			if (unique.count(v.filename)) {
				continue;
			}
			unique.insert(v.filename);
			globalProcessingObjects->sortedLasFiles.push_back(v);
		}
		globalProcessingObjects->log.logProgress(std::to_string(globalProcessingObjects->sortedLasFiles.size()) + " point cloud files identified");
	}

	void LapisObjects::identifyDEMFiles(const FullOptions& opt)
	{
		std::vector<DemFileAlignment> foundDem = iterateOverFileSpecifiers<DemFileAlignment>(opt.dataOptions.demFileSpecifiers, &tryDtmFile, globalProcessingObjects->log,
			opt.dataOptions.demCRS, opt.dataOptions.demUnits);
		std::unordered_set<std::string> unique;
		for (auto& v : foundDem) {
			if (unique.count(v.filename)) {
				continue;
			}
			unique.insert(v.filename);
			globalProcessingObjects->demFiles.push_back(v);
		}
		globalProcessingObjects->log.logProgress(std::to_string(globalProcessingObjects->demFiles.size()) + " DEM files identified");
	}

	void LapisObjects::setFilters(const FullOptions& opt)
	{
		auto& filters = lasProcessingObjects->filters;

		if (opt.processingOptions.whichReturns == ProcessingOptions::WhichReturns::only) {
			filters.push_back(std::make_shared<LasFilterOnlyReturns>());
		}
		else if (opt.processingOptions.whichReturns == ProcessingOptions::WhichReturns::first) {
			filters.push_back(std::make_shared<LasFilterFirstReturns>());
		}

		if (opt.processingOptions.classes.has_value()) {
			if (opt.processingOptions.classes.value().whiteList) {
				filters.push_back(std::make_shared<LasFilterClassWhitelist>(opt.processingOptions.classes.value().classes));
			}
			else {
				filters.push_back(std::make_shared<LasFilterClassBlacklist>(opt.processingOptions.classes.value().classes));
			}
		}
		if (!opt.processingOptions.useWithheld) {
			filters.push_back(std::make_shared<LasFilterWithheld>());
		}

		if (opt.processingOptions.maxScanAngle.has_value()) {
			filters.push_back(std::make_shared<LasFilterMaxScanAngle>(opt.processingOptions.maxScanAngle.value()));
		}
		globalProcessingObjects->log.logDiagnostic(std::to_string(filters.size()) + " filters applied");
	}

	void LapisObjects::setPointMetrics(const FullOptions& opt)
	{
		auto& pointMetrics = lasProcessingObjects->metricRasters;
		auto& metricAlign = globalProcessingObjects->metricAlign;
		auto& stratumMetrics = lasProcessingObjects->stratumRasters;

		auto addMetric = [&](std::string&& name, MetricFunc f) {
			pointMetrics.emplace_back(std::move(name), f, metricAlign);
		};

		addMetric("Mean_CanopyHeight", &PointMetricCalculator::meanCanopy);
		addMetric("StdDev_CanopyHeight", &PointMetricCalculator::stdDevCanopy);
		addMetric("25thPercentile_CanopyHeight", &PointMetricCalculator::p25Canopy);
		addMetric("50thPercentile_CanopyHeight", &PointMetricCalculator::p50Canopy);
		addMetric("75thPercentile_CanopyHeight", &PointMetricCalculator::p75Canopy);
		addMetric("95thPercentile_CanopyHeight", &PointMetricCalculator::p95Canopy);
		addMetric("TotalReturnCount", &PointMetricCalculator::returnCount);
		addMetric("CanopyCover", &PointMetricCalculator::canopyCover);


		auto& strataBreaks = opt.processingOptions.strataBreaks;
		if (strataBreaks.size()) {
			auto to_string_with_precision = [](coord_t v)->std::string {
				std::ostringstream out;
				out.precision(2);
				out << v;
				return out.str();
			};
			std::vector<std::string> stratumNames;
			stratumNames.push_back("LessThan" + to_string_with_precision(strataBreaks[0]));
			for (int i = 1; i < strataBreaks.size(); ++i) {
				stratumNames.push_back(to_string_with_precision(strataBreaks[i - 1]) + "To" + to_string_with_precision(strataBreaks[i]));
			}
			stratumNames.push_back("GreaterThan" + to_string_with_precision(strataBreaks[strataBreaks.size() - 1]));

			stratumMetrics.emplace_back("StratumCover_", stratumNames, &PointMetricCalculator::stratumCover, metricAlign);
			stratumMetrics.emplace_back("StratumReturnProportion_", stratumNames, &PointMetricCalculator::stratumPercent, metricAlign);
		}
		
	}

	void LapisObjects::setTopoMetrics(const FullOptions& opt) {
		lasProcessingObjects->topoMetrics.push_back({ viewSlope<coord_t,metric_t>,"Slope" });
		lasProcessingObjects->topoMetrics.push_back({ viewAspect<coord_t,metric_t>,"Aspect" });
	}

	void LapisObjects::setCSMMetrics(const FullOptions& opt) {
		auto& csmMetrics = globalProcessingObjects->csmMetrics;
		auto& align = globalProcessingObjects->metricAlign;
		csmMetrics.push_back({ &viewMax<csm_t>, "MaxCSM", align });
		csmMetrics.push_back({ &viewMean<csm_t>, "MeanCSM", align });
		csmMetrics.push_back({ &viewStdDev<csm_t>, "StdDevCSM", align });
		csmMetrics.push_back({ &viewRumple<csm_t>, "RumpleCSM", align });
	}

	void LapisObjects::createOutAlignment(const FullOptions& opt) {

		Alignment& metricAlign = globalProcessingObjects->metricAlign;

		bool alignFinalized = false;
		CoordRef outCRS;
		if (opt.processingOptions.outAlign.has_value()) {
			alignFinalized = true;
			outCRS = opt.processingOptions.outAlign.value().crs();
		}
		if (outCRS.isEmpty()) {
			for (size_t i = 0; i < globalProcessingObjects->sortedLasFiles.size(); ++i) {
				//if the user won't specify the CRS, just use the laz CRS
				if (!globalProcessingObjects->sortedLasFiles[i].ext.crs().isEmpty()) {
					outCRS = globalProcessingObjects->sortedLasFiles[i].ext.crs();
					if (globalProcessingObjects->cleanwkt) {
						outCRS = outCRS.getCleanEPSG();
					}
					break;
				}
			}
		}

		if (!opt.processingOptions.outUnits.isUnknown()) {
			outCRS.setZUnits(opt.processingOptions.outUnits);
		}
		
		if (!outCRS.isEmpty() && !outCRS.isProjected()) {
			globalProcessingObjects->log.logError("Latitude/Longitude output not supported");
			throw std::runtime_error("Latitude/Longitude output not supported");
		}

		Extent fullExtent;
		bool initExtent = false;

		for (size_t i = 0; i < globalProcessingObjects->sortedLasFiles.size(); ++i) {
			Extent& e = globalProcessingObjects->sortedLasFiles[i].ext;
			if (!e.crs().isConsistentHoriz(outCRS)) {
				QuadExtent q{ e, outCRS };
				e = q.outerExtent();
			}
			if (!initExtent) {
				fullExtent = e;
				initExtent = true;
			}
			else {
				fullExtent = extend(fullExtent, e);
			}
		}

		fullExtent.defineCRS(outCRS); //getting the units right and maybe cleaning up some wkt nonsense
		globalProcessingObjects->metricAlign.defineCRS(outCRS); //either harmless or cleans up the wkt
		if (!alignFinalized) {
			coord_t cellsize = convertUnits(30, linearUnitDefs::meter, outCRS.getXYUnits());
			metricAlign = Alignment(fullExtent, 0, 0, cellsize, cellsize);
		}
		else {
			metricAlign = opt.processingOptions.outAlign.value();
		}
		metricAlign = extend(metricAlign, fullExtent);
		metricAlign = crop(metricAlign, fullExtent);

		coord_t csmcellsize = 0;

		csmcellsize = convertUnits(opt.processingOptions.csmRes.value_or(0), opt.processingOptions.outUnits, outCRS.getXYUnits());
		if (csmcellsize <= 0) {
			csmcellsize = convertUnits(1, linearUnitDefs::meter, outCRS.getXYUnits());
		}
		globalProcessingObjects->csmAlign = Alignment(fullExtent, metricAlign.xOrigin(), metricAlign.yOrigin(), csmcellsize, csmcellsize);

		if (!opt.processingOptions.outUnits.isUnknown()) {
			globalProcessingObjects->metricAlign.setZUnits(opt.processingOptions.outUnits);
			globalProcessingObjects->csmAlign.setZUnits(opt.processingOptions.outUnits);
		}
		globalProcessingObjects->csmAlign.defineCRS(outCRS);
		

		globalProcessingObjects->log.logDiagnostic("Alignment calculated");
	}
	void LapisObjects::sortLasFiles(const FullOptions& opt)
	{
		//this just sorts north->south in a sort of naive way that ought to present a relatively minimal surface area when the data is tiled
		auto lasExtentSorter = [](const LasFileExtent& a, const LasFileExtent& b) {
			if (a.ext.ymax() > b.ext.ymax()) {
				return true;
			}
			if (a.ext.ymax() < b.ext.ymax()) {
				return false;
			}

			if (a.ext.ymin() > b.ext.ymin()) {
				return true;
			}
			if (a.ext.ymin() < b.ext.ymin()) {
				return false;
			}

			if (a.ext.xmax() > b.ext.xmax()) {
				return true;
			}
			if (a.ext.xmin() < b.ext.xmin()) {
				return false;
			}

			if (a.ext.xmax() > b.ext.xmax()) {
				return true;
			}
			if (a.ext.xmax() < b.ext.xmax()) {
				return false;
			}

			return true;
		};

		std::sort(globalProcessingObjects->sortedLasFiles.begin(), globalProcessingObjects->sortedLasFiles.end(), lasExtentSorter);
	}

	void LapisObjects::finalParams(const FullOptions& opt)
	{
		lasProcessingObjects->elevDenominator = Raster<coord_t>(globalProcessingObjects->metricAlign);
		lasProcessingObjects->elevNumerator = Raster<coord_t>(globalProcessingObjects->metricAlign);

		lasProcessingObjects->calculators = Raster<PointMetricCalculator>(globalProcessingObjects->metricAlign);
		lasProcessingObjects->cellMuts = std::vector<std::mutex>(LasProcessingObjects::mutexN);
		lasProcessingObjects->lasCRSOverride = opt.dataOptions.lasCRS;
		lasProcessingObjects->lasUnitOverride = opt.dataOptions.lasUnits;
		lasProcessingObjects->demCRSOverride = opt.dataOptions.demCRS;
		lasProcessingObjects->demUnitOverride = opt.dataOptions.demUnits;

		globalProcessingObjects->nThread = opt.computerOptions.nThread.value_or(defaultNThread());
		globalProcessingObjects->binSize = opt.computerOptions.performance ? 0.1 : 0.01;

		globalProcessingObjects->canopyCutoff = opt.processingOptions.canopyCutoff.
			value_or(convertUnits(2, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));

		globalProcessingObjects->minht = opt.processingOptions.minht.
			value_or(convertUnits(-2, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));
		globalProcessingObjects->maxht = opt.processingOptions.maxht.
			value_or(convertUnits(100, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));

		globalProcessingObjects->outfolder = opt.dataOptions.outfolder;

		PointMetricCalculator::setInfo(globalProcessingObjects->canopyCutoff, globalProcessingObjects->maxht, globalProcessingObjects->binSize,
			opt.processingOptions.strataBreaks);

		if (opt.processingOptions.footprintDiameter.has_value()) {
			lasProcessingObjects->footprintRadius = convertUnits(opt.processingOptions.footprintDiameter.value()/2,
				globalProcessingObjects->metricAlign.crs().getZUnits(),
				globalProcessingObjects->metricAlign.crs().getXYUnits());
		}
		else {
			lasProcessingObjects->footprintRadius = convertUnits(0.2,
				linearUnitDefs::meter,
				globalProcessingObjects->metricAlign.crs().getXYUnits());
		}

		globalProcessingObjects->smoothWindow = opt.processingOptions.smoothWindow.value_or(3);

		globalProcessingObjects->doFineIntensity = opt.processingOptions.doFineIntensity;
	}

	void LapisObjects::makeNLaz()
	{
		auto& nLaz = lasProcessingObjects->nLaz;
		nLaz = Raster<int>{ globalProcessingObjects->metricAlign };
		for (auto& file : globalProcessingObjects->sortedLasFiles) {
			std::vector<cell_t> cells = nLaz.cellsFromExtent(file.ext, SnapType::out);
			for (cell_t cell : cells) {
				nLaz[cell].value()++;
				nLaz[cell].has_value() = true;
			}
		}
	}

	template<class T>
	std::vector<T> LapisObjects::iterateOverFileSpecifiers(const std::vector<std::string>& specifiers, openFuncType<T> openFunc, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{

		namespace fs = std::filesystem;

		std::vector<T> fileList;

		std::queue<std::string> toCheck;

		for (const std::string& spec : specifiers) {
			toCheck.push(spec);
		}


		while (toCheck.size()) {
			fs::path specPath{ toCheck.front() };
			toCheck.pop();

			//specified directories get searched recursively
			if (fs::is_directory(specPath)) {
				for (auto& subpath : fs::directory_iterator(specPath)) {
					toCheck.push(subpath.path().string());
				}
				//because of the dumbass ESRI grid format, I have to try to open folders as rasters as well
				(*openFunc)(specPath, fileList, log, crsOverride, zUnitOverride);
			}

			if (fs::is_regular_file(specPath)) {
				//if it's a file, try to add it to the map
				(*openFunc)(specPath, fileList, log, crsOverride, zUnitOverride);
			}

			//wildcard specifiers (e.g. C:\data\*.laz) are basically a non-recursive directory check with an extension
			if (specPath.has_filename()) {
				if (fs::is_directory(specPath.parent_path())) {
					std::regex wildcard{ "^\\*\\..+" };
					std::string ext = "";
					if (std::regex_match(specPath.filename().string(), wildcard)) {
						ext = specPath.extension().string();
					}

					if (ext.size()) {
						for (auto& subpath : fs::directory_iterator(specPath.parent_path())) {
							if (subpath.path().has_extension() && subpath.path().extension() == ext) {
								toCheck.push(subpath.path().string());
							}
						}
					}
				}
			}
		}
		return fileList;
	}

	template std::vector<LasFileExtent> LapisObjects::iterateOverFileSpecifiers<LasFileExtent>(const std::vector<std::string>& specifiers,
		openFuncType<LasFileExtent> openFunc, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
	template std::vector<DemFileAlignment> LapisObjects::iterateOverFileSpecifiers<DemFileAlignment>(const std::vector<std::string>& specifiers,
		openFuncType<DemFileAlignment> openFunc, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);

	void tryLasFile(const std::filesystem::path& file, std::vector<LasFileExtent>& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
		if (!file.has_extension()) {
			return;
		}
		if (file.extension() != ".laz" && file.extension() != ".las") {
			return;
		}
		try {
			data.push_back({ file.string(), file.string() });
			if (!crsOverride.isEmpty()) {
				data[data.size() - 1].ext.defineCRS(crsOverride);
			}
			if (!zUnitOverride.isUnknown()) {
				data[data.size() - 1].ext.setZUnits(zUnitOverride);
			}
		}
		catch (InvalidLasFileException e) {
			log.logError(e.what());
		}
		catch (InvalidExtentException e) {
			log.logError(e.what());
		}
	}

	void tryDtmFile(const std::filesystem::path& file, std::vector<DemFileAlignment>& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
		if (file.extension() == ".aux" || file.extension() == ".ovr" || file.extension() == ".adf") { //pyramid files are openable by GDAL but not really rasters
			return;
		}
		try {
			data.push_back({ file.string(), file.string() });
			if (!crsOverride.isEmpty()) {
				data[data.size() - 1].align.defineCRS(crsOverride);
			}
			if (!zUnitOverride.isUnknown()) {
				data[data.size() - 1].align.setZUnits(zUnitOverride);
			}
		}
		catch (InvalidRasterFileException e) {
			log.logError(e.what());
		}
		catch (InvalidAlignmentException e) {
			log.logError(e.what());
		}
		catch (InvalidExtentException e) {
			log.logError(e.what());
		}
	}

	//returns a useful default for the number of concurrent threads to run
	unsigned int LapisObjects::defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}
}