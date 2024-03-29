#include"gis_pch.hpp"
#include"LasReader.hpp"

namespace lapis {
	LasReader::LasReader(const std::string& file) : _filters(), CurrentLasPoint(file), _filename(file) {}


	LasReader::LasReader(const Extent& e)
	{
		_xmin = e.xmin();
		_xmax = e.xmax();
		_ymin = e.ymin();
		_ymax = e.ymax();
		_crs = e.crs();
	}

	void LasReader::addFilter(const std::shared_ptr<LasFilter>& filter) {
		if (filter) {
			_filters.push_back(filter);
			std::sort(_filters.begin(), _filters.end(), [](const auto& a, const auto& b)->bool {return a.get()->priority > b.get()->priority; });
		}
	}

	LidarPointVector LasReader::getPoints(size_t n)
	{
		if (_currentPoint >= _nPoints) {
			return LidarPointVector();
		}
		size_t maxCount = std::min(n, _nPoints - _currentPoint);
		LidarPointVector points{ _crs };
		points.reserve(maxCount);

		while(_currentPoint < _nPoints && points.size() < n) {
			advance();
			bool filtered = false;

			for (auto& filter : _filters) {
				if (filter->isFiltered(*this)) {
					filtered = true;
					break;
				}
			}

			//These lines should do nothing if the las file is well-constructed.
			//However, if it isn't well constructed, a point off in the middle of nowhere could cause a crash if you don't check for it
			if (x() < xmin()) {
				continue;
			}
			if (x() > xmax()) {
				continue;
			}
			if (y() < ymin()) {
				continue;
			}
			if (y() > ymax()) {
				continue;
			}

			if (!filtered) {
				points.emplace_back(x(), y(), z(), intensity(), returnNumber());
			}
		}
	
		return points;
	}

	const std::string& LasReader::filename()
	{
		return _filename;
	}
}