#include"app_pch.hpp"
#include"LapisObjects.hpp"
#include"LapisData.hpp"

namespace lapis {

	LapisObjects::LapisObjects()
	{
		LapisData& opt = LapisData::getDataObject();
		opt.importBoostAndUpdateUnits();

		lp = std::make_unique<LasProcessingObjects>();
		gp = std::make_unique<GlobalProcessingObjects>();

		identifyLasFiles();
		createOutAlignment();
		sortLasFiles();

		identifyDEMFiles();
		setFilters();
		setPointMetrics();
		setTopoMetrics();
		setCSMMetrics();

		makeNLaz();

		finalParams();
	}
	void LapisObjects::cleanUpAfterPointMetrics()
	{
		lp.reset(nullptr);
	}
	void LapisObjects::identifyLasFiles()
	{
		Logger& log = Logger::getLogger();
		LapisData& opt = LapisData::getDataObject();
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

	void LapisObjects::identifyDEMFiles()
	{
		LapisData& opt = LapisData::getDataObject();
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

	void LapisObjects::setFilters()
	{
		Logger& log = Logger::getLogger();

		auto& filters = lp->filters;
		LapisData& opt = LapisData::getDataObject();
		auto cf = opt.getClassFilter();
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

	void LapisObjects::setPointMetrics()
	{
		LapisData& opt = LapisData::getDataObject();
		using oul = OutputUnitLabel;
		using pmc = PointMetricCalculator;
		auto& pointMetrics = lp->allReturnMetricRasters;
		auto& metricAlign = gp->metricAlign;
		auto& stratumMetrics = lp->allReturnStratumRasters;
		auto& frPointMetrics = lp->firstReturnMetricRasters;
		auto& frStratumMetrics = lp->firstReturnStratumRasters;

		auto addMetric = [&](const std::string& name, MetricFunc f, oul u) {
			pointMetrics.emplace_back(name, f, metricAlign, u);
			frPointMetrics.emplace_back(name, f, metricAlign, u);
		};

		addMetric("Mean_CanopyHeight", &pmc::meanCanopy,oul::Default);
		addMetric("StdDev_CanopyHeight", &pmc::stdDevCanopy,oul::Default);
		addMetric("25thPercentile_CanopyHeight", &pmc::p25Canopy,oul::Default);
		addMetric("50thPercentile_CanopyHeight", &pmc::p50Canopy,oul::Default);
		addMetric("75thPercentile_CanopyHeight", &pmc::p75Canopy,oul::Default);
		addMetric("95thPercentile_CanopyHeight", &pmc::p95Canopy,oul::Default);
		addMetric("TotalReturnCount", &pmc::returnCount,oul::Unitless);
		addMetric("CanopyCover", &pmc::canopyCover,oul::Percent);

		if (opt.getAdvancedPointFlag()) {
			addMetric("CoverAboveMean", &pmc::coverAboveMean, oul::Percent);
			addMetric("CanopyReliefRatio", &pmc::canopyReliefRatio, oul::Unitless);
			addMetric("CanopySkewness", &pmc::skewnessCanopy, oul::Unitless);
			addMetric("CanopyKurtosis", &pmc::kurtosisCanopy, oul::Unitless);
			addMetric("05thPercentile_CanopyHeight", &pmc::p05Canopy, oul::Default);
			addMetric("10thPercentile_CanopyHeight", &pmc::p10Canopy, oul::Default);
			addMetric("15thPercentile_CanopyHeight", &pmc::p15Canopy, oul::Default);
			addMetric("20thPercentile_CanopyHeight", &pmc::p20Canopy, oul::Default);
			addMetric("30thPercentile_CanopyHeight", &pmc::p30Canopy, oul::Default);
			addMetric("35thPercentile_CanopyHeight", &pmc::p35Canopy, oul::Default);
			addMetric("40thPercentile_CanopyHeight", &pmc::p40Canopy, oul::Default);
			addMetric("45thPercentile_CanopyHeight", &pmc::p45Canopy, oul::Default);
			addMetric("55thPercentile_CanopyHeight", &pmc::p55Canopy, oul::Default);
			addMetric("60thPercentile_CanopyHeight", &pmc::p60Canopy, oul::Default);
			addMetric("65thPercentile_CanopyHeight", &pmc::p65Canopy, oul::Default);
			addMetric("70thPercentile_CanopyHeight", &pmc::p70Canopy, oul::Default);
			addMetric("80thPercentile_CanopyHeight", &pmc::p80Canopy, oul::Default);
			addMetric("85thPercentile_CanopyHeight", &pmc::p85Canopy, oul::Default);
			addMetric("90thPercentile_CanopyHeight", &pmc::p90Canopy, oul::Default);
			addMetric("99thPercentile_CanopyHeight", &pmc::p99Canopy, oul::Default);
		}


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
			for (size_t i = 1; i < strataBreaks.size(); ++i) {
				stratumNames.push_back(to_string_with_precision(strataBreaks[i - 1]) + "To" + to_string_with_precision(strataBreaks[i]));
			}
			stratumNames.push_back("GreaterThan" + to_string_with_precision(strataBreaks[strataBreaks.size() - 1]));

			stratumMetrics.emplace_back("StratumCover_", stratumNames, &pmc::stratumCover, metricAlign,oul::Percent);
			stratumMetrics.emplace_back("StratumReturnProportion_", stratumNames, &pmc::stratumPercent, metricAlign,oul::Percent);
			frStratumMetrics.emplace_back("StratumCover_", stratumNames, &pmc::stratumCover, metricAlign, oul::Percent);
			frStratumMetrics.emplace_back("StratumReturnProportion_", stratumNames, &pmc::stratumPercent, metricAlign, oul::Percent);
		}
		
	}

	void LapisObjects::setTopoMetrics() {
		using oul = OutputUnitLabel;
		lp->topoMetrics.push_back({ viewSlope<coord_t,metric_t>,"Slope",oul::Radian});
		lp->topoMetrics.push_back({ viewAspect<coord_t,metric_t>,"Aspect",oul::Radian});
	}

	void LapisObjects::setCSMMetrics() {
		auto& csmMetrics = gp->csmMetrics;
		auto& align = gp->metricAlign;
		using oul = OutputUnitLabel;
		csmMetrics.push_back({ &viewMax<csm_t>, "MaxCSM", align,oul::Default});
		csmMetrics.push_back({ &viewMean<csm_t>, "MeanCSM", align,oul::Default});
		csmMetrics.push_back({ &viewStdDev<csm_t>, "StdDevCSM", align,oul::Default});
		csmMetrics.push_back({ &viewRumple<csm_t>, "RumpleCSM", align,oul::Unitless});
	}

	void LapisObjects::createOutAlignment() {

		Logger& log = Logger::getLogger();

		Alignment& metricAlign = gp->metricAlign;
		LapisData& opt = LapisData::getDataObject();

		auto optAlign = opt.getAlign();

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

		metricAlign = Alignment(fullExtent,optAlign.xorigin,optAlign.yorigin,
			optAlign.xres, optAlign.yres);


		coord_t csmcellsize = opt.getCSMCellsize();

		gp->csmAlign = Alignment(fullExtent, metricAlign.xOrigin(), metricAlign.yOrigin(), csmcellsize, csmcellsize);

		if (!opt.getUserUnits().isUnknown()) {
			gp->metricAlign.setZUnits(opt.getUserUnits());
			gp->csmAlign.setZUnits(opt.getUserUnits());
		}
		gp->csmAlign.defineCRS(outCRS);
		

		log.logDiagnostic("Alignment calculated");
	}
	void LapisObjects::sortLasFiles()
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

	void LapisObjects::finalParams()
	{
		LapisData& opt = LapisData::getDataObject();

		lp->elevDenominator = Raster<coord_t>(gp->metricAlign);
		lp->elevNumerator = Raster<coord_t>(gp->metricAlign);

		lp->allReturnCalculators = Raster<PointMetricCalculator>(gp->metricAlign);
		lp->firstReturnCalculators = Raster<PointMetricCalculator>(gp->metricAlign);
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

		gp->runName = opt.getName();
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
	std::vector<T> LapisObjects::iterateOverFileSpecifiers(const std::set<std::string>& specifiers, openFuncType<T> openFunc,
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

	template std::vector<LasFileExtent> LapisObjects::iterateOverFileSpecifiers<LasFileExtent>(const std::set<std::string>& specifiers,
		openFuncType<LasFileExtent> openFunc,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
	template std::vector<DemFileAlignment> LapisObjects::iterateOverFileSpecifiers<DemFileAlignment>(const std::set<std::string>& specifiers,
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