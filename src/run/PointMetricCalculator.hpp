#pragma once
#ifndef lp_pointmetriccalculator_h
#define lp_pointmetriccalculator_h

#include"run_pch.hpp"

namespace lapis {

	class SparseHistogram {
		
	public:
		static void setNHists(size_t nHists);

		SparseHistogram() = default;

		int countInBin(size_t bin) const;

		void incrementBin(size_t bin);

		size_t size();

		void cleanUp();

	private:
		inline static constexpr size_t binsPerHist = 100;
		using _storage = std::vector<std::unique_ptr<std::array<int, binsPerHist>>>;
		_storage _data;
		inline static size_t _nHists;
	};

	class PointMetricCalculator {
	public:
		PointMetricCalculator() = default;

		//This should be called before any PointMetricCalculators are constructed, and shouldn't be called after any are constructed
		//as you might guess from the names, max should be strictly greater than canopyCutoff, and binsize should be positive
		//it's the callers responsibility to ensure that these are in the right units
		static void setInfo(coord_t canopyCutoff, coord_t max, coord_t binsize, const std::vector<coord_t>& strataBreaks);

		//Adds an observed lidar return to this object
		//If this is the first point added, it will cause the histogram vector to be allocated
		void addPoint(const LasPoint& lp);
		//this function will deallocate the histogram vector. Call it once you're done with the data here.
		void cleanUp();

		//These functions insert the result of the given calculation into the given raster at the given cell
		void meanCanopy(Raster<metric_t>& r, cell_t cell);
		void stdDevCanopy(Raster<metric_t>& r, cell_t cell);
		void p25Canopy(Raster<metric_t>& r, cell_t cell);
		void p50Canopy(Raster<metric_t>& r, cell_t cell);
		void p75Canopy(Raster<metric_t>& r, cell_t cell);
		void p95Canopy(Raster<metric_t>& r, cell_t cell);
		void returnCount(Raster<metric_t>& r, cell_t cell);
		void canopyCover(Raster<metric_t>& r, cell_t cell);

		//these are currently classed as "advanced" metrics. Grouped separately for clarity
		void coverAboveMean(Raster<metric_t>& r, cell_t cell);
		void canopyReliefRatio(Raster<metric_t>& r, cell_t cell);
		void skewnessCanopy(Raster<metric_t>& r, cell_t cell);
		void kurtosisCanopy(Raster<metric_t>& r, cell_t cell);
		void p05Canopy(Raster<metric_t>& r, cell_t cell);
		void p10Canopy(Raster<metric_t>& r, cell_t cell);
		void p15Canopy(Raster<metric_t>& r, cell_t cell);
		void p20Canopy(Raster<metric_t>& r, cell_t cell);
		void p30Canopy(Raster<metric_t>& r, cell_t cell);
		void p35Canopy(Raster<metric_t>& r, cell_t cell);
		void p40Canopy(Raster<metric_t>& r, cell_t cell);
		void p45Canopy(Raster<metric_t>& r, cell_t cell);
		void p55Canopy(Raster<metric_t>& r, cell_t cell);
		void p60Canopy(Raster<metric_t>& r, cell_t cell);
		void p65Canopy(Raster<metric_t>& r, cell_t cell);
		void p70Canopy(Raster<metric_t>& r, cell_t cell);
		void p80Canopy(Raster<metric_t>& r, cell_t cell);
		void p85Canopy(Raster<metric_t>& r, cell_t cell);
		void p90Canopy(Raster<metric_t>& r, cell_t cell);
		void p99Canopy(Raster<metric_t>& r, cell_t cell);

		void stratumCover(Raster<metric_t>& r, cell_t cell, size_t stratumIdx);
		void stratumPercent(Raster<metric_t>& r, cell_t cell, size_t stratumIdx);

		//TODO: put in functions to dump the data to the harddrive, or to read it off

	private:
		inline static coord_t _max, _binsize, _canopy;
		inline static std::vector<coord_t> _strataBreaks;
		SparseHistogram _hist;
		coord_t _canopySum = 0.;
		int _count = 0;
		int _canopyCount = 0;
		std::vector<int> _strataCounts;

		void _quantileCanopy(Raster<metric_t>& r, cell_t cell, metric_t q);



		metric_t _estimatePointValue(size_t binNumber, int ordinal);
	};
}

#endif