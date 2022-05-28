#pragma once
#ifndef lp_lapisutils_h
#define lp_lapisutils_h

#include"lapis_pch.hpp"
#include"Logger.hpp"
#include"gis/LasExtent.hpp"
#include"gis/Raster.hpp"

namespace lapis {

	using fileToLasExtent = std::unordered_map<std::string, LasExtent>;
	using fileToAlignment = std::unordered_map<std::string, Alignment>;

	//attempts to open the given file as a las/laz file. If it succeeds, adds the given file to lasLocs
	//otherwise, logs an error and does nothing
	void tryLasFile(const std::filesystem::path& file, fileToLasExtent& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);

	//attempts to open the given file as a raster file. If it succeeds, adds the given file to dtmLocs
	//otherwise, logs an error and does nothing
	void tryDtmFile(const std::filesystem::path& file, fileToAlignment& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);

	//processes the input style the user might give for files, and adds them to filename->extent maps
	template<class T>
	using openFuncType = void(const std::filesystem::path&, std::unordered_map<std::string,T>&, Logger&,
		const CoordRef&, const Unit&);
	template<class T>
	void iterateOverInput(const std::vector<std::string>& specifiers, openFuncType<T> openFunc, std::unordered_map<std::string, T>& fileMap, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);

	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread();

	//sorts extents by north to south, then east to west. If the LAS files are well-tiled, this will result in a pattern where
	//the exposed surface area of already-processed files is minimized, since that's a big source of memory usage
	bool extentSorter(const Extent& lhs, const Extent& rhs);

	//returns a raster whose values are the number of extents that overlap that cell
	Raster<int> getNLazRaster(const Alignment& a, const fileToLasExtent& exts);

	std::string insertZeroes(int value, int maxvalue);
}

#endif