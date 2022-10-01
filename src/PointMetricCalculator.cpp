#include"app_pch.hpp"
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

	void PointMetricCalculator::addPoint(const LasPoint& lp)
	{
		const coord_t& z = lp.z;
		++_count;
		if (z >= _canopy) {
			_canopySum += z;
			++_canopyCount;
			int bin = (int)((z - _canopy) / _binsize);

			_hist.incrementBin(bin);
		}

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

	void PointMetricCalculator::cleanUp()
	{
		_hist.cleanUp();

		_strataCounts.clear();
		_strataCounts.shrink_to_fit();

		//zeroing these doesn't matter for normal runs but makes testing easier
		_canopySum = 0;
		_canopyCount = 0;
		_count = 0;
	}

	void PointMetricCalculator::meanCanopy(Raster<metric_t>& r, cell_t cell)
	{
		if (_canopyCount) {
			r[cell].has_value() = true;
			r[cell].value() = _canopySum / (metric_t)_canopyCount;
		}
		else {
			r[cell].has_value() = false;
		}
	}

	void PointMetricCalculator::stdDevCanopy(Raster<metric_t>& r, cell_t cell)
	{
		if (_canopyCount < 2) {
			r[cell].has_value() = false;
			return;
		}
		metric_t mean = _canopySum / (metric_t)_canopyCount;

		metric_t sd = 0;

		//right now this assumes all points fall at the midpoint of the two bins
		for (int i = 0; i < _hist.size(); ++i) {
			metric_t tmp = _canopy + _binsize * i + (_binsize / 2);
			tmp -= mean;
			tmp *= tmp;
			tmp *= _hist.countInBin(i);
			sd += tmp;
		}
		sd /= _canopyCount;
		sd = std::sqrt(sd);
		r[cell].has_value() = true;
		r[cell].value() = sd;
	}

	void PointMetricCalculator::p25Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.25);
	}

	void PointMetricCalculator::p50Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.5);
	}

	void PointMetricCalculator::p75Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.75);
	}

	void PointMetricCalculator::p95Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.95);
	}

	void PointMetricCalculator::returnCount(Raster<metric_t>& r, cell_t cell)
	{
		r[cell].has_value() = (_count > 0);
		r[cell].value() = _count;
	}

	void PointMetricCalculator::canopyCover(Raster<metric_t>& r, cell_t cell)
	{
		r[cell].has_value() = (_count > 0);
		r[cell].value() = (metric_t)_canopyCount / (metric_t)_count * 100.;
	}

	void PointMetricCalculator::coverAboveMean(Raster<metric_t>& r, cell_t cell)
	{
		if (_canopyCount == 0) {
			r[cell].has_value() = false;
			return;
		}
		r[cell].has_value() = true;

		metric_t mean = _canopySum / (metric_t)_canopyCount;
		int binWithMean = (int)((mean - _canopy) / _binsize);
		metric_t countAbove = 0;
		//assumes all points are at the center of their bins
		for (int i = 0; i < _hist.size(); ++i) {
			if ((_canopy + _binsize * i + (_binsize / 2)) > mean) {
				countAbove += _hist.countInBin(i);
			}
		}
		r[cell].value() = countAbove / _canopyCount * 100.;
	}

	void PointMetricCalculator::canopyReliefRatio(Raster<metric_t>& r, cell_t cell)
	{
		if (!_canopyCount) {
			r[cell].has_value() = false;
			return;
		}
		r[cell].has_value() = true;
		metric_t mean = _canopySum / (metric_t)_canopyCount;
		metric_t min = _canopy;
		//this loop could be eliminated by keeping track of the max as points are added
		metric_t max = 0;
		size_t highestBin = 0;
		for (size_t i = 0; i < _hist.size(); ++i) {
			if (_hist.countInBin(i)) {
				highestBin = i;
			}
		}
		max = _estimatePointValue(highestBin, _hist.countInBin(highestBin));

		r[cell].value() = (mean - min) / (max - min);
	}

	void PointMetricCalculator::skewnessCanopy(Raster<metric_t>& r, cell_t cell)
	{
		if (_canopyCount < 2) {
			r[cell].has_value() = false;
			return;
		}
		r[cell].has_value() = true;

		metric_t mean = _canopySum / (metric_t)_canopyCount;
		metric_t denominator = 0;
		metric_t numerator = 0;

		for (size_t i = 0; i < _hist.size(); ++i) {
			//this assumes all points are at the center of their bins
			metric_t diffFromMean = _canopy + _binsize * i + (_binsize / 2) - mean;
			metric_t tmp = diffFromMean * diffFromMean * _hist.countInBin(i);
			denominator += tmp;
			numerator += tmp * diffFromMean;
		}
		denominator /= _canopyCount;
		denominator = std::sqrt(denominator); //this is now the std dev
		denominator *= (denominator * denominator);
		denominator *= _canopyCount - 1;
		r[cell].value() = numerator / denominator;
	}

	void PointMetricCalculator::kurtosisCanopy(Raster<metric_t>& r, cell_t cell)
	{
		if (_canopyCount < 2) {
			r[cell].has_value() = false;
			return;
		}
		r[cell].has_value() = true;

		metric_t mean = _canopySum / (metric_t)_canopyCount;
		metric_t denominator = 0;
		metric_t numerator = 0;

		for (size_t i = 0; i < _hist.size(); ++i) {
			//this assumes all points are at the center of their bins
			metric_t diffFromMean = _canopy + _binsize * i + (_binsize / 2) - mean;
			metric_t tmp = diffFromMean * diffFromMean * _hist.countInBin(i);
			denominator += tmp;
			numerator += tmp * diffFromMean * diffFromMean;
		}
		denominator /= _canopyCount; //this is now the square of the std dev
		denominator *= denominator;
		denominator *= _canopyCount - 1;
		r[cell].value() = numerator / denominator;
	}

	void PointMetricCalculator::_quantileCanopy(Raster<metric_t>& r, cell_t cell, metric_t q)
	{
		//the exact threshold is arguable, but quantiles are meaningless at low point counts
		//and the math gets a lot more annoying if you need to account for the case where the very first point might be the quantile
		if (_canopyCount < 4) { 
			r[cell].has_value() = false;
			return;
		}

		coord_t previousvalue = std::numeric_limits<coord_t>::lowest(); //the estimated value of the last valid point, in case the quantile point is the first in its bin
		metric_t needed = (_canopyCount-1) * q; //the quantile is the value that exceeds exactly this many points (slightly shifted when needed isn't an integer)
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
		needed = std::fmod(needed, 1);
		metric_t quantile = followingvalue - needed * (followingvalue - previousvalue);

		r[cell].has_value() = true;
		r[cell].value() = quantile;
	}

	void PointMetricCalculator::stratumCover(Raster<metric_t>& r, cell_t cell, size_t stratumIdx) {

		if (!_strataCounts.size()) {
			r[cell].has_value() = false;
			return;
		}
		metric_t numerator = 0; metric_t denominator = 0;
		for (size_t i = 0; i <= stratumIdx; ++i) {
			denominator += _strataCounts[i];
		}
		numerator = _strataCounts[stratumIdx];
		if (denominator > 0) {
			r[cell].has_value() = true;
			r[cell].value() = numerator / denominator * 100.;
		}
	}

	void PointMetricCalculator::stratumPercent(Raster<metric_t>& r, cell_t cell, size_t stratumIdx) {

		if (!_strataCounts.size()) {
			r[cell].has_value() = false;
			return;
		}

		metric_t numerator = _strataCounts[stratumIdx];
		metric_t denominator = _count;
		if (denominator > 0) {
			r[cell].has_value() = true;
			r[cell].value() = numerator / denominator * 100.;
		}
		
	}

	//naming the parameter ordinal to emphasize that this math is 1-indexed; this is desirable throughout this algorithm,
	//but don't get tripped up trying to correct imaginary oboes
	metric_t PointMetricCalculator::_estimatePointValue(size_t binNumber, int ordinal)
	{
		//estimating that the sequence formed by the bin min, the observations, and the bin max is uniform
		metric_t binmin = _canopy + _binsize * binNumber;
		metric_t step = _binsize / (_hist.countInBin(binNumber) + 1);
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

	void SparseHistogram::incrementBin(size_t bin)
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
	}

	size_t SparseHistogram::size()
	{
		return binsPerHist * _nHists;
	}

	void SparseHistogram::cleanUp()
	{
		_data.clear();
		_data.shrink_to_fit();
	}

	void PointMetricCalculator::p05Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.05);
	}

	void PointMetricCalculator::p10Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.1);
	}

	void PointMetricCalculator::p15Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.15);
	}

	void PointMetricCalculator::p20Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.2);
	}

	void PointMetricCalculator::p30Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.3);
	}

	void PointMetricCalculator::p35Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.35);
	}

	void PointMetricCalculator::p40Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.4);
	}

	void PointMetricCalculator::p45Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.45);
	}

	void PointMetricCalculator::p55Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.55);
	}

	void PointMetricCalculator::p60Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.6);
	}

	void PointMetricCalculator::p65Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.65);
	}

	void PointMetricCalculator::p70Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.7);
	}

	void PointMetricCalculator::p80Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.8);
	}

	void PointMetricCalculator::p85Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.85);
	}

	void PointMetricCalculator::p90Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.9);
	}

	void PointMetricCalculator::p99Canopy(Raster<metric_t>& r, cell_t cell)
	{
		_quantileCanopy(r, cell, 0.99);
	}

}