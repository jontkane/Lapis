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
		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		static size_t handlerRegisteredIndex;

		std::filesystem::path topoDir() const;

	protected:
		Raster<coord_t> _elevNumerator;
		Raster<coord_t> _elevDenominator;

		using TopoFunc = ViewFunc<coord_t, metric_t>;
		struct TopoMetric {
			std::string name;
			TopoFunc fun;
			OutputUnitLabel unit;

			TopoMetric(const std::string& name, TopoFunc fun, OutputUnitLabel unit);
		};
		std::vector<TopoMetric> _topoMetrics;

		ParamGetter* _getter;
	};
}

#endif