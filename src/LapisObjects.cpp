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

		if (opt.metricOptions.whichReturns == MetricOptions::WhichReturns::only) {
			filters.push_back(std::make_shared<LasFilterOnlyReturns>());
		}
		else if (opt.metricOptions.whichReturns == MetricOptions::WhichReturns::first) {
			filters.push_back(std::make_shared<LasFilterFirstReturns>());
		}

		if (opt.metricOptions.classes.has_value()) {
			if (opt.metricOptions.classes.value().whiteList) {
				filters.push_back(std::make_shared<LasFilterClassWhitelist>(opt.metricOptions.classes.value().classes));
			}
			else {
				filters.push_back(std::make_shared<LasFilterClassBlacklist>(opt.metricOptions.classes.value().classes));
			}
		}
		if (!opt.metricOptions.useWithheld) {
			filters.push_back(std::make_shared<LasFilterWithheld>());
		}

		if (opt.metricOptions.maxScanAngle.has_value()) {
			filters.push_back(std::make_shared<LasFilterMaxScanAngle>(opt.metricOptions.maxScanAngle.value()));
		}
		globalProcessingObjects->log.logDiagnostic(std::to_string(filters.size()) + " filters applied");
	}

	void LapisObjects::setPointMetrics(const FullOptions& opt)
	{
		auto& pointMetrics = lasProcessingObjects->metricRasters;
		auto& metricAlign = globalProcessingObjects->metricAlign;

		auto addMetric = [&](std::string&& name, MetricFunc f) {
			pointMetrics.emplace_back(PointMetricDefn{ std::move(name),f }, metricAlign);
		};

		addMetric("Mean_CanopyHeight", &PointMetricCalculator::meanCanopy);
		addMetric("StdDev_CanopyHeight", &PointMetricCalculator::stdDevCanopy);
		addMetric("25thPercentile_CanopyHeight", &PointMetricCalculator::p25Canopy);
		addMetric("50thPercentile_CanopyHeight", &PointMetricCalculator::p50Canopy);
		addMetric("75thPercentile_CanopyHeight", &PointMetricCalculator::p75Canopy);
		addMetric("95thPercentile_CanopyHeight", &PointMetricCalculator::p95Canopy);
		addMetric("TotalReturnCount", &PointMetricCalculator::returnCount);
		addMetric("CanopyCover", &PointMetricCalculator::canopyCover);
	}

	void LapisObjects::createOutAlignment(const FullOptions& opt) {
		//this one is a bit of a doozy because of all the branching logic
		//if the user specifies an alignment from a file, that choice is respected
		//if they choose to crop by that file, then that's it: the alignment is trimmed to where laz files exist and nothing else
		//if they don't choose to crop, then the alignment from the file is instead extended to where the laz files are
		
		//if they don't specify by file, then they may specify a cellsize or a crs (both optional)
		//if they don't specify a crs, then the crs from one of the las files is used. No guarantee which is used if they disagree
		//if they don't specify a cellsize, it's set to 30 meters
		//if they do specify a cellsize, it's interpretted to be in the user units

		//the csm alignment will always be in the same crs as the metric alignment, with cellsize defaulting to 1 meter

		//in all cases, the vertical units will be set to the user-defined units

		Alignment& metricAlign = globalProcessingObjects->metricAlign;

		bool alignFinalized = false;
		CoordRef outCRS;
		coord_t cellsize = 0;
		if (std::holds_alternative<alignmentFromFile>(opt.dataOptions.outAlign)) {
			try {
				const alignmentFromFile& aff = std::get<alignmentFromFile>(opt.dataOptions.outAlign);
				metricAlign = Alignment(aff.filename);
			}
			catch (InvalidRasterFileException e) {
				globalProcessingObjects->log.logError(e.what());
				throw e;
			}
			catch (InvalidAlignmentException e) {
				globalProcessingObjects->log.logError(e.what());
				throw e;
			}
			catch (InvalidExtentException e) {
				globalProcessingObjects->log.logError(e.what());
				throw e;
			}

			alignFinalized = true;
			outCRS = metricAlign.crs();
		}
		else {
			const ManualAlignment& ma = std::get<ManualAlignment>(opt.dataOptions.outAlign);
			if (!ma.crs.isEmpty()) {
				outCRS = ma.crs;

			}
			else {
				for (size_t i = 0; i < globalProcessingObjects->sortedLasFiles.size(); ++i) {
					//if the user won't specify the CRS, just use the laz CRS
					if (!globalProcessingObjects->sortedLasFiles[i].ext.crs().isEmpty()) {
						outCRS = globalProcessingObjects->sortedLasFiles[i].ext.crs();
						break;
					}
				}
			}

			cellsize = convertUnits(ma.res.value_or(0), opt.dataOptions.outUnits, outCRS.getXYUnits());
			if (cellsize <= 0) {
				cellsize = convertUnits(30, linearUnitDefs::meter, outCRS.getXYUnits());
			}
		}

		if (!opt.dataOptions.outUnits.isUnknown()) {
			outCRS.setZUnits(opt.dataOptions.outUnits);
		}

		if (!outCRS.isProjected()) {
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

		if (!alignFinalized) {
			metricAlign = Alignment(fullExtent, 0, 0, cellsize, cellsize);
		}
		else {
			const alignmentFromFile& aff = std::get<alignmentFromFile>(opt.dataOptions.outAlign);
			if (aff.useType == alignmentFromFile::alignType::crop) {
				try {
					metricAlign = crop(metricAlign, fullExtent, SnapType::out);
				}
				catch (InvalidExtentException e) {
					globalProcessingObjects->log.logError("Snap raster does not ovarlap with las files and user specified to crop by snap raster");
					throw std::runtime_error("Snap raster does not ovarlap with las files and user specified to crop by snap raster");
				}
			}
			else {
				metricAlign = extend(metricAlign, fullExtent, SnapType::out);
				metricAlign = crop(metricAlign, fullExtent, SnapType::out);
			}
		}

		coord_t csmcellsize = 0;

		csmcellsize = convertUnits(opt.dataOptions.csmRes.value_or(0), opt.dataOptions.outUnits, outCRS.getXYUnits());
		if (csmcellsize <= 0) {
			csmcellsize = convertUnits(1, linearUnitDefs::meter, outCRS.getXYUnits());
		}
		globalProcessingObjects->csmAlign = Alignment(fullExtent, metricAlign.xOrigin(), metricAlign.yOrigin(), csmcellsize, csmcellsize);

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
		lasProcessingObjects->calculators = Raster<PointMetricCalculator>(globalProcessingObjects->metricAlign);
		lasProcessingObjects->cellMuts = std::vector<std::mutex>(LasProcessingObjects::mutexN);
		lasProcessingObjects->lasCRSOverride = opt.dataOptions.lasCRS;
		lasProcessingObjects->lasUnitOverride = opt.dataOptions.lasUnits;
		lasProcessingObjects->demCRSOverride = opt.dataOptions.demCRS;
		lasProcessingObjects->demUnitOverride = opt.dataOptions.demUnits;

		globalProcessingObjects->nThread = opt.processingOptions.nThread.value_or(defaultNThread());
		globalProcessingObjects->binSize = opt.processingOptions.binSize;

		globalProcessingObjects->canopyCutoff = opt.metricOptions.canopyCutoff.
			value_or(convertUnits(2, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));

		globalProcessingObjects->minht = opt.metricOptions.minht.
			value_or(convertUnits(-2, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));
		globalProcessingObjects->maxht = opt.metricOptions.maxht.
			value_or(convertUnits(100, linearUnitDefs::meter, globalProcessingObjects->metricAlign.crs().getZUnits()));

		globalProcessingObjects->outfolder = opt.dataOptions.outfolder;

		PointMetricCalculator::setInfo(globalProcessingObjects->canopyCutoff, globalProcessingObjects->maxht, globalProcessingObjects->binSize);
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
		if (file.extension() == ".aux" || file.extension() == ".ovr") { //pyramid files are openable by GDAL but not really rasters
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