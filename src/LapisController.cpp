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
	}

	void LapisController::processFullArea()
	{
		size_t soFarLas = 0;
		auto pointMetricThreadFunc = [&] {pointMetricThread(soFarLas); };
		fs::path pointMetricDir = gp->outfolder / "PointMetrics";
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

		gp->log.logProgress("Merging CSMs");

		cell_t targetNCell = gp->maxCSMBytes / sizeof(csm_data);
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		coord_t tileRes = targetNRowCol * gp->csmAlign.xres();
		Alignment layout{ gp->csmAlign,0,0,tileRes,tileRes };

		cell_t soFarCSM = 0;
		auto csmMergeThreadFunc = [&] {mergeCSMThread(layout, soFarCSM); };

		obj.cleanUpAfterPointMetrics();
		lp = nullptr;
		threads.clear();
		for (int i = 0; i < gp->nThread; ++i) {
			threads.push_back(std::thread(csmMergeThreadFunc));
		}
		for (int i = 0; i < gp->nThread; ++i) {
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
				++soFar;
				thisidx = soFar;
				gp->log.logProgress("las file " + std::to_string(soFar) + " out of " + std::to_string(gp->sortedLasFiles.size()) + " started");
			}
			

			auto startTime = chr::high_resolution_clock::now();

			std::optional<Raster<csm_data>> thiscsm;
			if (true) { //the ability to skip calculating the CSM might go here eventually
				thiscsm = Raster<csm_data>(crop(gp->csmAlign, lasExt.ext, SnapType::out));
			}

			
			LidarPointVector points = getPoints(thisidx);
			
			long long nPoints = 0;

			nPoints = assignPoints(points, thiscsm);
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

			auto endTime = chr::high_resolution_clock::now();
			auto duration = chr::duration_cast<chr::seconds>(endTime - startTime).count();
			gp->log.logDiagnostic(lasExt.filename + " " + std::to_string(nPoints) + " points read and processed in " + std::to_string(duration) + " seconds");
		}
	}

	long long LapisController::assignPoints(const LidarPointVector& points, std::optional<Raster<csm_data>>& csm) const
	{
		for (auto& p : points) {
			if (!lp->calculators.strictContains(p.x, p.y)) {
				continue;
			}
			cell_t cell = lp->calculators.cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ lp->cellMuts[cell % lp->mutexN] };
			lp->calculators[cell].value().addPoint(p);
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

	void LapisController::processPoints(const Extent& e) const
	{
		QuadExtent q{ e,lp->nLaz.crs() };
		std::vector<cell_t> cells = lp->nLaz.cellsFromExtent(q.outerExtent(),SnapType::out);
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
		fs::path baseout = gp->outfolder;
		fs::path csmtempdir = baseout / "CanopySurfaceModel" / "Temp";
		fs::create_directories(csmtempdir);
		return csmtempdir;
	}

	fs::path LapisController::getCSMPermanentDir() const
	{
		fs::path baseout = gp->outfolder;
		fs::path csmtempdir = baseout / "CanopySurfaceModel";
		fs::create_directories(csmtempdir);
		return csmtempdir;
	}

	void LapisController::mergeCSMThread(const Alignment& layout, cell_t& soFar)
	{
		cell_t thisidx = 0;
		Extent thistile;
		fs::path tempcsmdir = getCSMTempDir();
		fs::path permcsmdir = getCSMPermanentDir();
		while (true) {
			{
				std::lock_guard lock(gp->globalMut);
#pragma warning (suppress: 4018)
				if (soFar >= layout.ncell()) {
					break;
				}
				thistile = layout.extentFromCell(soFar);
				thisidx = soFar;
				++soFar;
				
			}
			gp->log.logProgress("Merging CSM tile " + std::to_string(thisidx+1) + " of approx. " + std::to_string(layout.ncell()));

			//when tree ID is added, we may need to introduce buffering of the CSM tiles to get an overlap zone
			//in the unbuffered case, lower-left snapping ensures that each output cell appears in exactly one tile
			Raster<csm_data> fullTile(crop(gp->csmAlign, thistile, SnapType::ll));

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
}