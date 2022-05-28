#pragma once
#ifndef lp_usableparameters_h
#define lp_usableparameters_h

#include"Options.hpp"
#include"gis/Raster.hpp"
#include"PointMetricCalculator.hpp"
#include"LapisUtils.hpp"

namespace lapis {

	//parameters that are only needed during the portion of the program where laz files are being read, and can be safely cleaned up after that
	//laz reading is guaranteed to be the first 
	struct LasParameters {

		//a raster, aligning to the metric grid, whose values are initialized to the number of LAZ files whose extents overlap that cell
		//as the progressing progresses, the values will be reduced to indicating the number of yet-unprocessed LAZ files whose extents overlap that cell
		Raster<int> nLaz;

		//a raster aligning to the metric grid that handles the actual point metric calculation
		Raster<PointMetricCalculator> calculators;

		//mutexes to handle shared access to the cells of a raster
		//the intent is to mod the cell you want to access by mutexN
		//10000 is intended as a balance between minimizing collisions without excessive memory usage
		inline static constexpr int mutexN = 10000;
		std::vector<std::mutex> cellMuts;

		//the rasters that actually hold the point metric results, along with the filename and function call
		std::vector<PointMetricRaster> metricRasters;

		//the filters to be applied to the las files
		std::vector<std::shared_ptr<LasFilter>> filters;

		//if the user specifies that the crs or units of the las files are wrong, store them here
		CoordRef lasCRSOverride; Unit lasUnitOverride;

		//if the user specifies that the crs or units of the dem files are wrong, store them here
		CoordRef demCRSOverride; Unit demUnitOverride;
	};

	//parameters that need to exist even after the laz reading is done
	struct GlobalParameters {

		//the (rough) max size a CSM tile should be
		inline static constexpr size_t maxCSMBytes = 1024 * 1024 * 1024; //1gb


		//The identified las files, in the order they are to be processed
		std::vector<LasFileExtent> sortedLasFiles;

		//The identified dem files, in no particular order
		std::vector<DemFileAlignment> demFiles;

		Logger log;

		//the number of threads to run on
		int nThread;

		//the width of the bins to use in histograms
		coord_t binSize;
		
		coord_t canopyCutoff;

		//The grid to store coarse metrics at
		Alignment metricAlign;

		//The grid to store fine metrics (such as the csm) at
		Alignment csmAlign;

		//the minimum and maximum values for non-outlier las returns
		coord_t minht, maxht;
	};

	//this class stores all the parameters of the run in a form usable by the actual logic of the app
	//as well as the logic to convert from user-specified options into app-usable parameters
	class UsableParameters {
	public:
		UsableParameters();
		UsableParameters(const FullOptions& opt);

		//there's a lot of memory that just isn't important after you finish with the point metrics and can be freed
		void cleanUpAfterPointMetrics();

		std::unique_ptr<LasParameters> lasParams;
		std::unique_ptr<GlobalParameters> globalParams;

		//these functions are protected instead of private to enable easier testing 
	protected:

		void otherParams(const FullOptions& opt);
		void identifyLasFiles(const FullOptions& opt);
		void identifyDEMFiles(const FullOptions& opt);
		void setFilters(const FullOptions& opt);
		void setPointMetrics(const FullOptions& opt);
		//as a necessary consequence of the complicated chain of dependencies, this function also reprojects the las extents into the output extent
		void createOutAlignment(const FullOptions& opt);
		void sortLasFiles(const FullOptions& opt);
		void makeNLaz();

	private:
		template<class T>
		using openFuncType = void(const std::filesystem::path&, std::vector<T>&, Logger&,
			const CoordRef&, const Unit&);
		template<class T>
		void iterateOverFileSpecifiers(const std::vector<std::string>& specifiers, openFuncType<T> openFunc, std::vector<T>& fileList, Logger& log,
			const CoordRef& crsOverride, const Unit& zUnitOverride);

		//returns a useful default for the number of concurrent threads to run
		unsigned int defaultNThread();
	};

	//attempts to open the given file as a las/laz file. If it succeeds, adds the given file to data
	//otherwise, logs an error and does nothing
	void tryLasFile(const std::filesystem::path& file, std::vector<LasFileExtent>& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);

	//attempts to open the given file as a raster file. If it succeeds, adds the given file to data
	//otherwise, logs an error and does nothing
	void tryDtmFile(const std::filesystem::path& file, std::vector<DemFileAlignment>& data, Logger& log,
		const CoordRef& crsOverride, const Unit& zUnitOverride);
}

#endif