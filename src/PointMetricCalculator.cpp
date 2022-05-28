#include"lapis_pch.hpp"
#include"PointMetricCalculator.hpp"

namespace lapis {

	void PointMetricCalculator::setInfo(coord_t canopyCutoff, coord_t max, coord_t binsize)
	{
		_max = max;
		_binsize = binsize;
		_canopy = canopyCutoff;
	}

	void PointMetricCalculator::addPoint(const LasPoint& lp)
	{
		if (!_hist.size()) {
			int nBins = (int)std::ceil((_max - _canopy) / _binsize);
			_hist.resize(nBins, 0);
		}
		const coord_t& z = lp.z;
		++_count;
		if (z < _canopy) {
			return;
		}

		_canopySum += z;
		++_canopyCount;
		int bin = (int)((z - _canopy) / _binsize);

		if (bin >= _hist.size()) { //this will happen if z==_max and (_max-_min)/_binsize is an integer
			bin = (int)(_hist.size() - 1);
		}
		++_hist[bin];
	}

	void PointMetricCalculator::cleanUp()
	{
		_hist.clear();
		_hist.shrink_to_fit();
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
		if (!_canopyCount) {
			r[cell].has_value() = false;
			return;
		}
		coord_t mean = _canopySum / (metric_t)_canopyCount;

		coord_t sd = 0;

		//right now this assumes all points fall at the midpoint of the two bins
		for (int i = 0; i < _hist.size(); ++i) {
			coord_t tmp = _canopy + _binsize * i + (_binsize / 2);
			tmp -= mean;
			tmp *= tmp;
			tmp *= _hist[i];
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
		r[cell].has_value() = true;
		r[cell].value() = _count;
	}

	void PointMetricCalculator::canopyCover(Raster<metric_t>& r, cell_t cell)
	{
		r[cell].has_value() = true;
		r[cell].value() = (metric_t)_canopyCount / (metric_t)_count * 100.;
	}

	void PointMetricCalculator::_quantileCanopy(Raster<metric_t>& r, cell_t cell, metric_t q)
	{
		//the exact threshold is arguable, but quantiles are meaningless at low point counts
		//and the math gets a lot more annoying if you need to account for the case with exactly one point
		if (_canopyCount < 4) { 
			r[cell].has_value() = false;
			return;
		}

		metric_t needed = q * _canopyCount;
		int prev = 0;
		int next = 0;
		int lastidxwithvalues = -1;
		for (int i = 0; i < _hist.size(); ++i) {
			next += _hist[i];
			if (next < needed) {
				if (_hist[i]) {
					lastidxwithvalues = i;
				}
				prev = next;
				continue;
			}

			if (next - needed < 1) { //the very first point in this bin crosses the threshold, so the true quantile should be less than this bin's lower bound
				//estimating this point at the bottom of the bin and prev point at the top of that bin
				metric_t thisbinmin = _canopy + i * _binsize;
				metric_t prevbinmax = _canopy + (lastidxwithvalues + 1) * _binsize;
				r[cell].has_value() = true;
				r[cell].value() = prevbinmax + (thisbinmin - prevbinmax) * (next - needed);
				return;
			}

			//this match assumes a uniform distribution of points within each bin
			metric_t perc = (needed - prev) / (next - prev);
			coord_t thisbinmin = _canopy + i * _binsize;
			metric_t quant = (1 - perc) * thisbinmin + perc * (thisbinmin + _binsize);
			r[cell].has_value() = true;
			r[cell].value() = quant;
			break;
		}
	}

}