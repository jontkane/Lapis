#include"gis_pch.hpp"
#include"CurrentLasPoint.hpp"

namespace lapis {
	CurrentLasPoint::CurrentLasPoint(const std::string& file)  {
		try {
			_las = LasIO(file);
		}
		catch (std::exception e) {
			throw InvalidLasFileException(file);
		}

		_setFromLasIO(_las);
		_nPoints = _las.header.NumberOfPoints();

		_ispoint14 = _las.header.PointFormat() >= 6;

		_currentPoint = 0;
		_updateXYZ();
	}

	size_t CurrentLasPoint::nPoints() const
	{
		return _nPoints;
	}

	size_t CurrentLasPoint::nPointsRemaining() const
	{
		return _nPoints - _currentPoint;
	}

}