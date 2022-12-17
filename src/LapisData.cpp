#include"app_pch.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"
#include"LapisParameters.hpp"
#include"DemAlgos.hpp"

namespace lapis {


	LapisData& LapisData::getDataObject()
	{
		static LapisData d;
		return d;
	}
	LapisData::LapisData() : needAbort(false)
	{
#ifndef NDEBUG
		CPLSetErrorHandler(LapisData::logGDALErrors);
		proj_log_func(ProjContextByThread::get(), nullptr, LapisData::logProjErrors);
#else
		CPLSetErrorHandler(LapisData::silenceGDALErrors);
		proj_log_level(ProjContextByThread::get(), PJ_LOG_NONE);
#endif

		constexpr ParamName FIRSTPARAM = (ParamName)0;
		_addParam<FIRSTPARAM>();
		_prevUnits = LinearUnitDefs::meter;
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
		return _getRawParam<ParamName::outUnits>().unit();
	}
	const Unit& LapisData::getPrevUnits() const
	{
		return _prevUnits;
	}
	const std::string& LapisData::getUnitSingular() const
	{
		return _getRawParam<ParamName::outUnits>().unitSingularName();
	}
	const std::string& LapisData::getUnitPlural() const
	{
		return _getRawParam<ParamName::outUnits>().unitPluralName();
	}
	void LapisData::importBoostAndUpdateUnits()
	{
		//this slightly odd way of ordering things is to ensure that only parameters
		//that weren't specified in the same ini file that changed the units get converted
		_getRawParam<ParamName::outUnits>().importFromBoost();
		updateUnits();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
	}
	void LapisData::updateUnits() {
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->updateUnits();
		}
		setPrevUnits(getCurrentUnits());
	}
	void LapisData::prepareForRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
#ifndef NDEBUG
			LapisLogger::getLogger().logMessage("Preparing parameter: " + std::to_string(i));
#endif
			_params[i]->prepareForRun();
			if (needAbort) {
				return;
			}
		}
		_cellMuts = std::make_unique<std::vector<std::mutex>>(_cellMutCount);

		if (isAnyDebug()) {
			needAbort = true;
		}

		cell_t targetNCell = tileFileSize() / std::max(sizeof(csm_t), sizeof(intensity_t));
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		coord_t fineCellSize = csmAlign()->xres();
		if (doFineInt()) {
			fineCellSize = std::min(fineCellSize, fineIntAlign()->xres());
		}
		coord_t tileRes = targetNRowCol * fineCellSize;
		Alignment layoutAlign{ *csmAlign(),0,0,tileRes,tileRes};
		if (doFineInt()) {
			layoutAlign = extend(layoutAlign, *fineIntAlign());
		}
		_layout = std::make_shared<Raster<bool>>(layoutAlign);

		_logMemoryAndHDDUse();
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
		_prevUnits = LinearUnitDefs::meter;
		_cellMuts.reset();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
		needAbort = false;
	}

	Extent LapisData::fullExtent()
	{
		return _getRawParam<ParamName::las>().getFullExtent();
	}

	const CoordRef& LapisData::userCrs()
	{
		return _getRawParam<ParamName::alignment>().getCurrentOutCrs();
	}

	std::shared_ptr<Alignment> LapisData::metricAlign()
	{
		return _getRawParam<ParamName::alignment>().metricAlign();
	}
	std::shared_ptr<Alignment> LapisData::csmAlign()
	{
		return _getRawParam<ParamName::csmOptions>().csmAlign();
	}
	std::shared_ptr<Alignment> LapisData::fineIntAlign() {
		return _getRawParam<ParamName::fineIntOptions>().fineIntAlign();
	}
	shared_raster<bool> LapisData::layout()
	{
		prepareForRun();
		return _layout;
	}
	shared_raster<int> LapisData::nLazRaster()
	{
		return _getRawParam<ParamName::alignment>().nLaz();
	}
	shared_raster<PointMetricCalculator> LapisData::allReturnPMC()
	{
		return _getRawParam<ParamName::pointMetricOptions>().allReturnCalculators();
	}
	shared_raster<PointMetricCalculator> LapisData::firstReturnPMC()
	{
		return _getRawParam<ParamName::pointMetricOptions>().firstReturnCalculators();
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
		return _getRawParam<ParamName::pointMetricOptions>().allReturnPointMetrics();
	}
	std::vector<PointMetricRaster>& LapisData::firstReturnPointMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().firstReturnPointMetrics();
	}
	std::vector<StratumMetricRasters>& LapisData::allReturnStratumMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().allReturnStratumMetrics();
	}
	std::vector<StratumMetricRasters>& LapisData::firstReturnStratumMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().firstReturnStratumMetrics();
	}
	std::vector<CSMMetric>& LapisData::csmMetrics()
	{
		return _getRawParam<ParamName::csmOptions>().csmMetrics();
	}
	std::vector<TopoMetric>& LapisData::topoMetrics()
	{
		return _getRawParam<ParamName::topoOptions>().topoMetrics();
	}
	shared_raster<coord_t> LapisData::elevNum()
	{
		return _getRawParam<ParamName::topoOptions>().elevNumerator();
	}
	shared_raster<coord_t> LapisData::elevDenom()
	{
		return _getRawParam<ParamName::topoOptions>().elevDenominator();
	}
	const std::vector<std::shared_ptr<LasFilter>>& LapisData::filters()
	{
		return _getRawParam<ParamName::filters>().filters();
	}
	coord_t LapisData::minHt()
	{
		return _getRawParam<ParamName::filters>().minht();
	}
	coord_t LapisData::maxHt()
	{
		return _getRawParam<ParamName::filters>().maxht();
	}
	const CoordRef& LapisData::lasCrsOverride()
	{
		return _getRawParam<ParamName::lasOverride>().crs();
	}
	const CoordRef& LapisData::demCrsOverride()
	{
		return _getRawParam<ParamName::demOverride>().crs();
	}
	const Unit& LapisData::lasUnitOverride()
	{
		return _getRawParam<ParamName::lasOverride>().zUnits();
	}
	const Unit& LapisData::demUnitOverride()
	{
		return _getRawParam<ParamName::demOverride>().zUnits();
	}
	coord_t LapisData::footprintDiameter()
	{
		return _getRawParam<ParamName::csmOptions>().footprintDiameter();
	}
	int LapisData::smooth()
	{
		return _getRawParam<ParamName::csmOptions>().smooth();
	}
	const std::vector<LasFileExtent>& LapisData::sortedLasList()
	{
		return _getRawParam<ParamName::las>().sortedLasExtents();
	}
	const std::set<DemFileAlignment>& LapisData::demList()
	{
		return _getRawParam<ParamName::dem>().demList();
	}
	std::shared_ptr<DemAlgorithm> LapisData::demAlgorithm()
	{
		static std::shared_ptr<DemAlgorithm> onlyAlgoRightNow = std::make_shared<VendorRaster>();
		return onlyAlgoRightNow;
	}
	int LapisData::nThread()
	{
		return _getRawParam<ParamName::computerOptions>().nThread();
	}
	coord_t LapisData::binSize()
	{
		return convertUnits(0.01, LinearUnitDefs::meter, getCurrentUnits());
	}
	size_t LapisData::tileFileSize()
	{
		return 500ll * 1024 * 1024; //500 MB
	}
	coord_t LapisData::canopyCutoff()
	{
		return _getRawParam<ParamName::pointMetricOptions>().canopyCutoff();
	}
	coord_t LapisData::minTaoHt() {
		return _getRawParam<ParamName::taoOptions>().minTaoHt();
	}
	coord_t LapisData::minTaoDist() {
		return _getRawParam<ParamName::taoOptions>().minTaoDist();
	}
	const std::vector<coord_t>& LapisData::strataBreaks()
	{
		static std::vector<coord_t> empty;
		return doStratumMetrics() ? _getRawParam<ParamName::pointMetricOptions>().strata() : empty;
	}
	std::filesystem::path LapisData::outFolder()
	{
		return _getRawParam<ParamName::output>().path();
	}
	std::string LapisData::name()
	{
		return _getRawParam<ParamName::name>().name();
	}
	coord_t LapisData::fineIntCanopyCutoff() {
		return _getRawParam<ParamName::fineIntOptions>().fineIntCutoff();
	}

	bool LapisData::doPointMetrics() {
		return _getRawParam<ParamName::whichProducts>().doPointMetrics();
	}
	bool LapisData::doFirstReturnMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().doFirstReturns();
	}
	bool LapisData::doAllReturnMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().doAllReturns();
	}
	bool LapisData::doCsm()
	{
		return _getRawParam<ParamName::whichProducts>().doCsm();
	}
	bool LapisData::doTaos()
	{
		return doCsm() && _getRawParam<ParamName::whichProducts>().doTao();
	}
	bool LapisData::doFineInt()
	{
		return _getRawParam<ParamName::whichProducts>().doFineInt();
	}
	bool LapisData::doTopo()
	{
		return _getRawParam<ParamName::whichProducts>().doTopo();
	}

	bool LapisData::doStratumMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().doStratumMetrics();
	}

	bool LapisData::isDebugNoAlloc() {
		return _getRawParam<ParamName::whichProducts>().isDebug();
	}

	bool LapisData::isDebugNoAlign()
	{
		return _getRawParam<ParamName::alignment>().isDebug();
	}

	bool LapisData::isDebugNoOutput()
	{
		return _getRawParam<ParamName::output>().isDebugNoOutput();
	}

	bool LapisData::isAnyDebug()
	{
		return isDebugNoAlign() || isDebugNoAlloc() || isDebugNoOutput();
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
		importBoostAndUpdateUnits();
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
			importBoostAndUpdateUnits();
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

	inline void LapisData::logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {
		LapisLogger::getLogger().logMessage(pszErrorMsg);
	}

	inline void LapisData::logProjErrors(void* v, int i, const char* c) {
		LapisLogger::getLogger().logMessage(c);
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

		double sparseHistEst = convertUnits(50, LinearUnitDefs::meter, metricAlign()->crs().getXYUnits()) / binSize() * sizeof(int);
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

	LapisData::DataIssues LapisData::checkForDataIssues() {
		LapisLogger& log = LapisLogger::getLogger();
		DataIssues out;

		_failedLas.clear();
		std::set<LasFileExtent> lasSet = iterateOverFileSpecifiers(_getRawParam<ParamName::las>().currentFileSpecifiers(),
			tryLasFile, CoordRef(), Unit());

		out.failedLas = _failedLas;

		for (auto& v : lasSet) {
			out.successfulLas.emplace_back(v.filename);
			size_t idx = out.successfulLas.size() - 1;

			//error checking here is more out of paranoia than an expectation of issues; all of these files opened successfully just a few lines earlier
			try {
				LasIO l{ v.filename };
				uint16_t year = l.header.FileCreationYear;
				CoordRef crs{ v.filename };
				out.byFileYear.try_emplace(year);
				out.byFileYear.at(year).insert(idx);
				out.byCrs.try_emplace(crs);
				out.byCrs.at(crs).insert(idx);
			}
			catch (InvalidLasFileException e) {
				out.byFileYear.try_emplace(0);
				out.byFileYear.at(0).insert(idx);
				out.byCrs.try_emplace(CoordRef());
				out.byCrs.at(CoordRef()).insert(idx);
			}
			catch (UnableToDeduceCRSException e) {
				out.byFileYear.try_emplace(0);
				out.byFileYear.at(0).insert(idx);
				out.byCrs.try_emplace(CoordRef());
				out.byCrs.at(CoordRef()).insert(idx);
			}
		}

		CoordRef demOver = _getRawParam<ParamName::demOverride>().crs();
		std::set<DemFileAlignment> demSet = iterateOverFileSpecifiers(_getRawParam<ParamName::dem>().currentFileSpecifiers(),
			tryDtmFile, demOver, demOver.getZUnits());
		size_t idx = -1;
		for (auto& d : demSet) {
			++idx;
			out.successfulDem.push_back(d.filename);
			out.demByCrs.try_emplace(d.align.crs());
			out.demByCrs.at(d.align.crs()).insert(idx);
		}

		_checkLasDemOverlap(out, lasSet, demSet);

		if (out.successfulLas.size()) {
			_checkSampleLas(out, out.successfulLas[0], demSet);
		}
		return out;
	}

	void LapisData::_checkLasDemOverlap(DataIssues& di, const std::set<LasFileExtent>& las, const std::set<DemFileAlignment>& dem)
	{

		CoordRef outCrs = _getRawParam<ParamName::alignment>().getCurrentOutCrs();
		CoordRef lasOverCrs = _getRawParam<ParamName::lasOverride>().crs();
		CoordRef demOverCrs = _getRawParam<ParamName::demOverride>().crs();

		//If there are multiple las CRSes present, and some of them are undefined, trying to figure out the output alignment is nonsense
		if (lasOverCrs.isEmpty() && di.byCrs.size() > 1 && di.byCrs.contains(CoordRef())) {
			return;
		}
		if (las.size() == 0) {
			return;
		}
		if (dem.size() == 0) {
			return;
		}

		std::vector<Extent> lasExtents;
		for (auto& e : las) {
			lasExtents.push_back(e.ext);
			if (!lasOverCrs.isEmpty()) {
				lasExtents[lasExtents.size() - 1].defineCRS(lasOverCrs);
			}
		}

		std::vector<Extent> demExtents;
		for (auto& e : dem) {
			demExtents.push_back(e.align);
			if (!demOverCrs.isEmpty()) {
				demExtents[demExtents.size() - 1].defineCRS(demOverCrs);
			}
		}

		if (outCrs.isEmpty()) {
			outCrs = lasExtents[0].crs();
		}

		
		lasExtents[0] = QuadExtent(lasExtents[0], outCrs).outerExtent();
		Extent fullExtent = lasExtents[0];
		for (size_t i = 1; i < lasExtents.size(); ++i) {
			lasExtents[i] = QuadExtent(lasExtents[i], outCrs).outerExtent();
			fullExtent = extend(fullExtent, lasExtents[i]);
		}
		coord_t cellsize = convertUnits(30, LinearUnitDefs::meter, outCrs.getXYUnits());
		Alignment fullAlign = Alignment(fullExtent, 0, 0, cellsize, cellsize);
		Raster<bool> lasCovers{ fullAlign };
		Raster<bool> demCovers{ fullAlign };

		for (auto& e : lasExtents) {
			Extent projE = QuadExtent(e, lasCovers.crs()).outerExtent();
			for (auto& cell : fullAlign.cellsFromExtent(projE)) {
				lasCovers[cell].value() = true;
			}
		}
		for (auto& e : demExtents) {
			Extent projE = QuadExtent(e, lasCovers.crs()).outerExtent();
			for (auto& cell : fullAlign.cellsFromExtent(projE)) {
				demCovers[cell].value() = true;
			}
		}

		for (cell_t cell = 0; cell < fullAlign.ncell(); ++cell) {
			if (lasCovers[cell].value()) {
				di.cellsInLas++;
				if (demCovers[cell].value()) {
					di.cellsInDem++;
				}
			}
		}


		for (size_t i = 0; i < lasExtents.size(); ++i) {
			Extent& thisExtent = lasExtents[i];
			di.totalArea += (thisExtent.ymax() - thisExtent.ymin()) * (thisExtent.xmax() - thisExtent.xmin());
			for (size_t j = i + 1; j < lasExtents.size(); ++j) {
				Extent& compareExtent = lasExtents[j];
				if (thisExtent.overlaps(compareExtent)) {
					Extent cr = crop(thisExtent, compareExtent);
					di.overlapArea += (cr.ymax() - cr.ymin()) * (cr.xmax() - cr.xmin());
				}
			}
		}
	}

	void LapisData::_checkSampleLas(DataIssues& di, const std::string& filename, const std::set<DemFileAlignment>& demSet) {
		auto& filterParam = _getRawParam<ParamName::filters>();
		filterParam.prepareForRun();
		auto& filtersVec = filters();

		LasReader sampleLas{ filename };
		if (!lasCrsOverride().isEmpty()) {
			sampleLas.defineCRS(lasCrsOverride());
		}
		if (!lasUnitOverride().isUnknown()) {
			sampleLas.setZUnits(lasUnitOverride());
		}

		for (auto& filter : filtersVec) {
			sampleLas.addFilter(filter);
		}
		di.pointsInSample = sampleLas.nPoints();

		LidarPointVector pointsAfterFilters = sampleLas.getPoints(di.pointsInSample);
		di.pointsAfterFilters = pointsAfterFilters.size();
		di.pointsAfterDem = demAlgorithm()->normalizeToGround(pointsAfterFilters, sampleLas).points.size();

		pointsAfterFilters.clear();
		pointsAfterFilters.shrink_to_fit();

		filterParam.cleanAfterRun();
	}

	void LapisData::_logMemoryAndHDDUse()
	{
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


		if (std::filesystem::is_directory(outFolder())) { //this should always be true unless a specific debug parameter is passed
			double avail = static_cast<double>(std::filesystem::space(outFolder()).available);
			avail /= 1024. * 1024. * 1024.;
			message.str("");
			message << "Hard drive space available: " << avail << " GB";
			log.logMessage(message.str());
		}
	}

	void LapisData::reportFailedLas(const std::string& s)
	{
		_failedLas.push_back(s);
	}
}