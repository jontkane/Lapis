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

	TopoHandler::TopoHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void TopoHandler::prepareForRun()
	{
		std::filesystem::remove_all(topoDir());

		if (!_getter->doTopo()) {
			return;
		}

		_elevNumerator = Raster<coord_t>(*_getter->metricAlign());
		_elevDenominator = Raster<coord_t>(*_getter->metricAlign());

		using oul = OutputUnitLabel;
		_topoMetrics.emplace_back("Slope", viewSlope<coord_t, metric_t>, oul::Radian);
		_topoMetrics.emplace_back("Aspect", viewAspect<coord_t, metric_t>, oul::Radian);
	}
	void TopoHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
	}
	void TopoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
		if (!_getter->doTopo()) {
			return;
		}

		Raster<coord_t> coarseSum = aggregate<coord_t, coord_t>(dem, *_getter->metricAlign(), &viewSum<coord_t>);
		_elevNumerator.overlay(coarseSum, [](coord_t a, coord_t b) {return a + b; });

		Raster<coord_t> coarseCount = aggregate<coord_t, coord_t>(dem, *_getter->metricAlign(), &viewCount<coord_t>);
		_elevDenominator.overlay(coarseCount, [](coord_t a, coord_t b) {return a + b; });
	}
	void TopoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
	}
	void TopoHandler::cleanup()
	{

		if (!_getter->doTopo()) {
			return;
		}

		Raster<coord_t> elev = _elevNumerator / _elevDenominator;

		writeRasterLogErrors(getFullFilename(topoDir(), "Elevation", OutputUnitLabel::Default), elev);

		for (TopoMetric& metric : _topoMetrics) {
			Raster<metric_t> r = focal(elev, 3, metric.fun);
			writeRasterLogErrors(getFullFilename(topoDir(), metric.name, metric.unit), r);
		}
	}
	std::filesystem::path TopoHandler::topoDir() const
	{
		return parentDir() / "Topography";
	}

	TopoHandler::TopoMetric::TopoMetric(const std::string& name, TopoFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit)
	{
	}
}