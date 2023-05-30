#include"run_pch.hpp"
#include"TopoHandler.hpp"
#include"LapisController.hpp"
#include"..\parameters\RunParameters.hpp"

namespace lapis {
	size_t TopoHandler::handlerRegisteredIndex = LapisController::registerHandler(new TopoHandler(&RunParameters::singleton()));
	void TopoHandler::reset()
	{
		*this = TopoHandler(_getter);
	}

	bool TopoHandler::doThisProduct()
	{
		return _getter->doTopo();
	}

	std::string TopoHandler::name()
	{
		return "Topo";
	}

	TopoHandler::TopoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void TopoHandler::prepareForRun()
	{
		tryRemove(topoDir());

		_elevNumerator = Raster<coord_t>(*_getter->metricAlign());
		_elevDenominator = Raster<coord_t>(*_getter->metricAlign());

		using oul = OutputUnitLabel;
		if (_getter->useRadians()) {
			_topoMetrics.emplace_back("Slope", viewSlopeRadians<metric_t, coord_t>, oul::Radian,
				"The slope of the terrain, calculated on a 3x3 window around each pixel. The units are radians.");
			_topoMetrics.emplace_back("Aspect", viewAspectRadians<metric_t, coord_t>, oul::Radian,
				"The aspect of the terrain, calculated on a 3x3 window around each pixel. The units are radians."
				"A value near 0 or 2pi indicated a northward-facing slope, and it continues clockwise, so pi/2 is east, pi is south, and 3pi/2 is west.");
		}
		else {
			_topoMetrics.emplace_back("Slope", viewSlopeDegrees<metric_t, coord_t>, oul::Degree,
				"The slope of the terrain, calculated on a 3x3 window around each pixel. The units are degrees.");
			_topoMetrics.emplace_back("Aspect", viewAspectDegrees<metric_t, coord_t>, oul::Degree,
				"The aspect of the terrain, calculated on a 3x3 window around each pixel. The units are degrees."
				"A value near 0 or 360 indicated a northward-facing slope, and it continues clockwise, so 90 is east, 180 is south, and 270 is west.");
		}

		_topoMetrics.emplace_back("Curvature", viewCurvature<metric_t, coord_t>, oul::Unitless,
			"The overall curvature of the terrain. Ranges from 0 (nearly flat) to 100 (extremely curved).");
		_topoMetrics.emplace_back("ProfileCurvature", viewCurvature<metric_t, coord_t>, oul::Unitless,
			"The curvature of the terrain in the direction of the slope. Ranges from 0 (flat) to 100 (extremely curved)");
		_topoMetrics.emplace_back("PlanCurvature", viewCurvature<metric_t, coord_t>, oul::Unitless,
			"The curvature of the terrain perpindicular to the direction of the slope. Ranges from 0 (flat) to 100 (extremely curved).");
		_topoMetrics.emplace_back("SolarRadiationIndex", viewSRI<metric_t, coord_t>, oul::Unitless,
			"An index of how much sunlight each pixel receives, based on its slope, aspect, and latitude. Ranges from 0 (very little) to 2 (a lot).");

		std::stringstream triss;
		triss << "A measure of how rugged the terrain is. Values below 100 meters (330 feet) indicate relative flatness. "
			<< "Values above 500 meters(1600 feet) indicate very rough terrain. The units are " << _getter->unitPlural() << ".";
			_topoMetrics.emplace_back("TopoRuggednessIndex", viewTRI<metric_t, coord_t>, oul::Default,
				triss.str());


		std::stringstream ss;
		ss << "The topographic position index (TPI) is a measure of the difference between the elevation at each pixel and the mean eleveation "
			"a certain radius away. A positive value indicates that the pixel is higher elevation than its surroundings, and is perhaps on a ridgetop. "
			"A negative value indicates a valley bottom. A value near 0 indicates either flat terrain, or a mid-slope position. The units are ";
		ss << _getter->unitPlural() << ".";

		_topoRadiusMetrics.emplace_back("TPI", topoPosIndex, oul::Default, 
			ss.str());
	}
	void TopoHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
	}
	void TopoHandler::finishLasFile(const Extent& e, size_t index)
	{
	}
	void TopoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
		LapisLogger& log = LapisLogger::getLogger();
		log.beginVerboseBenchmarkTimer("Calculating mean elevation");
		Raster<coord_t> coarseSum = aggregateSum<coord_t>(dem, *_getter->metricAlign());
		_elevNumerator.overlay(coarseSum, [](coord_t a, coord_t b) {return a + b; });

		Raster<coord_t> coarseCount = aggregateCount<coord_t>(dem, *_getter->metricAlign());
		_elevDenominator.overlay(coarseCount, [](coord_t a, coord_t b) {return a + b; });
		log.endVerboseBenchmarkTimer("Calculating mean elevation");
	}
	void TopoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
	}
	void TopoHandler::cleanup()
	{
		Raster<coord_t> elev = _elevNumerator / _elevDenominator;
		

		writeRasterLogErrors(getFullFilename(topoDir(), "Elevation", OutputUnitLabel::Default), elev);

		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress("Calculating Small-Scale Topography");
		for (TopoMetric& metric : _topoMetrics) {
			Raster<metric_t> r = focal<metric_t, coord_t>(elev, 3, metric.fun);
			writeRasterLogErrors(getFullFilename(topoDir(), metric.name, metric.unit), r);
		}

		log.setProgress("Calculating Large-Scale Topography");
		Extent unbuffered = (Extent)elev;
		Raster<coord_t> buffered = _getter->bufferedElev(elev);
		for (TopoRadiusMetric& metric : _topoRadiusMetrics) {
			auto& radii = _getter->topoWindows();
			auto& radiusNames = _getter->topoWindowNames();
			for (size_t i = 0; i < radii.size(); ++i) {
				Raster<metric_t> r = metric.fun(buffered, radii[i], unbuffered);
				if (r.ncell() != elev.ncell()) {
					r = cropRaster(r, elev, SnapType::near);
				}
				if (r.ncell() != elev.ncell()) {
					r = extendRaster(r, elev, SnapType::near);
				}
				r.mask(elev);
				std::string fullName = metric.name + "_" + radiusNames[i];
				writeRasterLogErrors(getFullFilename(topoDir(), fullName, metric.unit), r);
			}
		}
	}
	void TopoHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Topographic Metrics");
		
		for (TopoMetric& metric : _topoMetrics) {
			pdf.writeSubsectionTitle(getFullFilename("", metric.name, metric.unit).string());
			pdf.writeTextBlockWithWrap(metric.pdfDesc);
		}

		pdf.writeSubsectionTitle("Radius-based Metrics");
		pdf.writeTextBlockWithWrap("Some metrics are calculated on a variety of scales, specified by the user. The following radii were used in this run:");
		for (const std::string& s : _getter->topoWindowNames()) {
			pdf.writeTextBlockWithWrap(s);
		}
		
		for (TopoRadiusMetric& metric : _topoRadiusMetrics) {
			pdf.writeSubsectionTitle(getFullFilename("", metric.name + "_XX" + _getter->unitPlural(), metric.unit).string());
			pdf.writeTextBlockWithWrap(metric.pdfDesc);
		}
	}
	std::filesystem::path TopoHandler::topoDir() const
	{
		return parentDir() / "Topography";
	}

	TopoHandler::TopoMetric::TopoMetric(const std::string& name, TopoFunc fun, OutputUnitLabel unit, const std::string& pdfDesc)
		: name(name), fun(fun), unit(unit), pdfDesc(pdfDesc)
	{
	}
	TopoHandler::TopoRadiusMetric::TopoRadiusMetric(const std::string& name, TopoRadiusFunc fun, OutputUnitLabel unit, const std::string& pdfDesc)
		: name(name), fun(fun), unit(unit), pdfDesc(pdfDesc)
	{
	}
}