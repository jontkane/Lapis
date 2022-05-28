#include"lapis_pch.hpp"
#include"Options.hpp"
#include"Logger.hpp"
#include"gis/Alignment.hpp"


namespace lapis {
	
	namespace po = boost::program_options;

	std::variant<FullOptions,std::exception> parseOptions(int argc, char* argv[], Logger& log)
	{
		try {
			FullOptions opt;

			po::options_description coreOpts;
			coreOpts.add_options()
				("help", "Print this message and exit")
				("ini-file", po::value<std::vector<std::string>>(), "The .ini file containing parameters for this run\n"
					"You may specify this multiple times; values from earlier files will be preferred")
				;
			po::positional_options_description pos;
			pos.add("ini-file", -1);


			po::options_description dataOpts;
			dataOpts.add_options()
				("las,L", po::value(&opt.dataOptions.lasFileSpecifiers),
					"Specify input point cloud (las/laz) files in one of three ways:\n"
					"\tAs a file name pointing to a point cloud file\n"
					"As a folder name, which will haves it and its subfolders searched for .las or .laz files\n"
					"As a folder name with a wildcard specifier, e.g. C:\\data\\*.laz\n"
					"This option can be specified multiple times")
				("dem,D", po::value(&opt.dataOptions.demFileSpecifiers),
					"Specify input raster elevation models in one of three ways:\n"
					"\tAs a file name pointing to a raster file\n"
					"As a folder name, which will haves it and its subfolders searched for raster files\n"
					"As a folder name with a wildcard specifier, e.g. C:\\data\\*.tif\n"
					"This option can be specified multiple times\n"
					"Most raster formats are supported, but arcGIS .gdb geodatabases are not")
				("output,O", po::value(&opt.dataOptions.outfolder),
					"The output folder to store results in")
				("alignment,A",po::value<std::string>(),
					"A raster file you want the output metrics to align with\n"
					"Incompatible with --cellsize and --out-crs options")
				("cellsize",po::value<coord_t>(),
					"The desired cellsize of the output metric rasters\n"
					"Defaults to 30 meters\n"
					"Incompatible with the --alignment options")
				("csm-cellsize",po::value(&opt.dataOptions.csmRes),
					"The desired cellsize of the output canopy surface model\n"
					"Defaults to 1 meter")
				("out-crs",po::value<std::string>(),
					"The desired CRS for the output layers\n"
					"Incompatible with the --alignment options")
				
				;


			boost::optional<std::string> lascrs;
			boost::optional<std::string> demcrs;
			boost::optional<std::string> lasunit;
			boost::optional<std::string> demunit;

			po::options_description hiddenDataOpts;
			hiddenDataOpts.add_options()
				("las-units", po::value(&lasunit),"")
				("dem-units", po::value(&demunit),"")
				("las-crs", po::value(&lascrs),"")
				("dem-crs", po::value(&demcrs),"")
				("crop", "")
				;

			po::options_description processOpts;
			processOpts.add_options()
				("thread", po::value(&opt.processingOptions.nThread),
					"The number of threads to run Lapis on. Defaults to the number of cores on the computer")
				("performance", 
					"Run in performance mode, with a vertical resolution of 10cm instead of 1cm.")
				;


			boost::optional<std::string> userunit;

			po::options_description metricOpts;
			metricOpts.add_options()
				("user-units", po::value(&userunit),
					"The units you want to specify minht, maxht, and canopy in. Defaults to meters.\n"
					"\tValues: m (for meters), f (for international feet), usft (for us survey feet)")
				("canopy", po::value(&opt.metricOptions.canopyCutoff),
					"The height threshold for a point to be considered canopy.")
				("minht", po::value(&opt.metricOptions.minht),
					"The threshold for low outliers. Points with heights below this value will be excluded.")
				("maxht", po::value(&opt.metricOptions.maxht),
					"The threshold for high outliers. Points with heights above this value will be excluded.")
				("first",
					"Perform this run using only first returns.")
				("class", po::value<std::string>(),
					"A comma-separated list of LAS point classifications to use for this run.\n"
					"Alternatively, preface the list with ~ to specify a blacklist.")
				;

			po::options_description hiddenMetricOpts;
			hiddenMetricOpts.add_options()
				("use-withheld","")
				("max-scan-angle",po::value(&opt.metricOptions.maxScanAngle),"")
				("only","")
				;


			po::options_description cmdOptions;
			cmdOptions
				.add(coreOpts)
				.add(dataOpts)
				.add(hiddenDataOpts)
				.add(processOpts)
				.add(metricOpts)
				.add(hiddenMetricOpts)
				;

			po::options_description iniOptions;
			iniOptions
				.add(dataOpts)
				.add(hiddenDataOpts)
				.add(processOpts)
				.add(metricOpts)
				.add(hiddenMetricOpts)
				;


			po::variables_map vmFull;
			po::store(po::command_line_parser(argc, argv)
				.options(cmdOptions)
				.run(), vmFull);
			po::notify(vmFull);


			if (vmFull.count("ini-file")) {
				for (auto& ini : vmFull.at("ini-file").as<std::vector<std::string>>()) {
					po::store(po::parse_config_file(ini.c_str(), iniOptions), vmFull);
					po::notify(vmFull);
				}
			}


			if (vmFull.count("help") || argc < 2) {
				std::cout <<
					coreOpts <<
					dataOpts <<
					metricOpts <<
					"\n";
				return std::runtime_error("not an error");
			}

			if (vmFull.count("alignment") && (vmFull.count("out-crs") || vmFull.count("cellsize"))) {
				throw std::runtime_error("Don't specify output alignment via both a file and cellsize/crs");
			}

			if (vmFull.count("alignment")) {
				//this is just to get it to throw an exception if the string isn't a valid raster file name
#pragma warning(suppress: 26444)
				Alignment(vmFull["alignment"].as<std::string>());

				opt.dataOptions.outAlign = alignmentFromFile{};
				alignmentFromFile& align = std::get<alignmentFromFile>(opt.dataOptions.outAlign);
				align.filename = vmFull["alignment"].as<std::string>();
				if (vmFull.count("crop")) {
					align.useType = alignmentFromFile::alignType::crop;
				}
				else {
					align.useType = alignmentFromFile::alignType::alignOnly;
				}
			}
			else {
				opt.dataOptions.outAlign = manualAlignment{};
				manualAlignment& align = std::get<manualAlignment>(opt.dataOptions.outAlign);

				if (vmFull.count("cellsize")) {
					align.res = vmFull["cellsize"].as<coord_t>();
				}
				else {
					align.res = boost::optional<coord_t>();
				}

				
				align.crs = vmFull.count("out-crs") ? CoordRef(vmFull["out-crs"].as<std::string>()) : CoordRef("");
			}

			opt.dataOptions.lasCRS = CoordRef(lascrs.value_or(""));
			opt.dataOptions.demCRS = CoordRef(demcrs.value_or(""));
			opt.dataOptions.lasUnits = Unit(lasunit.value_or(""));
			opt.dataOptions.demUnits = Unit(demunit.value_or(""));
			opt.dataOptions.outUnits = Unit(userunit.value_or("m"));

			opt.processingOptions.binSize = vmFull.count("performance") ? 0.1 : 0.01;


			if (vmFull.count("only")) {
				opt.metricOptions.whichReturns = MetricOptions::WhichReturns::only;
			}
			else if (vmFull.count("first")) {
				opt.metricOptions.whichReturns = MetricOptions::WhichReturns::first;
			}
			else {
				opt.metricOptions.whichReturns = MetricOptions::WhichReturns::all;
			}

			opt.metricOptions.useWithheld = vmFull.count("use-withheld");

			std::regex whitelistregex{ "[0-9]+(,[0-9]+)*" };
			std::regex blacklistregex{ "~[0-9]+(,[0-9]+)*" };
			if (vmFull.count("class")) {
				std::string classlist = vmFull.at("class").as<std::string>();
				MetricOptions::classificationSet useclasses;
				if (std::regex_match(classlist, whitelistregex)) {
					useclasses.whiteList = true;
				}
				else if (std::regex_match(classlist, blacklistregex)) {
					useclasses.whiteList = false;
					classlist = classlist.substr(1, classlist.size());
				}
				else {
					throw std::runtime_error("Invalid formatting for classification list: " + classlist);
				}

				opt.metricOptions.classes = useclasses;
				std::stringstream tokenizer{ classlist };
				std::string temp;
				while (std::getline(tokenizer, temp, ',')) {
					opt.metricOptions.classes.value().classes.insert(std::stoi(temp));
				}
			}

			return opt;
		}
		catch (boost::program_options::error_with_option_name e) {
			log.logError(e.what());
			return e;
		}
		catch (std::exception e) {
			log.logError(e.what());
			return e;
		}
	}

}
