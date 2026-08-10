#pragma once
#ifndef LP_TOPOHANDLER_H
#define LP_TOPOHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class TopoHandler : public ProductHandler {
	public:
		using ParamGetter = TopoParameterGetter;
		TopoHandler(ParamGetter* p);
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

		std::filesystem::path topoDir() const;

	protected:

		using TopoFunc = ViewFunc<metric_t, coord_t>;
		struct TopoMetric {
			std::string name;
			TopoFunc fun;
			OutputUnitLabel unit;
			std::string pdfDesc;

			TopoMetric(const std::string& name, TopoFunc fun, OutputUnitLabel unit, const std::string& pdfDesc);
		};
		std::vector<TopoMetric> _topoMetrics;

		using TopoRadiusFunc = std::function<Raster<metric_t>(const Raster<coord_t>&, coord_t, const Extent&)>;
		struct TopoRadiusMetric {
			std::string name;
			TopoRadiusFunc fun;
			OutputUnitLabel unit;
			std::string pdfDesc;

			TopoRadiusMetric(const std::string& name, TopoRadiusFunc fun, OutputUnitLabel unit, const std::string& pdfDesc);
		};
		std::vector<TopoRadiusMetric> _topoRadiusMetrics;

		ParamGetter* _getter;

		class ElevMerger {
		public:
			ElevMerger(TopoHandler::ParamGetter* p);
			void addRaster(const Raster<coord_t>& dtm);
			Raster<coord_t> meanElev();

		private:
			std::once_flag init;
			Raster<coord_t> sum;
			Raster<coord_t> count;
			Alignment fineAlign;
			std::vector<bool> fineCellDone;
			std::vector<std::mutex> coarseCellMutexes;
			std::vector<std::mutex> fineCellMutexes;
		};

		std::unique_ptr<ElevMerger> merger;
	};
}

#endif