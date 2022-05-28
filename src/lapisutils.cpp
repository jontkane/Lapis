#include"lapis_pch.hpp"
#include"LapisUtils.hpp"

namespace fs = std::filesystem;

namespace lapis {
	void tryLasFile(const fs::path& file, fileToLasExtent& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
		if (!file.has_extension()) {
			return;
		}
		if (file.extension() != ".laz" && file.extension() != ".las") {
			return;
		}
		try {
			data.emplace(file.string(), file.string());
			if (!crsOverride.isEmpty()) {
				data[file.string()].defineCRS(crsOverride);
			}
			if (!zUnitOverride.isUnknown()) {
				data[file.string()].setZUnits(zUnitOverride);
			}
		}
		catch (InvalidLasFileException e) {
			log.logError(e.what());
		}
		catch (InvalidExtentException e) {
			log.logError(e.what());
		}
	}

	void tryDtmFile(const fs::path& file, fileToAlignment& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
		if (file.extension() == ".aux" || file.extension() == ".ovr") { //pyramid files are openable by GDAL but not really rasters
			return;
		}
		try {
			data.emplace(file.string(), file.string());
			if (!crsOverride.isEmpty()) {
				data[file.string()].defineCRS(crsOverride);
			}
			if (!zUnitOverride.isUnknown()) {
				data[file.string()].setZUnits(zUnitOverride);
			}
		}
		catch (InvalidRasterFileException e) {
			log.logError(e.what());
		}
		catch (InvalidAlignmentException e) {
			log.logError(e.what());
		}
		catch (InvalidExtentException e) {
			log.logError(e.what());
		}
	}

	template<class T>
	void iterateOverInput(const std::vector<std::string>& specifiers, openFuncType<T> openFunc, 
		std::unordered_map<std::string, T>& fileMap, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride)
	{
		std::queue<std::string> toCheck;

		for (const std::string& spec : specifiers) {
			toCheck.push(spec);
		}


		while (toCheck.size()) {
			fs::path specPath{ toCheck.front() };
			toCheck.pop();

			//specified directories get searched recursively
			if (fs::is_directory(specPath)) {
				for (auto& subpath : fs::directory_iterator(specPath)) {
					toCheck.push(subpath.path().string());
				}
			}

			if (fs::is_regular_file(specPath)) {

				//if it's a file, try to add it to the map
				(*openFunc)(specPath, fileMap, log, crsOverride, zUnitOverride);
			}

			//wildcard specifiers (e.g. C:\data\*.laz) are basically a non-recursive directory check with an extension
			if (specPath.has_filename()) {
				if (fs::is_directory(specPath.parent_path())) {
					std::regex wildcard{ "^\\*\\..+" };
					std::string ext = "";
					if (std::regex_match(specPath.filename().string(), wildcard)) {
						ext = specPath.extension().string();
					}

					if (ext.size()) {
						for (auto& subpath : fs::directory_iterator(specPath.parent_path())) {
							if (subpath.path().has_extension() && subpath.path().extension() == ext) {
								toCheck.push(subpath.path().string());
							}
						}
					}

				}
			}
		}
	}

	template void iterateOverInput<LasExtent>(const std::vector<std::string>& specifiers, openFuncType<LasExtent> openFunc,
		std::unordered_map<std::string, LasExtent>& fileMap, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
	template void iterateOverInput<Alignment>(const std::vector<std::string>& specifiers, openFuncType<Alignment> openFunc,
		std::unordered_map<std::string, Alignment>& fileMap, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);


	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}

	bool extentSorter(const Extent& lhs, const Extent& rhs) {
		if (lhs.ymax() != rhs.ymax()) {
			return lhs.ymax() < rhs.ymax();
		}
		if (lhs.xmax() != rhs.xmax()) {
			return lhs.xmax() < rhs.xmax();
		}
		if (lhs.ymin() != rhs.ymin()) {
			return lhs.ymin() < rhs.ymin();
		}
		return lhs.xmin() < rhs.ymin();
	}
	Raster<int> getNLazRaster(const Alignment& a, const fileToLasExtent& exts)
	{
		Raster<int> nLaz = Raster<int>{ a };
		for (auto& file : exts) {
			QuadExtent q{ file.second,a.crs() };
			//this is an overestimate of the number of cells, but the vast majority of the time, it should be a very small one, very unlikely to actually cross any cell borders
			std::vector<cell_t> cells = a.cellsFromExtent(q.outerExtent(), SnapType::out);
			for (cell_t cell : cells) {
				nLaz[cell].value()++;
				nLaz[cell].has_value() = true;
			}
		}
		return nLaz;
	}
	std::string insertZeroes(int value, int maxvalue)
	{
		int nDigits = (int)(std::log10(maxvalue) + 1);
		int thisDigitCount = std::max(1, (int)(std::log10(value) + 1));
		return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
	}
}