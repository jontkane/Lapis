#pragma once
#ifndef LP_METRICTYPEDEFS_H
#define LP_METRICTYPEDEFS_H

#include"PointMetricCalculator.hpp"
#include"gis/RasterAlgos.hpp"

namespace lapis {
	using MetricFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell);
	using StratumFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell, size_t stratumIdx);

	struct PointMetricRaster {
		std::string name;
		MetricFunc fun;
		Raster<metric_t> rast;
		OutputUnitLabel unit;
	};

	struct StratumMetricRasters {
		std::string baseName;
		std::vector<std::string> stratumNames;
		StratumFunc fun;
		std::vector<Raster<metric_t>> rasters;
		OutputUnitLabel unit;

		StratumMetricRasters(const std::string& bn, const std::vector<std::string>& sn, StratumFunc f, const Alignment& a, OutputUnitLabel u) :
			baseName(bn), stratumNames(sn), fun(f), rasters(sn.size(), a), unit(u) {
		}
	};

	struct TopoMetric {
		ViewFunc<coord_t, metric_t> metric;
		std::string name;
		OutputUnitLabel unit;
	};

	struct CSMMetric {
		ViewFunc<csm_t, metric_t> f;
		std::string name;
		Raster<metric_t> r;
		OutputUnitLabel unit;
	};
}


#endif