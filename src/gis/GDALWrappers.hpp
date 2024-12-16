#pragma once
#ifndef lapis_gdalwrappers_h
#define lapis_gdalwrappers_h

#include"..\LapisTypeDefs.hpp"
#include"LasIO.hpp"
#include"gis_pch.hpp"

//this file includes a number of basic wrapper classes around GDAL classes that ensure that they get properly destructed
//objects with complicated wrappers should be spun off into their own file--this is just for the simplest of wrappers that contain nothing but a constructor and destructor

namespace lapis {

	void gdalAllRegisterThreadSafe();

	using UniqueGdalDataset = std::unique_ptr<GDALDataset, void(*)(GDALDataset*)>;
	UniqueGdalDataset makeUniqueGdalDataset(GDALDataset* p);
	UniqueGdalDataset gdalCreateWrapper(const std::string& driver, const std::string& file, int ncol, int nrow, GDALDataType gdt);
	UniqueGdalDataset rasterGDALWrapper(const std::string& filename);
	UniqueGdalDataset vectorGDALWrapper(const std::string& filename);

	using UniqueCslString = std::unique_ptr<char*, void(*)(char**)>;
	UniqueCslString cslLoadWrapper(const std::string& filename);

	using UniqueGdalString = std::unique_ptr<char, void(*)(char*)>;
	UniqueGdalString exportToWktWrapper(const OGRSpatialReference& osr);

	using UniqueOGRFeature = std::unique_ptr<OGRFeature, void(*)(OGRFeature*)>;
	UniqueOGRFeature createFeatureWrapper(OGRLayer* layer);

	//pass this to CPLSetErrorHandler to make GDAL shut up
#pragma warning(push)
#pragma warning(disable : 4100)
	inline void silenceGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {}
#pragma warning(pop)
	
}

#endif