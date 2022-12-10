#include"app_pch.hpp"
#include"LapisController.hpp"


namespace chr = std::chrono;
namespace fs = std::filesystem;

#define LAPIS_CHECK_ABORT_AND_DEALLOC \
if (data->needAbort) { \
	data->cleanAfterRun(); \
	_isRunning = false; \
	LapisLogger::getLogger().setProgress(RunProgress::runAborted); \
	return; \
}

#define LAPIS_CHECK_ABORT \
if (data->needAbort) { \
	LapisLogger::getLogger().setProgress(RunProgress::canceling); \
	return; \
}

#define LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(type) \
if (data->needAbort) { \
	LapisLogger::getLogger().setProgress(RunProgress::canceling); \
	return type(); \
}

namespace lapis {
	LapisController::LapisController() : pr(), data(&LapisData::getDataObject())
	{
	}

	void LapisController::processFullArea()
	{
		_isRunning = true;
		LapisLogger& log = LapisLogger::getLogger();
		log.reset();

		data->prepareForRun();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		PointMetricCalculator::setInfo(data->canopyCutoff(), data->maxHt(), data->binSize(), data->strataBreaks());

		pr = std::make_unique<LapisPrivate>();

		writeParams();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		size_t soFar = 0;
		auto pointMetricThreadFunc = [&] {pointMetricThread(soFar); };

		cell_t targetNCell = data->tileFileSize() / std::max(sizeof(csm_t), sizeof(intensity_t));
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		std::shared_ptr<Alignment> csmAlign = data->csmAlign();
		std::shared_ptr<Alignment> fineIntAlign = data->fineIntAlign();
		coord_t tileRes = targetNRowCol * std::min(csmAlign->xres(), fineIntAlign->xres());
		Alignment layoutAlign{ *csmAlign,0,0,tileRes,tileRes };
		layoutAlign = extend(layoutAlign, *fineIntAlign);
		_layout = Raster<bool>{ layoutAlign };

		log.setProgress(RunProgress::lasFiles, (int)data->sortedLasList().size());

		std::vector<std::thread> threads;
		for (int i = 0; i < data->nThread(); ++i) {
			threads.push_back(std::thread(pointMetricThreadFunc));
		}
		for (int i = 0; i < data->nThread(); ++i) {
			threads[i].join();
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		size_t pointMetricCount = data->allReturnPointMetrics().size() +
			data->firstReturnPointMetrics().size() +
			data->allReturnStratumMetrics().size() * (data->strataBreaks().size() + 1) +
			data->firstReturnStratumMetrics().size() * (data->strataBreaks().size() + 1) +
			(data->doTopo()? data->topoMetrics().size() + 1 : 0); //+1 is elevation
		log.setProgress(RunProgress::writeMetrics, (int)pointMetricCount);

		if (data->doPointMetrics()) {
			for (auto& metric : data->allReturnPointMetrics()) {
				writeRasterWithFullName(getPointMetricDir(), metric.name, metric.rast, metric.unit);
				log.incrementTask();
				LAPIS_CHECK_ABORT_AND_DEALLOC;
			}
			for (auto& metric : data->firstReturnPointMetrics()) {
				writeRasterWithFullName(getFRPointMetricDir(), metric.name, metric.rast, metric.unit);
				log.incrementTask();
				LAPIS_CHECK_ABORT_AND_DEALLOC;
			}


			for (auto& metric : data->allReturnStratumMetrics()) {
				for (size_t i = 0; i < metric.stratumNames.size(); ++i) {
					writeRasterWithFullName(getStratumDir(), metric.baseName + metric.stratumNames[i], metric.rasters[i], metric.unit);
					log.incrementTask();
					LAPIS_CHECK_ABORT_AND_DEALLOC;
				}
			}
			for (auto& metric : data->firstReturnStratumMetrics()) {
				for (size_t i = 0; i < metric.stratumNames.size(); ++i) {
					writeRasterWithFullName(getFRStratumDir(), metric.baseName + metric.stratumNames[i], metric.rasters[i], metric.unit);
					log.incrementTask();
					LAPIS_CHECK_ABORT_AND_DEALLOC;
				}
			}
		}
		
		if (data->doTopo()) {
			calculateAndWriteTopo();
			LAPIS_CHECK_ABORT_AND_DEALLOC;
		}
		
		cell_t soFarCSM = 0;
		TaoIdMap idMap;
		if (data->doCsm()) {
			log.setProgress(RunProgress::csmTiles, (int)_layout.ncell());
			auto csmMergeThreadFunc = [&] {csmProcessingThread(soFarCSM, idMap); };


			threads.clear();
			for (int i = 0; i < data->nThread(); ++i) {
				threads.push_back(std::thread(csmMergeThreadFunc));
			}
			for (int i = 0; i < data->nThread(); ++i) {
				threads[i].join();
			}
			LAPIS_CHECK_ABORT_AND_DEALLOC;

			log.setProgress(RunProgress::writeCsmMetrics, (int)data->csmMetrics().size());
			writeCSMMetrics();
			LAPIS_CHECK_ABORT_AND_DEALLOC;
		}

		log.setProgress(RunProgress::cleanUp);

		if (data->doTaos()) {
			soFarCSM = 0;
			auto taoIdFixFunc = [&] {fixTAOIds(idMap, soFarCSM); };
			threads.clear();
			for (int i = 0; i < data->nThread(); ++i) {
				threads.push_back(std::thread(taoIdFixFunc));
			}
			for (int i = 0; i < data->nThread(); ++i) {
				threads[i].join();
			}
		}
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		writeLayout();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		pr->afterProcessing();
		LAPIS_CHECK_ABORT_AND_DEALLOC;

		fs::remove_all(getCSMTempDir());
		fs::remove_all(getTempTAODir());
		fs::remove_all(getFineIntTempDir());
		if (!data->doFineInt()) {
			fs::remove_all(getFineIntDir());
		}

		data->cleanAfterRun();

		log.setProgress(RunProgress::finished);
		_isRunning = false;
	}

	bool LapisController::isRunning() const
	{
		return _isRunning;
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

	void LapisController::pointMetricThread(size_t& soFar)
	{
		LapisLogger& log = LapisLogger::getLogger();
		while (true) {

			LasFileExtent lasExt;
			size_t thisidx = 0;
			{
				std::lock_guard lock1(data->globalMutex());
				auto& lasFiles = data->sortedLasList();
				if (soFar >= lasFiles.size()) {
					break;
				}
				lasExt = lasFiles[soFar];
				thisidx = soFar;

				++soFar;
			}

			for (auto& c : _layout.cellsFromExtent(lasExt.ext, SnapType::out)) {
				_layout[c].has_value() = true;
				_layout[c].value() = true;
			}


			std::optional<Raster<csm_t>> thiscsm;
			if (data->doCsm()) {
				thiscsm = Raster<csm_t>(crop(*data->csmAlign(), lasExt.ext, SnapType::out));
			}

			LAPIS_CHECK_ABORT;
			LidarPointVector points = getPointsAndDem(thisidx);
			LAPIS_CHECK_ABORT;

			if (data->doPointMetrics()) {
				assignPointsToCalculators(points);
			}
			LAPIS_CHECK_ABORT;
			if (data->doCsm()) {
				assignPointsToCSM(points, thiscsm);
			}
			LAPIS_CHECK_ABORT;

			pr->oncePerLas(lasExt.ext, points);
			LAPIS_CHECK_ABORT;

			if (thiscsm.has_value()) {
				try {
					writeTempRaster(getCSMTempDir(), std::to_string(thisidx), thiscsm.value());
				}
				catch (InvalidRasterFileException e) {
					log.logMessage("Error Writing Temporary CSM File");
					throw InvalidRasterFileException("Error Writing Temporary CSM File");
				}
			}
			LAPIS_CHECK_ABORT;
			
			processPoints(lasExt.ext);
			LAPIS_CHECK_ABORT;

			if (data->doFineInt()) {
				std::shared_ptr<Alignment> fineIntAlign = data->fineIntAlign();
				Raster<intensity_t> numerator{ crop(*fineIntAlign, lasExt.ext, SnapType::out) };
				Raster<intensity_t> denominator{ crop(*fineIntAlign, lasExt.ext, SnapType::out) };
				assignPointsToFineIntensity(points, numerator, denominator);
				LAPIS_CHECK_ABORT;
				try {
					writeTempRaster(getFineIntTempDir(), std::to_string(thisidx) + "_numerator", numerator);
					writeTempRaster(getFineIntTempDir(), std::to_string(thisidx) + "_denominator", denominator);
				}
				catch (InvalidRasterFileException e) {
					log.logMessage("Error Writing Temporary Fine Intensity File");
					throw InvalidRasterFileException("Error Writing Temporary Fine Intensity File");
				}
			}
			log.incrementTask();
		}
	}

	void LapisController::assignPointsToCalculators(const LidarPointVector& points) const
	{
		shared_raster<PointMetricCalculator> pmc = data->allReturnPMC();
		shared_raster<PointMetricCalculator> frpmc = data->firstReturnPMC();

		//this structure is kind of ugly and has copy/pasted code, but it guarantees that inside the core loop, only the code you actually care about in a given run is present
		//skipping a *lot* of if checks in a very time-intensive part of the run
		if (frpmc && pmc) {
			for (auto& p : points) {
				if (!pmc->strictContains(p.x, p.y)) {
					continue;
				}
				cell_t cell = pmc->cellFromXYUnsafe(p.x, p.y);

				std::lock_guard lock{ data->cellMutex(cell) };
				pmc->atCellUnsafe(cell).value().addPoint(p);
				if (p.returnNumber == 1) {
					frpmc->atCellUnsafe(cell).value().addPoint(p);
				}
			}
		}
		else if (pmc) {
			for (auto& p : points) {
				if (!pmc->strictContains(p.x, p.y)) {
					continue;
				}
				cell_t cell = pmc->cellFromXYUnsafe(p.x, p.y);

				std::lock_guard lock{ data->cellMutex(cell) };
				pmc->atCellUnsafe(cell).value().addPoint(p);
			}
		}
		else if (frpmc) {
			for (auto& p : points) {
				if (!pmc->strictContains(p.x, p.y)) {
					continue;
				}
				cell_t cell = pmc->cellFromXYUnsafe(p.x, p.y);

				std::lock_guard lock{ data->cellMutex(cell) };
				if (p.returnNumber == 1) {
					frpmc->atCellUnsafe(cell).value().addPoint(p);
				}
			}
		}
	}

	void LapisController::assignPointsToCSM(const LidarPointVector& points, std::optional<Raster<csm_t>>& csm) const
	{
		if (!csm) {
			return;
		}

		const coord_t circleradius = data->footprintDiameter() / 2;
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
		coord_t cutoff = data->fineIntCanopyCutoff();
		for (auto& p : points) {
			if (!numerator.contains(p.x, p.y)) {
				continue;
			}
			if (p.z < cutoff) {
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
		std::vector<cell_t> cells = data->nLazRaster()->cellsFromExtent(e,SnapType::out);
		for (cell_t cell : cells) {
			processCell(cell);
		}
	}

	void LapisController::processCell(cell_t cell) const
	{
		std::scoped_lock lock{ data->cellMutex(cell)};
		shared_raster<int> nLaz = data->nLazRaster();
		nLaz->atCellUnsafe(cell).value()--;
		if (nLaz->atCellUnsafe(cell).value() != 0) {
			return;
		}
		auto& pmc = data->allReturnPMC()->atCellUnsafe(cell).value();
		for (auto& v : data->allReturnPointMetrics()) {
			auto& f = v.fun;
			(pmc.*f)(v.rast, cell);
		}
		for (auto& v : data->allReturnStratumMetrics()) {
			auto& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(pmc.*f)(v.rasters[i], cell, i);
			}
		}
		auto& frpmc = data->firstReturnPMC()->atCellUnsafe(cell).value();
		for (auto& v : data->firstReturnPointMetrics()) {
			auto& f = v.fun;
			(frpmc.*f)(v.rast, cell);
		}
		for (auto& v : data->firstReturnStratumMetrics()) {
			auto& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(frpmc.*f)(v.rasters[i], cell, i);
			}
		}
		pmc.cleanUp();
		frpmc.cleanUp();
	}

	fs::path LapisController::getCSMTempDir() const
	{
		fs::path dir = data->outFolder() / "CanopySurfaceModel" / "Temp";
		return dir;
	}

	fs::path LapisController::getCSMPermanentDir() const
	{
		fs::path dir = data->outFolder() / "CanopySurfaceModel";
		return dir;
	}

	fs::path LapisController::getPointMetricDir() const
	{
		fs::path dir = data->outFolder() / "PointMetrics" / "AllReturns";
		return dir;
	}

	fs::path LapisController::getFRPointMetricDir() const
	{
		fs::path dir = data->outFolder() / "PointMetrics" / "FirstReturns";
		return dir;
	}

	fs::path LapisController::getParameterDir() const
	{
		fs::path dir = data->outFolder() / "RunParameters";
		return dir;
	}

	fs::path LapisController::getTAODir() const
	{
		fs::path dir = data->outFolder() / "TreeApproximateObjects";
		return dir;
	}

	fs::path LapisController::getTempTAODir() const {
		fs::path dir = data->outFolder() / "TreeApproximateObjects" / "Temp";
		return dir;
	}

	fs::path LapisController::getLayoutDir() const {
		fs::path layoutdir = data->outFolder() / "TileLayout";
		return layoutdir;
	}

	fs::path LapisController::getFineIntDir() const {
		fs::path dir = data->outFolder() / "FineIntensity";
		return dir;
	}

	fs::path LapisController::getFineIntTempDir() const {
		fs::path dir = data->outFolder() / "FineIntensity" / "Temp";
		return dir;
	}

	fs::path LapisController::getCSMMetricDir() const {
		fs::path dir = data->outFolder() / "CsmMetrics";
		return dir;
	}

	fs::path LapisController::getStratumDir() const {
		fs::path dir = data->outFolder() / "StratumMetrics" / "AllReturns";
		return dir;
	}
	fs::path LapisController::getFRStratumDir() const {
		fs::path dir = data->outFolder() / "StratumMetrics" / "FirstReturns";
		return dir;
	}

	fs::path LapisController::getTopoDir() const
	{
		fs::path dir = data->outFolder() / "TopoMetrics";
		return dir;
	}

	void LapisController::writeParams() const
	{
		LapisLogger& log = LapisLogger::getLogger();

		fs::path paramDir = getParameterDir();
		fs::create_directories(paramDir);
		
		auto& d = LapisData::getDataObject();

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

	void LapisController::csmProcessingThread(cell_t& soFar, TaoIdMap& idMap) const
	{
		LapisLogger& log = LapisLogger::getLogger();
		
		cell_t thisidx = 0;
		Extent thistile;
		fs::path tempcsmdir = getCSMTempDir();
		fs::path permcsmdir = getCSMPermanentDir();
		while (true) {
			{
				std::lock_guard lock(data->globalMutex());
				if (soFar >= _layout.ncell()) {
					break;
				}
				thistile = _layout.extentFromCell(soFar);
				thisidx = soFar;
				++soFar;
			}
			LAPIS_CHECK_ABORT;
			coord_t bufferDist = convertUnits(30, LinearUnitDefs::meter, _layout.crs().getXYUnits());
			std::shared_ptr<Alignment> metricAlign = data->metricAlign();
			std::shared_ptr<Alignment> csmAlign = data->csmAlign();
			bufferDist = std::max(std::max(metricAlign->xres(), metricAlign->yres()), bufferDist);
			Extent bufferExt = Extent(thistile.xmin() - bufferDist, thistile.xmax() + bufferDist, thistile.ymin() - bufferDist, thistile.ymax() + bufferDist);

			//when tree ID is added, we may need to introduce buffering of the CSM tiles to get an overlap zone
			//in the unbuffered case, lower-left snapping ensures that each output cell appears in exactly one tile
			Raster<csm_t> fullTile(crop(*csmAlign, bufferExt, SnapType::ll));

			std::optional<Raster<intensity_t>> canopyIntensityNumerator;
			std::optional<Raster<intensity_t>> canopyIntensityDenominator;
			if (data->doFineInt()) {
				canopyIntensityNumerator = Raster<intensity_t>((Alignment)fullTile);
				canopyIntensityDenominator = Raster<intensity_t>((Alignment)fullTile);
			}
			LAPIS_CHECK_ABORT;

			bool hasAnyValue = false;

			rowcol_t minrow = std::numeric_limits<rowcol_t>::max();
			rowcol_t mincol = std::numeric_limits<rowcol_t>::max();
			rowcol_t maxrow = std::numeric_limits<rowcol_t>::lowest();
			rowcol_t maxcol = std::numeric_limits<rowcol_t>::lowest();

			auto& lasFiles = data->sortedLasList();
			for (size_t i = 0; i < lasFiles.size(); ++i) {
				LAPIS_CHECK_ABORT;
				Extent thisext = lasFiles[i].ext;

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
				LAPIS_CHECK_ABORT;
				if (data->doFineInt()) {
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
			LAPIS_CHECK_ABORT;
			fullTile = smoothAndFill(fullTile, smoothWindow, neighborsNeeded, {});
			LAPIS_CHECK_ABORT;

			std::string tileName = nameFromLayout(thisidx);

			if (data->doTaos()) {
				std::vector<cell_t> highPoints = identifyHighPointsWithMinDist(fullTile, data->minTaoHt(), data->minTaoDist());
				LAPIS_CHECK_ABORT;
				
				Raster<taoid_t> segments = watershedSegment(fullTile, highPoints, thisidx, _layout.ncell());
				LAPIS_CHECK_ABORT;

				populateMap(segments, highPoints, idMap, bufferDist, thisidx);
				LAPIS_CHECK_ABORT;

				if (hasAnyValue) {
					writeHighPoints(highPoints, segments, fullTile, tileName);
					LAPIS_CHECK_ABORT;
					Raster<csm_t> maxHeight = maxHeightBySegment(segments, fullTile, highPoints);

					Extent cropExt = fullTile;
					if (minrow > 0 || mincol > 0 || maxrow < fullTile.nrow() - 1 || maxcol < fullTile.ncol() - 1) {
						cropExt = Extent{ fullTile.xFromCol(mincol),fullTile.xFromCol(maxcol),fullTile.yFromRow(maxrow),fullTile.yFromRow(minrow) };
						cropExt = crop(cropExt, thistile);
						segments = crop(segments, cropExt, SnapType::out);
						maxHeight = crop(maxHeight, cropExt, SnapType::out);
					}
					LAPIS_CHECK_ABORT;
					writeTempRaster(getTempTAODir(), "Segments_" + tileName, segments);
					writeRasterWithFullName(getTAODir(), "MaxHeight_" + tileName, maxHeight, OutputUnitLabel::Default);
					segments = Raster<taoid_t>();
					maxHeight = Raster<csm_t>();
					LAPIS_CHECK_ABORT;
				}
			}

			
			if (hasAnyValue) {
				LAPIS_CHECK_ABORT;
				calcCSMMetrics(fullTile);

				Extent cropExt = fullTile;
				if (minrow > 0 || mincol > 0 || maxrow < fullTile.nrow() - 1 || maxcol < fullTile.ncol() - 1) {
					cropExt = Extent{ fullTile.xFromCol(mincol),fullTile.xFromCol(maxcol),fullTile.yFromRow(maxrow),fullTile.yFromRow(minrow) };
					cropExt = crop(cropExt, thistile);
					fullTile = crop(fullTile, cropExt, SnapType::out);
					
				}
				LAPIS_CHECK_ABORT;
				std::string outname = "CanopySurfaceModel_" + tileName;
				writeRasterWithFullName(getCSMPermanentDir(), outname, fullTile, OutputUnitLabel::Default);
				

				fullTile = Raster<csm_t>(); //freeing up the memory

				if (data->doFineInt()) {
					Raster<intensity_t> meanCanopyIntensity = canopyIntensityNumerator.value() / canopyIntensityDenominator.value();
					meanCanopyIntensity = crop(meanCanopyIntensity, cropExt);
					writeRasterWithFullName(getFineIntDir(), "MeanCanopyIntensity_" + tileName, meanCanopyIntensity, OutputUnitLabel::Unitless);
				}
			}

			pr->oncePerCsmTile(fullTile);
			log.incrementTask();
		}
	}

	void LapisController::calcCSMMetrics(const Raster<csm_t>& tile) const {
		for (auto& metric : data->csmMetrics()) {
			Raster<metric_t> tileMetric = aggregate(tile, crop(*data->metricAlign(), tile, SnapType::out), metric.f);
			overlayInside(metric.r, tileMetric);
		}
	}
	
	LidarPointVector LapisController::getPointsAndDem(size_t n) const
	{
		LapisLogger& log = LapisLogger::getLogger();

		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
		LasReader lr;
		try {
			lr = LasReader(data->sortedLasList()[n].filename);
		}
		catch (InvalidLasFileException e) {
			log.logMessage(e.what());
			return LidarPointVector();
		}
		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);

		if (!data->lasCrsOverride().isEmpty()) {
			lr.defineCRS(data->lasCrsOverride());
		}
		if (!data->lasUnitOverride().isUnknown()) {
			lr.setZUnits(data->lasUnitOverride());
		}

		lr.setHeightLimits(data->minHt(), data->maxHt(), data->metricAlign()->crs().getZUnits());
		for (auto& f : data->filters()) {
			lr.addFilter(f);
		}

		for (auto& d : data->demList()) {
			LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
			try {
				lr.addDEM(d.filename, data->demCrsOverride(), data->demUnitOverride());
			}
			catch (InvalidRasterFileException e) {
				log.logMessage(e.what());
			}
		}
		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
		auto points = lr.getPoints(lr.nPoints());
		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
		points.transform(data->allReturnPMC()->crs());

		Raster<coord_t> fineDem = lr.unifiedDEM(*data->csmAlign());
		Raster<coord_t> coarseSum = aggregate<coord_t, coord_t>(fineDem, *data->metricAlign(), &viewSum<coord_t>);
		Raster<coord_t> coarseCount = aggregate<coord_t, coord_t>(fineDem, *data->metricAlign(), &viewCount<coord_t>);
		std::scoped_lock<std::mutex> lock{ data->globalMutex()};
		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
		overlaySum(*data->elevNum(), coarseSum);
		LAPIS_CHECK_ABORT_WITH_RETURN_VALUE(LidarPointVector);
		overlaySum(*data->elevDenom(), coarseCount);

		double ratio = (double)points.size() / lr.nPoints() * 100.;

		if (ratio < 50.) {
			log.logMessage("Only " + (int)std::round(ratio) + std::string("% of points in ") + 
				data->sortedLasList()[n].filename + " passed filters. Perhaps an issue with the DEMs?");
		}

		return points;
	}

	void LapisController::populateMap(const Raster<taoid_t> segments, const std::vector<cell_t>& highPoints, TaoIdMap& map, coord_t bufferDist, cell_t tileidx) const
	{
		Extent unchanging = Extent(segments.xmin() + 2 * bufferDist, segments.xmax() - 2 * bufferDist, segments.ymin() + 2 * bufferDist, segments.ymax() - 2 * bufferDist);
		Extent unbuffered = Extent(segments.xmin() + bufferDist, segments.xmax() - bufferDist, segments.ymin() + bufferDist, segments.ymax() - bufferDist);

		{
			std::scoped_lock<std::mutex> lock(data->globalMutex());
			map.tileToLocalNames.emplace(tileidx, TaoIdMap::IDToCoord());
		}
		for (cell_t cell : highPoints) {
			coord_t x = segments.xFromCellUnsafe(cell);
			coord_t y = segments.yFromCellUnsafe(cell);
			if (unchanging.contains(x, y)) { //not an ID that will need changing
				continue;
			}
			if (unbuffered.contains(x, y)) { //this tao id is the final id
				std::scoped_lock<std::mutex> lock(data->globalMutex());
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
		fs::create_directories(getTAODir());
		std::string filename = (getTAODir() / ("TAOs_" + name + ".shp")).string();
		
		GDALDatasetWrapper outshp("ESRI Shapefile", filename.c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(csm.crs().getCleanEPSG().getCompleteWKT().c_str());
		
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

	void LapisController::fixTAOIds(const TaoIdMap& idMap, cell_t& soFar) const
	{
		while (true) {
			cell_t thisidx;
			{
				std::lock_guard lock(data->globalMutex());
				if (soFar >= _layout.ncell()) {
					break;
				}
				thisidx = soFar;
				++soFar;
			}

			std::string tileName = "_Col" + insertZeroes(_layout.colFromCell(thisidx) + 1, _layout.ncol()) +
				"_Row" + insertZeroes(_layout.rowFromCell(thisidx) + 1, _layout.nrow());

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
			writeRasterWithFullName(getTAODir(), "Segments" + tileName, segments, OutputUnitLabel::Unitless);
		}
	}

	std::string LapisController::nameFromLayout(cell_t cell) const {
		return "Col" + insertZeroes(_layout.colFromCell(cell) + 1, _layout.ncol()) +
			"_Row" + insertZeroes(_layout.rowFromCell(cell) + 1, _layout.nrow());
	}

	void LapisController::writeLayout() const {
		fs::path filename = getLayoutDir() / "TileLayout.shp";
		fs::create_directories(getLayoutDir());

		GDALRegisterWrapper::allRegister();

		GDALDatasetWrapper outshp("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);

		OGRSpatialReference crs;
		crs.importFromWkt(_layout.crs().getCleanEPSG().getCompleteWKT().c_str());

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
		for (cell_t cell = 0; cell < _layout.ncell(); ++cell) {
			std::string name = nameFromLayout(cell);
			if (!_layout[cell].has_value()) {
				continue;
			}
			OGRFeatureWrapper feature(layer);
			feature->SetField("Name", nameFromLayout(cell).c_str());
			feature->SetField("ID", cell);
			feature->SetField("Column", _layout.colFromCell(cell)+1);
			feature->SetField("Row", _layout.rowFromCell(cell)+1);
			
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
			coord_t xCenter = _layout.xFromCell(cell);
			coord_t yCenter = _layout.yFromCell(cell);
			tile.points[0].x = tile.points[3].x = tile.points[4].x = xCenter - _layout.xres() / 2;
			tile.points[1].x = tile.points[2].x = xCenter + _layout.xres() / 2;
			tile.points[0].y = tile.points[1].y = tile.points[4].y = yCenter + _layout.yres() / 2;
			tile.points[2].y = tile.points[3].y = yCenter - _layout.yres() / 2;

			OGRGeometry* geom;
			size_t consumed = 0;
			OGRGeometryFactory::createFromWkb((const void*)&tile, &crs, &geom, sizeof(WkbRectangle), wkbVariantPostGIS1, consumed);

			feature->SetGeometry(geom);
			layer->CreateFeature(feature.ptr);
		}
	}

	void LapisController::writeCSMMetrics() const {
		LapisLogger& log = LapisLogger::getLogger();
		for (auto& metric : data->csmMetrics()) {
			try {
				writeRasterWithFullName(getCSMMetricDir(), metric.name, metric.r, metric.unit);
			}
			catch (...) {
				log.logMessage("Error writing " + metric.name);
			}
			log.incrementTask();
		}
	}
	void LapisController::calculateAndWriteTopo() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		Raster<coord_t> elev = *data->elevNum() / *data->elevDenom();
		try {
			writeRasterWithFullName(getTopoDir(), "Elevation", elev, OutputUnitLabel::Default);
		}
		catch (...) {
			log.logMessage("Error writing elevation");
		}
		log.incrementTask();
		
		for (auto& metric : data->topoMetrics()) {
			Raster<metric_t> r = focal(elev, 3, metric.metric);
			try {
				writeRasterWithFullName(getTopoDir(), metric.name, r, metric.unit);
			}
			catch (...) {
				log.logMessage("Error writing" + metric.name);
			}
			log.incrementTask();
		}
	}

}