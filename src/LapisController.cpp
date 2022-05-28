#include"lapis_pch.hpp"
#include"LapisController.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

namespace lapis {

	LapisController::LapisController(const FullOptions& opt)
	{
		outFolder = opt.dataOptions.outfolder;

		binsize = convertUnits(opt.processingOptions.binSize, linearUnitDefs::meter, opt.dataOptions.outUnits);

		nThread = opt.processingOptions.nThread.value_or(defaultNThread());

		lasCRSOverride = opt.dataOptions.lasCRS;
		lasUnitOverride = opt.dataOptions.lasUnits;
		dtmCRSOverride = opt.dataOptions.demCRS;
		dtmUnitOverride = opt.dataOptions.demUnits;

		iterateOverFileSpecifiers<LasExtent>(opt.dataOptions.lasFileSpecifiers, tryLasFile, lasLocs, log,
			lasCRSOverride,lasUnitOverride);
		log.logProgress(std::to_string(lasLocs.size()) + " point cloud files identified");
		iterateOverFileSpecifiers<Alignment>(opt.dataOptions.demFileSpecifiers, tryDtmFile, dtmLocs, log,
			dtmCRSOverride,dtmUnitOverride);
		log.logProgress(std::to_string(dtmLocs.size()) + " DEM files identified");

		minht = opt.metricOptions.minht.value_or(convertUnits(-2,linearUnitDefs::meter,opt.dataOptions.outUnits));
		maxht = opt.metricOptions.maxht.value_or(convertUnits(100, linearUnitDefs::meter, opt.dataOptions.outUnits));
		
		canopy_cutoff = opt.metricOptions.canopyCutoff.value_or(convertUnits(2, linearUnitDefs::meter, opt.dataOptions.outUnits));

		_setFilters(opt);
		_setMetrics(opt);
		_setOutAlign(opt);

		docsm = true;
	}

	void LapisController::processFullArea()
	{
		ThreadController con{ *this };
		
		auto pointMetricThreadFunc = [&] {pointMetricThread(con); };
		fs::path pointMetricDir = fs::path(outFolder) / "PointMetrics";
		fs::create_directories(pointMetricDir);

		std::vector<std::thread> threads;
		for (int i = 0; i < nThread; ++i) {
			threads.push_back(std::thread(pointMetricThreadFunc));
		}
		for (int i = 0; i < nThread; ++i) {
			threads[i].join();
		}

		log.logProgress("Writing point metric rasters");
		for (auto& metric : con.metrics) {
			std::string filename = (pointMetricDir / (metric.metric.name + ".tif")).string();
			try {
				metric.rast.writeRaster(filename);
			}
			catch (InvalidRasterFileException e) {
				log.logError("Error Writing " + filename);
			}
		}

		log.logProgress("Merging CSMs");

		cell_t targetNCell = maxCsmBytes / sizeof(csm_data);
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		coord_t tileRes = targetNRowCol * csmAlign.xres();
		Alignment layout{ csmAlign,0,0,tileRes,tileRes };

		auto csmMergeThreadFunc = [&] {mergeCSMThread(con, layout); };

		con.getReadyForCSMMerging();
		threads.clear();
		for (int i = 0; i < nThread; ++i) {
			threads.push_back(std::thread(csmMergeThreadFunc));
		}
		for (int i = 0; i < nThread; ++i) {
			threads[i].join();
		}

		//deleting the temp files goes here once I don't need them for testing
	}

	template<class T>
	T LapisController::filesByExtent(const Extent& e, const T& files) const
	{
		T out{};
		QuadExtent blockQuad{ e };
		for (auto& file : files) {
			if (blockQuad.overlaps(file.second)) {
				out.emplace(file.first, file.second);
			}
		}
		return out;
	}

	void LapisController::pointMetricThread(ThreadController& con) const
	{
		while (true) {

			std::string lasname;
			size_t thisidx = 0;
			{
				std::lock_guard lock1(con.globalMut);
				if (con.sofar >= con.lasFiles.size()) {
					break;
				}
				lasname = con.lasFiles[con.sofar];
				++con.sofar;
				thisidx = con.sofar;
				log.logProgress("las file " + std::to_string(con.sofar) + " out of " + std::to_string(con.lasFiles.size()) + " started");
			}
			LasReader lr;
			try {
				lr = LasReader(lasname);
				if (!lasCRSOverride.isEmpty()) {
					lr.defineCRS(lasCRSOverride);
				}
				if (!lasUnitOverride.isUnknown()) {
					lr.setZUnits(lasUnitOverride);
				}
			}
			catch (InvalidLasFileException e) {
				log.logError(e.what());
				continue;
			}

			auto startTime = chr::high_resolution_clock::now();

			lr.setHeightLimits(minht, maxht, outAlign.crs().getZUnits());
			for (auto& f : filters) {
				lr.addFilter(f);
			}
			QuadExtent q{ lr,outAlign.crs() };

			std::optional<Raster<csm_data>> thiscsm;
			if (docsm) {
				thiscsm = Raster<csm_data>(crop(csmAlign, q.outerExtent(), SnapType::out));
			}

			
			for (auto& d : dtmLocs) {
				try {
					lr.addDTM(d.first, dtmCRSOverride, dtmUnitOverride);
				}
				catch (InvalidRasterFileException e) {
					log.logError(e.what());
				}
			}
			
			long long nPoints = 0;

			nPoints = assignPoints(lr, con, thiscsm);
			if (thiscsm.has_value()) {
				std::string filename = (getCSMTempDir() / (std::to_string(thisidx) + ".tif")).string();
				try {
					thiscsm.value().writeRaster(filename);
				}
				catch (InvalidRasterFileException e) {
					log.logError("Error Writing " + filename);
					throw InvalidRasterFileException(filename);
				}
			}
			
			processPoints(lr, con);

			auto endTime = chr::high_resolution_clock::now();
			auto duration = chr::duration_cast<chr::seconds>(endTime - startTime).count();
			log.logDiagnostic(lasname + " " + std::to_string(nPoints) + " points read and processed in " + std::to_string(duration) + " seconds");
		}
	}

	long long LapisController::assignPoints(LasReader& lr, ThreadController& con, std::optional<Raster<csm_data>>& csm) const
	{

		auto points = lr.getPoints(lr.nPoints());
		points.transform(con.pointRast.crs());
		for (auto& p : points) {
			if (!con.pointRast.strictContains(p.x, p.y)) {
				continue;
			}
			cell_t cell = con.pointRast.cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ con.cellMuts[cell % con.mutexN] };
			con.pointRast[cell].value().addPoint(p);
		}
		if (csm.has_value()) {
			auto& csmv = csm.value();
			for (auto& p : points) {
				if (!csmv.strictContains(p.x, p.y)) {
					continue;
				}
				cell_t cell = csmv.cellFromXYUnsafe(p.x, p.y);
				if (!csmv[cell].has_value()) {
					csmv[cell].has_value() = true;
					csmv[cell].value() = (csm_data)p.z;
				}
				else {
					csmv[cell].value() = std::max(csmv[cell].value(), (csm_data)p.z);
				}
				
			}
		}

		return points.size();
		
	}

	void LapisController::processPoints(const Extent& e, ThreadController& con) const
	{
		QuadExtent q{ e,con.nLaz.crs() };
		std::vector<cell_t> cells = con.nLaz.cellsFromExtent(q.outerExtent(),SnapType::out);
		for (cell_t cell : cells) {
			processCell(con, cell);
		}
	}

	void LapisController::processCell(ThreadController& con, cell_t cell) const
	{
		std::scoped_lock lock{ con.cellMuts[cell % con.mutexN] };
		con.nLaz[cell].value()--;
		if (con.nLaz[cell].value() != 0) {
			return;
		}
		auto& pmc = con.pointRast[cell].value();
		for (size_t i = 0; i < con.metrics.size(); ++i) {
			auto& f = con.metrics[i].metric.fun;
			(pmc.*f)(con.metrics[i].rast, cell);
		}
		pmc.cleanUp();
	}

	fs::path LapisController::getCSMTempDir() const
	{
		fs::path baseout{ outFolder };
		fs::path csmtempdir = baseout / "CanopySurfaceModel" / "Temp";
		fs::create_directories(csmtempdir);
		return csmtempdir;
	}

	fs::path LapisController::getCSMPermanentDir() const
	{
		fs::path baseout{ outFolder };
		fs::path csmtempdir = baseout / "CanopySurfaceModel";
		fs::create_directories(csmtempdir);
		return csmtempdir;
	}

	void LapisController::mergeCSMThread(ThreadController& con, const Alignment& layout)
	{
		cell_t thisidx = 0;
		Extent thistile;
		fs::path tempcsmdir = getCSMTempDir();
		fs::path permcsmdir = getCSMPermanentDir();
		while (true) {
			{
				std::lock_guard lock(con.globalMut);
#pragma warning (suppress: 4018)
				if (con.sofar >= layout.ncell()) {
					break;
				}
				thistile = layout.extentFromCell(con.sofar);
				thisidx = con.sofar;
				++con.sofar;
				
			}
			log.logProgress("Merging CSM tile " + std::to_string(thisidx+1) + " of approx. " + std::to_string(layout.ncell()));

			//when tree ID is added, we may need to introduce buffering of the CSM tiles to get an overlap zone
			//in the unbuffered case, lower-left snapping ensures that each output cell appears in exactly one tile
			Raster<csm_data> fullTile(crop(csmAlign, thistile, SnapType::ll));

			bool hasAnyValue = false;

			rowcol_t minrow = std::numeric_limits<rowcol_t>::max();
			rowcol_t mincol = std::numeric_limits<rowcol_t>::max();
			rowcol_t maxrow = std::numeric_limits<rowcol_t>::lowest();
			rowcol_t maxcol = std::numeric_limits<rowcol_t>::lowest();
			for (size_t i = 0; i < con.lasFiles.size(); ++i) {
				//this calculation has already been done and if this code is slow enough to bother optimizing, this is a good place to look
				Extent thisext = QuadExtent(lasLocs[con.lasFiles[i]], fullTile.crs()).outerExtent();

				//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
				//and the WKT you get by writing and then reading a tif are not the same
				//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
				//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
				//so we give it an empty CRS to force all isConsistent functions to return true
				thisext.defineCRS(CoordRef());

				if (!thisext.overlaps(fullTile)) {
					continue;
				}
				thisext = crop(thisext, fullTile);
				Raster<csm_data> thisr{ (tempcsmdir / (std::to_string(i+1) + ".tif")).string(),thisext, SnapType::near };

				//for the same reason as the above comment
				thisr.defineCRS(fullTile.crs());

				auto rcExt = fullTile.rowColExtent(thisr);
				minrow = std::min(minrow, rcExt.minrow);
				maxrow = std::max(maxrow, rcExt.maxrow);
				mincol = std::min(mincol, rcExt.mincol);
				maxcol = std::max(maxcol, rcExt.maxcol);

				for (rowcol_t row = 0; row < thisr.nrow(); ++row) {
					for (rowcol_t col = 0; col < thisr.ncol(); ++col) {
						const auto vthis = thisr.atRCUnsafe(row, col);
						auto vout = fullTile.atRCUnsafe(row + rcExt.minrow, col + rcExt.mincol);

						if (vthis.has_value()) {
							if (!vout.has_value()) {
								vout.has_value() = true;
								vout.value() = vthis.value();
								hasAnyValue = true;
							}
							else {
								vout.value() = std::max(vout.value(), vthis.value());
							}
						}
					}
				}
			}

			//filling and smoothing go here
			//Treeseg and CSM metrics go here

			if (hasAnyValue) {
				if (minrow > 0 || mincol > 0 || maxrow < fullTile.nrow() - 1 || maxcol < fullTile.ncol() - 1) {
					Extent cropExt{ fullTile.xFromCol(mincol),fullTile.xFromCol(maxcol),fullTile.yFromRow(maxrow),fullTile.yFromRow(minrow) };
					fullTile = crop(fullTile, cropExt, SnapType::out);
				}
				std::string outname = "CanopySurfaceModel_Col" + insertZeroes(layout.colFromCell(thisidx)+1,layout.ncol()) + 
					"_Row" + insertZeroes(layout.rowFromCell(thisidx)+1,layout.nrow()) + ".tif";
				fullTile.writeRaster((permcsmdir / outname).string());
			}
		}
	}

	void LapisController::_setOutAlign(const FullOptions& opt)
	{
		Extent lasFullExtent;
		bool extInit = false;
		for (auto& v : lasLocs) {
			if (!extInit) {
				lasFullExtent = v.second;
				extInit = true;
			}
			else {
				if (v.second.crs().isConsistentHoriz(lasFullExtent.crs())) {
					lasFullExtent = extend(lasFullExtent, v.second);
				}
				else {
					QuadExtent q{ v.second,lasFullExtent.crs() };
					lasFullExtent = extend(lasFullExtent, q.outerExtent());
				}
			}
		}

		if (std::holds_alternative<alignmentFromFile>(opt.dataOptions.outAlign)) {
			const alignmentFromFile& optAlign = std::get<alignmentFromFile>(opt.dataOptions.outAlign);
			Alignment fileAlign;
			try {
				fileAlign = Alignment(optAlign.filename);
			}
			catch (std::runtime_error e) {
				log.logError(e.what());
				throw e;
			}

			QuadExtent q{ lasFullExtent,fileAlign.crs() };
			lasFullExtent = q.outerExtent();
			outAlign = Alignment(lasFullExtent, fileAlign.xOrigin(), fileAlign.yOrigin(), fileAlign.xres(), fileAlign.yres());
			if (optAlign.useType == alignmentFromFile::alignType::crop) {
				try {
					outAlign = crop(outAlign, fileAlign);
				}
				catch (InvalidExtentException e) {
					log.logError("Specified output alignment does not overlap with input point cloud files");
					throw e;
				}
			}
		}
		else {
			const manualAlignment& optAlign = std::get<manualAlignment>(opt.dataOptions.outAlign);
			if (!optAlign.crs.isEmpty()) {
				QuadExtent q{ lasFullExtent, optAlign.crs };
				lasFullExtent = q.outerExtent();
			}
			coord_t res = 30;
			if (optAlign.res.has_value()) {
				res = convertUnits(optAlign.res.value(), opt.dataOptions.outUnits, lasFullExtent.crs().getXYUnits());
			}
			else {
				res = convertUnits(res, linearUnitDefs::meter, lasFullExtent.crs().getXYUnits());
			}
			outAlign = Alignment(lasFullExtent, res / 2, res / 2, res, res);
		}

		coord_t csmres = 1;
		if (opt.dataOptions.csmRes.has_value()) {
			csmres = convertUnits(opt.dataOptions.csmRes.value(), opt.dataOptions.outUnits, lasFullExtent.crs().getXYUnits());
		}
		else {
			csmres = convertUnits(csmres, linearUnitDefs::meter, lasFullExtent.crs().getXYUnits());
		}

		csmAlign = Alignment(lasFullExtent, csmres / 2, csmres / 2, csmres, csmres);

		if (!opt.dataOptions.outUnits.isUnknown()) {
			outAlign.setZUnits(opt.dataOptions.outUnits);
			csmAlign.setZUnits(opt.dataOptions.outUnits);
		}
		else if (!lasUnitOverride.isUnknown()) {
			outAlign.setZUnits(lasUnitOverride);
			csmAlign.setZUnits(lasUnitOverride);
		}
	}

	void LapisController::_setFilters(const FullOptions& opt)
	{
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
		log.logDiagnostic(std::to_string(filters.size()) + " filters applied");
	}

	void LapisController::_setMetrics(const FullOptions& opt)
	{
		point_metrics.emplace_back("Mean_CanopyHeight", &PointMetricCalculator::meanCanopy);
		point_metrics.emplace_back("StdDev_CanopyHeight", &PointMetricCalculator::stdDevCanopy);
		point_metrics.emplace_back("25thPercentile_CanopyHeight", &PointMetricCalculator::p25Canopy);
		point_metrics.emplace_back("50thPercentile_CanopyHeight", &PointMetricCalculator::p50Canopy);
		point_metrics.emplace_back("75thPercentile_CanopyHeight", &PointMetricCalculator::p75Canopy);
		point_metrics.emplace_back("95thPercentile_CanopyHeight", &PointMetricCalculator::p95Canopy);
		point_metrics.emplace_back("TotalReturnCount", &PointMetricCalculator::returnCount);
		point_metrics.emplace_back("CanopyCover", &PointMetricCalculator::canopyCover);
	}

	LapisController::ThreadController::ThreadController(const LapisController& lc) :
		pointRast(lc.outAlign), sofar(0), globalMut(), cellMuts(mutexN)
	{

		PointMetricCalculator::setInfo(lc.canopy_cutoff, lc.maxht, lc.binsize);

		nLaz = getNLazRaster(lc.outAlign, lc.lasLocs);

		lasFiles = std::vector<std::string>();
		lasFiles.reserve(lc.lasLocs.size());
		for (auto& v : lc.lasLocs) {
			lasFiles.emplace_back(v.first);
		}
		std::sort(lasFiles.begin(), lasFiles.end(), [&](const std::string& a, const std::string& b) {
			return extentSorter(lc.lasLocs.at(a), lc.lasLocs.at(b));
			});

		for (auto& m : lc.point_metrics) {
			metrics.emplace_back(m, lc.outAlign);
		}
	}


}