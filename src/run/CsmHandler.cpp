#include"run_pch.hpp"
#include"CsmHandler.hpp"
#include"LapisController.hpp"
#include"..\parameters\RunParameters.hpp"

namespace lapis {

	size_t CsmHandler::handlerRegisteredIndex = LapisController::registerHandler(new CsmHandler(&RunParameters::singleton()));
	void CsmHandler::reset()
	{
		*this = CsmHandler(_getter);
	}

	CsmHandler::CsmHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void CsmHandler::prepareForRun()
	{

		std::filesystem::remove_all(csmDir());
		std::filesystem::remove_all(csmTempDir());
		std::filesystem::remove_all(csmMetricDir());

		if (!_getter->doCsmMetrics()) {
			return;
		}
		_initMetrics();
	}
	void CsmHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doCsm()) {
			return;
		}

		Raster<csm_t> csm = _getter->csmAlgorithm()->createCsm(points, cropAlignment(*_getter->csmAlign(), e, SnapType::out));

		writeRasterLogErrors(getFullTempFilename(csmTempDir(), _csmBaseName, OutputUnitLabel::Default, index), csm);
	}
	void CsmHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void CsmHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
		if (!_getter->doCsm()) {
			return;
		}
		for (CSMMetricRaster& metric : _csmMetrics) {
			Raster<metric_t> tileMetric = aggregate<metric_t, csm_t>(bufferedCsm, cropAlignment(*_getter->metricAlign(), bufferedCsm, SnapType::out), metric.fun);
			metric.raster.overlayInside(tileMetric);
		}

		Extent cropExt = _getter->layout()->extentFromCell(tile); //unbuffered extent
		Raster<csm_t> unbuffered = cropRaster(bufferedCsm, cropExt, SnapType::out);

		writeRasterLogErrors(getFullTileFilename(csmDir(), _csmBaseName, OutputUnitLabel::Default, tile), unbuffered);
	}
	void CsmHandler::cleanup()
	{
		if (!_getter->doCsm()) {
			return;
		}
		std::filesystem::remove_all(csmTempDir());
		deleteTempDirIfEmpty();

		for (CSMMetricRaster& metric : _csmMetrics) {
			writeRasterLogErrors(getFullFilename(csmMetricDir(), metric.name, metric.unit),metric.raster);
		}
		_csmMetrics.clear();
		_csmMetrics.shrink_to_fit();
	}

	void CsmHandler::describeInPdf(MetadataPdf& pdf)
	{
		if (!_getter->doCsm()) {
			return;
		}
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
				convertUnits(_getter->csmAlign()->xres(),
					_getter->csmAlign()->crs().getXYUnits(),
					_getter->outUnits()),
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

		Raster<csm_t> bufferedCsm = getEmptyRasterFromTile<csm_t>(tile, *csmAlign, 30);

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
