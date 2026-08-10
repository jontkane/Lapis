#include"run_pch.hpp"
#include"PointMetricHandler.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisController.hpp"
#include"..\parameters\LapisParameters.hpp"

namespace lapis {

	HANDLER_REGISTER_DEFINITION(PointMetricHandler);
	void PointMetricHandler::reset()
	{
		_writeBatcher.flush();

		_pointMetrics.clear();
		_pointMetrics.shrink_to_fit();
		_stratumMetrics.clear();
		_stratumMetrics.shrink_to_fit();

		_allReturnPMC.reset();
		_firstReturnPMC.reset();
		_nLaz = Raster<int>();
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

		using namespace std::chrono;
		for (PointMetricRasters& v : _pointMetrics) {
			MetricFunc& f = v.fun;
            std::optional<DiskBackedRaster<metric_t>>& rasterOpt = v.rasters.get(r);
			if (rasterOpt.has_value()) {
                _writeBatcher.addTask(&rasterOpt.value(), cell, (pmc.*f)());
			}
		}
		std::vector<long long> stratumTimes;
		for (StratumMetricRasters& v : _stratumMetrics) {
			StratumFunc& f = v.fun;
			for (size_t i = 0; i < v.rasters.size(); ++i) {
                std::optional<DiskBackedRaster<metric_t>>& rasterOpt = v.rasters[i].get(r);
                if (rasterOpt.has_value()) {
                    _writeBatcher.addTask(&rasterOpt.value(), cell, (pmc.*f)(i));
                }
			}
		}
		pmc.cleanUp();
	}
	void PointMetricHandler::_initMetrics()
	{
		using pmc = PointMetricCalculator;
		using oul = OutputUnitLabel;
		namespace fs = std::filesystem;

		std::optional<fs::path> firstDir = std::nullopt;
        if (_getter->doFirstReturnMetrics()) {
            firstDir = _getter->doAllReturnMetrics() ? pointMetricDir() / "FirstReturns" : pointMetricDir();
        }
        std::optional<fs::path> allDir = std::nullopt;
        if (_getter->doAllReturnMetrics()) {
            allDir = _getter->doFirstReturnMetrics() ? pointMetricDir() / "AllReturns" : pointMetricDir();
        }
		auto addPointMetric = [&](const std::string& name, MetricFunc f, oul u,
			const std::string& pdfDesc) {
			_pointMetrics.emplace_back(_getter, name, f, u, pdfDesc, firstDir, allDir);
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
			addPointMetric("Mean_Intensity", &pmc::meanIntensity, oul::Unitless,
				"The mean intensity of all points in the cell. "
				"Useful for assessing at-a-glance whether intensity values are comparable across the entire area.");
		}


        std::optional<fs::path> firstStratumDir = std::nullopt;
        if (_getter->doFirstReturnMetrics()) {
            firstStratumDir = firstDir.value() / "StratumMetrics";
        }
        std::optional<fs::path> allStratumDir = std::nullopt;
        if (_getter->doAllReturnMetrics()) {
            allStratumDir = allDir.value() / "StratumMetrics";
        }

		if (_getter->doStratumMetrics()) {
			if (_getter->strataBreaks().size()) {
				_stratumMetrics.emplace_back(_getter, "StratumCover_",
					&pmc::stratumCover, oul::Percent,
					"The number of returns that fall in this stratum, as a percentage of "
				"the number of returns in this stratum or lower. A proxy for the cover present in this stratum.",
					firstStratumDir, allStratumDir);
				_stratumMetrics.emplace_back(_getter, "StratumPercent_", 
					&pmc::stratumPercent, oul::Percent,
					"The number of returns that fall in this stratum, as a percentage of the total number of returns.",
					firstStratumDir, allStratumDir);
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
			pdf.writeSubsectionTitle(getFullFilename(_getter,"",genericName,metric.unit).string());
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
			getFullFilename(_getter,"", "XXthPercentile_CanopyHeight", OutputUnitLabel::Default).string());
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
			pdf.writeSubsectionTitle(getFullFilename(_getter,"", metric.name, metric.unit).string());
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
	PointMetricHandler::PointMetricHandler(ParamGetter* p) : ProductHandler(p)
	{
		_getter = p;
	}
	void PointMetricHandler::prepareForRun()
	{

		tryRemove(pointMetricDir());

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
		LapisLogger& log = LapisLogger::getLogger();
		log.beginVerboseBenchmarkTimer("Assigning points to metric cells");
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
		log.pauseVerboseBenchmarkTimer("Assigning points to metric cells");
	}
	void PointMetricHandler::finishLasFile(const Extent& e, size_t index)
	{
		LapisLogger& log = LapisLogger::getLogger();
		log.endVerboseBenchmarkTimer("Assigning points to metric cells");

		log.beginVerboseBenchmarkTimer("Calculating point metrics");

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
		log.endVerboseBenchmarkTimer("Calculating point metrics");
	}
	void PointMetricHandler::afterLasFiles()
	{
		_writeBatcher.flush();

		_pointMetrics.clear();
		_pointMetrics.shrink_to_fit(); 
		_stratumMetrics.clear();
		_stratumMetrics.shrink_to_fit();

		_allReturnPMC.reset();
		_firstReturnPMC.reset();
		_nLaz = Raster<int>();
	}
	void PointMetricHandler::handleDem(const Raster<coord_t>& dem, size_t index)
	{}
	void PointMetricHandler::handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) {}
	void PointMetricHandler::cleanup() {
		_pointMetrics.clear();
		_pointMetrics.shrink_to_fit();
		_stratumMetrics.clear();
		_stratumMetrics.shrink_to_fit();

		_allReturnPMC.reset();
		_firstReturnPMC.reset();
		_nLaz = Raster<int>();
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
		MetricFunc fun, OutputUnitLabel unit, const std::string& pdfDesc,
		const std::optional<std::filesystem::path>& firstDir,
		const std::optional<std::filesystem::path>& allDir)
		: name(name), fun(fun), unit(unit), pdfDesc(pdfDesc)
	{
        namespace fs = std::filesystem;
        std::optional<fs::path> firstPath = std::nullopt;
        std::optional<fs::path> allPath = std::nullopt;
        if (getter->doFirstReturnMetrics() && firstDir.has_value()) {
            firstPath = ProductHandler::getFullFilename(getter, firstDir.value(), name, unit);
        }
		if (getter->doAllReturnMetrics() && allDir.has_value()) {
            allPath = ProductHandler::getFullFilename(getter, allDir.value(), name, unit);
        }
		rasters = TwoRasters(getter, firstPath, allPath);
	}
	PointMetricHandler::StratumMetricRasters::StratumMetricRasters(ParamGetter* getter, const std::string& baseName,
		StratumFunc fun, OutputUnitLabel unit, const std::string& pdfDesc,
		const std::optional<std::filesystem::path>& firstDir,
		const std::optional<std::filesystem::path>& allDir)
		: baseName(baseName), fun(fun), unit(unit), pdfDesc(pdfDesc)
	{
        namespace fs = std::filesystem;
		for (size_t i = 0; i < getter->strataBreaks().size() + 1; ++i) {
            std::optional<fs::path> firstPath = std::nullopt;
            std::optional<fs::path> allPath = std::nullopt;
            if (getter->doFirstReturnMetrics() && firstDir.has_value()) {
                firstPath = ProductHandler::getFullFilename(getter, firstDir.value(), baseName + getter->strataNames()[i], unit);
            }
            if (getter->doAllReturnMetrics() && allDir.has_value()) {
                allPath = ProductHandler::getFullFilename(getter, allDir.value(), baseName + getter->strataNames()[i], unit);
            }
			rasters.emplace_back(getter, firstPath, allPath);
		}
	}
	PointMetricHandler::TwoRasters::TwoRasters(ParamGetter* getter,
		const std::optional<std::filesystem::path>& firstPath,
		const std::optional<std::filesystem::path>& allPath)
	{
		if (getter->doAllReturnMetrics() && allPath.has_value()) {
			all = ProductHandler::makeDiskBackedRasterLogErrors<metric_t>(allPath.value(), *getter->metricAlign());
		}
		if (getter->doFirstReturnMetrics() && firstPath.has_value()) {
			first = ProductHandler::makeDiskBackedRasterLogErrors<metric_t>(firstPath.value(), *getter->metricAlign());
		}
	}
	std::optional<DiskBackedRaster<metric_t>>& PointMetricHandler::TwoRasters::get(ReturnType r)
	{
		if (r == ReturnType::ALL) {
			return all;
		}
		return first;
	}
	PointMetricHandler::WriteBatcher::WriteBatcher()
	{
        _tasks.reserve(MAX_BATCH_SIZE);
	}
	void PointMetricHandler::WriteBatcher::addTask(DiskBackedRaster<metric_t>* raster, cell_t cell, xtl::xoptional<metric_t> value)
	{
        std::scoped_lock lock{ _mutex };
        _tasks.push_back(WriteTask{ raster, cell, value });
        if (_tasks.size() >= MAX_BATCH_SIZE) {
            _flushUnsafe();
        }
	}
	void PointMetricHandler::WriteBatcher::flush()
	{
        std::scoped_lock lock{ _mutex };
		_flushUnsafe();
	}
	PointMetricHandler::WriteBatcher::~WriteBatcher()
	{
		flush();
	}
	void PointMetricHandler::WriteBatcher::_flushUnsafe()
	{
		//sort first by raster, then by cell, to minimize disk seeks
		std::sort(_tasks.begin(), _tasks.end(), [](const WriteTask& a, const WriteTask& b) {
			if (a.raster != b.raster) {
				return a.raster < b.raster;
			}
			return a.cell < b.cell;
			});

		std::unordered_set<DiskBackedRaster<metric_t>*> rastersToFlush;

		for (const WriteTask& task : _tasks) {
			task.raster->setCellUnsafe(task.cell, task.value);
			rastersToFlush.insert(task.raster);
		}
		for (DiskBackedRaster<metric_t>* raster : rastersToFlush) {
			auto lock = parameterManager().ioLock(raster->filename());
			raster->flush();
		}

		_tasks.clear();
		_tasks.reserve(MAX_BATCH_SIZE);
	}
}