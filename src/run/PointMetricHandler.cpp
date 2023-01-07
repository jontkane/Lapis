#include"run_pch.hpp"
#include"PointMetricHandler.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisController.hpp"
#include"..\parameters\RunParameters.hpp"

namespace lapis {
	size_t PointMetricHandler::handlerRegisteredIndex = LapisController::registerHandler(new PointMetricHandler(&RunParameters::singleton()));
	void PointMetricHandler::reset()
	{
		*this = PointMetricHandler(_getter);
	}

	template<bool ALL_RETURNS, bool FIRST_RETURNS>
	void PointMetricHandler::_assignPointsToCalculators(const LidarPointVector& points)
	{
		for (const LasPoint& p : points) {
			cell_t cell = _getter->metricAlign()->cellFromXYUnsafe(p.x, p.y);

			std::lock_guard lock{ _getter->cellMutex(cell) };
			if constexpr (ALL_RETURNS) {
				_allReturnPMC->atCellUnsafe(cell).value().addPoint(p);
			}
			if constexpr (FIRST_RETURNS) {
				if (p.returnNumber == 1) {
					_firstReturnPMC->atCellUnsafe(cell).value().addPoint(p);
				}
			}
		}
	}
	void PointMetricHandler::_processPMCCell(cell_t cell, PointMetricCalculator& pmc, ReturnType r) {

		for (PointMetricRasters& v : _pointMetrics) {
			MetricFunc& f = v.fun;
			if (_getter->doAllReturnMetrics()) {
				(pmc.*f)(v.rasters.get(r), cell);
			}
		}
		for (StratumMetricRasters& v : _stratumMetrics) {
			StratumFunc& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(pmc.*f)(v.rasters[i].get(r), cell, i);
			}
		}

		pmc.cleanUp();
	}
	void PointMetricHandler::_writePointMetricRasters(const std::filesystem::path& dir, ReturnType r) {
		for (PointMetricRasters& metric : _pointMetrics) {
			writeRasterLogErrors(getFullFilename(dir, metric.name, metric.unit), metric.rasters.get(r));
		}
		for (StratumMetricRasters& metric : _stratumMetrics) {
			for (size_t i = 0; i < metric.rasters.size(); ++i) {
				writeRasterLogErrors(getFullFilename(dir / "StratumMetrics", metric.baseName + _getter->strataNames()[i],
					metric.unit), metric.rasters[i].get(r));
			}
		}
	}
	PointMetricHandler::PointMetricHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void PointMetricHandler::prepareForRun()
	{

		std::filesystem::remove_all(pointMetricDir());

		if (!_getter->doPointMetrics()) {
			return;
		}

		using pmc = PointMetricCalculator;
		using oul = OutputUnitLabel;

		pmc::setInfo(_getter->canopyCutoff(), _getter->maxHt(), _getter->binSize(), _getter->strataBreaks());

		_nLaz = Raster<int>(*_getter->metricAlign());
		for (const Extent& e : _getter->lasExtents()) {
			for (cell_t cell : _nLaz.cellsFromExtent(e, SnapType::out)) {
				_nLaz[cell].has_value() = true;
				_nLaz[cell].value()++;
			}
		}

		if (_getter->doAllReturnMetrics()) {
			_allReturnPMC = std::make_unique<Raster<pmc>>(*_getter->metricAlign());
		}
		if (_getter->doFirstReturnMetrics()) {
			_firstReturnPMC = std::make_unique<Raster<pmc>>(*_getter->metricAlign());
		}

		auto addPointMetric = [&](const std::string& name, MetricFunc f, oul u) {
			_pointMetrics.emplace_back(_getter, name, f, u);
		};

		addPointMetric("Mean_CanopyHeight", &pmc::meanCanopy, oul::Default);
		addPointMetric("StdDev_CanopyHeight", &pmc::stdDevCanopy, oul::Default);
		addPointMetric("25thPercentile_CanopyHeight", &pmc::p25Canopy, oul::Default);
		addPointMetric("50thPercentile_CanopyHeight", &pmc::p50Canopy, oul::Default);
		addPointMetric("75thPercentile_CanopyHeight", &pmc::p75Canopy, oul::Default);
		addPointMetric("95thPercentile_CanopyHeight", &pmc::p95Canopy, oul::Default);
		addPointMetric("TotalReturnCount", &pmc::returnCount, oul::Unitless);
		addPointMetric("CanopyCover", &pmc::canopyCover, oul::Percent);
		if (_getter->doAdvancedPointMetrics()) {
			addPointMetric("CoverAboveMean", &pmc::coverAboveMean, oul::Percent);
			addPointMetric("CanopyReliefRatio", &pmc::canopyReliefRatio, oul::Unitless);
			addPointMetric("CanopySkewness", &pmc::skewnessCanopy, oul::Unitless);
			addPointMetric("CanopyKurtosis", &pmc::kurtosisCanopy, oul::Unitless);
			addPointMetric("05thPercentile_CanopyHeight", &pmc::p05Canopy, oul::Default);
			addPointMetric("10thPercentile_CanopyHeight", &pmc::p10Canopy, oul::Default);
			addPointMetric("15thPercentile_CanopyHeight", &pmc::p15Canopy, oul::Default);
			addPointMetric("20thPercentile_CanopyHeight", &pmc::p20Canopy, oul::Default);
			addPointMetric("30thPercentile_CanopyHeight", &pmc::p30Canopy, oul::Default);
			addPointMetric("35thPercentile_CanopyHeight", &pmc::p35Canopy, oul::Default);
			addPointMetric("40thPercentile_CanopyHeight", &pmc::p40Canopy, oul::Default);
			addPointMetric("45thPercentile_CanopyHeight", &pmc::p45Canopy, oul::Default);
			addPointMetric("55thPercentile_CanopyHeight", &pmc::p55Canopy, oul::Default);
			addPointMetric("60thPercentile_CanopyHeight", &pmc::p60Canopy, oul::Default);
			addPointMetric("65thPercentile_CanopyHeight", &pmc::p65Canopy, oul::Default);
			addPointMetric("70thPercentile_CanopyHeight", &pmc::p70Canopy, oul::Default);
			addPointMetric("80thPercentile_CanopyHeight", &pmc::p80Canopy, oul::Default);
			addPointMetric("85thPercentile_CanopyHeight", &pmc::p85Canopy, oul::Default);
			addPointMetric("90thPercentile_CanopyHeight", &pmc::p90Canopy, oul::Default);
			addPointMetric("99thPercentile_CanopyHeight", &pmc::p99Canopy, oul::Default);
		}

		if (_getter->doStratumMetrics()) {
			if (_getter->strataBreaks().size()) {
				_stratumMetrics.emplace_back(_getter, "StratumCover_", &pmc::stratumCover, oul::Percent);
				_stratumMetrics.emplace_back(_getter, "StratumReturnProportion_", &pmc::stratumPercent, oul::Percent);
			}
		}
	}
	void PointMetricHandler::handlePoints(const LidarPointVector& points, const Extent& e, size_t index)
	{
		if (!_getter->doPointMetrics()) {
			return;
		}

		//This structure is kind of ugly and inelegant, but it ensures that all returns and first returns can share a call to cellFromXY
		//without needing to pollute the loop with a bunch of if checks
		//Right now, with only two booleans, the combinatorics are bearable; if it increases, then it probably won't be
		if (_getter->doFirstReturnMetrics() && _getter->doAllReturnMetrics()) {
			_assignPointsToCalculators<true, true>(points);
		}
		else if (_getter->doAllReturnMetrics()) {
			_assignPointsToCalculators<true, false>(points);
		}
		else if (_getter->doFirstReturnMetrics()) {
			_assignPointsToCalculators<false, true>(points);
		}

		std::vector<cell_t> cells = _nLaz.cellsFromExtent(e, SnapType::out);
		for (cell_t cell : cells) {
			std::scoped_lock lock{ _getter->cellMutex(cell) };
			_nLaz.atCellUnsafe(cell).value()--;
			if (_nLaz.atCellUnsafe(cell).value() != 0) {
				continue;
			}
			if (_getter->doAllReturnMetrics())
				_processPMCCell(cell, _allReturnPMC->atCellUnsafe(cell).value(), ReturnType::ALL);
			if (_getter->doFirstReturnMetrics())
				_processPMCCell(cell, _firstReturnPMC->atCellUnsafe(cell).value(), ReturnType::FIRST);
		}

	}
	void PointMetricHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{
	}
	void PointMetricHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) {}
	void PointMetricHandler::cleanup() {
		namespace fs = std::filesystem;

		if (!_getter->doPointMetrics()) {
			return;
		}

		if (_getter->doAllReturnMetrics()) {
			fs::path allReturnsMetricDir = _getter->doFirstReturnMetrics() ? pointMetricDir() / "AllReturns" : pointMetricDir();
			_writePointMetricRasters(allReturnsMetricDir, ReturnType::ALL);
		}
		if (_getter->doFirstReturnMetrics()) {
			fs::path firstReturnsMetricDir = _getter->doAllReturnMetrics() ? pointMetricDir() / "FirstReturns" : pointMetricDir();
			_writePointMetricRasters(firstReturnsMetricDir, ReturnType::FIRST);
		}

		_pointMetrics.clear();
		_pointMetrics.shrink_to_fit();

		_stratumMetrics.clear();
		_stratumMetrics.shrink_to_fit();
	}
	std::filesystem::path PointMetricHandler::pointMetricDir() const
	{
		return parentDir() / "PointMetrics";
	}

	PointMetricHandler::PointMetricRasters::PointMetricRasters(ParamGetter* getter, const std::string& name, MetricFunc fun, OutputUnitLabel unit)
		: name(name), fun(fun), unit(unit), rasters(getter)
	{
	}
	PointMetricHandler::StratumMetricRasters::StratumMetricRasters(ParamGetter* getter, const std::string& baseName, StratumFunc fun, OutputUnitLabel unit)
		: baseName(baseName), fun(fun), unit(unit)
	{
		for (size_t i = 0; i < getter->strataBreaks().size() + 1; ++i) {
			rasters.emplace_back(getter);
		}
	}
	PointMetricHandler::TwoRasters::TwoRasters(ParamGetter* getter)
	{
		if (getter->doAllReturnMetrics()) {
			all = Raster<metric_t>(*getter->metricAlign());
		}
		if (getter->doFirstReturnMetrics()) {
			first = Raster<metric_t>(*getter->metricAlign());
		}
	}
	Raster<metric_t>& PointMetricHandler::TwoRasters::get(ReturnType r)
	{
		if (r == ReturnType::ALL) {
			return all.value();
		}
		return first.value();
	}
}