#include"gis_pch.hpp"
#include"GDALWrappers.hpp"


namespace lapis {
	//creates according to GDALOpen()

	GDALDatasetWrapper::GDALDatasetWrapper(const std::string& filename, unsigned int openFlags) {
		GDALRegisterWrapper::allRegister();
		gd = (GDALDataset*)GDALOpenEx(filename.c_str(), openFlags, nullptr, nullptr, nullptr);
	}

	//creates according to GDALDrive::Create()

	GDALDatasetWrapper::GDALDatasetWrapper(const std::string& driver, const std::string& file, int ncol, int nrow, GDALDataType gdt) {
		GDALRegisterWrapper::allRegister();
		GDALDriver* d = GetGDALDriverManager()->GetDriverByName(driver.c_str());
		char** options = nullptr;
		gd = d->Create(file.c_str(), ncol, nrow, 1, gdt, options);
	}

	GDALDatasetWrapper::~GDALDatasetWrapper() {
		GDALClose(gd);
	}

	GDALDatasetWrapper::GDALDatasetWrapper(GDALDatasetWrapper&& other) noexcept {
		*this = std::move(other);
	}

	GDALDatasetWrapper& GDALDatasetWrapper::operator=(GDALDatasetWrapper&& other) noexcept
	{
		auto temp = gd;
		gd = other.gd;
		other.gd = temp;
		_file = std::move(other._file);
		return *this;
	}

	GDALDatasetWrapper rasterGDALWrapper(const std::string& filename) {
		return GDALDatasetWrapper(filename, GDAL_OF_RASTER);
	}

	GDALDatasetWrapper vectorGDALWrapper(const std::string& filename) {
		return GDALDatasetWrapper(filename, GDAL_OF_VECTOR);
	}


}