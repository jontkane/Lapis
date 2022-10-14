#include"app_pch.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"

namespace lapis {
	LapisData& LapisData::getDataObject()
	{
		static LapisData d;
		return d;
	}
	LapisData::LapisData()
	{
		CPLSetErrorHandler(LapisData::silenceGDALErrors);

		constexpr ParamName FIRSTPARAM = (ParamName)0;
		_addParam<FIRSTPARAM>();
		_prevUnits = linearUnitDefs::meter;
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
	}
	void LapisData::renderGui(ParamName pn)
	{
		_params[(size_t)pn]->renderGui();
	}
	void LapisData::setPrevUnits(const Unit& u)
	{
		_prevUnits = u;
	}
	const Unit& LapisData::getCurrentUnits() const
	{
		auto& p = _getRawParam<ParamName::outUnits>();
		return ParamBase::unitRadioOrder[p._radio];
	}
	const Unit& LapisData::getPrevUnits() const
	{
		return _prevUnits;
	}
	void LapisData::importBoostAndUpdateUnits()
	{
		//this slightly odd way of ordering things is to ensure that only parameters
		//that weren't specified in the same ini file that changed the units get converted
		_getRawParam<ParamName::outUnits>().importFromBoost();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->updateUnits();
		}
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
	}

	void LapisData::prepareForRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->prepareForRun();
		}
		_cellMuts = std::make_unique<std::vector<std::mutex>>(_cellMutCount);
	}

	void LapisData::cleanAfterRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->cleanAfterRun();
		}
		_cellMuts.reset();
	}

	void LapisData::resetObject() {
		_params.clear();
		constexpr ParamName FIRSTPARAM = (ParamName)0;
		_addParam<FIRSTPARAM>();
		_prevUnits = linearUnitDefs::meter;
		_cellMuts.reset();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
	}

	std::shared_ptr<Alignment> LapisData::metricAlign()
	{
		return _getRawParam<ParamName::alignment>()._align;
	}

	std::shared_ptr<Alignment> LapisData::csmAlign()
	{
		return _getRawParam<ParamName::csmOptions>()._csmAlign;
	}

	shared_raster<int> LapisData::nLazRaster()
	{
		return _getRawParam<ParamName::alignment>()._nLaz;
	}

	shared_raster<PointMetricCalculator> LapisData::allReturnPMC()
	{
		return _getRawParam<ParamName::optionalMetrics>()._allReturnCalculators;
	}

	shared_raster<PointMetricCalculator> LapisData::firstReturnPMC()
	{
		return _getRawParam<ParamName::optionalMetrics>()._firstReturnCalculators;
	}

	std::mutex& LapisData::cellMutex(cell_t cell)
	{
		return (*_cellMuts)[cell % _cellMutCount];
	}

	std::mutex& LapisData::globalMutex()
	{
		return _globalMut;
	}

	std::vector<PointMetricRaster>& LapisData::allReturnPointMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._allReturnPointMetrics;
	}

	std::vector<PointMetricRaster>& LapisData::firstReturnPointMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._firstReturnPointMetrics;
	}

	std::vector<StratumMetricRasters>& LapisData::allReturnStratumMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._allReturnStratumMetrics;
	}

	std::vector<StratumMetricRasters>& LapisData::firstReturnStratumMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._firstReturnStratumMetrics;
	}

	std::vector<CSMMetric>& LapisData::csmMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._csmMetrics;
	}

	std::vector<TopoMetric>& LapisData::topoMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._topoMetrics;
	}

	shared_raster<coord_t> LapisData::elevNum()
	{
		return _getRawParam<ParamName::optionalMetrics>()._elevNumerator;
	}

	shared_raster<coord_t> LapisData::elevDenom()
	{
		return _getRawParam<ParamName::optionalMetrics>()._elevDenominator;
	}

	const std::vector<std::shared_ptr<LasFilter>>& LapisData::filters()
	{
		return _getRawParam<ParamName::filterOptions>()._filters;
	}

	coord_t LapisData::minHt()
	{
		return _getRawParam<ParamName::filterOptions>()._minhtCache;
	}

	coord_t LapisData::maxHt()
	{
		return _getRawParam<ParamName::filterOptions>()._maxhtCache;
	}

	const CoordRef& LapisData::lasCrsOverride()
	{
		return _getRawParam<ParamName::lasOverride>()._spec.crs;
	}

	const CoordRef& LapisData::demCrsOverride()
	{
		return _getRawParam<ParamName::demOverride>()._spec.crs;
	}

	const Unit& LapisData::lasUnitOverride()
	{
		return _getRawParam<ParamName::lasOverride>()._spec.crs.getZUnits();
	}

	const Unit& LapisData::demUnitOverride()
	{
		return _getRawParam<ParamName::demOverride>()._spec.crs.getZUnits();
	}

	coord_t LapisData::footprintDiameter()
	{
		return _getRawParam<ParamName::csmOptions>()._footprintDiamCache;
	}

	int LapisData::smooth()
	{
		return _getRawParam<ParamName::csmOptions>()._smooth;
	}

	const std::vector<LasFileExtent>& LapisData::sortedLasList()
	{
		return _getRawParam<ParamName::las>()._fileExtents;
	}

	const std::set<DemFileAlignment>& LapisData::demList()
	{
		return _getRawParam<ParamName::dem>()._fileAligns;
	}

	int LapisData::nThread()
	{
		return _getRawParam<ParamName::computerOptions>()._threadCache;
	}

	coord_t LapisData::binSize()
	{
		bool perf = _getRawParam<ParamName::computerOptions>()._perfCheck;
		coord_t x = perf ? 0.1 : 0.01;
		x = convertUnits(x, linearUnitDefs::meter, getCurrentUnits());
		return x;
	}

	size_t LapisData::csmFileSize()
	{
		return 500ll * 1024 * 1024; //500 MB
	}

	coord_t LapisData::canopyCutoff()
	{
		return _getRawParam<ParamName::metricOptions>()._canopyCache;
	}

	const std::vector<coord_t>& LapisData::strataBreaks()
	{
		return _getRawParam<ParamName::metricOptions>()._strataCache;
	}

	const std::filesystem::path& LapisData::outFolder()
	{
		return _getRawParam<ParamName::output>()._path;
	}

	const std::string& LapisData::name()
	{
		return _getRawParam<ParamName::name>()._runString;
	}

	bool LapisData::doFirstReturnMetrics()
	{
		return true;
	}

	bool LapisData::doAllReturnMetrics()
	{
		return true;
	}

	bool LapisData::doCsm()
	{
		return true;
	}

	bool LapisData::doTaos()
	{
		return true;
	}

	bool LapisData::doFineInt()
	{
		return _getRawParam<ParamName::optionalMetrics>()._fineIntCheck;
	}

	bool LapisData::doTopo()
	{
		return true;
	}

	bool LapisData::doAdvPointMetrics()
	{
		return _getRawParam<ParamName::optionalMetrics>()._advPointCheck;
	}

	LapisData::ParseResults LapisData::parseArgs(const std::vector<std::string>& args)
	{
		try {
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
			for (size_t i = 0; i < _params.size(); ++i) {
				_params[i]->addToCmd(visibleOpts, hiddenOpts);
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
		}
		catch (po::error_with_option_name e) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Error reading parameters: ");
			log.logMessage(e.what());
			return ParseResults::invalidOpts;
		}
		return ParseResults::validOpts;
	}
	LapisData::ParseResults LapisData::parseIni(const std::string& path)
	{
		try {
			po::options_description visibleOpts;
			po::options_description hiddenOpts;
			for (size_t i = 0; i < _params.size(); ++i) {
				_params[i]->addToCmd(visibleOpts, hiddenOpts);
			}

			po::options_description iniOptions;
			iniOptions
				.add(visibleOpts)
				.add(hiddenOpts)
				;

			po::variables_map vmFull;
			po::store(po::parse_config_file(path.c_str(), iniOptions), vmFull);
			po::notify(vmFull);
			return ParseResults::validOpts;
		}
		catch (po::error e) {
			return ParseResults::invalidOpts;
		}
	}
	std::ostream& LapisData::writeOptions(std::ostream& out, ParamCategory cat)
	{
		switch (cat) {
		case ParamCategory::computer:
			out << "#Computer-specific options\n";
			break;
		case ParamCategory::data:
			out << "#Data options\n";
			break;
		case ParamCategory::process:
			out << "#Processing options\n";
			break;
		}

		for (size_t i = 0; i < _params.size(); ++i) {
			if (cat==_params[i]->getCategory())
				_params[i]->printToIni(out);
		}
		return out;
	}
}