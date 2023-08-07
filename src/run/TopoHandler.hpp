#pragma once
#ifndef LP_TOPOHANDLER_H
#define LP_TOPOHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class TopoHandler : public ProductHandler {
	public:
		using ParamGetter = TopoParameterGetter;
		TopoHandler(ParamGetter* p);

		void prepareForRun() override;
		void handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index) override;
		void finishLasFile(const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		bool doThisProduct() override;
		std::string name() override;
		static size_t handlerRegisteredIndex;

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

		class ElevMergerRasterCell {
		public:
			void addRaster(const Raster<coord_t>& dtm, const Extent& thisCell);
			xtl::xoptional<coord_t> reportMeanAndDealloc();

		private:
			xtl::xoptional<coord_t> finishedMean;
			std::unique_ptr<Raster<coord_t>> miniRaster;
			bool anyData = false;
		};

		class ElevMerger {
		public:
			ElevMerger(TopoHandler::ParamGetter* p);
			void addRaster(const Raster<coord_t>& dtm);
			Raster<coord_t> meanElev();

		private:
			Raster<ElevMergerRasterCell> cells;
			TopoHandler::ParamGetter* getter;
		};

		std::unique_ptr<ElevMerger> merger;
	};
}

#endif