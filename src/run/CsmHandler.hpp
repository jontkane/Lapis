#pragma once
#ifndef LP_CSMHANDLER_H
#define LP_CSMHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class CsmHandler : public ProductHandler {
	public:
		using ParamGetter = CsmParameterGetter;
		CsmHandler(ParamGetter* p);

		void prepareForRun() override;
		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		std::string name() override;
		static size_t handlerRegisteredIndex;

		void describeInPdf(MetadataPdf& pdf) override;

		virtual Raster<csm_t> getBufferedCsm(cell_t tile) const;

		std::filesystem::path csmTempDir() const;
		std::filesystem::path csmDir() const;
		std::filesystem::path csmMetricDir() const;

	protected:
		using CsmMetricFunc = ViewFunc<metric_t, csm_t>;
		struct CSMMetricRaster {
			std::string name;
			CsmMetricFunc fun;
			OutputUnitLabel unit;
			Raster<metric_t> raster;
			std::string pdfDesc;

			CSMMetricRaster(ParamGetter* getter, const std::string& name, CsmMetricFunc fun,
				OutputUnitLabel unit, const std::string& pdfDesc);
		};
		std::vector<CSMMetricRaster> _csmMetrics;
		std::string _csmBaseName = "CanopySurfaceModel";

		ParamGetter* _getter;

		void _initMetrics();
		
	};
}

#endif