#include"app_pch.hpp"
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
				("csm-cellsize",po::value(&opt.processingOptions.csmRes),
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
				("footprint", po::value(&opt.processingOptions.footprintDiameter))
				;

			po::options_description processOpts;
			processOpts.add_options()
				("thread", po::value(&opt.computerOptions.nThread),
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
				("canopy", po::value(&opt.processingOptions.canopyCutoff),
					"The height threshold for a point to be considered canopy.")
				("minht", po::value(&opt.processingOptions.minht),
					"The threshold for low outliers. Points with heights below this value will be excluded.")
				("maxht", po::value(&opt.processingOptions.maxht),
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
				("max-scan-angle",po::value(&opt.processingOptions.maxScanAngle),"")
				("only","")
				("xres",po::value<coord_t>())
				("yres",po::value<coord_t>())
				("xorigin",po::value<coord_t>())
				("yorigin",po::value<coord_t>())
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

			auto readCRS = [&](CoordRef& crs, std::string s) {
				try {
					crs = CoordRef(s);
				}
				catch (UnableToDeduceCRSException e) {
					log.logError("Could not read as crs:\n" + s);
					throw e;
				}
			};

			try {
				readCRS(opt.dataOptions.lasCRS, lascrs.value_or(""));
				readCRS(opt.dataOptions.demCRS, demcrs.value_or(""));
			}
			catch (UnableToDeduceCRSException e) {
				return e;
			}
			opt.dataOptions.lasUnits = Unit(lasunit.value_or(""));
			opt.dataOptions.demUnits = Unit(demunit.value_or(""));
			opt.processingOptions.outUnits = Unit(userunit.value_or("m"));

			if (vmFull.count("alignment")) {
				try {
					opt.processingOptions.outAlign = Alignment(vmFull["alignment"].as<std::string>());
				}
				catch (InvalidRasterFileException e) {
					log.logError("Could not open as raster: " + vmFull["alignment"].as<std::string>());
					return e;
				}
			}
			else {
				CoordRef crs;
				try {
					crs = vmFull.count("out-crs") ? CoordRef(vmFull["out-crs"].as<std::string>()) : CoordRef("");
					crs.setZUnits(opt.processingOptions.outUnits);
				}
				catch (UnableToDeduceCRSException e) {
					log.logError("Unable to read out-crs. Is it correctly formatted?");
					return e;
				}

				coord_t xres = NAN; coord_t yres = NAN; coord_t xorigin = NAN; coord_t yorigin = NAN;
				if (vmFull.count("cellsize")) {
					xres = vmFull["cellsize"].as<coord_t>();
					yres = vmFull["cellsize"].as<coord_t>();
				}
				else {
					if (vmFull.count("xres") && vmFull.count("yres")) {
						xres = vmFull["xres"].as<coord_t>();
						yres = vmFull["yres"].as<coord_t>();
					}
					else if (vmFull.count("xres")) {
						xres = vmFull["xres"].as<coord_t>();
						yres = vmFull["xres"].as<coord_t>();
					}
					else if (vmFull.count("yres")) {
						xres = vmFull["yres"].as<coord_t>();
						yres = vmFull["yres"].as<coord_t>();
					}
					//no default value here. xres and yres being NAN is a sign the user didn't specify enough info
				}
				if (vmFull.count("xorigin") && vmFull.count("yOrigin")) {
					xorigin = vmFull["xorigin"].as<coord_t>();
					yorigin = vmFull["yorigin"].as<coord_t>();
				}
				else if (vmFull.count("xorigin")) {
					xorigin = vmFull["xorigin"].as<coord_t>();
					yorigin = vmFull["xorigin"].as<coord_t>();
				}
				else if (vmFull.count("yorigin")) {
					xorigin = vmFull["yorigin"].as<coord_t>();
					yorigin = vmFull["yorigin"].as<coord_t>();
				}
				else {
					xorigin = 0;
					yorigin = 0;
				}
				if (!std::isnan(xres)) {
					opt.processingOptions.outAlign = Alignment(Extent(0, 100, 0, 100, crs), xorigin, yorigin, xres, yres);
				}
				
			}

			opt.computerOptions.performance = vmFull.count("performance");


			if (vmFull.count("only")) {
				opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::only;
			}
			else if (vmFull.count("first")) {
				opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::first;
			}
			else {
				opt.processingOptions.whichReturns = ProcessingOptions::WhichReturns::all;
			}

			opt.processingOptions.useWithheld = vmFull.count("use-withheld");

			std::regex whitelistregex{ "[0-9]+(,[0-9]+)*" };
			std::regex blacklistregex{ "~[0-9]+(,[0-9]+)*" };
			if (vmFull.count("class")) {
				std::string classlist = vmFull.at("class").as<std::string>();
				ProcessingOptions::classificationSet useclasses;
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

				opt.processingOptions.classes = useclasses;
				std::stringstream tokenizer{ classlist };
				std::string temp;
				while (std::getline(tokenizer, temp, ',')) {
					opt.processingOptions.classes.value().classes.insert(std::stoi(temp));
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

	void DataOptions::write(std::ostream& out) const
	{
		out << std::setprecision(std::numeric_limits<coord_t>::digits10 + 1);
		out << "#Data Parameters\n";
		for (auto& v : lasFileSpecifiers) {
			out << "las=" << v << "\n";
		}
		for (auto& v : demFileSpecifiers) {
			out << "dem=" << v << "\n";
		}
		out << "output=" << outfolder << "\n";
		if (!lasUnits.isUnknown()) {
			out << "las-units=" << lasUnits.name << "\n";
		}
		if (!lasCRS.isEmpty()) {
			std::string wkt = lasCRS.getSingleLineWKT();
			out << "las-crs=" << wkt << "\n";
		}
		if (!demUnits.isUnknown()) {
			out << "dem-units=" << demUnits.name << "\n";
		}
		if (!demCRS.isEmpty()) {
			std::string wkt = demCRS.getSingleLineWKT();
			out << "dem-crs=" << wkt << "\n";
		}
		out << "\n";
	}
	void ComputerOptions::write(std::ostream& out) const
	{
		out << "#Computer-specific Parameters\n";
		if (nThread.has_value()) {
			out << "thread=" << nThread.value() << "\n";
		}

		if (performance) {
			out << "performance=\n";
		}
		out << "\n";
	}
	void ProcessingOptions::write(std::ostream& out) const
	{
		out << "#Processing Parameters\n";
		out << std::setprecision(std::numeric_limits<coord_t>::digits10 + 1);

		if (outAlign.has_value()) {
			auto& a = outAlign.value();
			if (!a.crs().isEmpty()) {
				std::string wkt = a.crs().getSingleLineWKT();
				out << "out-crs=" << wkt << "\n";
			}
			out << "xres=" << convertUnits(a.xres(),a.crs().getXYUnits(),outUnits) << "\n";
			out << "yres=" << convertUnits(a.yres(), a.crs().getXYUnits(), outUnits) << "\n";
			out << "xorigin=" << convertUnits(a.xOrigin(), a.crs().getXYUnits(), outUnits) << "\n";
			out << "yorigin=" << convertUnits(a.yOrigin(), a.crs().getXYUnits(), outUnits) << "\n";
		}

		if (csmRes.has_value()) {
			out << "csm-cellsize=" << csmRes.value() << "\n";
		}


		out << "user-units=" << outUnits.name << "\n";
		if (canopyCutoff.has_value()) {
			out << "canopy=" << canopyCutoff.value() << "\n";
		}
		if (minht.has_value()) {
			out << "minht=" << minht.value() << "\n";
		}
		if (maxht.has_value()) {
			out << "maxht=" << maxht.value() << "\n";
		}

		if (whichReturns == WhichReturns::first) {
			out << "first=\n";
		}
		else if (whichReturns == WhichReturns::only) {
			out << "only=\n";
		}
		if (classes.has_value()) {
			auto& c = classes.value();
			out << "class=";
			if (!c.whiteList) {
				out << "~";
			}
			size_t count = 0;
			for (auto& v : c.classes) {
				out << (int)v;
				if (count < c.classes.size() - 1) {
					out << ",";
				}
				++count;
			}
			out << "\n";
		}
		if (useWithheld) {
			out << "use-withheld\n";
		}
		if (maxScanAngle.has_value()) {
			out << "max-scan-angle=" << maxScanAngle.value() << "\n";
		}
		out << "\n";
	}
}
