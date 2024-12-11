#pragma once
#ifndef LP_GISEXCEPTIONS_H
#define LP_GISEXCEPTION_H

#include"gis_pch.hpp"

namespace lapis {

	class LapisGisException : public std::runtime_error {
	public:
		LapisGisException(const std::string& s = "") : std::runtime_error(s) {}
	};

	class CRSMismatchException : public LapisGisException {
	public:
		CRSMismatchException(const std::string& s = "") : LapisGisException("CRS do not match: " + s) {}
	};
	class UnableToDeduceCRSException : public LapisGisException {
	public:
		UnableToDeduceCRSException(const std::string& s = "") : LapisGisException("Unable to deduce CRS: " + s) {}
	};
	class EmptyCRSException : public LapisGisException {
	public:
		EmptyCRSException(const std::string& s = "") : LapisGisException("EmptyCRSException: ") {}
	};

	class InvalidExtentException : public LapisGisException {
	public:
		InvalidExtentException(const std::string& s = "") : LapisGisException("InvalidExtentException: " + s) {}
	};
	class OutsideExtentException : public LapisGisException {
	public:
		OutsideExtentException(const std::string& s = "") : LapisGisException("OutsideExtentException: " + s) {}
	};

	class InvalidRasterFileException : public LapisGisException {
	public:
		InvalidRasterFileException(const std::string& s = "") : LapisGisException("Unable to open raster file: " + s) {}
	};

	class InvalidAlignmentException : public LapisGisException {
	public:
		InvalidAlignmentException(const std::string& s = "") : LapisGisException("InvalidAlignmentException: " + s) {}
	};
	class AlignmentMismatchException : public LapisGisException {
	public:
		AlignmentMismatchException(const std::string& s = "") : LapisGisException("AlignmentMismatchException: " + s) {}
	};

	class InvalidVectorFileException : public LapisGisException {
	public:
		InvalidVectorFileException(const std::string& s) : LapisGisException("Unable to open vector file: " + s) {}
	};

	class InvalidLasFileException : public LapisGisException {
	public:
		InvalidLasFileException(const std::string& s) : LapisGisException("Unable to open LAS file: " + s) {}
	};

	class UnitMismatchException : public LapisGisException {
	public:
		UnitMismatchException(const std::string& s) : LapisGisException("UnitMismatchException: " + s) {}
	};
	class ImproperUnitsException : public LapisGisException {
	public:
		ImproperUnitsException(const std::string& s = "") : LapisGisException("ImproperUnitsException: " + s) {}
	};
}

#endif