#include"app_pch.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"

namespace lapis {
	LapisData& LapisData::getDataObject()
	{
		static LapisData d;
		return d;
	}
	LapisData::LapisData() : needAbort(false)
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
			if (needAbort) {
				return;
			}
		}
		_cellMuts = std::make_unique<std::vector<std::mutex>>(_cellMutCount);

		LapisLogger& log = LapisLogger::getLogger();

		double mem = estimateMemory();
		mem = std::round(mem * 100.) / 100.;
		std::ostringstream message;
		message.precision(1);
		message << std::fixed;
		message << "Estimated memory usage: " << mem << " GB";
		log.logMessage(message.str());

		double hdd = estimateHardDrive();
		message.str("");
		message << "Estimated HDD space needed: " << hdd << " GB";
		log.logMessage(message.str());

		double avail = static_cast<double>(std::filesystem::space(outFolder()).available);
		avail /= 1024. * 1024. * 1024.;
		message.str("");
		message << "Hard drive space available: " << avail << " GB";
		log.logMessage(message.str());
	}

	void LapisData::cleanAfterRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->cleanAfterRun();
		}
		_cellMuts.reset();
		needAbort = false;
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
		needAbort = false;
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
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Error in ini file " + path);
			log.logMessage(e.what());
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

	double LapisData::estimateMemory()
	{
		cell_t metricNCell = metricAlign()->ncell();
		double metricMem = 0;
		metricMem += sizeof(int) * metricNCell; //nlaz
		metricMem += allReturnPointMetrics().size() * sizeof(metric_t) * metricNCell;
		metricMem += firstReturnPointMetrics().size() * sizeof(metric_t) * metricNCell;
		metricMem += allReturnStratumMetrics().size() * (strataBreaks().size() + 1) * sizeof(metric_t) * metricNCell;
		metricMem += firstReturnStratumMetrics().size() * (strataBreaks().size() + 1) * sizeof(metric_t) * metricNCell;
		metricMem += csmMetrics().size() * sizeof(metric_t) * metricNCell;
		metricMem += topoMetrics().size() * sizeof(metric_t) * metricNCell;
		if (doTopo())
			metricMem += sizeof(coord_t) * metricNCell * 2; //elevation numerator and denominator
		if (doAllReturnMetrics())
			metricMem += sizeof(PointMetricCalculator) * metricNCell;
		if (doFirstReturnMetrics())
			metricMem += sizeof(PointMetricCalculator) * metricNCell;

		double perThread = 0;
		double meanPointsPerTile = 0;
		double meanArea = 0;
		for (auto& l : sortedLasList()) {
			meanPointsPerTile += l.ext.nPoints();
			meanArea += (l.ext.ymax() - l.ext.ymin()) * (l.ext.xmax() - l.ext.xmin());
		}
		meanPointsPerTile /= sortedLasList().size();
		meanArea /= sortedLasList().size();
		perThread += sizeof(LasPoint) * meanPointsPerTile * 2; //while figuring out which points pass filters, a copy of the data is made

		coord_t demCellArea = std::numeric_limits<coord_t>::max();
		for (auto& d : demList()) {
			demCellArea = std::min(demCellArea, d.align.xres() * d.align.yres());
		}
		perThread += sizeof(coord_t) * meanArea / demCellArea;

		coord_t csmCellArea = csmAlign()->xres() * csmAlign()->yres();
		perThread += meanArea / csmCellArea * sizeof(csm_t);

		if (doTopo()) {
			perThread += meanArea / csmCellArea * sizeof(coord_t); //temporary csm-scale elevation raster
			perThread += sizeof(coord_t) * metricNCell * 2; //inputs to elevation numerator and denominator
		}

		if(doFineInt()) {
			perThread += meanArea / csmCellArea * sizeof(intensity_t) * 2;
		}

		double sparseHistEst = convertUnits(50, linearUnitDefs::meter, metricAlign()->crs().getXYUnits()) / binSize() * sizeof(int);
		sparseHistEst *= meanArea / (metricAlign()->xres() * metricAlign()->yres());
		perThread += sparseHistEst;

		perThread *= 1.1; //there's a ton of ephemeral memory usage and difficult-to-estimate stuff so I'll just inflate by 10% to be safe
		return (perThread * nThread() + metricMem) / 1024. / 1024. / 1024.;
	}
	double LapisData::estimateHardDrive()
	{
		cell_t metricNCell = 0;
		cell_t csmNCell = 0;

		//essentially the same logic as cellsFromExtent but doesn't allocate the actual list of cells
		auto nCellsInOverlap = [&](const Alignment& a, const Extent& e)->size_t {
			if (!a.overlaps(e)) {
				return 0;
			}
			Extent alignE = a.alignExtent(e, SnapType::out);
			alignE = crop(alignE, a);

			//Bringing the extent in slightly so you don't have to deal with the edge of the extent aligning with the edges of this object's cells.
			alignE = Extent(alignE.xmin() + 0.25 * a.xres(), alignE.xmax() - 0.25 * a.xres(),
				alignE.ymin() + 0.25 * a.yres(), alignE.ymax() - 0.25 * a.yres());

			//The mins and maxes are *inclusive*
			size_t mincol = a.colFromX(alignE.xmin());
			size_t maxcol = a.colFromX(alignE.xmax());
			size_t minrow = a.rowFromY(alignE.ymax());
			size_t maxrow = a.rowFromY(alignE.ymin());
			return (maxcol - mincol + 1) * (maxrow - minrow + 1);
		};

		//tifs are good at compressing large blocks of nodata so I'm going to pretend nodata has no hard drive usage
		//I'm wildly overestimating everywhere else so it should be fine
		for (auto& l : sortedLasList()) {
			metricNCell += nCellsInOverlap(*metricAlign(), l.ext);
			csmNCell += nCellsInOverlap(*csmAlign(), l.ext);
		}

		double usage = 0;
		if (doCsm()) {
			usage += csmNCell * sizeof(csm_t) * 2;
			usage += metricNCell * sizeof(metric_t) * csmMetrics().size();
		}
		if (doAllReturnMetrics()) {
			usage += metricNCell * sizeof(metric_t) * allReturnPointMetrics().size();
			usage += metricNCell * sizeof(metric_t) * allReturnStratumMetrics().size() * (strataBreaks().size() + 1);
		}
		if (doFirstReturnMetrics()) {
			usage += metricNCell * sizeof(metric_t) * firstReturnPointMetrics().size();
			usage += metricNCell * sizeof(metric_t) * firstReturnStratumMetrics().size() * (strataBreaks().size() + 1);
		}

		if (doTopo()) {
			usage += metricNCell * sizeof(metric_t) * (topoMetrics().size() + 1);
		}
		
		if (doFineInt()) {
			usage += csmNCell * sizeof(intensity_t) * 3;
		}
		if (doTaos()) {
			usage += csmNCell * (sizeof(csm_t) + 2 * sizeof(taoid_t));

			//this estimate is a bit of hack and way overestimating, to get at the size of the high point polygons.
			//estimating 3% of points are TAOs (empirical 1%) and each TAO uses 200 bytes (empirical 177)
			usage += csmNCell * 0.03 * 200;
		}
		return usage / 1024. / 1024. / 1024.;
	}
}