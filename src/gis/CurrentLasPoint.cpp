#include"gis_pch.hpp"
#include"CurrentLasPoint.hpp"
#include"GisExceptions.hpp"

namespace lapis {
	CurrentLasPoint::CurrentLasPoint(const std::string& file)  {

		_las = LasIO(file);

		_buffer.resize((size_t)_las.header.PointLength + 256);

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