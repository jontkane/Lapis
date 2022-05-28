#pragma once
#ifndef lp_options_h
#define lp_options_h

#include"lapis_pch.hpp"
#include"Logger.hpp"
#include"gis/BaseDefs.hpp"
#include"gis/CoordRef.hpp"

//these 26495 warnings come from the implementation of boost::optional
//and completely don't matter because a default-constructed optional will have has_value false
#pragma warning (push)
#pragma warning (disable : 26495)
namespace lapis {

	//These structs contain the parameters that define an individual lapis run
	//Generally, they will be stored as provided by the user--turning them into something usable is the responsibility of other classes

	//these are structs for how a user specifies an alignment
	struct manualAlignment {
		boost::optional<coord_t> res = 0.;
		CoordRef crs;
	};
	struct alignmentFromFile {
		enum class alignType {
			alignOnly, crop
		};
		std::string filename;
		alignType useType;
	};

	//this struct contains options like the location of the input and the alignment of the output
	struct DataOptions {
		//generally either folders or filenames; if more than 5 las files are specified by filename
		//then their names will be written to a separate file, whose name will be stored here instead
		std::vector<std::string> lasFileSpecifiers;

		//same as lasFileSpecifiers, but for the DTM rasters
		std::vector<std::string> demFileSpecifiers;

		//something here for how the user can specify the units of the las/dtm if they can't be inferred, or if the inferred units are wrong

		//the location for the output
		std::string outfolder;

		//the output alignment, either as a manually specified resolution+crs, or as a filename+whether to align, crop, or mask
		std::variant<manualAlignment, alignmentFromFile> outAlign;

		//some sort of syntax for specifying the units and crs of the las and dem files
		Unit lasUnits;
		Unit demUnits;
		CoordRef lasCRS;
		CoordRef demCRS;

		//the output units for metrics with units, and also for user-specified heights like canopy cutoff
		Unit outUnits;

		//the cellsize of the output CSM
		boost::optional<coord_t> csmRes;
	};

	//this struct contains options like the amount of memory and threads to use
	struct ProcessingOptions {
		//how much to print
		boost::optional<Logger::Level> logLevel;

		//how many threads to run on
		boost::optional<int> nThread;

		coord_t binSize;
	};

	//this struct contains options that change how the metrics are calculated, like canopy cutoffs
	struct MetricOptions {

		//the outlier cutoffs for minimum and maximum height
		boost::optional<coord_t> minht, maxht;

		//the canopy cutoff
		boost::optional<coord_t> canopyCutoff;

		enum class WhichReturns {
			all, first, only
		};

		//should this run use all returns, first returns, or first and only returns?
		boost::optional<WhichReturns> whichReturns;

		struct classificationSet {
			bool whiteList; //is the below set a whitelist or a blacklist?
			std::unordered_set<std::uint8_t> classes;
		};

		//which classifications should metrics be calculated on?
		boost::optional<classificationSet> classes;

		//should withheld points be excluded?
		bool useWithheld;

		//max absolute scan angle to use
		boost::optional<int> maxScanAngle;

		//the strata breaks to calculate cover on
		boost::optional<std::vector<coord_t>> strata;
	};

	struct FullOptions {
		DataOptions dataOptions;
		ProcessingOptions processingOptions;
		MetricOptions metricOptions;
	};

	//takes command line input and parses it into a FullOptions object
	std::variant<FullOptions, std::exception> parseOptions(int argc, char* argv[], Logger& log);

}
#pragma warning (pop)

#endif