#pragma once
#ifndef lp_pointmetriccontroller_h
#define lp_pointmetriccontroller_h


#include"Logger.hpp"
#include"PointMetricCalculator.hpp"
#include"Options.hpp"
#include"LapisUtils.hpp"
#include"UsableParameters.hpp"

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

		LapisController();
		LapisController(const FullOptions& opt);

		void processFullArea();

	protected:

		//returns the filenames of the las/dtm files that overlap the given extent
		template<class T>
		T filesByExtent(const Extent& e, const T& files) const;

		//handles the logic to decide which las files should be processed by the thread that calls this function
		void pointMetricThread(size_t& soFar) const;

		//assigns points from a file to the correct cells of the output
		//returns the number of points that pass the filters and get used in later processing
		long long assignPoints(LasReader& las, std::optional<Raster<csm_data>>& csm) const;

		//processes the points that have already been found and assigns the output to rasters
		//returns the number of points freed from memory
		void processPoints(const Extent& e) const;

		//runs metrics on an individual cell
		//returns the number of points processed
		void processCell(cell_t cell) const;

		fs::path getCSMTempDir() const;

		fs::path getCSMPermanentDir() const;

		//This is the function that performs the work of merging the temporary CSM files into their final tiles
		//layout is an alignment whose cells represent the extents of the final tiles.
		void mergeCSMThread(const Alignment& layout, cell_t& soFar);


		UsableParameters params;
		GlobalParameters* gp;
		LasParameters* lp;
	};
}

#endif