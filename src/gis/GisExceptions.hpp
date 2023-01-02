#pragma once
#ifndef LP_GISEXCEPTIONS_H
#define LP_GISEXCEPTION_H

#include"gis_pch.hpp"

namespace lapis {
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

#endif