#include"app_pch.hpp"
#include"Options.hpp"
#include"Logger.hpp"
#include"gis/Alignment.hpp"
#include"LapisGui.hpp"


namespace lapis {
	

	ParseResults parseOptions(const std::vector<std::string>& args)
	{
		namespace po = boost::program_options;

		auto& log = Logger::getLogger();
		try {
			auto& opt = Options::getOptionsObject();

			po::options_description cmdOnlyOpts;
			cmdOnlyOpts.add_options()
				("help", "Print this message and exit")
				("gui", "Display the GUI")
				("ini-file", po::value<std::vector<std::string>>(), "The .ini file containing parameters for this run\n"
					"You may specify this multiple times; values from earlier files will be preferred")
				;
			po::positional_options_description pos;
			pos.add("ini-file", -1);

			po::options_description visibleOpts;
			po::options_description hiddenOpts;

			auto& params = opt.getParamInfo();
			for (auto& kv : params) {
				Options::FullParam& p = kv.second;
				const std::string& key = kv.first;
				std::string paramNames = key;
				if (p.cmdAlt.size()) {
					paramNames += "," + p.cmdAlt;
				}
				po::options_description* boostOpt = nullptr;
				if (p.hidden) {
					boostOpt = &hiddenOpts;
				}
				else {
					boostOpt = &visibleOpts;
				}
				if (std::holds_alternative<std::string>(p.value)) {
					std::string* ptr = &(std::get<std::string>(p.value));
					boostOpt->add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
				else if (std::holds_alternative<std::vector<std::string>>(p.value)) {
					std::vector<std::string>* ptr = &(std::get<std::vector<std::string>>(p.value));
					boostOpt->add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
				else {
					bool* ptr = &(std::get<bool>(p.value));
					boostOpt->add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
			}

			po::options_description cmdOptions;
			cmdOptions
				.add(cmdOnlyOpts)
				.add(visibleOpts)
				.add(hiddenOpts)
				;

			po::options_description iniOptions;
			iniOptions
				.add(visibleOpts)
				.add(hiddenOpts)
				;


			po::variables_map vmFull;
			po::store(po::command_line_parser(args)
				.options(cmdOptions)
				.run(), vmFull);
			po::notify(vmFull);


			if (vmFull.count("ini-file")) {
				for (auto& ini : vmFull.at("ini-file").as<std::vector<std::string>>()) {
					po::store(po::parse_config_file(ini.c_str(), iniOptions), vmFull);
					po::notify(vmFull);
				}
			}


			if (vmFull.count("help") || !args.size()) {
				std::cout << "\n" <<
					cmdOnlyOpts <<
					visibleOpts <<
					"\n";
				return ParseResults::helpPrinted;
			}
			if (vmFull.count("gui")) {
				return ParseResults::guiRequested;
			}

			
			return ParseResults::validOpts;
		}
		catch (boost::program_options::error_with_option_name e) {
			log.logError(e.what());
			return ParseResults::invalidOpts;
		}
		catch (std::exception e) {
			log.logError(e.what());
			return ParseResults::invalidOpts;
		}
	}

	ParseResults parseGui(const LapisGuiObjects& lgo)
	{
		Options& opt = Options::getOptionsObject();
		namespace pn = paramNames;
		opt.reset();
		for (auto& s : lgo.lasSpecs) {
			opt.updateValue(pn::lasFiles, s);
		}
		for (auto& s : lgo.demSpecs) {
			opt.updateValue(pn::demFiles, s);
		}
		opt.updateValue(pn::lasCrs, lgo.lasAdvancedData.crsString);
		opt.updateValue(pn::demCrs, lgo.demAdvancedData.crsString);

		opt.updateValue(pn::lasUnits, lgo.unitRadioOrder[lgo.lasAdvancedData.unitsRadio].name);
		opt.updateValue(pn::demUnits, lgo.unitRadioOrder[lgo.demAdvancedData.unitsRadio].name);

		opt.updateValue(pn::output, lgo.outputFolderBuffer.data());

		opt.updateValue(pn::canopy, lgo.withUnits.canopyBuffer.data());

		int nallowed = 0;
		int nblocked = 0;
		for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
			if (lgo.classesCheck[i]) {
				nallowed++;
			}
			else {
				nblocked++;
			}
		}
		std::string classList = "";

		if (nblocked > 0) {
			if (nblocked < nallowed) {
				classList = "~";
			}
			bool comma = false;

			for (size_t i = 0; i < lgo.classesCheck.size(); ++i) {
				if (lgo.classesCheck[i] && (nblocked >= nallowed)) {
					if (comma) {
						classList += ",";
					}
					else {
						comma = true;
					}
					classList += std::to_string(i);
				}
				else if (!lgo.classesCheck[i] && (nblocked < nallowed)) {
					if (comma) {
						classList += ",";
					}
					else {
						comma = true;
					}
					classList += std::to_string(i);
				}
			}
			opt.updateValue(pn::classFilter, classList);
		}

		opt.updateValue(pn::csmCellsize, lgo.withUnits.csmCellsizeBuffer.data());
		if (lgo.fineint) {
			opt.updateValue(pn::fineint, "");
		}
		if (lgo.firstReturnsCheck) {
			opt.updateValue(pn::first, "");
		}
		opt.updateValue(pn::maxht, lgo.withUnits.maxHtBuffer.data());
		opt.updateValue(pn::minht, lgo.withUnits.minHtBuffer.data());

		opt.updateValue(pn::outCrs, lgo.outCRSString);
		if (!lgo.filterWithheldCheck) {
			opt.updateValue(pn::useWithheld, "");
		}
		opt.updateValue(pn::userUnits, lgo.unitRadioOrder[lgo.userUnitsRadio].name);
		opt.updateValue(pn::xorigin, lgo.withUnits.xOriginBuffer.data());
		opt.updateValue(pn::yorigin, lgo.withUnits.yOriginBuffer.data());
		opt.updateValue(pn::xres, lgo.withUnits.xResBuffer.data());
		opt.updateValue(pn::yres, lgo.withUnits.yResBuffer.data());
		
		if (lgo.performance) {
			opt.updateValue(pn::performance, "");
		}
		opt.updateValue(pn::thread, lgo.nThreadBuffer.data());

		opt.updateValue(pn::footprint, lgo.withUnits.footprintDiamBuffer.data());
		opt.updateValue(pn::smooth, std::to_string(lgo.smoothRadio));

		return ParseResults::validOpts;
	}

	ParseResults parseIni(const std::string& path)
	{

		namespace po = boost::program_options;
		auto& opt = Options::getOptionsObject();
		opt.reset();
		auto& params = opt.getParamInfo();
		auto& log = Logger::getLogger();
		po::options_description od;
		po::variables_map vm;
		try {
			
			for (auto& kv : params) {
				Options::FullParam& p = kv.second;
				const std::string& key = kv.first;
				std::string paramNames = key;
				if (p.cmdAlt.size()) {
					paramNames += "," + p.cmdAlt;
				}
				if (std::holds_alternative<std::string>(p.value)) {
					std::string* ptr = &(std::get<std::string>(p.value));
					od.add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
				else if (std::holds_alternative<std::vector<std::string>>(p.value)) {
					std::vector<std::string>* ptr = &(std::get<std::vector<std::string>>(p.value));
					od.add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
				else {
					bool* ptr = &(std::get<bool>(p.value));
					od.add_options()(paramNames.c_str(), po::value(ptr), p.cmdDoc.c_str());
				}
			}

			po::store(po::parse_config_file(path.c_str(), od), vm);
			po::notify(vm);
		}
		catch (boost::program_options::error_with_option_name e) {
			log.logError(e.what());
			return ParseResults::invalidOpts;
		}
		catch (std::exception e) {
			log.logError(e.what());
			return ParseResults::invalidOpts;
		}
		return ParseResults::validOpts;
	}

	void writeOptions(std::ostream& out, Options::ParamCategory cat)
	{
		const std::unordered_map<Options::ParamCategory, std::string> catNames =
		{ {Options::ParamCategory::computer,"#Computer-Specific Parameters\n"},
			{Options::ParamCategory::data,"#Data Parameters\n"},
			{Options::ParamCategory::processing,"#Processing Parameters\n"} };

		out << std::setprecision((std::streamsize)std::numeric_limits<coord_t>::digits10 + 1);
		out << catNames.at(cat);
		Options& opt = Options::getOptionsObject();
		for (auto& kv : opt.getParamInfo()) {
			if (kv.second.cat == cat) {
				out << opt.getIniText(kv.first);
			}
		}
		out << "\n";
	}

	Options::Options()
	{
		namespace pn = paramNames;
		using pc = ParamCategory;
		using dt = DataType;

		params.emplace(pn::lasFiles, FullParam(dt::multi, pc::data,
			"Specify input point cloud (las/laz) files in one of three ways:\n"
			"\tAs a file name pointing to a point cloud file\n"
			"As a folder name, which will haves it and its subfolders searched for .las or .laz files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.laz\n"
			"This option can be specified multiple times",
			"L",
			false));

		params.emplace(pn::demFiles, FullParam(dt::multi, pc::data,
			"Specify input raster elevation models in one of three ways:\n"
			"\tAs a file name pointing to a raster file\n"
			"As a folder name, which will haves it and its subfolders searched for raster files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.tif\n"
			"This option can be specified multiple times\n"
			"Most raster formats are supported, but arcGIS .gdb geodatabases are not",
			"D",
			false));

		params.emplace(pn::name, FullParam(dt::single, pc::data,
			"The name of the acquisition. If specified, will be included in the filenames and metadata.",
			"N",
			false));

		params.emplace(pn::output, FullParam(dt::single, pc::data,
			"The output folder to store results in",
			"O",
			false));

		params.emplace(pn::lasUnits, FullParam(dt::single, pc::data));
		params.emplace(pn::demUnits, FullParam(dt::single, pc::data));
		params.emplace(pn::lasCrs, FullParam(dt::single, pc::data));
		params.emplace(pn::demCrs, FullParam(dt::single, pc::data));
		params.emplace(pn::footprint, FullParam(dt::single, pc::data));
		params.emplace(pn::smooth, FullParam(dt::single, pc::processing));

		params.emplace(pn::alignment, FullParam(dt::single, pc::processing,
			"A raster file you want the output metrics to align with\n"
			"Incompatible with --cellsize and --out-crs options",
			"A",
			false));

		params.emplace(pn::cellsize, FullParam(dt::single, pc::processing,
			"The desired cellsize of the output metric rasters\n"
			"Defaults to 30 meters\n"
			"Incompatible with the --alignment options",
			"",
			false));

		params.emplace(pn::csmCellsize, FullParam(dt::single, pc::processing,
			"The desired cellsize of the output canopy surface model\n"
			"Defaults to 1 meter",
			"",
			false));

		params.emplace(pn::outCrs, FullParam(dt::single, pc::processing,
			"The desired CRS for the output layers\n"
			"Incompatible with the --alignment options",
			"",
			false));

		params.emplace(pn::userUnits, FullParam(dt::single, pc::processing,
			"The units you want to specify minht, maxht, and canopy in. Defaults to meters.\n"
			"\tValues: m (for meters), f (for international feet), usft (for us survey feet)",
			"",
			false));

		params.emplace(pn::canopy, FullParam(dt::single, pc::processing,
			"The height threshold for a point to be considered canopy.",
			"",
			false));

		params.emplace(pn::minht, FullParam(dt::single, pc::processing,
			"The threshold for low outliers. Points with heights below this value will be excluded.",
			"",
			false));

		params.emplace(pn::maxht, FullParam(dt::single, pc::processing,
			"The threshold for high outliers. Points with heights above this value will be excluded.",
			"",
			false));

		params.emplace(pn::strata, FullParam(dt::single, pc::processing,
			"A comma-separated list of strata breaks on which to calculate strata metrics.",
			"",
			false));

		params.emplace(pn::first, FullParam(dt::binary, pc::processing,
			"Perform this run using only first returns.",
			"",
			false));

		params.emplace(pn::advancedPoint, FullParam(dt::binary, pc::processing,
			"Calculate a larger suite of point metrics.",
			"",
			false));

		params.emplace(pn::fineint, FullParam(dt::binary, pc::processing,
			"Create a canopy mean intensity raster with the same resolution as the CSM",
			"",
			false));

		params.emplace(pn::classFilter, FullParam(dt::single, pc::processing,
			"A comma-separated list of LAS point classifications to use for this run.\n"
			"Alternatively, preface the list with ~ to specify a blacklist.",
			"",
			false));

		params.emplace(pn::useWithheld, FullParam(dt::binary, pc::processing));
		params.emplace(pn::maxScanAngle, FullParam(dt::single, pc::processing));
		params.emplace(pn::onlyReturns, FullParam(dt::binary, pc::processing));
		params.emplace(pn::xres, FullParam(dt::single, pc::processing));
		params.emplace(pn::yres, FullParam(dt::single, pc::processing));
		params.emplace(pn::xorigin, FullParam(dt::single, pc::processing));
		params.emplace(pn::yorigin, FullParam(dt::single, pc::processing));
		params.emplace(pn::gdalprojDisplay, FullParam(dt::binary, pc::computer));

		params.emplace(pn::thread, FullParam(dt::single, pc::computer,
			"The number of threads to run Lapis on. Defaults to the number of cores on the computer",
			"",
			false));

		params.emplace(pn::performance, FullParam(dt::binary, pc::computer,
			"Run in performance mode, with a vertical resolution of 10cm instead of 1cm.",
			"",
			false));
	}

	const std::string& Options::getAsString(const std::string& key) const {
		return std::get<std::string>(params.at(key).value);
	}

	const std::vector<std::string>& Options::getAsVector(const std::string& key) const {
		return std::get<std::vector<std::string>>(params.at(key).value);
	}

	bool Options::getAsBool(const std::string& key) const {
		return std::get<bool>(params.at(key).value);
	}

	double Options::getAsDouble(const std::string& key, double defaultValue) const {
		double v = defaultValue;
		auto& s = getAsString(key);
		if (s.size()) {
			try {
				v = std::atof(s.c_str());
			}
			catch (std::invalid_argument e) {
				Logger::getLogger().logError("Invalid formatting for " + key + " argument");
				throw std::runtime_error("Invalid formatting for " + key + " argument");
			}
		}
		return v;
	}

	//For parameters of type single, this replaces the old value.
	//For parameters of type multi, it adds to the list.
	//For parameters of type binary, it sets the flag to true

	Options& Options::getOptionsObject()
	{
		static Options opt;
		return opt;
	}

	void Options::updateValue(const std::string& key, const std::string& value) {
		auto& v = params.at(key).value;
		if (std::holds_alternative<bool>(v)) {
			std::get<bool>(v) = true;
		}
		else if (std::holds_alternative<std::string>(v)) {
			std::get<std::string>(v) = value;
		}
		else {
			std::get<std::vector<std::string>>(v).push_back(value);
		}
	}
	std::string Options::getIniText(const std::string& key) {

		//Because alignment is something that should be consistent across projects, but will often be specified as a file which can be deleted or moved, or on a different computer
		//Some special casing is done here to ensure that the ini file always specifies the alignment in a universal way.

		if (key == paramNames::alignment) {
			return "";
		}
		else if (key == paramNames::outCrs) {
			return key + "=" + getAlign().crs.getSingleLineWKT() + "\n";
		}
		else if (key == paramNames::cellsize) {
			return "";
		}
		else if (key == paramNames::xres) {
			return key + "=" + std::to_string(std::abs(getAlign().xres)) + "\n";
		}
		else if (key == paramNames::yres) {
			return key + "=" + std::to_string(std::abs(getAlign().yres)) + "\n";
		}
		else if (key == paramNames::xorigin) {
			return key + "=" + std::to_string(std::abs(getAlign().xorigin)) + "\n";
		}
		else if (key == paramNames::yorigin) {
			return key + "=" + std::to_string(std::abs(getAlign().yorigin)) + "\n";
		}

		auto& v = params.at(key).value;
		if (std::holds_alternative<bool>(v)) {
			bool b = std::get<bool>(v);
			if (b) {
				return key + "=\n";
			}
			else {
				return "#" + key + "=\n";
			}
		}
		if (std::holds_alternative<std::string>(v)) {
			auto& s = std::get<std::string>(v);
			if (!s.size()) {
				return "";
			}
			return key + "=" + std::get<std::string>(v) + "\n";
		}

		auto& vec = std::get<std::vector<std::string>>(v);
		std::string out = "";
		for (auto& entry : vec) {
			out += key + "=" + entry + "\n";
		}
		return out;
	}
	const std::vector<std::string>& Options::getLas() const {
		return getAsVector(paramNames::lasFiles);
	}
	const std::vector<std::string>& Options::getDem() const {
		return getAsVector(paramNames::demFiles);
	}
	const std::string& Options::getOutput() const {
		return getAsString(paramNames::output);
	}
	Unit Options::getLasUnits() const {
		return Unit(getAsString(paramNames::lasUnits));
	}
	Unit Options::getDemUnits() const {
		return Unit(getAsString(paramNames::demUnits));
	}
	CoordRef Options::getLasCrs() const {
		return CoordRef(getAsString(paramNames::lasCrs));
	}
	CoordRef Options::getDemCrs() const {
		return CoordRef(getAsString(paramNames::demCrs));
	}
	coord_t Options::getFootprintDiameter() const {
		coord_t v = getAsDouble(paramNames::footprint, 0.4);
		return convertUnits(v, getUserUnits(), getAlign().crs.getXYUnits());
	}
	int Options::getSmoothWindow() const {
		return (int)getAsDouble(paramNames::smooth, 3);
	}
	const Options::AlignWithoutExtent& Options::getAlign() const {
		if (outAlign) {
			return *outAlign;
		}
		outAlign = std::make_unique<AlignWithoutExtent>();
		auto& s = getAsString(paramNames::alignment);
		if (s.size()) {
			try {
				Alignment fileAlign = Alignment(s);
				outAlign->crs = fileAlign.crs();
				outAlign->xres = fileAlign.xres();
				outAlign->yres = fileAlign.yres();
				outAlign->xorigin = fileAlign.xOrigin();
				outAlign->yorigin = fileAlign.yOrigin();
			}
			catch (InvalidRasterFileException e) {
				Logger::getLogger().logError(s + " is not a valid raster file.");
				throw std::runtime_error(s + " is not a valid raster file.");
			}
		}

		auto& s2 = getAsString(paramNames::outCrs);
		try {
			CoordRef crs(s2);
			if (!crs.isEmpty()) {
				outAlign->crs = crs;
			}
		}
		catch (UnableToDeduceCRSException e) {
			Logger::getLogger().logError("Unable to construct CRS from " + s2);
		}
		Unit userUnits = getUserUnits();
		if (!userUnits.isUnknown()) {
			outAlign->crs.setZUnits(userUnits);
		}

		coord_t cellsize = getAsDouble(paramNames::cellsize, 0);
		if (cellsize > 0) {
			outAlign->xres = -cellsize;
			outAlign->yres = -cellsize;
		}

		coord_t xres = getAsDouble(paramNames::xres, 0);
		if (xres > 0) {
			outAlign->xres = -xres;
		}
		coord_t yres = getAsDouble(paramNames::yres, 0);
		if (yres > 0) {
			outAlign->yres = -yres;
		}
		coord_t xorigin = getAsDouble(paramNames::xorigin, 0);
		if (xorigin > 0) {
			outAlign->xorigin = -xorigin;
		}
		coord_t yorigin = getAsDouble(paramNames::yorigin, 0);
		if (yorigin > 0) {
			outAlign->yorigin = -yorigin;
		}
		return *outAlign;
	}
	coord_t Options::getCSMCellsize() const {
		return getAsDouble(paramNames::csmCellsize, 0);
	}
	Unit Options::getUserUnits() const {
		return Unit(getAsString(paramNames::userUnits));
	}
	coord_t Options::getCanopyCutoff() const {
		return getAsDouble(paramNames::canopy, convertUnits(2, linearUnitDefs::meter, getUserUnits()));
	}
	coord_t Options::getMinHt() const {
		return getAsDouble(paramNames::minht, convertUnits(-2, linearUnitDefs::meter, getUserUnits()));
	}
	coord_t Options::getMaxHt() const {
		return getAsDouble(paramNames::maxht, convertUnits(100, linearUnitDefs::meter, getUserUnits()));
	}
	std::vector<coord_t> Options::getStrataBreaks() const {
		std::vector<coord_t> out;
		auto& s = getAsString(paramNames::strata);
		if (!s.size()) {
			return out;
		}
		std::stringstream tokenizer{ s };
		std::string temp;
		while (std::getline(tokenizer, temp, ',')) {
			try {
				out.push_back(std::stod(temp));
			}
			catch (std::invalid_argument e) {
				throw std::runtime_error("Invalid formatting for strata list");
			}
		}
		std::sort(out.begin(), out.end());
		return out;
	}
	bool Options::getFirstFlag() const {
		return getAsBool(paramNames::first);
	}
	bool Options::getFineIntFlag() const {
		return getAsBool(paramNames::fineint);
	}
	Options::ClassFilter Options::getClassFilter() const {
		std::regex whitelistregex{ "[0-9]+(,[0-9]+)*" };
		std::regex blacklistregex{ "~[0-9]+(,[0-9]+)*" };

		std::string s = getAsString(paramNames::classFilter);
		ClassFilter useclasses;
		if (!s.size()) {
			return useclasses;
		}

		if (std::regex_match(s, whitelistregex)) {
			useclasses.isWhiteList = true;
		}
		else if (std::regex_match(s, blacklistregex)) {
			useclasses.isWhiteList = false;
			s = s.substr(1, s.size());
		}
		else {
			throw std::runtime_error("Invalid formatting for classification list: " + s);
		}

		std::stringstream tokenizer{ s };
		std::string temp;
		while (std::getline(tokenizer, temp, ',')) {
			useclasses.list.insert(std::stoi(temp));
		}

		return useclasses;
	}
	bool Options::getUseWithheldFlag() const {
		return getAsBool(paramNames::useWithheld);
	}
	int8_t Options::getMaxScanAngle() const
	{
		return (int8_t)getAsDouble(paramNames::maxScanAngle, -1);
	}
	bool Options::getOnlyFlag() const {
		return getAsBool(paramNames::onlyReturns);
	}
	int Options::getNThread() const {
		return (int)getAsDouble(paramNames::thread, defaultNThread());
	}
	bool Options::getPerformanceFlag() const {
		return getAsBool(paramNames::performance);
	}
	const std::string& Options::getName() const
	{
		return getAsString(paramNames::name);
	}
	bool Options::getGdalProjWarningFlag() const
	{
		return getAsBool(paramNames::gdalprojDisplay);
	}
	bool Options::getAdvancedPointFlag() const
	{
		return getAsBool(paramNames::advancedPoint);
	}
	void Options::reset()
	{
		*this = Options();
	}
	std::map<std::string, Options::FullParam>& Options::getParamInfo()
	{
		return params;
	}
	Options::FullParam::FullParam(Options::DataType type, ParamCategory cat, const std::string& cmdDoc, const std::string& cmdAlt, bool hidden) :
		cmdDoc(cmdDoc), cmdAlt(cmdAlt), hidden(hidden) {
		if (type==DataType::multi) {
			value = std::vector<std::string>();
		}
		else if (type==DataType::single) {
			value = std::string();
		}
		else {
			value = false;
		}
		this->cat = cat;
	}
	Options::FullParam::FullParam(Options::DataType type, ParamCategory cat) : FullParam(type, cat, "", "", true) {}
}
