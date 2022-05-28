#pragma once
#ifndef lp_pointmetriccontroller_h
#define lp_pointmetriccontroller_h


#include"Logger.hpp"
#include"PointMetricCalculator.hpp"
#include"Options.hpp"
#include"LapisUtils.hpp"

namespace std {
	namespace filesystem {
		class path;
	}
}

namespace lapis {

	namespace fs = std::filesystem;
	
	class LapisController {

		using csm_data = coord_t;

	public:

		LapisController(const FullOptions& opt);

		void processFullArea();

	private:

		//a struct for holding all the objects that the processing threads need shared access to
		struct ThreadController {

			ThreadController(const LapisController& lc);
			void getReadyForCSMMerging() {

				sofar = 0;

				//this cleanup probably isn't strictly necessary and if it ends up being slow it can be removed
				pointRast = Raster<PointMetricCalculator>();
				metrics.clear();
				metrics.shrink_to_fit();
			}

			Raster<int> nLaz;
			Raster<PointMetricCalculator> pointRast;
			size_t sofar;
			std::mutex globalMut; //for general operations
			static const cell_t mutexN = 10000; //the number of mutexes--ideally, a balance between reducing the chance of two threads colliding, and not wanting to overuse memory
			std::vector<std::mutex> cellMuts; //for locking individual cells of the output rasters

			//a sorted list of files in a reasonable order to process them in to minimize stray points
			std::vector<std::string> lasFiles;

			std::vector<PointMetricRaster> metrics;
		};

		//returns the filenames of the las/dtm files that overlap the given extent
		template<class T>
		T filesByExtent(const Extent& e, const T& files) const;

		//handles the logic to decide which las files should be processed by the thread that calls this function
		void pointMetricThread(ThreadController& con) const;

		//assigns points from a file to the correct cells of the output
		//returns the number of points that pass the filters and get used in later processing
		long long assignPoints(LasReader& lasname, ThreadController& con, std::optional<Raster<csm_data>>& csm) const;

		//processes the points that have already been found and assigns the output to rasters
		//returns the number of points freed from memory
		void processPoints(const Extent& e, ThreadController& con) const;

		//runs metrics on an individual cell
		//returns the number of points processed
		void processCell(ThreadController& con, cell_t cell) const;

		fs::path getCSMTempDir() const;

		fs::path getCSMPermanentDir() const;

		//This is the function that performs the work of merging the temporary CSM files into their final tiles
		//layout is an alignment whose cells represent the extents of the final tiles.
		void mergeCSMThread(ThreadController& lasFileOrder, const Alignment& layout);


		//a map between the las filenames being used and their extents
		fileToLasExtent lasLocs;
		//a map between the dtm filenames and their extents
		fileToAlignment dtmLocs;

		//controls the logging options for this run
		Logger log;

		//the number of threads to multithread over. Is derived from the similar parameter in the options,
		//but will cap at a reasonable number in case the user provided bad input
		int nThread;

		//the minimum and maximum height of lidar points to be accepted
		coord_t minht, maxht;

		//the resolution to store the vertical data at
		coord_t binsize;

		//The filters to use on this data
		std::vector<std::shared_ptr<LasFilter>> filters;

		//A list of point metrics to calculate
		std::vector<PointMetricDefn> point_metrics;

		//The canopy cutoff, in the units of the desired output
		coord_t canopy_cutoff;

		//The folder to write point metric rasters to
		std::string outFolder;

		//user-specified overrides for the CRS and units of the las and dtm files
		CoordRef lasCRSOverride;
		CoordRef dtmCRSOverride;
		Unit lasUnitOverride;
		Unit dtmUnitOverride;

		//the alignment for point metrics to write to
		Alignment outAlign;

		//the alignment for the CSM to write to
		Alignment csmAlign;

		//these functions are basically part of the constructor, and are just separated out for tidiness
		void _setOutAlign(const FullOptions& opt);
		void _setFilters(const FullOptions& opt);
		void _setMetrics(const FullOptions& opt);

		//not currently configurable, but keeping the switch ready
		bool docsm;

		//the max size in bytes to make a CSM raster
		//not intended to be configurable
		const long long maxCsmBytes = 1024 * 1024 * 1024; //1 GB
	};
}

#endif