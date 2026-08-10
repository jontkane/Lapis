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

		inline void incrementBin(size_t bin)
		{
			if (!_data.size()) {
				_data.resize(_nHists);
			}

			if (bin >= _nHists * binsPerHist) {
				bin = _nHists * binsPerHist - 1;
			}
			auto& thisHist = _data[bin / binsPerHist];
			size_t binInHist = bin % binsPerHist;
			if (!thisHist) {
				thisHist = std::make_unique<std::array<int, binsPerHist>>();
				for (auto& v : *thisHist) {
					v = 0;
				}
			}
			(*thisHist)[binInHist]++;

			_sizeWithData = std::max(_sizeWithData, bin+1);
		}

		size_t size()
		{
			return _sizeWithData;
		}

		void cleanUp();

	private:
		inline static constexpr size_t binsPerHist = 100;
		using _storage = std::vector<std::unique_ptr<std::array<int, binsPerHist>>>;
		_storage _data;
		inline static size_t _nHists;
		size_t _sizeWithData = 0;
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
		inline void addPoint(const LasPoint& lp) {
			const coord_t& z = lp.z;
			++_count;
			if (z >= _canopy) {
				_canopySum += z;
				++_canopyCount;
				int bin = (int)((z - _canopy) / _binsize);

				_hist.incrementBin(bin);
			}

			_totalIntensity += lp.intensity;

			//because we're using return here as a control flow, the stratum logic has to go last even if we add more features to this function
			if (!_strataCounts.size()) {
				_strataCounts = std::vector<int>(_strataBreaks.size() + 1, 0);
			}
			for (size_t i = 0; i < _strataBreaks.size(); ++i) {
				if (lp.z < _strataBreaks[i]) {
					_strataCounts[i]++;
					return;
				}
			}
			_strataCounts[_strataBreaks.size()]++;
		}
		//this function will deallocate the histogram vector. Call it once you're done with the data here.
		void cleanUp();

		xtl::xoptional<metric_t> meanCanopy();
		xtl::xoptional<metric_t> stdDevCanopy();
		xtl::xoptional<metric_t> p25Canopy();
		xtl::xoptional<metric_t> p50Canopy();
		xtl::xoptional<metric_t> p75Canopy();
		xtl::xoptional<metric_t> p95Canopy();
		xtl::xoptional<metric_t> returnCount();
		xtl::xoptional<metric_t> canopyCover();

		//these are currently classed as "advanced" metrics. Grouped separately for clarity
		xtl::xoptional<metric_t> coverAboveMean();
		xtl::xoptional<metric_t> canopyReliefRatio();
		xtl::xoptional<metric_t> skewnessCanopy();
		xtl::xoptional<metric_t> kurtosisCanopy();
		xtl::xoptional<metric_t> p05Canopy();
		xtl::xoptional<metric_t> p10Canopy();
		xtl::xoptional<metric_t> p15Canopy();
		xtl::xoptional<metric_t> p20Canopy();
		xtl::xoptional<metric_t> p30Canopy();
		xtl::xoptional<metric_t> p35Canopy();
		xtl::xoptional<metric_t> p40Canopy();
		xtl::xoptional<metric_t> p45Canopy();
		xtl::xoptional<metric_t> p55Canopy();
		xtl::xoptional<metric_t> p60Canopy();
		xtl::xoptional<metric_t> p65Canopy();
		xtl::xoptional<metric_t> p70Canopy();
		xtl::xoptional<metric_t> p80Canopy();
		xtl::xoptional<metric_t> p85Canopy();
		xtl::xoptional<metric_t> p90Canopy();
		xtl::xoptional<metric_t> p99Canopy();
		xtl::xoptional<metric_t> meanIntensity();

		xtl::xoptional<metric_t> stratumCover(size_t stratumIdx);
		xtl::xoptional<metric_t> stratumPercent(size_t stratumIdx);

		//TODO: put in functions to dump the data to the harddrive, or to read it off

	private:
		inline static coord_t _max, _binsize, _canopy;
		inline static std::vector<coord_t> _strataBreaks;
		SparseHistogram _hist;
		coord_t _canopySum = 0.;
		int _count = 0;
		int _canopyCount = 0;
		std::vector<int> _strataCounts;

		intensity_t _totalIntensity = 0;

		xtl::xoptional<metric_t> _quantileCanopy(metric_t q);



		metric_t _estimatePointValue(size_t binNumber, int ordinal);
	};
}

#endif