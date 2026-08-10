#pragma once
#ifndef LP_POINTMETRICHANDLER_H
#define LP_POINTMETRICHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class PointMetricHandler : public ProductHandler {
	public:
		using ParamGetter = PointMetricParameterGetter;
		PointMetricHandler(ParamGetter* p);
		HANDLER_REGISTER_DECLARATION;

		void prepareForRun() override;
		void handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index) override;
		void finishLasFile(const Extent& e, size_t index) override;
		void afterLasFiles() override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		bool doThisProduct() override;
		std::string name() override;

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
			std::optional<DiskBackedRaster<metric_t>> first;
			std::optional<DiskBackedRaster<metric_t>> all;
			std::optional<DiskBackedRaster<metric_t>>& get(ReturnType r);

			TwoRasters() = default;
            TwoRasters(ParamGetter* getter,
				const std::optional<std::filesystem::path>& firstPath,
				const std::optional<std::filesystem::path>& allPath);
		};

		using MetricFunc = xtl::xoptional<metric_t>(PointMetricCalculator::*)();
		struct PointMetricRasters {
			std::string name;
			MetricFunc fun;
			OutputUnitLabel unit;
			TwoRasters rasters;
			std::string pdfDesc;

			PointMetricRasters(ParamGetter* getter, const std::string& name,
				MetricFunc fun, OutputUnitLabel unit, const std::string& pdfDesc,
				const std::optional<std::filesystem::path>& firstDir,
				const std::optional<std::filesystem::path>& allDir);
		};
		std::vector<PointMetricRasters> _pointMetrics;

        using StratumFunc = xtl::xoptional<metric_t>(PointMetricCalculator::*)(size_t stratumIdx);
		struct StratumMetricRasters {
			std::string baseName;
			StratumFunc fun;
			OutputUnitLabel unit;
			std::vector<TwoRasters> rasters;
			std::string pdfDesc;

			StratumMetricRasters(ParamGetter* getter, const std::string& baseName,
				StratumFunc fun, OutputUnitLabel unit, const std::string& pdfDesc,
				const std::optional<std::filesystem::path>& firstDir,
				const std::optional<std::filesystem::path>& allDir);
		};
		std::vector<StratumMetricRasters> _stratumMetrics;

		ParamGetter* _getter;

		template<bool ALL_RETURNS, bool FIRST_RETURNS>
		void _assignPointsToCalculators(const std::span<LasPoint>& points);
		void _processPMCCell(cell_t cell, PointMetricCalculator& pmc, ReturnType r);

		void _initMetrics();
		void _stratumPdf(MetadataPdf& pdf);
		void _metricPdf(MetadataPdf& pdf);

		class WriteBatcher {
		public:

			WriteBatcher();
            void addTask(DiskBackedRaster<metric_t>* raster, cell_t cell, xtl::xoptional<metric_t> value);
            void flush();
			~WriteBatcher();

		private:
			struct WriteTask {
                DiskBackedRaster<metric_t>* raster;
                cell_t cell;
                xtl::xoptional<metric_t> value;
			};

			//150MB worth of tasks
			inline constexpr static size_t MAX_BATCH_SIZE = 150 * 1024 * 1024 / sizeof(WriteTask);

            std::vector<WriteTask> _tasks;
            std::mutex _mutex;

			void _flushUnsafe();
		};

        WriteBatcher _writeBatcher;
	};
}

#endif