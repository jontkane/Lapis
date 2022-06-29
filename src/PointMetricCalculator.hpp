#pragma once
#ifndef lp_pointmetriccalculator_h
#define lp_pointmetriccalculator_h

#include"app_pch.hpp"
#include"LapisTypedefs.hpp"
#include"gis/Raster.hpp"
#include"gis/LasReader.hpp"

namespace lapis {

	class SparseHistogram {
	public:
		static void setNHists(size_t nHists);

		SparseHistogram() = default;

		int countInBin(size_t bin);

		void incrementBin(size_t bin);

		size_t size();

		void cleanUp();

	private:
		inline static constexpr size_t binsPerHist = 100;
		std::vector<std::unique_ptr<std::array<int,binsPerHist>>> _data;
		inline static size_t _nHists;
	};

	class PointMetricCalculator {
	public:
		PointMetricCalculator() = default;

		//This should be called before any PointMetricCalculators are constructed, and shouldn't be called after any are constructed
		//as you might guess from the names, max should be strictly greater than canopyCutoff, and binsize should be positive
		//it's the callers responsibility to ensure that these are in the right units
		static void setInfo(coord_t canopyCutoff, coord_t max, coord_t binsize);

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

		//TODO: put in functions to dump the data to the harddrive, or to read it off

	private:
		inline static coord_t _max, _binsize, _canopy;
		SparseHistogram _hist;
		coord_t _canopySum = 0.;
		int _count = 0;
		int _canopyCount = 0;

		void _quantileCanopy(Raster<metric_t>& r, cell_t cell, metric_t q);

		metric_t _estimatePointValue(size_t binNumber, int ordinal);
	};

	using MetricFunc = void(PointMetricCalculator::*)(Raster<metric_t>& r, cell_t cell);
	struct PointMetricDefn {
		std::string name;
		MetricFunc fun =  nullptr;
	};
	struct PointMetricRaster {
		Raster<metric_t> rast;
		PointMetricDefn metric;

		PointMetricRaster(const PointMetricDefn& m, const Alignment& a) {
			metric = m;
			rast = Raster<metric_t>(a);
		}
	};
}

#endif