#include"app_pch.hpp"
#include"LapisData.hpp"

namespace lapis {
	LapisData& LapisData::getDataObject()
	{
		static LapisData d;
		return d;
	}
	LapisData::LapisData()
	{
		constexpr ParamName FIRSTPARAM = (ParamName)0;
		_addParam<FIRSTPARAM>();
		_prevUnits = linearUnitDefs::meter;
	}
	void LapisData::renderGui(ParamName pn)
	{
		_params[(size_t)pn]->renderGui();
	}
	void LapisData::setPrevUnits(const Unit& u)
	{
		_prevUnits = u;
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
	const std::set<std::string>& LapisData::getLas() const
	{
		auto& p = _getRawParam<ParamName::las>();
		return p._spec.getFileSpecsSet();
	}
	const std::set<std::string>& LapisData::getDem() const
	{
		auto& p = _getRawParam<ParamName::dem>();
		return p._spec.getFileSpecsSet();
	}
	const std::string& LapisData::getOutput() const
	{
		auto& p = _getRawParam<ParamName::output>();
		return p._string;
	}
	const Unit& LapisData::getLasUnits() const
	{
		auto& p = _getRawParam<ParamName::lasOverride>();
		return ParamBase::unitRadioOrder[p._spec.unitRadio];
	}
	const Unit& LapisData::getDemUnits() const
	{
		auto& p = _getRawParam<ParamName::demOverride>();
		return ParamBase::unitRadioOrder[p._spec.unitRadio];
	}
	CoordRef LapisData::getLasCrs() const
	{
		auto& p = _getRawParam<ParamName::lasOverride>();
		CoordRef crs;
		try {
			crs = CoordRef(p._spec.crsDisplayString);
		}
		catch (UnableToDeduceCRSException e) {}
		return crs;
	}
	CoordRef LapisData::getDemCrs() const
	{
		auto& p = _getRawParam<ParamName::demOverride>();
		CoordRef crs;
		try {
			crs = CoordRef(p._spec.crsDisplayString);
		}
		catch (UnableToDeduceCRSException e) {}
		return crs;
	}
	coord_t LapisData::getFootprintDiameter() const
	{
		auto& p = _getRawParam<ParamName::csmOptions>();
		return std::stod(p._footprintDiamBuffer.data());
	}
	int LapisData::getSmoothWindow() const
	{
		auto& p = _getRawParam<ParamName::csmOptions>();
		return p._smooth;
	}
	LapisData::AlignWithoutExtent LapisData::getAlign() const
	{
		auto& p = _getRawParam<ParamName::alignment>();
		AlignWithoutExtent a;
		a.xres = std::stod(p._xresBuffer.data());
		a.yres = std::stod(p._yresBuffer.data());
		a.xorigin = std::stod(p._xoriginBuffer.data());
		a.yorigin = std::stod(p._yoriginBuffer.data());
		try {
			a.crs = CoordRef(p._crsDisplayString);
		}
		catch (UnableToDeduceCRSException e) {}
		a.crs.setZUnits(getUserUnits());
		return a;
	}
	coord_t LapisData::getCSMCellsize() const
	{
		auto& p = _getRawParam<ParamName::csmOptions>();
		return std::stod(p._csmCellsizeBuffer.data());
	}
	const Unit& LapisData::getUserUnits() const
	{
		auto& p = _getRawParam<ParamName::outUnits>();
		return ParamBase::unitRadioOrder[p._radio];
	}
	coord_t LapisData::getCanopyCutoff() const
	{
		auto& p = _getRawParam<ParamName::metricOptions>();
		return std::stod(p._canopyBuffer.data());
	}
	coord_t LapisData::getMinHt() const
	{
		auto& p = _getRawParam<ParamName::filterOptions>();
		return std::stod(p._minhtBuffer.data());
	}
	coord_t LapisData::getMaxHt() const
	{
		auto& p = _getRawParam<ParamName::filterOptions>();
		return std::stod(p._maxhtBuffer.data());
	}
	std::vector<coord_t> LapisData::getStrataBreaks() const
	{
		std::vector<coord_t> out;
		auto& p = _getRawParam<ParamName::metricOptions>();
		for (auto& b : p._strataBuffers) {
			out.push_back(std::stod(b.data()));
		}
		return out;
	}
	bool LapisData::getFineIntFlag() const
	{
		auto& p = _getRawParam<ParamName::optionalMetrics>();
		return p._fineIntCheck;
	}
	LapisData::ClassFilter LapisData::getClassFilter() const
	{
		auto& p = _getRawParam<ParamName::filterOptions>();
		ClassFilter out;
		int nallowed = 0;
		int nblocked = 0;
		for (size_t i = 0 ; i < p._classChecks.size(); ++i) {
			if (p._classChecks[i]) {
				nallowed++;
			}
			else {
				nblocked++;
			}
		}
		out.isWhiteList = (nallowed < nblocked);
		for (size_t i = 0; i < p._classChecks.size(); ++i) {
			if (p._classChecks[i] && out.isWhiteList) {
				out.list.insert((uint8_t)i);
			}
			else if (!p._classChecks[i] && !out.isWhiteList) {
				out.list.insert((uint8_t)i);
			}
		}
		return out;
	}
	bool LapisData::getUseWithheldFlag() const
	{
		auto& p = _getRawParam<ParamName::filterOptions>();
		return !p._filterWithheldCheck;
	}
	int8_t LapisData::getMaxScanAngle() const
	{
		return -1;
	}
	bool LapisData::getOnlyFlag() const
	{
		return false;
	}
	int LapisData::getNThread() const
	{
		auto& p = _getRawParam<ParamName::computerOptions>();
		return std::stoi(p._threadBuffer.data());
	}
	bool LapisData::getPerformanceFlag() const
	{
		auto& p = _getRawParam<ParamName::computerOptions>();
		return p._perfCheck;
	}
	std::string LapisData::getName() const
	{
		auto& p = _getRawParam<ParamName::name>();
		return std::string(p._nameBuffer.data());
	}
	bool LapisData::getGdalProjWarningFlag() const
	{
		auto& p = _getRawParam<ParamName::computerOptions>();
		return p._gdalprojCheck;
	}
	bool LapisData::getAdvancedPointFlag() const
	{
		auto& p = _getRawParam<ParamName::optionalMetrics>();
		return p._advPointCheck;
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
		catch (po::error e) {
			std::cout << e.what() << "\n";
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