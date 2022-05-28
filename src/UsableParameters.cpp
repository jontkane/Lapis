#include"lapis_pch.hpp"
#include"UsableParameters.hpp"

namespace lapis {
	UsableParameters::UsableParameters()
	{
		lasParams = std::make_unique<LasParameters>();
		globalParams = std::make_unique<GlobalParameters>();
	}
	UsableParameters::UsableParameters(const FullOptions& opt)
	{
		lasParams = std::make_unique<LasParameters>();
		globalParams = std::make_unique<GlobalParameters>();

		identifyLasFiles(opt);
		createOutAlignment(opt);
		sortLasFiles(opt);

		identifyDEMFiles(opt);
		setFilters(opt);
		setPointMetrics(opt);

		makeNLaz();

		otherParams(opt);
	}
	void UsableParameters::cleanUpAfterPointMetrics()
	{
		lasParams.reset(nullptr);
	}
	void UsableParameters::identifyLasFiles(const FullOptions& opt)
	{
		iterateOverFileSpecifiers<LasFileExtent>(opt.dataOptions.lasFileSpecifiers, &tryLasFile, globalParams->sortedLasFiles,
			globalParams->log, lasParams->lasCRSOverride, lasParams->lasUnitOverride);
		globalParams->log.logProgress(std::to_string(globalParams->sortedLasFiles.size()) + " point cloud files identified");
	}

	void UsableParameters::identifyDEMFiles(const FullOptions& opt)
	{
		iterateOverFileSpecifiers<DemFileAlignment>(opt.dataOptions.demFileSpecifiers, &tryDtmFile, globalParams->demFiles,globalParams->log,
			opt.dataOptions.demCRS, opt.dataOptions.demUnits);
		globalParams->log.logProgress(std::to_string(globalParams->demFiles.size()) + " DEM files identified");
	}

	void UsableParameters::setFilters(const FullOptions& opt)
	{
		auto& filters = lasParams->filters;

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
		globalParams->log.logDiagnostic(std::to_string(filters.size()) + " filters applied");
	}

	void UsableParameters::setPointMetrics(const FullOptions& opt)
	{
		auto& pointMetrics = lasParams->metricRasters;
		pointMetrics.emplace_back("Mean_CanopyHeight", &PointMetricCalculator::meanCanopy);
		pointMetrics.emplace_back("StdDev_CanopyHeight", &PointMetricCalculator::stdDevCanopy);
		pointMetrics.emplace_back("25thPercentile_CanopyHeight", &PointMetricCalculator::p25Canopy);
		pointMetrics.emplace_back("50thPercentile_CanopyHeight", &PointMetricCalculator::p50Canopy);
		pointMetrics.emplace_back("75thPercentile_CanopyHeight", &PointMetricCalculator::p75Canopy);
		pointMetrics.emplace_back("95thPercentile_CanopyHeight", &PointMetricCalculator::p95Canopy);
		pointMetrics.emplace_back("TotalReturnCount", &PointMetricCalculator::returnCount);
		pointMetrics.emplace_back("CanopyCover", &PointMetricCalculator::canopyCover);
	}

	void UsableParameters::createOutAlignment(const FullOptions& opt) {
		Alignment& metricAlign = globalParams->metricAlign;

		bool alignFinalized = false;
		CoordRef outCRS;
		coord_t cellsize = 0;
		if (std::holds_alternative<alignmentFromFile>(opt.dataOptions.outAlign)) {
			try {
				const alignmentFromFile& aff = std::get<alignmentFromFile>(opt.dataOptions.outAlign);
				metricAlign = Alignment(aff.filename);
			}
			catch (InvalidRasterFileException e) {
				globalParams->log.logError(e.what());
				throw e;
			}
			catch (InvalidAlignmentException e) {
				globalParams->log.logError(e.what());
				throw e;
			}
			catch (InvalidExtentException e) {
				globalParams->log.logError(e.what());
				throw e;
			}

			alignFinalized = true;
			outCRS = metricAlign.crs();
		}
		else {
			const manualAlignment& ma = std::get<manualAlignment>(opt.dataOptions.outAlign);
			if (!ma.crs.isEmpty()) {
				outCRS = ma.crs;

			}
			else {
				for (size_t i = 0; i < globalParams->sortedLasFiles.size(); ++i) {
					//if the user won't specify the CRS, just use the laz CRS
					if (!globalParams->sortedLasFiles[i].ext.crs().isEmpty()) {
						outCRS = globalParams->sortedLasFiles[i].ext.crs();
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
			globalParams->log.logError("Latitude/Longitude output not supported");
			throw std::runtime_error("Latitude/Longitude output not supported");
		}

		Extent fullExtent;
		bool initExtent = false;

		for (size_t i = 0; i < globalParams->sortedLasFiles.size(); ++i) {
			Extent& e = globalParams->sortedLasFiles[i].ext;
			if (!e.crs().isConsistentHoriz(outCRS)) {
				QuadExtent q{ e, outCRS };
				e = q.outerExtent();
			}
			if (!initExtent) {
				fullExtent = e;
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
					globalParams->log.logError("Snap raster does not ovarlap with las files and user specified to crop by snap raster");
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
		globalParams->csmAlign = Alignment((Extent)metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), csmcellsize, csmcellsize);
	}
	void UsableParameters::sortLasFiles(const FullOptions& opt)
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

		std::sort(globalParams->sortedLasFiles.begin(), globalParams->sortedLasFiles.end(), &lasExtentSorter);
	}

	void UsableParameters::otherParams(const FullOptions& opt)
	{
		lasParams->calculators = Raster<PointMetricCalculator>(globalParams->metricAlign);
		lasParams->cellMuts = std::vector<std::mutex>(lasParams->mutexN);
		lasParams->lasCRSOverride = opt.dataOptions.lasCRS;
		lasParams->lasUnitOverride = opt.dataOptions.lasUnits;
		lasParams->demCRSOverride = opt.dataOptions.demCRS;
		lasParams->demUnitOverride = opt.dataOptions.demUnits;

		globalParams->nThread = opt.processingOptions.nThread.value_or(defaultNThread());
		globalParams->binSize = opt.processingOptions.binSize;

		globalParams->canopyCutoff = opt.metricOptions.canopyCutoff.
			value_or(convertUnits(2, linearUnitDefs::meter, globalParams->metricAlign.crs().getZUnits()));

		globalParams->minht = opt.metricOptions.minht.
			value_or(convertUnits(-2, linearUnitDefs::meter, globalParams->metricAlign.crs().getZUnits()));
		globalParams->maxht = opt.metricOptions.maxht.
			value_or(convertUnits(100, linearUnitDefs::meter, globalParams->metricAlign.crs().getZUnits()));
	}

	void UsableParameters::makeNLaz()
	{
		auto& nLaz = lasParams->nLaz;
		nLaz = Raster<int>{ globalParams->metricAlign };
		for (auto& file : globalParams->sortedLasFiles) {
			std::vector<cell_t> cells = nLaz.cellsFromExtent(file.ext, SnapType::out);
			for (cell_t cell : cells) {
				nLaz[cell].value()++;
				nLaz[cell].has_value() = true;
			}
		}
	}

	template<class T>
	void UsableParameters::iterateOverFileSpecifiers(const std::vector<std::string>& specifiers, openFuncType<T> openFunc, std::vector<T>& fileList, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
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
				(*openFunc)(specPath, fileMap, log, crsOverride, zUnitOverride);
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
	}

	template void UsableParameters::iterateOverFileSpecifiers<LasFileExtent>(const std::vector<std::string>& specifiers, openFuncType<LasFileExtent> openFunc, std::vector<LasFileExtent>& fileList, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
	template void UsableParameters::iterateOverFileSpecifiers<DemFileAlignment>(const std::vector<std::string>& specifiers, openFuncType<DemFileAlignment> openFunc,
		std::vector<DemFileAlignment>& fileMap, Logger& log,
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
	unsigned int UsableParameters::defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}
}