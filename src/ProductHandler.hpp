#pragma once
#ifndef LP_PRODUCT_HANDLER_H
#define LP_PRODUCT_HANDLER_H

#include"app_pch.hpp"
#include"ParameterGetter.hpp"
#include"csmalgos.hpp"

namespace lapis {

	class PointMetricCalculator;
	class MetricFunc;

	class ProductHandler {
	public:
		using ParamGetter = SharedParameterGetter;
		ProductHandler(ParamGetter* p);

		//this function can assume that all points pass all filters, are normalized to the ground
		//are in the same projection (including Z units) as the extent, and are contained in the extent
		virtual void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) = 0;
		virtual void handleDem(const Raster<coord_t>& dem, size_t index) = 0;
		virtual void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) = 0;
		virtual void cleanup() = 0;

		std::filesystem::path parentDir() const;
		std::filesystem::path tempDir() const;
		std::filesystem::path pointMetricDir() const;
		std::filesystem::path csmTempDir() const;
		std::filesystem::path csmDir() const;
		std::filesystem::path csmMetricDir() const;
		std::filesystem::path taoDir() const;
		std::filesystem::path taoTempDir() const;
		std::filesystem::path fineIntDir() const;
		std::filesystem::path fineIntTempDir() const;
		std::filesystem::path topoDir() const;

	protected:
		ParamGetter* _sharedGetter;
		void deleteTempDirIfEmpty() const;
	};

	class PointMetricHandler : public ProductHandler {
	public:
		using ParamGetter = PointMetricParameterGetter;
		PointMetricHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;

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

			PointMetricRasters(ParamGetter* getter, const std::string& name, MetricFunc fun, OutputUnitLabel unit);
		};
		std::vector<PointMetricRasters> _pointMetrics;

		using StratumFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell, size_t stratumIdx);
		struct StratumMetricRasters {
			std::string baseName;
			StratumFunc fun;
			OutputUnitLabel unit;
			std::vector<TwoRasters> rasters;

			StratumMetricRasters(ParamGetter* getter, const std::string& baseName, StratumFunc fun, OutputUnitLabel unit);
		};
		std::vector<StratumMetricRasters> _stratumMetrics;

	private:
		ParamGetter* _getter;

		template<bool ALL_RETURNS, bool FIRST_RETURNS>
		void _assignPointsToCalculators(const LidarPointVector& points);
		void _writePointMetricRasters(const std::filesystem::path& dir, ReturnType r);
		void _processPMCCell(cell_t cell, PointMetricCalculator& pmc, ReturnType r);
	};

	class CsmHandler : public ProductHandler {
	public:
		using ParamGetter = CsmParameterGetter;
		CsmHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;

		Raster<csm_t> getBufferedCsm(cell_t tile) const;

	protected:
		using CsmMetricFunc = ViewFunc<csm_t, metric_t>;
		struct CSMMetricRaster {
			std::string name;
			CsmMetricFunc fun;
			OutputUnitLabel unit;
			Raster<metric_t> raster;

			CSMMetricRaster(ParamGetter* getter, const std::string& name, CsmMetricFunc fun, OutputUnitLabel unit);
		};
		std::vector<CSMMetricRaster> _csmMetrics;

	private:
		ParamGetter* _getter;
	};

	class TaoHandler : public ProductHandler {
	public:
		using ParamGetter = TaoParameterGetter;
		TaoHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;

	protected:
		TaoIdMap idMap;

		void _writeHighPointsAsXYZArray(const std::vector<cell_t>& highPoints, const Raster<csm_t>& csm, const Extent& unbufferedExtent, cell_t tile) const;
		std::vector<CoordXYZ> _readHighPointsFromXYZArray(cell_t tile) const;

	private:
		ParamGetter* _getter;

		void _updateMap(const Raster<taoid_t>& segments, const std::vector<cell_t>& highPoints, const Extent& unbufferedExtent, cell_t tileidx);
		void _fixTaoIdsThread(cell_t tile) const;
		void _writeHighPointsAsShp(const Raster<taoid_t>& segments, cell_t tile) const;
	};

	class FineIntHandler : public ProductHandler {
	public:
		using ParamGetter = FineIntParameterGetter;
		FineIntHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;

	private:
		ParamGetter* _getter;
	};

	class TopoHandler : public ProductHandler {
	public:
		using ParamGetter = TopoParameterGetter;
		TopoHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;

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

	private:
		ParamGetter* _getter;
	};
}

#endif