#pragma once
#ifndef lapis_basedefs_h
#define lapis_basedefs_h

#include"gis_pch.hpp"

//this file includes a bunch of miscellaneous definitions used widely across the various GIS objects

namespace lapis {
	typedef double coord_t;
	typedef std::int64_t cell_t;
	typedef int rowcol_t;
	constexpr coord_t LAPIS_EPSILON = 0.0001;

	struct CoordXY {
		coord_t x, y;
		CoordXY() : x(0), y(0) {}
		CoordXY(coord_t x, coord_t y) : x(x), y(y) {}
	};
	struct CoordXYZ {
		coord_t x, y, z;
		CoordXYZ() : x(0), y(0), z(0) {}
		CoordXYZ(coord_t x, coord_t y, coord_t z) : x(x), y(y), z(z) {}
	};

	//All the various exception classes are stored in the one header I can guarantee will always be included

	class CRSMismatchException : public std::runtime_error {
	public:
		CRSMismatchException(std::string s = "") : std::runtime_error("CRS do not match " + s) {}
	};
	class UnableToDeduceCRSException : public std::runtime_error {
	public:
		UnableToDeduceCRSException(std::string s = "") : std::runtime_error("Unable to deduce CRS: " + s) {}
	};
	class EmptyCRSException : public std::runtime_error {
	public:
		EmptyCRSException(std::string s = "") : std::runtime_error("EmptyCRSException: ") {}
	};

	class InvalidExtentException : public std::runtime_error {
	public:
		InvalidExtentException(std::string s = "") : std::runtime_error("InvalidExtentException: " + s) {}
	};
	class OutsideExtentException : public std::runtime_error {
	public:
		OutsideExtentException(std::string s = "") : std::runtime_error("OutsideExtentException: " + s) {}
	};

	class InvalidRasterFileException : public std::runtime_error {
	public:
		InvalidRasterFileException(std::string s = "") : std::runtime_error("Unable to open raster file: " + s) {}
	};

	class InvalidAlignmentException : public std::runtime_error {
	public:
		InvalidAlignmentException(std::string s = "") : std::runtime_error("InvalidAlignmentException: " + s) {}
	};
	class AlignmentMismatchException : public std::runtime_error {
	public:
		AlignmentMismatchException(std::string s = "") : std::runtime_error("AlignmentMismatchException: " + s) {}
	};

	class InvalidVectorFileException : public std::runtime_error {
	public:
		InvalidVectorFileException(std::string s) : std::runtime_error("Unable to open vector file: " + s) {}
	};

	class InvalidLasFileException : public std::runtime_error {
	public:
		InvalidLasFileException(std::string s) : std::runtime_error("Unable to open LAS file: " + s) {}
	};

	class UnitMismatchException : public std::runtime_error {
	public:
		UnitMismatchException(std::string s) : std::runtime_error("UnitMismatchException: " + s) {}
	};
	class ImproperUnitsException : public std::runtime_error {
	public:
		ImproperUnitsException(std::string s = "") : std::runtime_error("ImproperUnitsException: " + s) {}
	};
}

#endif // !#spatial_sp_h