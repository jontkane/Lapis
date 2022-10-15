#pragma once
#ifndef lp_pointmetriccontroller_h
#define lp_pointmetriccontroller_h


#include"LapisLogger.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisUtils.hpp"
#include"csmalgos.hpp"
#include"LapisPrivate.hpp"
#include"LapisData.hpp"

namespace std {
	namespace filesystem {
		class path;
	}
}

namespace lapis {

	namespace fs = std::filesystem;
	
	class LapisController {
	public:

		LapisController();

		void processFullArea();

		bool isRunning() const;

	protected:
		//returns the filenames of the las/dtm files that overlap the given extent
		template<class T>
		T filesByExtent(const Extent& e, const T& files) const;

		//handles the logic to decide which las files should be processed by the thread that calls this function
		void pointMetricThread(size_t& soFar) const;

		//assigns points from a file to the correct cells of the output
		//returns the number of points that pass the filters and get used in later processing
		void assignPointsToCalculators(const LidarPointVector& points) const;

		void assignPointsToCSM(const LidarPointVector& points, std::optional<Raster<csm_t>>& csm) const;

		void assignPointsToFineIntensity(const LidarPointVector& points, Raster<intensity_t>& numerator, Raster<intensity_t>& denominator) const;

		//processes the points that have already been found and assigns the output to rasters
		//returns the number of points freed from memory
		void processPoints(const Extent& e) const;

		//runs metrics on an individual cell
		//returns the number of points processed
		void processCell(cell_t cell) const;

		fs::path getCSMTempDir() const;
		fs::path getCSMPermanentDir() const;
		fs::path getPointMetricDir() const;
		fs::path getFRPointMetricDir() const;
		fs::path getParameterDir() const;
		fs::path getTAODir() const;

		fs::path getTempTAODir() const;

		fs::path getLayoutDir() const;

		fs::path getFineIntDir() const;

		fs::path getFineIntTempDir() const;

		fs::path getCSMMetricDir() const;

		fs::path getStratumDir() const;

		fs::path getFRStratumDir() const;

		fs::path getTopoDir() const;

		void writeParams() const;

		//This is the function that performs the work of merging the temporary CSM files into their final tiles
		//layout is an alignment whose cells represent the extents of the final tiles.
		void csmProcessingThread(const Alignment& layout, cell_t& soFar, TaoIdMap& idMap) const;

		void calcCSMMetrics(const Raster<csm_t>& tile) const;

		//returns the points belonging to the nth las file in sortedlasextents
		//may throw an InvalidLasFileException if something goes wrong
		//it also updates elevNumerator and elevDenominator
		LidarPointVector getPointsAndDem(size_t n) const;

		void populateMap(const Raster<taoid_t> segments, const std::vector<cell_t>& highPoints, TaoIdMap& map, coord_t bufferDist, cell_t tileidx) const;

		void writeHighPoints(const std::vector<cell_t>& highPoints, const Raster<taoid_t>& segments,
			const Raster<csm_t>& csm, const std::string& name) const;

		void fixTAOIds(const TaoIdMap& idMap, const Alignment& layout, cell_t& sofar) const;

		std::string nameFromLayout(const Alignment& layout, cell_t cell) const;

		void writeLayout(const Alignment& layout) const;

		void writeCSMMetrics() const;

		void calculateAndWriteTopo() const;

		template<class T>
		void writeTempRaster(const fs::path& dir, const std::string& name, Raster<T>& r) const;

		template<class T>
		void writeRasterWithFullName(const fs::path& dir, const std::string& baseName, Raster<T>& r, OutputUnitLabel u) const;

		std::unique_ptr<LapisPrivate> pr;

		mutable std::atomic_bool _isRunning = false;

		LapisData* data;
	};

	template<class T>
	void LapisController::writeTempRaster(const fs::path& dir, const std::string& name, Raster<T>& r) const {
		fs::create_directories(dir);
		r.writeRaster((dir / fs::path(name + ".tif")).string());
	}

	template<class T>
	void LapisController::writeRasterWithFullName(const fs::path& dir, const std::string& baseName, Raster<T>& r, OutputUnitLabel u) const {
		try {
			fs::create_directories(dir);
			std::string unitString;
			using oul = OutputUnitLabel;
			if (u == oul::Default) {
				Unit outUnit = data->metricAlign()->crs().getZUnits();
				std::regex meterregex{ ".*met.*",std::regex::icase };
				std::regex footregex{ ".*f(o|e)(o|e)t.*",std::regex::icase };
				if (std::regex_match(outUnit.name, meterregex)) {
					unitString = "_Meters";
				}
				else if (std::regex_match(outUnit.name, footregex)) {
					unitString = "_Feet";
				}
			}
			else if (u == oul::Percent) {
				unitString = "_Percent";
			}
			else if (u == oul::Radian) {
				unitString = "_Radians";
			}
			std::string runName = data->name().size() ? data->name() + "_" : "";
			fs::path fullPath = dir / (runName + baseName + unitString + ".tif");
			r.writeRaster(fullPath.string());
		}
		catch (InvalidRasterFileException e) {
			LapisLogger::getLogger().logMessage("Error writing " + baseName);
		}
		LapisLogger::getLogger().incrementTask();
	}
}

#endif