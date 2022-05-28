#include"gis_pch.hpp"
#include"GDALWrappers.hpp"


namespace lapis {
	//creates according to GDALOpen()

	GDALDatasetWrapper::GDALDatasetWrapper(const std::string& filename, unsigned int openFlags) {
		GDALAllRegister();
		gd = std::shared_ptr<GDALDataset>((GDALDataset*)GDALOpenEx(filename.c_str(), openFlags, nullptr, nullptr, nullptr),
			[](auto x) {GDALClose(x); });
	}

	//creates according to GDALDrive::Create()

	GDALDatasetWrapper::GDALDatasetWrapper(const std::string& driver, const std::string& file, int ncol, int nrow, GDALDataType gdt) {
		GDALAllRegister();
		GDALDriver* d = GetGDALDriverManager()->GetDriverByName(driver.c_str());
		char** options = nullptr;
		gd = std::shared_ptr<GDALDataset>(d->Create(file.c_str(), ncol, nrow, 1, gdt, options),
			[](auto x) {GDALClose(x); });
	}

	GDALDatasetWrapper rasterGDALWrapper(const std::string& filename) {
		return GDALDatasetWrapper(filename, GDAL_OF_RASTER);
	}

	GDALDatasetWrapper vectorGDALWrapper(const std::string& filename) {
		return GDALDatasetWrapper(filename, GDAL_OF_VECTOR);
	}


}