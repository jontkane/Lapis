#include"app_pch.hpp"
#include"LapisController.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

namespace lapis {
	LapisController::LapisController() : obj()
	{
		gp = obj.globalProcessingObjects.get();
		lp = obj.lasProcessingObjects.get();
	}
	LapisController::LapisController(const FullOptions& opt) : obj(opt)
	{
		gp = obj.globalProcessingObjects.get();
		lp = obj.lasProcessingObjects.get();

		//this is the last chance we get to use the original options object, and thus we have to write them to the harddrive as ini files right away
		writeParams(opt);

		pr = std::make_unique<LapisPrivate>(obj);
	}

	void LapisController::processFullArea()
	{
		size_t soFarLas = 0;
		auto pointMetricThreadFunc = [&] {pointMetricThread(soFarLas); };
		fs::path pointMetricDir = getPointMetricDir();
		fs::create_directories(pointMetricDir);

		std::vector<std::thread> threads;
		for (int i = 0; i < gp->nThread; ++i) {
			threads.push_back(std::thread(pointMetricThreadFunc));
		}
		for (int i = 0; i < gp->nThread; ++i) {
			threads[i].join();
		}

		gp->log.logProgress("Writing point metric rasters");
		for (auto& metric : lp->metricRasters) {
			std::string filename = (pointMetricDir / (metric.metric.name + ".tif")).string();
			try {
				metric.rast.writeRaster(filename);
			}
			catch (InvalidRasterFileException e) {
				gp->log.logError("Error Writing " + filename);
			}
		}
		obj.cleanUpAfterPointMetrics();
		lp = nullptr;

		gp->log.logProgress("Merging CSMs");

		cell_t targetNCell = gp->maxCSMBytes / sizeof(csm_t);
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		coord_t tileRes = targetNRowCol * gp->csmAlign.xres();
		Alignment layout{ gp->csmAlign,0,0,tileRes,tileRes };

		cell_t soFarCSM = 0;
		TaoIdMap idMap;
		auto csmMergeThreadFunc = [&] {csmProcessingThread(layout, soFarCSM, idMap); };

		
		threads.clear();
		for (int i = 0; i < gp->nThread; ++i) {
			threads.push_back(std::thread(csmMergeThreadFunc));
		}
		for (int i = 0; i < gp->nThread; ++i) {
			threads[i].join();
		}

		gp->log.logProgress("Final Cleanup");

		soFarCSM = 0;
		auto taoIdFixFunc = [&] {fixTAOIds(idMap, layout, soFarCSM); };
		threads.clear();
		for (int i = 0; i < gp->nThread; ++i) {
			threads.push_back(std::thread(taoIdFixFunc));
		}
		for (int i = 0; i < gp->nThread; ++i) {
			threads[i].join();
		}

		writeCSMMetrics();

		writeLayout(layout);

		pr->afterProcessing();

		fs::remove_all(getCSMTempDir());
		fs::remove_all(getTempTAODir());
		fs::remove_all(getFineIntTempDir());
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

	void LapisController::pointMetricThread(size_t& soFar) const
	{
		while (true) {

			LasFileExtent lasExt;
			size_t thisidx = 0;
			{
				std::lock_guard lock1(gp->globalMut);
				if (soFar >= gp->sortedLasFiles.size()) {
					break;
				}
				lasExt = gp->sortedLasFiles[soFar];
				thisidx = soFar;
				++soFar;

				gp->log.logProgress("las file " + std::to_string(thisidx+1) + " out of " + std::to_string(gp->sortedLasFiles.size()) + " started");
			}
			

			auto startTime = chr::high_resolution_clock::now();

			std::optional<Raster<csm_t>> thiscsm;
			if (true) { //the ability to skip calculating the CSM might go here eventually
				thiscsm = Raster<csm_t>(crop(gp->csmAlign, lasExt.ext, SnapType::out));
			}

			
			LidarPointVector points = getPoints(thisidx);

			assignPointsToCalculators(points);
			assignPointsToCSM(points, thiscsm);

			pr->oncePerLas(lasExt.ext, points);

			if (thiscsm.has_value()) {
				std::string filename = (getCSMTempDir() / (std::to_string(thisidx) + ".tif")).string();
				try {
					thiscsm.value().writeRaster(filename);
				}
				catch (InvalidRasterFileException e) {
					gp->log.logError("Error Writing " + filename);
					throw InvalidRasterFileException(filename);
				}
			}
			
			processPoints(lasExt.ext);

			if (gp->doFineIntensity) {
				Raster<intensity_t> numerator{ crop(gp->csmAlign, lasExt.ext, SnapType::out) };
				Raster<intensity_t> denominator{ crop(gp->csmAlign, lasExt.ext, SnapType::out) };
				assignPointsToFineIntensity(points, numerator, denominator);
				numerator.writeRaster((getFineIntTempDir() / (std::to_string(thisidx) + "_numerator.tif")).string());
				denominator.writeRaster((getFineIntTempDir() / (std::to_string(thisidx) + "_denominator.tif")).string());
			}

			auto endTime = chr::high_resolution_clock::now();
			auto duration = chr::duration_cast<chr::seconds>(endTime - startTime).count();
			gp->log.logDiagnostic(lasExt.filename + " " + std::to_string(points.size()) + " points read and processed in " + std::to_string(duration) + " seconds");
		}
	}

	void LapisController::assignPointsToCalculators(const LidarPointVector& points) const
	{
		for (auto& p : points) {
			if (!lp->calculators.strictContains(p.x, p.y)) {
				continue;
			}
			cell_t cell = lp->calculators.cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ lp->cellMuts[cell % lp->mutexN] };
			lp->calculators[cell].value().addPoint(p);
		}
		
		
	}

	void LapisController::assignPointsToCSM(const LidarPointVector& points, std::optional<Raster<csm_t>>& csm) const
	{
		if (!csm) {
			return;
		}

		const coord_t circleradius = lp->footprintRadius;
		const coord_t diagonal = circleradius/std::sqrt(2);
		const coord_t epsilon = -0.000001;
		struct XYEpsilon {
			coord_t x, y, epsilon;
		};
		std::vector<XYEpsilon> circle;
		if (circleradius == 0) {
			circle = { {0.,0.} };
		}
		else {
			circle = { {0,0,0},
				{circleradius,0,epsilon},
				{-circleradius,0,epsilon},
				{0,circleradius,epsilon},
				{0,-circleradius,epsilon},
				{diagonal,diagonal,2 * epsilon},
				{diagonal,-diagonal,2 * epsilon},
				{-diagonal,diagonal,2 * epsilon},
				{-diagonal,-diagonal,2 * epsilon} };
		}


		auto& csmv = csm.value();
		for (auto& p : points) {
			for (auto& direction : circle) {
				coord_t x = p.x + direction.x;
				coord_t y = p.y + direction.y;
				coord_t z = p.z + direction.epsilon;
				if (!csmv.strictContains(x, y)) {
					continue;
				}
				cell_t cell = csmv.cellFromXYUnsafe(x, y);
				if (!csmv[cell].has_value()) {
					csmv[cell].has_value() = true;
					csmv[cell].value() = (csm_t)z;
				}
				else {
					csmv[cell].value() = std::max(csmv[cell].value(), (csm_t)z);
				}
			}
		}
	}

	void LapisController::assignPointsToFineIntensity(const LidarPointVector& points, Raster<intensity_t>& numerator, Raster<intensity_t>& denominator) const
	{
		for (auto& p : points) {
			if (!numerator.contains(p.x, p.y)) {
				continue;
			}
			if (p.z < gp->canopyCutoff) {
				continue;
			}
			cell_t cell = numerator.cellFromXYUnsafe(p.x, p.y);
			numerator[cell].value() += p.intensity;
			numerator[cell].has_value() = true;
			denominator[cell].value()++;
			denominator[cell].has_value() = true;
		}
	}

	void LapisController::processPoints(const Extent& e) const
	{
		std::vector<cell_t> cells = lp->nLaz.cellsFromExtent(e,SnapType::out);
		for (cell_t cell : cells) {
			processCell(cell);
		}
	}

	void LapisController::processCell(cell_t cell) const
	{
		std::scoped_lock lock{ lp->cellMuts[cell % lp->mutexN] };
		lp->nLaz[cell].value()--;
		if (lp->nLaz[cell].value() != 0) {
			return;
		}
		auto& pmc = lp->calculators[cell].value();
		for (size_t i = 0; i < lp->metricRasters.size(); ++i) {
			auto& f = lp->metricRasters[i].metric.fun;
			(pmc.*f)(lp->metricRasters[i].rast, cell);
		}
		pmc.cleanUp();
	}

	fs::path LapisController::getCSMTempDir() const
	{
		fs::path csmtempdir = gp->outfolder / "CanopySurfaceModel" / "Temp";
		fs::create_directories(csmtempdir);
		return csmtempdir;
	}

	fs::path LapisController::getCSMPermanentDir() const
	{
		fs::path csmpermdir = gp->outfolder / "CanopySurfaceModel";
		fs::create_directories(csmpermdir);
		return csmpermdir;
	}

	fs::path LapisController::getPointMetricDir() const
	{
		fs::path pmdir = gp->outfolder / "PointMetrics";
		fs::create_directories(pmdir);
		return pmdir;
	}

	fs::path LapisController::getParameterDir() const
	{
		fs::path inidir = gp->outfolder / "RunParameters";
		fs::create_directories(inidir);
		return inidir;
	}

	fs::path LapisController::getTAODir() const
	{
		fs::path taodir = gp->outfolder / "TreeApproximateObjects";
		fs::create_directories(taodir);
		return taodir;
	}

	fs::path LapisController::getTempTAODir() const {
		fs::path taodir = gp->outfolder / "TreeApproximateObjects" / "Temp";
		fs::create_directories(taodir);
		return taodir;
	}

	fs::path LapisController::getLayoutDir() const {
		fs::path layoutdir = gp->outfolder / "TileLayout";
		fs::create_directories(layoutdir);
		return layoutdir;
	}

	fs::path LapisController::getFineIntDir() const {
		fs::path dir = gp->outfolder / "FineIntensity";
		fs::create_directories(dir);
		return dir;
	}

	fs::path LapisController::getFineIntTempDir() const {
		fs::path dir = gp->outfolder / "FineIntensity" / "Temp";
		fs::create_directories(dir);
		return dir;
	}

	fs::path LapisController::getCSMMetricDir() const {
		fs::path dir = gp->outfolder / "CsmMetrics";
		fs::create_directories(dir);
		return dir;
	}

	void LapisController::writeParams(const FullOptions& opt) const
	{
		fs::path paramDir = getParameterDir();
		fs::create_directories(paramDir);

		std::ofstream fullParams{ paramDir / "FullParameters.ini" };
		if (!fullParams) {
			gp->log.logError("Could not open " + (paramDir / "FullParameters.ini").string() + " for writing");
		}
		else {
			opt.dataOptions.write(fullParams);
			opt.processingOptions.write(fullParams);
			opt.computerOptions.write(fullParams);
			
		}

		std::ofstream runAndComp{ paramDir / "ProcessingAndComputerParameters.ini" };
		if (!runAndComp) {
			gp->log.logError("Could not open " + (paramDir / "ProcessingAndComputerParameters.ini").string() + " for writing");
		}
		else {
			opt.processingOptions.write(runAndComp);
			opt.computerOptions.write(runAndComp);
		}

		std::ofstream data{ paramDir / "DataParameters.ini" };
		if (!data) {
			gp->log.logError("Could not open " + (paramDir / "DataParameters.ini").string() + " for writing");
		}
		else {
			opt.dataOptions.write(data);
		}

		std::ofstream metric{ paramDir / "ProcessingParameters.ini" };
		if (!data) {
			gp->log.logError("Could not open " + (paramDir / "ProcessingParameters.ini").string() + " for writing");
		}
		else {
			opt.processingOptions.write(metric);
		}

		std::ofstream computer{ paramDir / "ComputerParameters.ini" };
		if (!data) {
			gp->log.logError("Could not open " + (paramDir / "ComputerParameters.ini").string() + " for writing");
		}
		else {
			opt.computerOptions.write(computer);
		}
	}

	void LapisController::csmProcessingThread(const Alignment& layout, cell_t& soFar, TaoIdMap& idMap) const
	{
		cell_t thisidx = 0;
		Extent thistile;
		fs::path tempcsmdir = getCSMTempDir();
		fs::path permcsmdir = getCSMPermanentDir();
		while (true) {
			{
				std::lock_guard lock(gp->globalMut);
				if (soFar >= layout.ncell()) {
					break;
				}
				thistile = layout.extentFromCell(soFar);
				thisidx = soFar;
				++soFar;
				
			}
			gp->log.logProgress("Processing CSM tile " + std::to_string(thisidx+1) + " of " + std::to_string(layout.ncell()));

			coord_t bufferDist = convertUnits(30, linearUnitDefs::meter, layout.crs().getXYUnits());
			bufferDist = std::max(std::max(gp->metricAlign.xres(), gp->metricAlign.yres()), bufferDist);
			Extent bufferExt = Extent(thistile.xmin() - bufferDist, thistile.xmax() + bufferDist, thistile.ymin() - bufferDist, thistile.ymax() + bufferDist);

			//when tree ID is added, we may need to introduce buffering of the CSM tiles to get an overlap zone
			//in the unbuffered case, lower-left snapping ensures that each output cell appears in exactly one tile
			Raster<csm_t> fullTile(crop(gp->csmAlign, bufferExt, SnapType::ll));

			std::optional<Raster<intensity_t>> canopyIntensityNumerator;
			std::optional<Raster<intensity_t>> canopyIntensityDenominator;
			if (gp->doFineIntensity) {
				canopyIntensityNumerator = Raster<intensity_t>((Alignment)fullTile);
				canopyIntensityDenominator = Raster<intensity_t>((Alignment)fullTile);
			}

			bool hasAnyValue = false;

			rowcol_t minrow = std::numeric_limits<rowcol_t>::max();
			rowcol_t mincol = std::numeric_limits<rowcol_t>::max();
			rowcol_t maxrow = std::numeric_limits<rowcol_t>::lowest();
			rowcol_t maxcol = std::numeric_limits<rowcol_t>::lowest();
			for (size_t i = 0; i < gp->sortedLasFiles.size(); ++i) {
				Extent thisext = gp->sortedLasFiles[i].ext;

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
				Raster<csm_t> thisr;
				try {
					thisr = Raster<csm_t>{ (tempcsmdir / (std::to_string(i) + ".tif")).string(),thisext, SnapType::out };
				}
				//these errors shouldn't be happening anymore, but if they do, it's because of a dumb floating point rounding error causing a spurious overlap
				//and safe to ignore
				catch (InvalidAlignmentException e) {
					continue;
				}
				catch (InvalidExtentException e) {
					continue;
				}
				

				//for the same reason as the above comment
				thisr.defineCRS(fullTile.crs());

				auto rcExt = fullTile.rowColExtent(thisr);
				minrow = std::min(minrow, rcExt.minrow);
				maxrow = std::max(maxrow, rcExt.maxrow);
				mincol = std::min(mincol, rcExt.mincol);
				maxcol = std::max(maxcol, rcExt.maxcol);

				for (rowcol_t row = 0; row < thisr.nrow(); ++row) {
					for (rowcol_t col = 0; col < thisr.ncol(); ++col) {
						rowcol_t tilerow = row + rcExt.minrow;
						rowcol_t tilecol = col + rcExt.mincol;
						//I think this should only happen due to float imprecision
						if (tilerow < 0 || tilerow >= fullTile.nrow() || tilecol < 0 || tilecol >= fullTile.ncol()) {
							continue;
						}
						const auto vthis = thisr.atRCUnsafe(row, col);
						auto vout = fullTile.atRCUnsafe(tilerow, tilecol);

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

				if (gp->doFineIntensity) {
					Raster<intensity_t> thisNumerator{ (getFineIntTempDir() / (std::to_string(i) + "_numerator.tif")).string(), thisext, SnapType::near };
					Raster<intensity_t> thisDenominator{ (getFineIntTempDir() / (std::to_string(i) + "_denominator.tif")).string(), thisext, SnapType::near };
					for (rowcol_t row = 0; row < thisNumerator.nrow(); ++row) {
						for (rowcol_t col = 0; col < thisNumerator.ncol(); ++col) {
							cell_t thisCell = thisNumerator.cellFromRowColUnsafe(row, col);
							cell_t fullCell = canopyIntensityDenominator.value().cellFromRowColUnsafe(row + rcExt.minrow, col + rcExt.mincol);
							if (thisNumerator[thisCell].has_value()) {
								canopyIntensityNumerator.value()[fullCell].has_value() = true;
								canopyIntensityNumerator.value()[fullCell].value() += thisNumerator[thisCell].value();

								canopyIntensityDenominator.value()[fullCell].has_value() = true;
								canopyIntensityDenominator.value()[fullCell].value() += thisDenominator[thisCell].value();
							}
						}
					}
				}
			}

			int smoothWindow = 3;
			int neighborsNeeded = 6;
			fullTile = smoothAndFill(fullTile, smoothWindow, neighborsNeeded, {});

			std::vector<cell_t> highPoints = identifyHighPoints(fullTile, gp->canopyCutoff);
			
			Raster<taoid_t> segments = watershedSegment(fullTile, highPoints, *gp, thisidx, layout.ncell());

			populateMap(segments, highPoints,idMap, bufferDist, thisidx);

			std::string tileName = nameFromLayout(layout, thisidx);
			if (hasAnyValue) {
				calcCSMMetrics(fullTile);

				writeHighPoints(highPoints, segments, fullTile, tileName);

				Raster<csm_t> maxHeight = maxHeightBySegment(segments, fullTile, highPoints);

				Extent cropExt = fullTile;
				if (minrow > 0 || mincol > 0 || maxrow < fullTile.nrow() - 1 || maxcol < fullTile.ncol() - 1) {
					cropExt = Extent{ fullTile.xFromCol(mincol),fullTile.xFromCol(maxcol),fullTile.yFromRow(maxrow),fullTile.yFromRow(minrow) };
					cropExt = crop(cropExt, thistile);
					fullTile = crop(fullTile, cropExt, SnapType::out);
					segments = crop(segments, cropExt, SnapType::out);
				}
				std::string outname = "CanopySurfaceModel_" + tileName + ".tif";
				fullTile.writeRaster((permcsmdir / outname).string());
				segments.writeRaster((getTempTAODir() / ("Segments_" + tileName + ".tif")).string());
				maxHeight.writeRaster((getTAODir() / ("MaxHeight_" + tileName + ".tif")).string());

				fullTile = Raster<csm_t>(); //freeing up the memory
				segments = Raster<taoid_t>();
				maxHeight = Raster<csm_t>();

				if (gp->doFineIntensity) {
					Raster<intensity_t> meanCanopyIntensity = canopyIntensityNumerator.value() / canopyIntensityDenominator.value();
					meanCanopyIntensity = crop(meanCanopyIntensity, cropExt);
					meanCanopyIntensity.writeRaster((getFineIntDir() / ("MeanCanopyIntensity_" + tileName + ".tif")).string());
				}
			}

			pr->oncePerCsmTile(fullTile);
		}
	}

	void LapisController::calcCSMMetrics(const Raster<csm_t>& tile) const {
		for (auto& metric : gp->csmMetrics) {
			Raster<metric_t> tileMetric = aggregate(tile, crop(gp->metricAlign, tile, SnapType::out), metric.f);
			overlayInside(metric.r, tileMetric);
		}
	}
	
	LidarPointVector LapisController::getPoints(size_t n) const
	{
		LasReader lr;
		lr = LasReader(gp->sortedLasFiles[n].filename); //may throw
		if (!lp->lasCRSOverride.isEmpty()) {
			lr.defineCRS(lp->lasCRSOverride);
		}
		if (!lp->lasUnitOverride.isUnknown()) {
			lr.setZUnits(lp->lasUnitOverride);
		}

		lr.setHeightLimits(gp->minht, gp->maxht, gp->metricAlign.crs().getZUnits());
		for (auto& f : lp->filters) {
			lr.addFilter(f);
		}

		for (auto& d : gp->demFiles) {
			try {
				lr.addDEM(d.filename, lp->demCRSOverride, lp->demUnitOverride);
			}
			catch (InvalidRasterFileException e) {
				gp->log.logError(e.what());
			}
		}

		auto points = lr.getPoints(lr.nPoints());
		points.transform(lp->calculators.crs());

		return points;
	}

	void LapisController::populateMap(const Raster<taoid_t> segments, const std::vector<cell_t>& highPoints, TaoIdMap& map, coord_t bufferDist, cell_t tileidx) const
	{
		Extent unchanging = Extent(segments.xmin() + 2 * bufferDist, segments.xmax() - 2 * bufferDist, segments.ymin() + 2 * bufferDist, segments.ymax() - 2 * bufferDist);
		Extent unbuffered = Extent(segments.xmin() + bufferDist, segments.xmax() - bufferDist, segments.ymin() + bufferDist, segments.ymax() - bufferDist);

		{
			std::scoped_lock<std::mutex> lock(gp->globalMut);
			map.tileToLocalNames.emplace(tileidx, TaoIdMap::IDToCoord());
		}
		for (cell_t cell : highPoints) {
			coord_t x = segments.xFromCellUnsafe(cell);
			coord_t y = segments.yFromCellUnsafe(cell);
			if (unchanging.contains(x, y)) { //not an ID that will need changing
				continue;
			}
			if (unbuffered.contains(x, y)) { //this tao id is the final id
				std::scoped_lock<std::mutex> lock(gp->globalMut);
				map.coordsToFinalName.emplace(TaoIdMap::XY{ x,y }, segments[cell].value());
			}
			else {
				map.tileToLocalNames[tileidx].emplace(segments[cell].value(), TaoIdMap::XY{ x,y });
			}
		}
	}

	void LapisController::writeHighPoints(const std::vector<cell_t>& highPoints, const Raster<taoid_t>& segments,
		const Raster<csm_t>& csm, const std::string& name) const
	{
		GDALRegisterWrapper::allRegister();
		std::string filename = (getTAODir() / ("TAOs_" + name + ".shp")).string();
		
		GDALDatasetWrapper outshp("ESRI Shapefile", filename.c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(csm.crs().getCompleteWKT().c_str());
		
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

		const coord_t cellArea = convertUnits(csm.xres(), csm.crs().getXYUnits(), csm.crs().getZUnits())
			* convertUnits(csm.yres(), csm.crs().getXYUnits(), csm.crs().getZUnits());

		std::unordered_map<taoid_t, coord_t> areas;
		for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
			if (segments[cell].has_value()) {
				areas.try_emplace(segments[cell].value(), 0);
				areas[segments[cell].value()] += cellArea;
			}
		}
		
		for (cell_t cell : highPoints) {
			coord_t x = segments.xFromCell(cell);
			coord_t y = segments.yFromCell(cell);
			if (!csm.contains(x, y)) {
				continue;
			}
			OGRFeatureWrapper feature(layer);
			feature->SetField("ID", (int64_t)segments[cell].value());
			feature->SetField("X", x);
			feature->SetField("Y", y);
			feature->SetField("Height", csm.atXYUnsafe(x, y).value());
			feature->SetField("Area", areas[segments[cell].value()]);

			feature->SetField("Radius", std::sqrt(areas[segments[cell].value()] / M_PI));

			OGRPoint point;
			point.setX(x);
			point.setY(y);
			feature->SetGeometry(&point);

			layer->CreateFeature(feature.ptr);
		}
	}

	void LapisController::fixTAOIds(const TaoIdMap& idMap, const Alignment& layout, cell_t& soFar) const
	{
		while (true) {
			cell_t thisidx;
			{
				std::lock_guard lock(gp->globalMut);
				if (soFar >= layout.ncell()) {
					break;
				}
				thisidx = soFar;
				++soFar;
			}

			std::string tileName = "_Col" + insertZeroes(layout.colFromCell(thisidx) + 1, layout.ncol()) +
				"_Row" + insertZeroes(layout.rowFromCell(thisidx) + 1, layout.nrow());

			Raster<taoid_t> segments;
			try {
				segments = Raster<taoid_t>{ (getTempTAODir() / ("Segments" + tileName + ".tif")).string() };
			}
			catch (InvalidRasterFileException e) {
				continue;
			}


			auto& localNameMap = idMap.tileToLocalNames.at(thisidx);
			for (cell_t cell = 0; cell < segments.ncell(); ++cell) {
				auto v = segments[cell];
				if (!v.has_value()) {
					continue;
				}
				if (!localNameMap.contains(v.value())) {
					continue;
				}
				if (!idMap.coordsToFinalName.contains(localNameMap.at(v.value()))) { //edge of the acquisition
					continue;
				}
				v.value() = idMap.coordsToFinalName.at(localNameMap.at(v.value()));
			}
			segments.writeRaster((getTAODir() / ("Segments" + tileName + ".tif")).string());
		}
	}

	std::string LapisController::nameFromLayout(const Alignment& layout, cell_t cell) const {
		return "Col" + insertZeroes(layout.colFromCell(cell) + 1, layout.ncol()) +
			"_Row" + insertZeroes(layout.rowFromCell(cell) + 1, layout.nrow());
	}

	void LapisController::writeLayout(const Alignment& layout) const {
		fs::path filename = getLayoutDir() / "TileLayout.shp";

		GDALRegisterWrapper::allRegister();

		GDALDatasetWrapper outshp("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(layout.crs().getCompleteWKT().c_str());

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
			std::string name = nameFromLayout(layout, cell);
			if (!fs::exists(getCSMPermanentDir() / ("CanopySurfaceModel_" + name + ".tif"))) {
				continue;
			}
			OGRFeatureWrapper feature(layer);
			feature->SetField("Name", nameFromLayout(layout, cell).c_str());
			feature->SetField("ID", cell);
			feature->SetField("Column", layout.colFromCell(cell)+1);
			feature->SetField("Row", layout.rowFromCell(cell)+1);
			
#pragma pack(push)
#pragma pack(1)
			struct WkbRectangle {
				uint8_t endianness = 1;
				uint32_t type = 3;
				uint32_t numRings = 1;
				uint32_t numPoints = 5; //because the first point is repeated
				struct Point {
					double x, y;
				};
				Point points[5];
			};
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

	void LapisController::writeCSMMetrics() const {
		for (auto& metric : gp->csmMetrics) {
			metric.r.writeRaster((getCSMMetricDir() / (metric.name + ".tif")).string());
		}
	}
}