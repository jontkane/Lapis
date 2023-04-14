#include"run_pch.hpp"
#include"CsmHandler.hpp"
#include"LapisController.hpp"
#include"..\parameters\RunParameters.hpp"

namespace lapis {

	//A variant of overlay that will not take values from the outer edge of the overlaid raster unless it would overwrite a nodata value
	//Intended for use when mosaicing together rasters that have edge effects, but also have a buffer
	void overlayExcludingEdgeThreadSafe(Raster<metric_t>& base, const Raster<metric_t>& over, CsmParameterGetter* getter) {
		Alignment::RowColExtent rcExt = base.rowColExtent(over, SnapType::near); //snap type doesn't matter with consistent alignments, but 'near' will correct for floating point issues

		auto distFromEdge = [](const Alignment& a, rowcol_t row, rowcol_t col) {
			rowcol_t dist = std::min(col, row);
			dist = std::min(dist, a.ncol() - 1 - col);
			dist = std::min(dist, a.nrow() - 1 - row);
			return dist;
		};

		for (rowcol_t row = rcExt.minrow; row <= rcExt.maxrow; ++row) {
			for (rowcol_t col = rcExt.mincol; col <= rcExt.maxcol; ++col) {

				cell_t baseCell = base.cellFromRowColUnsafe(row, col);
				cell_t overCell = over.cellFromRowColUnsafe(row - rcExt.minrow, col - rcExt.mincol);

				const auto otherValue = over[overCell];
				auto thisValue = base[baseCell];
				if (!otherValue.has_value()) {
					continue;
				}
				std::scoped_lock lock{ getter->cellMutex(baseCell) };
				if (!thisValue.has_value()) {
					thisValue.has_value() = true;
					thisValue.value() = otherValue.value();
					continue;
				}

				if (row == rcExt.minrow || row == rcExt.maxrow || col == rcExt.mincol || col == rcExt.maxcol) {
					continue;
				}

				thisValue.value() = otherValue.value();
			}
		}
	}

	size_t CsmHandler::handlerRegisteredIndex = LapisController::registerHandler(new CsmHandler(&RunParameters::singleton()));
	void CsmHandler::reset()
	{
		*this = CsmHandler(_getter);
	}

	bool CsmHandler::doThisProduct()
	{
		return _getter->doCsm();
	}

	std::string CsmHandler::name()
	{
		return "CSM";
	}

	CsmHandler::CsmHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void CsmHandler::prepareForRun()
	{

		tryRemove(csmDir());
		tryRemove(csmTempDir());
		tryRemove(csmMetricDir());

		if (!_getter->doCsmMetrics()) {
			return;
		}
		_initMetrics();
	}
	void CsmHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
		if (!_csmGenerators.contains(index)) {
			Alignment thisAlign = cropAlignment(*_getter->csmAlign(), e, SnapType::out);
			_csmGenerators.emplace(index, _getter->csmAlgorithm()->getCsmMaker(thisAlign));
		}
		_csmGenerators[index]->addPoints(points);
	}
	void CsmHandler::finishLasFile(const Extent& e, size_t index)
	{
		writeRasterLogErrors(getFullTempFilename(csmTempDir(), _csmBaseName, OutputUnitLabel::Default, index), *_csmGenerators[index]->currentCsm());
		_csmGenerators.erase(index);
	}
	void CsmHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void CsmHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		for (CSMMetricRaster& metric : _csmMetrics) {
			Raster<metric_t> tileMetric = aggregate<metric_t, csm_t>(bufferedCsm, cropAlignment(*_getter->metricAlign(), bufferedCsm, SnapType::out), metric.fun);
			static std::mutex mut;

			overlayExcludingEdgeThreadSafe(metric.raster, tileMetric, _getter);
		}

		Extent cropExt = _getter->layout()->extentFromCell(tile); //unbuffered extent

		Raster<csm_t> unbuffered = cropRaster(bufferedCsm, cropExt, SnapType::near);

		writeRasterLogErrors(getFullTileFilename(csmDir(), _csmBaseName, OutputUnitLabel::Default, tile), unbuffered);
	}
	void CsmHandler::cleanup()
	{
		tryRemove(csmTempDir());
		deleteTempDirIfEmpty();

		for (CSMMetricRaster& metric : _csmMetrics) {
			writeRasterLogErrors(getFullFilename(csmMetricDir(), metric.name, metric.unit),metric.raster);
		}
		_csmMetrics = std::vector<CSMMetricRaster>();
	}

	void CsmHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Canopy Surface Model");
		pdf.writeSubsectionTitle("Overall Description");
		
		std::stringstream overall;
		overall << "A canopy surface model (CSM) is a raster product representing Lapis' best guess at the height of the canopy "
			"at each point in the area. ";
		overall << "The CSM files can be found in the CanopySurfaceModel directory. They have names like ";
		overall << getFullTileFilename("", _csmBaseName, OutputUnitLabel::Default, 0, "tif") << ". ";
		
		int nTiles = 0;
		for (cell_t cell = 0; cell < _getter->layout()->ncell(); ++cell) {
			if (_getter->layout()->atCellUnsafe(cell).has_value()) {
				nTiles++;
				if (nTiles > 1) {
					break;
				}
			}
		}
		if (nTiles > 1) {
			overall << "To avoid unusably large files, the CSM has been split into tiles. "
				"The filename indicates the column and row of each tile. The location of each tile is contained in the "
				"TileLayout.shp file in the Layout directory.";
		}
		pdf.writeTextBlockWithWrap(overall.str());
		pdf.blankLine();
		
		std::stringstream cellAndUnits;
		cellAndUnits << "The cellsize of the CSM is " <<
			pdf.numberWithUnits(
				_getter->outUnits().convertOneToThis(_getter->csmAlign()->xres(),
					_getter->csmAlign()->crs().getXYLinearUnits()),
				_getter->unitSingular(), _getter->unitPlural()) << ". ";
		cellAndUnits << "The values of the rasters are in " << pdf.strToLower(_getter->unitPlural()) << ".";

		_getter->csmAlgorithm()->describeInPdf(pdf, _getter);
		_getter->csmPostProcessAlgorithm()->describeInPdf(pdf, _getter);

		if (!_getter->doCsmMetrics()) {
			return;
		}
		pdf.newPage();
		pdf.writePageTitle("Canopy Metrics");
		pdf.writeTextBlockWithWrap("The metrics in the CanopyMetrics directory are "
			"derived products from the canopy surface model. Each one is calculated by applying a summary statistic to "
			"the canopy pixels contained in the larger metric pixel.");

		for (auto& metric : _csmMetrics) {
			pdf.writeSubsectionTitle(getFullFilename("", metric.name, metric.unit).string());
			std::stringstream desc;
			desc << metric.pdfDesc << " ";
			switch (metric.unit) {
			case OutputUnitLabel::Default:
				desc << "The units are " << pdf.strToLower(_getter->unitPlural()) << ".";
				break;
			case OutputUnitLabel::Percent:
				desc << "The units are percent.";
				break;
			case OutputUnitLabel::Unitless:
				desc << "This metric is unitless.";
				break;
			}
			pdf.writeTextBlockWithWrap(desc.str());
		}
	}

	Raster<csm_t> CsmHandler::getBufferedCsm(cell_t tile) const
	{
		if (!_getter->doCsm()) {
			return Raster<csm_t>();
		}

		std::shared_ptr<Alignment> layout = _getter->layout();
		std::shared_ptr<Alignment> csmAlign = _getter->csmAlign();

		Extent thistile = layout->extentFromCell(tile);

		coord_t bufferAmount = std::max(_getter->metricAlign()->xres(), _getter->metricAlign()->yres()) * 1.5;
		bufferAmount = linearUnitPresets::meter.convertOneToThis(bufferAmount, _getter->metricAlign()->crs().getXYLinearUnits().value());

		Raster<csm_t> bufferedCsm = getEmptyRasterFromTile<csm_t>(tile, *csmAlign, bufferAmount);

		Extent extentWithData;
		bool extentInit = false;

		const std::vector<Extent>& lasExtents = _getter->lasExtents();
		for (size_t i = 0; i < lasExtents.size(); ++i) {
			Extent thisext = lasExtents[i]; //intentional copy

			//Because the geotiff format doesn't store the entire WKT, you will sometimes end up in the situation where the WKT you set
			//and the WKT you get by writing and then reading a tif are not the same
			//This is mostly harmless but it does cause proj's is_equivalent functions to return false sometimes
			//In this case, we happen to know that the files we're going to be reading are the same CRS as thisext, regardless of slight WKT differences,
			//so we give it an empty CRS to force all isConsistent functions to return true
			thisext.defineCRS(CoordRef());

			if (!thisext.overlaps(bufferedCsm)) {
				continue;
			}
			thisext = cropExtent(thisext, bufferedCsm);
			Raster<csm_t> thisr;
			std::string filename = getFullTempFilename(csmTempDir(), _csmBaseName, OutputUnitLabel::Default, i).string();
			try {
				thisr = Raster<csm_t>{ filename,thisext, SnapType::out };
			}
			catch (InvalidRasterFileException e) {
				LapisLogger::getLogger().logMessage("Unable to open " + filename);
				continue;
			}

			if (!extentInit) {
				extentWithData = thisr;
				extentInit = true;
			}
			else {
				extentWithData = extendExtent(extentWithData, thisr);
			}


			//for the same reason as the above comment
			thisr.defineCRS(bufferedCsm.crs());
			auto overlayFunc = [&](csm_t a, csm_t b) {
				return _getter->csmAlgorithm()->combineCells(a, b);
			};
			bufferedCsm.overlay(thisr, overlayFunc);
		}

		if (!bufferedCsm.hasAnyValue()) {
			return Raster<csm_t>();
		}

		bufferedCsm = cropRaster(bufferedCsm, extentWithData, SnapType::out);

		bufferedCsm = _getter->csmPostProcessAlgorithm()->postProcess(bufferedCsm);

		return bufferedCsm;
	}
	std::filesystem::path CsmHandler::csmTempDir() const
	{
		return tempDir() / "CSM";
	}
	std::filesystem::path CsmHandler::csmDir() const
	{
		return parentDir() / "CanopySurfaceModel";
	}
	std::filesystem::path CsmHandler::csmMetricDir() const
	{
		return parentDir() / "CanopyMetrics";
	}

	void CsmHandler::_initMetrics()
	{
		using oul = OutputUnitLabel;

		auto addMetric = [&](CsmMetricFunc fun, const std::string& name, 
			oul unit, const std::string& desc) {
			_csmMetrics.emplace_back(_getter, name, fun, unit, desc);
		};

		addMetric(&viewMax<metric_t, csm_t>, "MaxCSM", oul::Default,
			"The maximum value attained in the CSM.");
		addMetric(&viewMean<metric_t, csm_t>, "MeanCSM", oul::Default,
			"The mean of the values of the CSM.");
		addMetric(&viewStdDev<metric_t, csm_t>, "StdDevCSM", oul::Default,
			"The standard deviation of the values of the CSM.");
		addMetric(&viewRumple<metric_t, csm_t>, "RumpleCSM", oul::Unitless,
			"The surface area of the CSM, divided by the area of the ground beneath it. "
			"A larger value indicates a more complex canopy.");
	}

	CsmHandler::CSMMetricRaster::CSMMetricRaster(ParamGetter* getter, const std::string& name, CsmMetricFunc fun,
		OutputUnitLabel unit, const std::string& pdfDesc)
		: name(name), fun(fun), unit(unit), raster(*getter->metricAlign()), pdfDesc(pdfDesc)
	{
	}
}
