#include"run_pch.hpp"
#include"TopoHandler.hpp"
#include"LapisController.hpp"
#include"..\parameters\LapisParameters.hpp"

namespace lapis {

	HANDLER_REGISTER_DEFINITION(TopoHandler);
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

		merger = std::make_unique<ElevMerger>(_getter);

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
		_topoMetrics.emplace_back("ProfileCurvature", viewProfileCurvature<metric_t, coord_t>, oul::Unitless,
			"The curvature of the terrain in the direction of the slope. Ranges from 0 (flat) to 100 (extremely curved)");
		_topoMetrics.emplace_back("PlanCurvature", viewPlanCurvature<metric_t, coord_t>, oul::Unitless,
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

		_topoRadiusMetrics.emplace_back("TopoPositionIndex", topoPosIndex, oul::Default, 
			ss.str());
	}
	void TopoHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
	}
	void TopoHandler::finishLasFile(const Extent& e, size_t index)
	{
	}
	void TopoHandler::afterLasFiles()
	{}
	void TopoHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
		LapisLogger& log = LapisLogger::getLogger();
		log.beginVerboseBenchmarkTimer("Calculating mean elevation");
		merger->addRaster(dem);
		log.endVerboseBenchmarkTimer("Calculating mean elevation");
	}
	void TopoHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile)
	{
	}
	void TopoHandler::cleanup()
	{

		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress("Calculating Topography Metrics");

		Raster<coord_t> elev = merger->meanElev();

		writeRasterLogErrors(getFullFilename(_getter, topoDir(), "MeanElevation", OutputUnitLabel::Default), elev);

		for (TopoMetric& metric : _topoMetrics) {
			Raster<metric_t> r = focal<metric_t, coord_t>(elev, 3, metric.fun);
			writeRasterLogErrors(getFullFilename(_getter, topoDir(), metric.name, metric.unit), r);
		}

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
				writeRasterLogErrors(getFullFilename(_getter, topoDir(), fullName, metric.unit), r);
			}
		}
	}
	void TopoHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Topographic Metrics");

		pdf.writeSubsectionTitle(getFullFilename(_getter, "", "MeanElevation", OutputUnitLabel::Default).string());
		std::stringstream elevss;
		elevss << "The mean elevation in each pixel. The units are " << _getter->unitPlural() << ".";
		pdf.writeTextBlockWithWrap(elevss.str());
		
		for (TopoMetric& metric : _topoMetrics) {
			pdf.writeSubsectionTitle(getFullFilename(_getter, "", metric.name, metric.unit).string());
			pdf.writeTextBlockWithWrap(metric.pdfDesc);
		}

		pdf.writeSubsectionTitle("Radius-based Metrics");
		pdf.writeTextBlockWithWrap("Some metrics are calculated on a variety of scales, specified by the user. The following radii were used in this run:");
		for (const std::string& s : _getter->topoWindowNames()) {
			pdf.writeTextBlockWithWrap(s);
		}
		
		for (TopoRadiusMetric& metric : _topoRadiusMetrics) {
			pdf.writeSubsectionTitle(getFullFilename(_getter, "", metric.name + "_XX" + _getter->unitPlural(), metric.unit).string());
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
	
	TopoHandler::ElevMerger::ElevMerger(TopoHandler::ParamGetter* p) : 
        sum(*p->metricAlign()), count(*p->metricAlign()), coarseCellMutexes(p->metricAlign()->ncell()), fineCellMutexes(10000)
	{
        coord_t maxWindow = p->topoWindows().empty() ? 0 : *std::max_element(p->topoWindows().begin(), p->topoWindows().end());

	}
	void TopoHandler::ElevMerger::addRaster(const Raster<coord_t>& dtm)
	{
		std::call_once(init, [&]() {
			fineAlign = cropAlignment(extendAlignment(dtm, sum, SnapType::out), sum, SnapType::out);
			fineCellDone.resize(fineAlign.ncell(), false);
			});

		for (cell_t fineCell : CellIterator(fineAlign, dtm, SnapType::near)) {
            std::scoped_lock fineLock{ fineCellMutexes[fineCell % fineCellMutexes.size()] };
			if (fineCellDone[fineCell]) {
				continue;
			}

            coord_t x = fineAlign.xFromCellUnsafe(fineCell);
            coord_t y = fineAlign.yFromCellUnsafe(fineCell);

			if (!sum.contains(x, y)) {
				continue;
			}
			cell_t dtmCell = dtm.cellFromXY(x, y);
            auto dtmV = dtm.atCellUnsafe(dtmCell);
			if (!dtmV.has_value()) {
				continue;
			}

            cell_t coarseCell = sum.cellFromXY(x, y);
			std::scoped_lock coarseLock{ coarseCellMutexes[coarseCell] };

            fineCellDone[fineCell] = true;
			auto sumV = sum.atCellUnsafe(coarseCell);
			sumV.has_value() = true;
			sumV.value() += dtmV.value();
            auto countV = count.atCellUnsafe(coarseCell);
            countV.has_value() = true;
			countV.value() += 1;

		}
	}
	Raster<coord_t> TopoHandler::ElevMerger::meanElev()
	{
		return sum / count;
	}
}