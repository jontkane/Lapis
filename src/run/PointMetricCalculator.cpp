#include"run_pch.hpp"
#include"PointMetricCalculator.hpp"

namespace lapis {

	void PointMetricCalculator::setInfo(coord_t canopyCutoff, coord_t max, coord_t binsize, const std::vector<coord_t>& strataBreaks)
	{
		_max = max;
		_binsize = binsize;
		_canopy = canopyCutoff;
		SparseHistogram::setNHists((size_t)std::ceil((_max - _canopy) / _binsize));
		_strataBreaks = strataBreaks;
	}

	void PointMetricCalculator::cleanUp()
	{
		_hist.cleanUp();

		_strataCounts = std::vector<int>();

		//zeroing these doesn't matter for normal runs but makes testing easier
		_canopySum = 0;
		_canopyCount = 0;
		_count = 0;
		_totalIntensity = 0;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::meanCanopy()
	{
		if (_canopyCount) {
			return (metric_t)_canopySum / (metric_t)_canopyCount;
		}
		else {
			return xtl::missing<metric_t>();
		}
	}

	xtl::xoptional<metric_t> PointMetricCalculator::stdDevCanopy()
	{
		if (_canopyCount < 2) {
			return xtl::missing<metric_t>();
		}
		metric_t mean = (metric_t)_canopySum / (metric_t)_canopyCount;

		metric_t sd = 0;

		//right now this assumes all points fall at the midpoint of the two bins
		for (int i = 0; i < _hist.size(); ++i) {
			metric_t tmp = (metric_t)(_canopy + _binsize * i + (_binsize / 2));
			tmp -= mean;
			tmp *= tmp;
			tmp *= _hist.countInBin(i);
			sd += tmp;
		}
		sd /= _canopyCount;
		sd = std::sqrt(sd);
		return sd;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p25Canopy()
	{
		return _quantileCanopy(0.25f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p50Canopy()
	{
		return _quantileCanopy(0.5f);
	}
	
	xtl::xoptional<metric_t> PointMetricCalculator::p75Canopy()
	{
		return _quantileCanopy(0.75f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p95Canopy()
	{
		return _quantileCanopy(0.95f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::returnCount()
	{
		if (_count > 0) {
			return (metric_t)_count;
		} else {
			return xtl::missing<metric_t>();
		}
	}

	xtl::xoptional<metric_t> PointMetricCalculator::canopyCover()
	{
		if (_count > 0) {
			return (metric_t)_canopyCount / (metric_t)_count * 100.f;
		} else {
			return xtl::missing<metric_t>();
		}
	}

	xtl::xoptional<metric_t> PointMetricCalculator::coverAboveMean()
	{
		if (_canopyCount == 0) {
			return xtl::missing<metric_t>();

		}

		metric_t mean = (metric_t)(_canopySum / (metric_t)_canopyCount);
		int binWithMean = (int)((mean - _canopy) / _binsize);
		metric_t countAbove = 0;

		//assumes all points are at the center of their bins
		if (_canopy + _binsize * binWithMean + (_binsize / 2) > mean) {
			countAbove += _hist.countInBin(binWithMean);
		}
		for (int i = binWithMean+1; i < _hist.size(); ++i) {
			countAbove += _hist.countInBin(i);
		}
		return countAbove / _count * 100.f;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::canopyReliefRatio()
	{
		if (!_canopyCount) {
			return xtl::missing<metric_t>();
		}
		metric_t mean = (metric_t)_canopySum / (metric_t)_canopyCount;
		metric_t min = (metric_t)_canopy;
		//this loop could be eliminated by keeping track of the max as points are added
		metric_t max = 0;
		size_t highestBin = 0;
		for (size_t i = 0; i < _hist.size(); ++i) {
			if (_hist.countInBin(i)) {
				highestBin = i;
			}
		}
		max = _estimatePointValue(highestBin, _hist.countInBin(highestBin));

		return (mean - min) / (max - min);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::skewnessCanopy()
	{
		if (_canopyCount < 2) {
			return xtl::missing<metric_t>();
		}

		metric_t mean = (metric_t)_canopySum / (metric_t)_canopyCount;
		metric_t denominator = 0;
		metric_t numerator = 0;

		for (size_t i = 0; i < _hist.size(); ++i) {
			//this assumes all points are at the center of their bins
			metric_t diffFromMean = (metric_t)(_canopy + _binsize * i + (_binsize / 2) - mean);
			metric_t tmp = diffFromMean * diffFromMean * _hist.countInBin(i);
			denominator += tmp;
			numerator += tmp * diffFromMean;
		}
		denominator /= _canopyCount;
		denominator = std::sqrt(denominator); //this is now the std dev
		denominator *= (denominator * denominator);
		denominator *= _canopyCount - 1ll;
		return numerator / denominator;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::kurtosisCanopy()
	{
		if (_canopyCount < 2) {
			return xtl::missing<metric_t>();
		}

		metric_t mean = (metric_t)_canopySum / (metric_t)_canopyCount;
		metric_t denominator = 0;
		metric_t numerator = 0;

		for (size_t i = 0; i < _hist.size(); ++i) {
			//this assumes all points are at the center of their bins
			metric_t diffFromMean = (metric_t)(_canopy + _binsize * i + (_binsize / 2) - mean);
			metric_t tmp = diffFromMean * diffFromMean * _hist.countInBin(i);
			denominator += tmp;
			numerator += tmp * diffFromMean * diffFromMean;
		}
		denominator /= _canopyCount; //this is now the square of the std dev
		denominator *= denominator;
		denominator *= _canopyCount;
		return numerator / denominator;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::_quantileCanopy(metric_t q)
	{
		//the exact threshold is arguable, but quantiles are meaningless at low point counts
		//and the math gets a lot more annoying if you need to account for the case where the very first point might be the quantile
		if (_canopyCount < 4) { 
			return xtl::missing<metric_t>();
		}

		coord_t previousvalue = std::numeric_limits<coord_t>::lowest(); //the estimated value of the last valid point, in case the quantile point is the first in its bin
		metric_t needed = (_canopyCount - 1) * q; //the quantile is the value that exceeds exactly this many points (slightly shifted when needed isn't an integer)
		size_t binIdx = -1;
		while (true) {
			binIdx++;
			int fudgedBin = _hist.countInBin(binIdx);
			//the very first element doesn't "count" when calculating quantiles
			if (previousvalue < _canopy && fudgedBin > 0) {
				previousvalue = _estimatePointValue(binIdx, 1);
				fudgedBin = fudgedBin - 1;
			}
			needed -= fudgedBin;
			if (needed <= 0) {
				break;
			}
			if (fudgedBin>0) {
				previousvalue = _estimatePointValue(binIdx, _hist.countInBin(binIdx));
			}
			
		}

		needed = std::abs(needed); //needed now contains the degree by which we've gone too far by jumping to the end of the bin
		if (needed > 1) {
			previousvalue = _estimatePointValue(binIdx, (int)needed);
		}
		metric_t followingvalue = _estimatePointValue(binIdx, (int)(needed + 1));
		needed = std::fmod(needed, 1.f);
		metric_t quantile = (metric_t)(followingvalue - needed * (followingvalue - previousvalue));

		return quantile;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::stratumCover(size_t stratumIdx) {

		if (!_strataCounts.size()) {
			return xtl::missing<metric_t>();
		}
		metric_t numerator = 0; metric_t denominator = 0;
		for (size_t i = 0; i <= stratumIdx; ++i) {
			denominator += _strataCounts[i];
		}
		numerator = (metric_t)_strataCounts[stratumIdx];
		if (denominator > 0) {
			return numerator / denominator * 100.f;
		}
		return xtl::missing<metric_t>();
	}

	xtl::xoptional<metric_t> PointMetricCalculator::stratumPercent(size_t stratumIdx) {

		if (!_strataCounts.size()) {
			return xtl::missing<metric_t>();
		}

		metric_t numerator = (metric_t)_strataCounts[stratumIdx];
		metric_t denominator = (metric_t)(_count);
		if (denominator > 0) {
			return numerator / denominator * 100.f;
		}
		return xtl::missing<metric_t>();
	}

	//naming the parameter ordinal to emphasize that this math is 1-indexed; this is desirable throughout this algorithm,
	//but don't get tripped up trying to correct imaginary oboes
	metric_t PointMetricCalculator::_estimatePointValue(size_t binNumber, int ordinal)
	{
		//estimating that the sequence formed by the bin min, the observations, and the bin max is uniform
		metric_t binmin = (metric_t)(_canopy + _binsize * binNumber);
		metric_t step = (metric_t)(_binsize / (metric_t)(_hist.countInBin(binNumber) + 1.f));
		return binmin + step * ordinal;
	}

	void SparseHistogram::setNHists(size_t nHists)
	{
		_nHists = nHists;
	}

	int SparseHistogram::countInBin(size_t bin) const
	{
		if (!_data.size()) {
			return 0;
		}

		auto& thisHist = _data[bin / binsPerHist];
		if (thisHist) {
			size_t binInHist = bin % binsPerHist;
			return (*thisHist)[binInHist];
		}
		return 0;
	}

	void SparseHistogram::cleanUp()
	{
		_data = _storage();
		_sizeWithData = 0;
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p05Canopy()
	{
		return _quantileCanopy(0.05f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p10Canopy()
	{
		return _quantileCanopy(0.1f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p15Canopy()
	{
		return _quantileCanopy(0.15f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p20Canopy()
	{
		return _quantileCanopy(0.2f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p30Canopy()
	{
		return _quantileCanopy(0.3f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p35Canopy()
	{
		return _quantileCanopy(0.35f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p40Canopy()
	{
		return _quantileCanopy(0.4f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p45Canopy()
	{
		return _quantileCanopy(0.45f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p55Canopy()
	{
		return _quantileCanopy(0.55f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p60Canopy()
	{
		return _quantileCanopy(0.6f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p65Canopy()
	{
		return _quantileCanopy(0.65f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p70Canopy()
	{
		return _quantileCanopy(0.7f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p80Canopy()
	{
		return _quantileCanopy(0.8f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p85Canopy()
	{
		return _quantileCanopy(0.85f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p90Canopy()
	{
		return _quantileCanopy(0.9f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::p99Canopy()
	{
		return _quantileCanopy(0.99f);
	}

	xtl::xoptional<metric_t> PointMetricCalculator::meanIntensity()
	{
		if (_count) {
			return (metric_t)_totalIntensity / (metric_t)_count;
		}
		else {
			return xtl::missing<metric_t>();
		}
	}

}