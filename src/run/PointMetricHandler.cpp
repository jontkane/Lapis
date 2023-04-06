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

	bool PointMetricHandler::doThisProduct()
	{
		return _getter->doPointMetrics();
	}

	std::string PointMetricHandler::name()
	{
		return "Point Metrics";
	}

	template<bool ALL_RETURNS, bool FIRST_RETURNS>
	void PointMetricHandler::_assignPointsToCalculators(const std::span<LasPoint>& points)
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
			(pmc.*f)(v.rasters.get(r), cell);
		}
		for (StratumMetricRasters& v : _stratumMetrics) {
			StratumFunc& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
				(pmc.*f)(v.rasters[i].get(r), cell, i);
			}
		}

		pmc.cleanUp();
	}
	void PointMetricHandler::_initMetrics()
	{
		using pmc = PointMetricCalculator;
		using oul = OutputUnitLabel;

		auto addPointMetric = [&](const std::string& name, MetricFunc f, oul u,
			const std::string& pdfDesc) {
			_pointMetrics.emplace_back(_getter, name, f, u, pdfDesc);
		};

		addPointMetric("Mean_CanopyHeight", &pmc::meanCanopy, oul::Default,
			"The mean height of all canopy returns.");
		addPointMetric("StdDev_CanopyHeight", &pmc::stdDevCanopy, oul::Default,
			"The standard deviation of height of all canopy returns.");
		addPointMetric("25thPercentile_CanopyHeight", &pmc::p25Canopy, oul::Default,
			"");
		addPointMetric("50thPercentile_CanopyHeight", &pmc::p50Canopy, oul::Default,
			"");
		addPointMetric("75thPercentile_CanopyHeight", &pmc::p75Canopy, oul::Default,
			"");
		addPointMetric("95thPercentile_CanopyHeight", &pmc::p95Canopy, oul::Default,
			"");
		addPointMetric("TotalReturnCount", &pmc::returnCount, oul::Unitless,
			"The total number of returns. Mostly useful for data validation.");
		addPointMetric("CanopyCover", &pmc::canopyCover, oul::Percent,
			"The percentage of returns which are canopy returns. A proxy for canopy cover.");
		if (_getter->doAdvancedPointMetrics()) {
			addPointMetric("CoverAboveMean", &pmc::coverAboveMean, oul::Percent,
				"The percentage of returns with a height greater than the mean height.");
			addPointMetric("CanopyReliefRatio", &pmc::canopyReliefRatio, oul::Unitless,
				"A combination of the mean, minimum, and maximum values of return height. The formula "
			"is (mean-min)/(max-min).");
			addPointMetric("CanopySkewness", &pmc::skewnessCanopy, oul::Unitless,
				"The mathematical concept of skewness, calculated on the height of canopy returns. "
				"A measure of how symmetric the canopy profile is around the center of the canopy. "
				"A high value indicates a lack of symmetry.");
			addPointMetric("CanopyKurtosis", &pmc::kurtosisCanopy, oul::Unitless,
				"The mathematical concept of kurtosis, calculated on the height of canopy returns. "
				"A measure of how spread the canopy profile is. A high value indicates a lot of spread.");
			addPointMetric("05thPercentile_CanopyHeight", &pmc::p05Canopy, oul::Default,
				"");
			addPointMetric("10thPercentile_CanopyHeight", &pmc::p10Canopy, oul::Default,
				"");
			addPointMetric("15thPercentile_CanopyHeight", &pmc::p15Canopy, oul::Default,
				"");
			addPointMetric("20thPercentile_CanopyHeight", &pmc::p20Canopy, oul::Default,
				"");
			addPointMetric("30thPercentile_CanopyHeight", &pmc::p30Canopy, oul::Default,
				"");
			addPointMetric("35thPercentile_CanopyHeight", &pmc::p35Canopy, oul::Default,
				"");
			addPointMetric("40thPercentile_CanopyHeight", &pmc::p40Canopy, oul::Default,
				"");
			addPointMetric("45thPercentile_CanopyHeight", &pmc::p45Canopy, oul::Default,
				"");
			addPointMetric("55thPercentile_CanopyHeight", &pmc::p55Canopy, oul::Default,
				"");
			addPointMetric("60thPercentile_CanopyHeight", &pmc::p60Canopy, oul::Default,
				"");
			addPointMetric("65thPercentile_CanopyHeight", &pmc::p65Canopy, oul::Default,
				"");
			addPointMetric("70thPercentile_CanopyHeight", &pmc::p70Canopy, oul::Default,
				"");
			addPointMetric("80thPercentile_CanopyHeight", &pmc::p80Canopy, oul::Default,
				"");
			addPointMetric("85thPercentile_CanopyHeight", &pmc::p85Canopy, oul::Default,
				"");
			addPointMetric("90thPercentile_CanopyHeight", &pmc::p90Canopy, oul::Default,
				"");
			addPointMetric("99thPercentile_CanopyHeight", &pmc::p99Canopy, oul::Default,
				"");
		}

		if (_getter->doStratumMetrics()) {
			if (_getter->strataBreaks().size()) {
				_stratumMetrics.emplace_back(_getter, "StratumCover_",
					&pmc::stratumCover, oul::Percent,
					"The number of returns that fall in this stratum, as a percentage of "
				"the number of returns in this stratum or lower. A proxy for the cover present in this stratum.");
				_stratumMetrics.emplace_back(_getter, "StratumPercent_", 
					&pmc::stratumPercent, oul::Percent,
					"The number of returns that fall in this stratum, as a percentage of the total number of returns.");
			}
		}
	}
	void PointMetricHandler::_stratumPdf(MetadataPdf& pdf)
	{
		pdf.writeSubsectionTitle("Stratum metrics");
		std::stringstream strata;
		pdf.writeTextBlockWithWrap("Some metrics are calculated on vertical slices of the lidar returns. "
			"Their filename will indicate what slice was used to calculate them. "
			"These metrics exist in the StratumMetrics directory, under the point metrics directory. "
			"The strata used in this run are: ");
		for (size_t i = 0; i < _getter->strataNames().size(); ++i) {
			pdf.writeTextBlockWithWrap(_getter->strataNames()[i]);
		}

		for (auto& metric : _stratumMetrics) {
			std::string genericName = metric.baseName + "XXtoXX";
			pdf.writeSubsectionTitle(getFullFilename("",genericName,metric.unit).string());
			std::stringstream metricDesc;
			metricDesc << metric.pdfDesc << " ";
			if (metric.unit == OutputUnitLabel::Default) {
				metricDesc << "The units are " << pdf.strToLower(_getter->unitPlural()) << ".";
			}
			else if (metric.unit == OutputUnitLabel::Percent) {
				metricDesc << "The units are percent.";
			}
			else if (metric.unit == OutputUnitLabel::Unitless) {
				metricDesc << "This metric is unitless.";
			}
			pdf.writeTextBlockWithWrap(metricDesc.str());
		}
	}
	void PointMetricHandler::_metricPdf(MetadataPdf& pdf)
	{
		pdf.writeSubsectionTitle(
			getFullFilename("", "XXthPercentile_CanopyHeight", OutputUnitLabel::Default).string());
		std::stringstream percentiles;
		percentiles << "There are a large number of metrics of this form, replacing XX with a specific number. "
			"They are calculated as the given percentile of the heights of canopy returns. The 25th percentile is "
			"a proxy for height to live crown. The 95th percentile is a proxy for the height of the tallest tree. "
			"They are measured in " << pdf.strToLower(_getter->unitPlural()) << ".";
		pdf.writeTextBlockWithWrap(percentiles.str());

		for (auto& metric : _pointMetrics) {
			if (!metric.pdfDesc.size()) {
				continue;
			}
			pdf.writeSubsectionTitle(getFullFilename("", metric.name, metric.unit).string());
			std::stringstream metricDesc;
			metricDesc << metric.pdfDesc << " ";
			if (metric.unit == OutputUnitLabel::Default) {
				metricDesc << "The units are " << pdf.strToLower(_getter->unitPlural()) << ".";
			}
			else if (metric.unit == OutputUnitLabel::Percent) {
				metricDesc << "The units are percent.";
			}
			else if (metric.unit == OutputUnitLabel::Unitless) {
				metricDesc << "This metric is unitless.";
			}
			pdf.writeTextBlockWithWrap(metricDesc.str());
		}
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

		tryRemove(pointMetricDir());

		if (!_getter->doPointMetrics()) {
			return;
		}

		using pmc = PointMetricCalculator;
		using oul = OutputUnitLabel;

		pmc::setInfo(_getter->canopyCutoff(), _getter->maxHt(), _getter->binSize(), _getter->strataBreaks());

		_nLaz = Raster<int>(*_getter->metricAlign());
		for (const Extent& e : _getter->lasExtents()) {
			for (cell_t cell : CellIterator(_nLaz, e, SnapType::out)) {
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

		_initMetrics();
	}
	void PointMetricHandler::handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index)
	{
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
	}
	void PointMetricHandler::finishLasFile(const Extent& e, size_t index)
	{
		for (cell_t cell : CellIterator(_nLaz, e, SnapType::out)) {
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

		if (_getter->doAllReturnMetrics()) {
			fs::path allReturnsMetricDir = _getter->doFirstReturnMetrics() ? pointMetricDir() / "AllReturns" : pointMetricDir();
			_writePointMetricRasters(allReturnsMetricDir, ReturnType::ALL);
		}
		if (_getter->doFirstReturnMetrics()) {
			fs::path firstReturnsMetricDir = _getter->doAllReturnMetrics() ? pointMetricDir() / "FirstReturns" : pointMetricDir();
			_writePointMetricRasters(firstReturnsMetricDir, ReturnType::FIRST);
		}

		_pointMetrics = std::vector<PointMetricRasters>();

		_stratumMetrics = std::vector<StratumMetricRasters>();
	}
	void PointMetricHandler::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Point Metrics");
		std::stringstream overall;
		overall << "The PointMetrics folder contains summary statistics calculated on the height values of lidar returns "
			"contained in each cell of the raster. "
			"They are commonly used as inputs to models, and some are interpretable values on their own. "
			"In many of them, returns below the canopy are excluded. It is important for most purposes to exclude "
			"ground returns from these metrics, but by adjusting the definition of canopy, you can exclude or include "
			"shrubs. In this run, canopy points are defined as points at least "
			<< pdf.numberWithUnits(_getter->canopyCutoff(), _getter->unitSingular(), _getter->unitPlural())
			<< " above the ground. ";
		if (_getter->doAllReturnMetrics() && _getter->doFirstReturnMetrics()) {
			overall << "Two versions of each metric exist: one calculated using only first returns, "
				"and one calculated with all returns. The first return versions are in the PointMetrics/FirstReturns "
				"directory, and the all return versions are in the PointMetrics/AllReturns directory.";
		}
		else if (_getter->doAllReturnMetrics()) {
			overall << "These metrics were calculated using all returns.";
		}
		else if (_getter->doFirstReturnMetrics()) {
			overall << "These metrics were calculated using only first returns.";
		}
		pdf.writeTextBlockWithWrap(overall.str());


		_metricPdf(pdf);
		if (_getter->doStratumMetrics()) {
			_stratumPdf(pdf);
		}
	}
	std::filesystem::path PointMetricHandler::pointMetricDir() const
	{
		return parentDir() / "PointMetrics";
	}

	PointMetricHandler::PointMetricRasters::PointMetricRasters(ParamGetter* getter, const std::string& name,
		MetricFunc fun, OutputUnitLabel unit, const std::string& pdfDesc)
		: name(name), fun(fun), unit(unit), rasters(getter), pdfDesc(pdfDesc)
	{
	}
	PointMetricHandler::StratumMetricRasters::StratumMetricRasters(ParamGetter* getter, const std::string& baseName,
		StratumFunc fun, OutputUnitLabel unit, const std::string& pdfDesc)
		: baseName(baseName), fun(fun), unit(unit), pdfDesc(pdfDesc)
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