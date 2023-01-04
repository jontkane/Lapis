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

		std::filesystem::remove_all(csmDir());
		std::filesystem::remove_all(csmTempDir());
		std::filesystem::remove_all(csmMetricDir());

		if (!_getter->doCsmMetrics()) {
			return;
		}
		using oul = OutputUnitLabel;

		auto addMetric = [&](CsmMetricFunc fun, const std::string& name, oul unit) {
			_csmMetrics.emplace_back(_getter, name, fun, unit);
		};

		addMetric(&viewMax<csm_t>, "MaxCSM", oul::Default);
		addMetric(&viewMean<csm_t>, "MeanCSM", oul::Default);
		addMetric(&viewStdDev<csm_t>, "StdDevCSM", oul::Default);
		addMetric(&viewRumple<csm_t>, "RumpleCSM", oul::Unitless);
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
		for (CSMMetricRaster& metric : _csmMetrics) {
			Raster<metric_t> tileMetric = aggregate(bufferedCsm, cropAlignment(*_getter->metricAlign(), bufferedCsm, SnapType::out), metric.fun);
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

	CsmHandler::CSMMetricRaster::CSMMetricRaster(ParamGetter* getter, const std::string& name, CsmMetricFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit), raster(*getter->metricAlign())
	{
	}
}
