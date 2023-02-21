#pragma once
#ifndef LP_POINTMETRICHANDLER_H
#define LP_POINTMETRICHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class PointMetricHandler : public ProductHandler {
	public:
		using ParamGetter = PointMetricParameterGetter;
		PointMetricHandler(ParamGetter* p);

		void prepareForRun() override;
		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		std::string name() override;
		static size_t handlerRegisteredIndex;

		void describeInPdf(MetadataPdf& pdf) override;

		std::filesystem::path pointMetricDir() const;

		//protected to make testing easier
	protected:
		Raster<int> _nLaz;

		template<class T>
		using unique_raster = std::unique_ptr<Raster<T>>;
		unique_raster<PointMetricCalculator> _allReturnPMC;
		unique_raster<PointMetricCalculator> _firstReturnPMC;

		enum class ReturnType {
			FIRST, ALL
		};
		struct TwoRasters {
			std::optional<Raster<metric_t>> first;
			std::optional<Raster<metric_t>> all;
			TwoRasters(ParamGetter* getter);
			Raster<metric_t>& get(ReturnType r);
		};

		using MetricFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell);
		struct PointMetricRasters {
			std::string name;
			MetricFunc fun;
			OutputUnitLabel unit;
			TwoRasters rasters;
			std::string pdfDesc;

			PointMetricRasters(ParamGetter* getter, const std::string& name,
				MetricFunc fun, OutputUnitLabel unit, const std::string& pdfDesc);
		};
		std::vector<PointMetricRasters> _pointMetrics;

		using StratumFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell, size_t stratumIdx);
		struct StratumMetricRasters {
			std::string baseName;
			StratumFunc fun;
			OutputUnitLabel unit;
			std::vector<TwoRasters> rasters;
			std::string pdfDesc;

			StratumMetricRasters(ParamGetter* getter, const std::string& baseName,
				StratumFunc fun, OutputUnitLabel unit, const std::string& pdfDesc);
		};
		std::vector<StratumMetricRasters> _stratumMetrics;

		ParamGetter* _getter;

		template<bool ALL_RETURNS, bool FIRST_RETURNS>
		void _assignPointsToCalculators(const LidarPointVector& points);
		void _writePointMetricRasters(const std::filesystem::path& dir, ReturnType r);
		void _processPMCCell(cell_t cell, PointMetricCalculator& pmc, ReturnType r);

		void _initMetrics();
		void _stratumPdf(MetadataPdf& pdf);
		void _metricPdf(MetadataPdf& pdf);
	};
}

#endif