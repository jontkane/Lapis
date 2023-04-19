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
		Raster<coord_t> _elevNumerator;
		Raster<coord_t> _elevDenominator;

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
	};
}

#endif