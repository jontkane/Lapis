#pragma once
#ifndef lapis_gdalwrappers_h
#define lapis_gdalwrappers_h

#include"BaseDefs.hpp"
#include"LasIO.hpp"

//this file includes a number of basic wrapper classes around GDAL classes that ensure that they get properly destructed
//objects with complicated wrappers should be spun off into their own file--this is just for the simplest of wrappers that contain nothing but a constructor and destructor

namespace lapis {

	class GDALDatasetWrapper { //to ensure GDALClose always gets called on your GDALDataset
	public:
		GDALDatasetWrapper() = default;

		//creates according to GDALOpen()
		GDALDatasetWrapper(const std::string& filename, unsigned int openFlags);
		//creates according to GDALDrive::Create()
		GDALDatasetWrapper(const std::string& driver, const std::string& file, int ncol, int nrow, GDALDataType gdt);
		GDALDataset* operator->() {
			return gd.get();
		}
		bool isNull() const {
			return gd == nullptr;
		} //equivalent to checking if the unwrapper pointer is nullptr

	private:
		std::shared_ptr<GDALDataset> gd;
	};

	GDALDatasetWrapper rasterGDALWrapper(const std::string& filename);
	GDALDatasetWrapper vectorGDALWrapper(const std::string& filename);

	class GDALStringWrapper { //for use with OGRSpatialReference's export functions
	public:
		GDALStringWrapper() : ptr(nullptr) {}
		~GDALStringWrapper() {
			CPLFree(ptr);
		}
		char** operator&() {
			return &ptr;
		}
		char* ptr;

		//there's no protections against multiple destruction so just delete the copy constructors
		GDALStringWrapper(const GDALStringWrapper& wgd) = delete;
		GDALStringWrapper& operator=(const GDALStringWrapper& wgd) = delete;
	};

	class GDALPrjWrapper { //for calling CSLLoad in a memory-safe way
	public:
		GDALPrjWrapper() : ptr(nullptr) {}
		GDALPrjWrapper(const std::string& filename) {
			ptr = CSLLoad(filename.c_str());
		}
		~GDALPrjWrapper() {
			CSLDestroy(ptr);
		}
		char** ptr;

		//there's no protections against multiple destruction so just delete the copy constructors
		GDALPrjWrapper(const GDALPrjWrapper& wgd) = delete;
		GDALPrjWrapper& operator=(const GDALPrjWrapper& wgd) = delete;
	};

	//pass this to CPLSetErrorHandler to make GDAL shut up
#pragma warning(push)
#pragma warning(disable : 4100)
	inline void silenceGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {}
#pragma warning(pop)
	
}

#endif