#include"gis_pch.hpp"
#include"LasExtent.hpp"

namespace lapis {
	LasExtent::LasExtent(const std::string& s)
	{
		LasIO las{ s };
		_setFromLasIO(las);
		_nPoints = las.header.NumberOfPoints();
		_versionMinor = las.header.VersionMinor;
	}

	LasExtent::LasExtent(const LasIO& las)
	{
		_setFromLasIO(las);
		_nPoints = las.header.NumberOfPoints();
		_versionMinor = las.header.VersionMinor;
	}

}