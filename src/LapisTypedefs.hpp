#pragma once
#ifndef lp_lapistypedefs_h
#define lp_lapistypedefs_h

#include"app_pch.hpp"

namespace lapis {
	using metric_t = coord_t;
	using intensity_t = uint32_t;
	using csm_t = coord_t;
	using taoid_t = uint32_t; //int64 would be ideal but none of the common raster formats support it

	template<class T>
	using shared_raster = std::shared_ptr<Raster<T>>;

	enum class OutputUnitLabel {
		Default, Radian, Percent, Unitless
	};

	namespace UnitRadio {
		enum {
			UNKNOWN = 0,
			METERS = 1,
			INTFEET = 2,
			USFEET = 3
		};
	}

	const std::vector<std::string> unitPluralNames = {
			"Units",
			"Meters",
			"Feet",
			"Feet"
	};

	const std::vector<std::string> unitSingularNames = {
		"Unit",
		"Meter",
		"Foot",
		"Foot"
	};

	namespace IdAlgo {
		enum {
			
			OTHER,
			HIGHPOINT,
		};
	}
	namespace SegAlgo {
		enum {
			OTHER,
			WATERSHED,
		};
	}

	struct PointsAndDem {
		LidarPointVector points;
		Raster<coord_t> dem;
	};
}

#endif