#include"gis_pch.hpp"
#include"GDALWrappers.hpp"
#include"CoordRef.hpp"


namespace lapis {
	void gdalAllRegisterThreadSafe()
	{
		static std::mutex mut;
		static bool registered = false;
		std::scoped_lock<std::mutex> lock{ mut };
		if (!registered) {
			registered = true;
			GDALAllRegister();
		}
	}
	UniqueGdalDataset makeUniqueGdalDataset(GDALDataset* p)
	{
		return UniqueGdalDataset(p,
			[](GDALDataset* x) {
				if (x) {
					GDALClose(x);
				}
			}
		);
	}
	UniqueGdalDataset gdalCreateWrapper(const std::string& driver, const std::string& file, int ncol, int nrow, GDALDataType gdt)
	{
		gdalAllRegisterThreadSafe();
		GDALDriver* d = GetGDALDriverManager()->GetDriverByName(driver.c_str());
		return makeUniqueGdalDataset(d->Create(file.c_str(), ncol, nrow, 1, gdt, nullptr));
	}
	UniqueGdalDataset rasterGDALWrapper(const std::string& filename)
	{
		gdalAllRegisterThreadSafe();
		return makeUniqueGdalDataset((GDALDataset*)GDALOpenEx(filename.c_str(), GDAL_OF_RASTER,
			nullptr, nullptr, nullptr));
	}
	UniqueGdalDataset vectorGDALWrapper(const std::string& filename)
	{
		gdalAllRegisterThreadSafe();
		return makeUniqueGdalDataset((GDALDataset*)GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR,
			nullptr, nullptr, nullptr));
	}
	UniqueCslString cslLoadWrapper(const std::string& filename)
	{
		return UniqueCslString(CSLLoad(filename.c_str()),
			[](char** x) {
				CSLDestroy(x);
			}
		);
	}
	UniqueGdalString exportToWktWrapper(const OGRSpatialReference& osr)
	{
		char* out = nullptr;
		osr.exportToWkt(&out);
		return UniqueGdalString(out,
			[](char* x) {
				if (x) {
					CPLFree(x);
				}
			}
		);
	}
	UniqueOGRFeature createFeatureWrapper(OGRLayer* layer)
	{
		return UniqueOGRFeature(
			OGRFeature::CreateFeature(layer->GetLayerDefn()),
			[](OGRFeature* x) {
				if (x) {
					OGRFeature::DestroyFeature(x);
				}
			}
		);
	}
}