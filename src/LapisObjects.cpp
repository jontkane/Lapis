#include"app_pch.hpp"
#include"LapisObjects.hpp"

namespace lapis {

	LapisObjects::LapisObjects()
	{
		Options& opt = Options::getOptionsObject();

		lp = std::make_unique<LasProcessingObjects>();
		gp = std::make_unique<GlobalProcessingObjects>();

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
		lp.reset(nullptr);
	}
	void LapisObjects::identifyLasFiles(const Options& opt)
	{
		Logger& log = Logger::getLogger();

		std::vector<LasFileExtent> foundLaz = iterateOverFileSpecifiers<LasFileExtent>(opt.getLas(), &tryLasFile,
			lp->lasCRSOverride, lp->lasUnitOverride);
		std::unordered_set<std::string> unique;
		for (auto& v : foundLaz) {
			if (unique.count(v.filename)) {
				continue;
			}
			unique.insert(v.filename);
			gp->sortedLasFiles.push_back(v);
		}
		log.logProgress(std::to_string(gp->sortedLasFiles.size()) + " point cloud files identified");
	}

	void LapisObjects::identifyDEMFiles(const Options& opt)
	{
		Logger& log = Logger::getLogger();
		std::vector<DemFileAlignment> foundDem = iterateOverFileSpecifiers<DemFileAlignment>(opt.getDem(), &tryDtmFile,
			opt.getDemCrs(), opt.getDemUnits());
		std::unordered_set<std::string> unique;
		for (auto& v : foundDem) {
			if (unique.count(v.filename)) {
				continue;
			}
			unique.insert(v.filename);
			gp->demFiles.push_back(v);
		}
		log.logProgress(std::to_string(gp->demFiles.size()) + " DEM files identified");
	}

	void LapisObjects::setFilters(const Options& opt)
	{
		Logger& log = Logger::getLogger();

		auto& filters = lp->filters;

		if (opt.getOnlyFlag()) {
			filters.push_back(std::make_shared<LasFilterOnlyReturns>());
		}
		else if (opt.getFirstFlag()) {
			filters.push_back(std::make_shared<LasFilterFirstReturns>());
		}

		Options::ClassFilter cf = opt.getClassFilter();
		if (cf.list.size()) {
			if (cf.isWhiteList) {
				filters.push_back(std::make_shared<LasFilterClassWhitelist>(cf.list));
			}
			else {
				filters.push_back(std::make_shared<LasFilterClassBlacklist>(cf.list));
			}
		}
		if (!opt.getUseWithheldFlag()) {
			filters.push_back(std::make_shared<LasFilterWithheld>());
		}

		if (opt.getMaxScanAngle() > 0) {
			filters.push_back(std::make_shared<LasFilterMaxScanAngle>(opt.getMaxScanAngle()));
		}
		log.logDiagnostic(std::to_string(filters.size()) + " filters applied");
	}

	void LapisObjects::setPointMetrics(const Options& opt)
	{
		auto& pointMetrics = lp->metricRasters;
		auto& metricAlign = gp->metricAlign;
		auto& stratumMetrics = lp->stratumRasters;

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


		std::vector<coord_t> strataBreaks = opt.getStrataBreaks();
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

	void LapisObjects::setTopoMetrics(const Options& opt) {
		lp->topoMetrics.push_back({ viewSlope<coord_t,metric_t>,"Slope" });
		lp->topoMetrics.push_back({ viewAspect<coord_t,metric_t>,"Aspect" });
	}

	void LapisObjects::setCSMMetrics(const Options& opt) {
		auto& csmMetrics = gp->csmMetrics;
		auto& align = gp->metricAlign;
		csmMetrics.push_back({ &viewMax<csm_t>, "MaxCSM", align });
		csmMetrics.push_back({ &viewMean<csm_t>, "MeanCSM", align });
		csmMetrics.push_back({ &viewStdDev<csm_t>, "StdDevCSM", align });
		csmMetrics.push_back({ &viewRumple<csm_t>, "RumpleCSM", align });
	}

	void LapisObjects::createOutAlignment(const Options& opt) {

		Logger& log = Logger::getLogger();

		Alignment& metricAlign = gp->metricAlign;

		Options::AlignWithoutExtent optAlign = opt.getAlign();

		CoordRef outCRS;
		if (!optAlign.crs.isEmpty()) {
			outCRS = optAlign.crs;
		}
		if (outCRS.isEmpty()) {
			for (size_t i = 0; i < gp->sortedLasFiles.size(); ++i) {
				//if the user won't specify the CRS, just use the laz CRS
				if (!gp->sortedLasFiles[i].ext.crs().isEmpty()) {
					outCRS = gp->sortedLasFiles[i].ext.crs();
					if (gp->cleanwkt) {
						outCRS = outCRS.getCleanEPSG();
					}
					break;
				}
			}
		}

		if (!outCRS.isEmpty() && !outCRS.isProjected()) {
			log.logError("Latitude/Longitude output not supported");
			throw std::runtime_error("Latitude/Longitude output not supported");
		}

		Extent fullExtent;
		bool initExtent = false;

		for (size_t i = 0; i < gp->sortedLasFiles.size(); ++i) {
			Extent& e = gp->sortedLasFiles[i].ext;
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

		auto getCorrectedValue = [&](coord_t value, coord_t defaultValue)->coord_t {
			if (value > 0) {
				return value;
			}
			if (value == 0) {
				return convertUnits(defaultValue, linearUnitDefs::meter, outCRS.getXYUnits());
			}
			return convertUnits(-value, opt.getUserUnits(), outCRS.getXYUnits());
		};

		metricAlign = Alignment(fullExtent,getCorrectedValue(optAlign.xorigin,0),
			getCorrectedValue(optAlign.yorigin,0),
			getCorrectedValue(optAlign.xres,30),
			getCorrectedValue(optAlign.yres, 30));


		coord_t csmcellsize = opt.getCSMCellsize();
		if (csmcellsize == 0) {
			csmcellsize = convertUnits(1, linearUnitDefs::meter, outCRS.getXYUnits());
		}
		else {
			csmcellsize = convertUnits(csmcellsize, opt.getUserUnits(), outCRS.getXYUnits());
		}

		gp->csmAlign = Alignment(fullExtent, metricAlign.xOrigin(), metricAlign.yOrigin(), csmcellsize, csmcellsize);

		if (!opt.getUserUnits().isUnknown()) {
			gp->metricAlign.setZUnits(opt.getUserUnits());
			gp->csmAlign.setZUnits(opt.getUserUnits());
		}
		gp->csmAlign.defineCRS(outCRS);
		

		log.logDiagnostic("Alignment calculated");
	}
	void LapisObjects::sortLasFiles(const Options& opt)
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

		std::sort(gp->sortedLasFiles.begin(), gp->sortedLasFiles.end(), lasExtentSorter);
	}

	void LapisObjects::finalParams(const Options& opt)
	{
		lp->elevDenominator = Raster<coord_t>(gp->metricAlign);
		lp->elevNumerator = Raster<coord_t>(gp->metricAlign);

		lp->calculators = Raster<PointMetricCalculator>(gp->metricAlign);
		lp->cellMuts = std::vector<std::mutex>(LasProcessingObjects::mutexN);
		lp->lasCRSOverride = opt.getLasCrs();
		lp->lasUnitOverride = opt.getLasUnits();
		lp->demCRSOverride = opt.getDemCrs();
		lp->demUnitOverride = opt.getDemUnits();

		gp->nThread = opt.getNThread();
		gp->binSize = opt.getPerformanceFlag() ? 0.1 : 0.01;

		gp->canopyCutoff = opt.getCanopyCutoff();

		gp->minht = opt.getMinHt();
		gp->maxht = opt.getMaxHt();

		gp->outfolder = opt.getOutput();

		PointMetricCalculator::setInfo(gp->canopyCutoff, gp->maxht, gp->binSize,
			opt.getStrataBreaks());

		lp->footprintRadius = opt.getFootprintDiameter() / 2;

		gp->smoothWindow = opt.getSmoothWindow();

		gp->doFineIntensity = opt.getFineIntFlag();
	}

	void LapisObjects::makeNLaz()
	{
		auto& nLaz = lp->nLaz;
		nLaz = Raster<int>{ gp->metricAlign };
		for (auto& file : gp->sortedLasFiles) {
			std::vector<cell_t> cells = nLaz.cellsFromExtent(file.ext, SnapType::out);
			for (cell_t cell : cells) {
				nLaz[cell].value()++;
				nLaz[cell].has_value() = true;
			}
		}
	}

	template<class T>
	std::vector<T> LapisObjects::iterateOverFileSpecifiers(const std::vector<std::string>& specifiers, openFuncType<T> openFunc,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{

		namespace fs = std::filesystem;

		Logger& log = Logger::getLogger();

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
							if (subpath.path().has_extension() && subpath.path().extension() == ext || ext == ".*") {
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
		openFuncType<LasFileExtent> openFunc,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
	template std::vector<DemFileAlignment> LapisObjects::iterateOverFileSpecifiers<DemFileAlignment>(const std::vector<std::string>& specifiers,
		openFuncType<DemFileAlignment> openFunc,
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
		if (file.extension() == ".aux" || file.extension() == ".ovr" || file.extension() == ".adf"
			|| file.extension() == ".xml") { //excluding commonly-found non-raster files to prevent slow calls to GDAL
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
}