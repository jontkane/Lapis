#include"run_pch.hpp"
#include"TaoHandler.hpp"
#include"..\parameters\LapisParameters.hpp"
#include"LapisController.hpp"

namespace lapis {
	HANDLER_REGISTER_DEFINITION(TaoHandler);
	void TaoHandler::reset()
	{
		*this = TaoHandler(_getter);
	}

	bool TaoHandler::doThisProduct()
	{
		return _getter->doTaos();
	}

	std::string TaoHandler::name()
	{
		return "TAOs";
	}

	TaoHandler::TaoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void TaoHandler::prepareForRun()
	{
		tryRemove(taoDir());
		tryRemove(taoTempDir());
	}
	void TaoHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
	}
	void TaoHandler::finishLasFile(const Extent& e, size_t index)
	{
	}
	void TaoHandler::afterLasFiles()
	{}
	void TaoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void TaoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		namespace fs = std::filesystem;
		//ask IDer to ID
		//make and write point vector
		//for each segmenter:
		//ask segmenter to segment
		//add fields to vector, if present
		//write segment raster as temp, if present
		//make max height raster (if raster present), write as temp

        LapisLogger& log = LapisLogger::getLogger();
		if (!bufferedCsm.hasAnyValue()) {
			return;
		}

		log.beginVerboseBenchmarkTimer("Identifying TAOs");
		GenerateIdByTile idGenerator{ _getter->layout()->ncell(),tile };
        std::vector<IDedTao> highPoints = _getter->taoIdAlgorithm()->identifyTaos(bufferedCsm, idGenerator);
		Extent unbufferedExtent = _getter->layout()->extentFromCell(tile);

        VectorDataset<Point> pointsVector{ bufferedCsm.crs() };
        pointsVector.addIntegerField("ID");
        pointsVector.addRealField("X");
        pointsVector.addRealField("Y");
        pointsVector.addRealField("Height");
		pointsVector.reserve(highPoints.size());
		for (IDedTao tao : highPoints) {
            coord_t x = bufferedCsm.xFromCellUnsafe(tao.location);
            coord_t y = bufferedCsm.yFromCellUnsafe(tao.location);
			if (!unbufferedExtent.contains(x, y)) {
				continue;
            }
			if (!bufferedCsm.atCellUnsafe(tao.location).has_value()) {
				continue;
			}
            pointsVector.addGeometry(Point{ x, y });
            auto&& feature = pointsVector.back();
            feature.setNumericField("ID", tao.id);
            feature.setNumericField("X", x);
            feature.setNumericField("Y", y);
			feature.setNumericField("Height", bufferedCsm.atCellUnsafe(tao.location).value());
		}
        fs::path highPointFilename = getHighPointFilename(tile);
        writeVectorLogErrors(highPointFilename, pointsVector);
        log.endVerboseBenchmarkTimer("Identifying TAOs");

		if (!_getter->taoSegAlgorithms().size()) {
			return;
		}
		struct TaoHighPointInfo {
			coord_t x;
			coord_t y;
			csm_t height;
		};
		std::unordered_map<taoid_t, TaoHighPointInfo> taoLookup;
		for (auto&& feature : pointsVector) {
			taoid_t id = feature.getNumericField<taoid_t>("ID");
			coord_t x = feature.getNumericField<coord_t>("X");
			coord_t y = feature.getNumericField<coord_t>("Y");
			csm_t height = feature.getNumericField<csm_t>("Height");
            taoLookup.emplace(id, TaoHighPointInfo{ x, y, height });
		}

        log.beginVerboseBenchmarkTimer("Segmenting TAOs");
		for (const auto& segmenter : _getter->taoSegAlgorithms()) {
            SegmentResults results = segmenter->segment(bufferedCsm, highPoints, unbufferedExtent);
			if (results.raster.has_value()) {
				writeRasterLogErrors(getSegmentRasterFilename(tile, segmenter.get(), true), *results.raster);

                Raster<csm_t> taoHeight{ (Alignment)results.raster.value() };
				for (cell_t cell : CellIterator(taoHeight)) {
					if (!results.raster->atCellUnsafe(cell).has_value()) {
						continue;
					}
					taoid_t id = results.raster->atCellUnsafe(cell).value();
					if (!taoLookup.contains(id)) {
						continue;
					}
                    auto v = taoHeight.atCellUnsafe(cell);
					v.has_value() = true;
                    v.value() = taoLookup.at(id).height;
				}
                writeRasterLogErrors(getTaoHeightRasterFilename(tile, segmenter.get(), true), taoHeight);
			}

			if (results.vector.has_value()) {
				results.vector->addNumericField<coord_t>("X");
                results.vector->addNumericField<coord_t>("Y");
                results.vector->addNumericField<csm_t>("Height");
                results.vector->addNumericField<coord_t>("Area");

				coord_t convFactor = LinearUnitConverter{ bufferedCsm.crs().getXYLinearUnits(),_getter->outUnits() }.convertOne(1.);
                convFactor *= convFactor; //conversion factor for area

				for (auto&& feature : *results.vector) {
                    taoid_t id = feature.getNumericField<taoid_t>("ID");
					if (!taoLookup.contains(id)) {
						//this shouldn't ever happen; if it does, the algorithm has a bug
#ifndef NDEBUG
                        throw std::runtime_error("TaoHandler::handleCsmTile: segmenter returned a feature with an ID that was not in the high points vector");
#endif
						continue;
					}
                    feature.setNumericField("X", taoLookup.at(id).x);
                    feature.setNumericField("Y", taoLookup.at(id).y);
                    feature.setNumericField("Height", taoLookup.at(id).height);
					feature.setNumericField("Area", feature.getGeometry().area() * convFactor);
				}

                fs::path segmentVectorFilename = getSegmentPolygonFilename(tile, segmenter.get());
                writeVectorLogErrors(segmentVectorFilename, *results.vector);
			}
		}
        log.endVerboseBenchmarkTimer("Segmenting TAOs");
	}
	void TaoHandler::cleanup()
	{
		LapisLogger::getLogger().setProgress("Finalizing TAOs");
		cell_t sofar = 0;
		std::vector<std::thread> threads;
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads.push_back(std::thread(
				[&sofar, this]() {
					while (true) {
						cell_t thisidx;
						{
							std::lock_guard lock(_getter->globalMutex());
							if (sofar >= _getter->layout()->ncell()) {
								break;
							}
							thisidx = sofar;
							++sofar;
						}
						_cleanupThreadFunc(thisidx);
					}
				}
			));
		}
		for (int i = 0; i < _getter->nThread(); ++i) {
			threads[i].join();
		}

		tryRemove(taoTempDir());
		deleteTempDirIfEmpty();
	}
	void TaoHandler::_cleanupThreadFunc(cell_t tile) const
	{
		//for each segmenter:
        //for both segment and height rasters:
		//read the temp file, if it exists
		//crop it to the tile extent
		//read the overlapping portions of the eight surrounding tiles
		//overlay those overlapping portions
        //write the result

        LapisLogger& log = LapisLogger::getLogger();
        log.beginVerboseBenchmarkTimer("Overlaying TAO rasters");

		for (const auto& segmenter : _getter->taoSegAlgorithms()) {
			if (!segmenter->producesRaster()) {
				continue;
			}
			std::optional<Raster<taoid_t>> segmentsOpt = tryOpenRaster<taoid_t>(
                getSegmentRasterFilename(tile, segmenter.get(), true), false);
			if (!segmentsOpt) {
				continue;
			}
			std::optional<Raster<csm_t>> taoHeightOpt = tryOpenRaster<csm_t>(
                getTaoHeightRasterFilename(tile, segmenter.get(), true), false);
			if (!taoHeightOpt) {
				continue;
			}

            rowcol_t tileRow = _getter->layout()->rowFromCell(tile);
            rowcol_t tileCol = _getter->layout()->colFromCell(tile);
			for (rowcol_t rowBudge : {-1, 0, 1}) {
                rowcol_t thisRow = tileRow + rowBudge;
				if (thisRow < 0 || thisRow >= _getter->layout()->nrow()) {
					continue;
                }
				for (rowcol_t colBudge : {-1, 0, 1}) {
					rowcol_t thisCol = tileCol + colBudge;
					if (thisCol < 0 || thisCol >= _getter->layout()->ncol()) {
						continue;
					}
					if (rowBudge == 0 && colBudge == 0) {
						continue;
                    }

                    cell_t otherTile = _getter->layout()->cellFromRowCol(thisRow, thisCol);

					std::optional<Raster<taoid_t>> otherSegmentsOpt = tryOpenRaster<taoid_t>(
                        getSegmentRasterFilename(otherTile, segmenter.get(), false),
						*segmentsOpt, SnapType::out, false);
					if (!otherSegmentsOpt) {
						continue;
                    }
					segmentsOpt->overlay(*otherSegmentsOpt, [](auto a, auto b) {return a; });

					std::optional<Raster<csm_t>> otherTaoHeight = tryOpenRaster<csm_t>(
                        getTaoHeightRasterFilename(otherTile, segmenter.get(), false),
						*segmentsOpt, SnapType::out, false);
                    if (!otherTaoHeight) {
						continue;
                    }
                    taoHeightOpt->overlay(*otherTaoHeight, [](auto a, auto b) {return a; });
				}
			}
            Extent tileExtent = _getter->layout()->extentFromCell(tile);
            *segmentsOpt = cropRaster(*segmentsOpt, tileExtent, SnapType::near);
            *taoHeightOpt = cropRaster(*taoHeightOpt, tileExtent, SnapType::near);

			writeRasterLogErrors(getSegmentRasterFilename(tile, segmenter.get(), false).string(), *segmentsOpt);
			writeRasterLogErrors(getTaoHeightRasterFilename(tile, segmenter.get(), false).string(), *taoHeightOpt);
		}

        log.endVerboseBenchmarkTimer("Overlaying TAO rasters");
    }
	std::filesystem::path TaoHandler::getHighPointFilename(cell_t tile) const
	{
        return getFullTileFilename(_getter, taoDir() / _highPointFolderName, _highPointBasename, OutputUnitLabel::Unitless, tile, "shp");
	}
	std::filesystem::path TaoHandler::getSegmentRasterFilename(cell_t tile, TaoSegmentAlgorithm* segmenter, bool temp) const
	{
        namespace fs = std::filesystem;
        fs::path baseFolder = temp ? taoTempDir() : taoDir();
        return getFullTileFilename(_getter, baseFolder / segmenter->name() / _segmentRasterFolderName, _segmentsBasename, OutputUnitLabel::Unitless, tile, "tif");
	}
	std::filesystem::path TaoHandler::getTaoHeightRasterFilename(cell_t tile, TaoSegmentAlgorithm* segmenter, bool temp) const
	{
		namespace fs = std::filesystem;
		fs::path baseFolder = temp ? taoTempDir() : taoDir();
        return getFullTileFilename(_getter, baseFolder / segmenter->name() / _taoHeightFolderName, _taoHeightBasename, OutputUnitLabel::Default, tile, "tif");
	}
	std::filesystem::path TaoHandler::getSegmentPolygonFilename(cell_t tile, TaoSegmentAlgorithm* segmenter) const
	{
        return getFullTileFilename(_getter, taoDir() / segmenter->name() / _segmentPolygonFolderName, _segmentsBasename, OutputUnitLabel::Unitless, tile, "shp");
	}
	void TaoHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Tree-Approximate Objects");
		pdf.writeTextBlockWithWrap("Tree-approximate objects (TAOs) represent Lapis' best guess at where individual trees are. The name TAO reflects "
			"the uncertainty inherent in this task.");

		_getter->taoIdAlgorithm()->describeInPdf(pdf, _getter);
		for (const auto& segmenter : _getter->taoSegAlgorithms()) {
            segmenter->describeInPdf(pdf, _getter);
        }

		pdf.writeSubsectionTitle("Products");
		pdf.writeTextBlockWithWrap("The output TAO data can be found in the TreeApproximateObjects directory. There are three kinds of products: the TAOs themselves, the segment "
			"raster and polygons, and the tao height rasters. To avoid unusably large filesizes, they are tiled. "
			"Their filenames indicate the row and column each tile belongs to. "
			"The location of each tile is available in TileLayout.shp, in the Layout directory.");

		pdf.writeSubsectionTitle("TAOs");
		std::stringstream ss;
		ss << "The TAOs themselves are stored in files with names like " << getFullTileFilename(_getter, "", _highPointBasename, OutputUnitLabel::Unitless, 0, "shp") << ". ";
		ss << "These are point vector files, whose locations represent the TAOs identified by the identification algorithm. ";
		ss << "There are six attributes in the attribute table. ID is a unique identifier for each TAO. X and Y are the coordinates of the TAO. ";
		ss << "Height is the height of the TAO, measured from the canopy surface model, in " << pdf.strToLower(_getter->unitPlural()) << ". ";
		ss << "Area is the area of the region assigned to each TAO by the segmentation algorithm, in square " << pdf.strToLower(_getter->unitPlural()) << ". ";
		ss << "Radius is the estimated crown radius of the TAO, derived directly from the area, as if the TAO was a circle, in " << pdf.strToLower(_getter->unitPlural()) << ".";
		pdf.writeTextBlockWithWrap(ss.str());

		pdf.writeSubsectionTitle("Segments");
		ss.str("");
		ss.clear();
		ss << "The files with names like " << getFullTileFilename(_getter, "", _segmentsBasename, OutputUnitLabel::Unitless, 0) << " ";
		ss << "represent the division of the landscape into TAOs. There are two variants, raster and polygon. The raster variant assigns ";
		ss << "each pixel's value to be the ID of the TAO it belongs to, or nodata if it doesn't belong to any TAO. ";
		ss << "The rasters have the same resolution as the canopy surface model. The polygons represent the same data, converted into polygons,";
		ss << "each TAO receiving a unique polygon.";
		pdf.writeTextBlockWithWrap(ss.str());

		pdf.writeSubsectionTitle("Tao Height");
		ss.str("");
		ss.clear();
		ss << "The files with names like " << getFullTileFilename(_getter, "", _taoHeightBasename, OutputUnitLabel::Default, 0) << " ";
		ss << "are similar to the segments files, but instead of using the TAO's ID as their value, they use the TAO's height. ";
		ss << "These are thus similar in concept to a canopy surface model, but as if trees were a constant height, instead of having varying heights throughout their area.";
		pdf.writeTextBlockWithWrap(ss.str());

	}
	std::filesystem::path TaoHandler::taoDir() const
	{
		return parentDir() / "TreeApproximateObjects";
	}
	std::filesystem::path TaoHandler::taoTempDir() const
	{
		return tempDir() / "TAO";
	}
}