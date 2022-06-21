#pragma once
#ifndef lp_pointmetriccontroller_h
#define lp_pointmetriccontroller_h


#include"Logger.hpp"
#include"PointMetricCalculator.hpp"
#include"Options.hpp"
#include"LapisUtils.hpp"
#include"LapisObjects.hpp"
#include"csmalgos.hpp"
#include"LapisPrivate.hpp"

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
		void assignPointsToCalculators(const LidarPointVector& points) const;

		void assignPointsToCSM(const LidarPointVector& points, std::optional<Raster<csm_t>>& csm) const;

		//processes the points that have already been found and assigns the output to rasters
		//returns the number of points freed from memory
		void processPoints(const Extent& e) const;

		//runs metrics on an individual cell
		//returns the number of points processed
		void processCell(cell_t cell) const;

		fs::path getCSMTempDir() const;
		fs::path getCSMPermanentDir() const;
		fs::path getPointMetricDir() const;
		fs::path getParameterDir() const;
		fs::path getTAODir() const;

		fs::path getTempTAODir() const;

		fs::path getLayoutDir() const;

		void writeParams(const FullOptions& opt) const;

		//This is the function that performs the work of merging the temporary CSM files into their final tiles
		//layout is an alignment whose cells represent the extents of the final tiles.
		void csmProcessingThread(const Alignment& layout, cell_t& soFar, TaoIdMap& idMap) const;

		//returns the points belonging to the nth las file in sortedlasextents
		//may throw an InvalidLasFileException if something goes wrong
		LidarPointVector getPoints(size_t n) const;

		void populateMap(const Raster<taoid_t> segments, const std::vector<cell_t>& highPoints, TaoIdMap& map, coord_t bufferDist, cell_t tileidx) const;

		void writeHighPoints(const std::vector<cell_t>& highPoints, const Raster<taoid_t>& segments,
			const Raster<csm_t>& csm, const std::string& name) const;

		void fixTAOIds(const TaoIdMap& idMap, const Alignment& layout, cell_t& sofar) const;

		std::string nameFromLayout(const Alignment& layout, cell_t cell) const;

		void writeLayout(const Alignment& layout) const;


		LapisObjects obj;
		GlobalProcessingObjects* gp;
		LasProcessingObjects* lp;
		std::unique_ptr<LapisPrivate> pr;
	};
}

#endif