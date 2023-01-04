#include"param_pch.hpp"
#include"RunParameters.hpp"
#include"AllParameters.hpp"
#include"..\logger\LapisLogger.hpp"
#include"LapisGui.hpp"

namespace lapis {


	RunParameters& RunParameters::singleton()
	{
		static RunParameters d;
		return d;
	}
	size_t RunParameters::registerParameter(Parameter* param)
	{
		_params.emplace_back(param);
		return _params.size() - 1;
	}
	RunParameters::RunParameters()
	{
#ifndef NDEBUG
		CPLSetErrorHandler(&RunParameters::logGDALErrors);
		proj_log_func(ProjContextByThread::get(), nullptr, &RunParameters::logProjErrors);
#else
		CPLSetErrorHandler(&RunParameters::silenceGDALErrors);
		proj_log_level(ProjContextByThread::get(), PJ_LOG_NONE);
#endif
	}
	void RunParameters::setPrevUnits(const Unit& u)
	{
		_prevUnits = u;
	}
	const Unit& RunParameters::outUnits() 
	{
		return _getRawParam<OutUnitParameter>().unit();
	}
	const Unit& RunParameters::prevUnits() 
	{
		return _prevUnits;
	}
	const std::string& RunParameters::unitSingular() 
	{
		return _getRawParam<OutUnitParameter>().unitSingularName();
	}
	const std::string& RunParameters::unitPlural() 
	{
		return _getRawParam<OutUnitParameter>().unitPluralName();
	}
	void RunParameters::importBoostAndUpdateUnits()
	{
		//this slightly odd way of ordering things is to ensure that only parameters
		//that weren't specified in the same ini file that changed the units get converted
		_getRawParam<OutUnitParameter>().importFromBoost();
		updateUnits();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->importFromBoost();
		}
	}
	void RunParameters::updateUnits() {
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->updateUnits();
		}
		setPrevUnits(outUnits());
	}
	bool RunParameters::prepareForRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			if (!_params[i]->prepareForRun()) {
				LapisLogger::getLogger().logMessage("Aborting");
				return false;
			}
		}
		_cellMuts = std::make_unique<std::vector<std::mutex>>(_cellMutCount);

		cell_t targetNCell = tileFileSize() / std::max(sizeof(csm_t), sizeof(intensity_t));
		rowcol_t targetNRowCol = (rowcol_t)std::sqrt(targetNCell);
		coord_t fineCellSize = csmAlign()->xres();
		if (doFineInt()) {
			fineCellSize = std::min(fineCellSize, fineIntAlign()->xres());
		}
		coord_t tileRes = targetNRowCol * fineCellSize;
		Alignment layoutAlign{ *csmAlign(),0,0,tileRes,tileRes};
		if (doFineInt()) {
			layoutAlign = extendAlignment(layoutAlign, *fineIntAlign(), SnapType::out);
		}
		_layout = std::make_shared<Raster<bool>>(layoutAlign);
		return true;
	}
	void RunParameters::cleanAfterRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->cleanAfterRun();
		}
		_cellMuts.reset();
	}
	void RunParameters::resetObject() {

		_prevUnits = LinearUnitDefs::meter;
		_cellMuts.reset();
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->reset();
			_params[i]->importFromBoost();
		}
	}

	const Extent& RunParameters::fullExtent()
	{
		return _getRawParam<LasFileParameter>().getFullExtent();
	}

	const CoordRef& RunParameters::userCrs() 
	{
		return _getRawParam<AlignmentParameter>().getCurrentOutCrs();
	}

	const std::shared_ptr<Alignment> RunParameters::metricAlign()
	{
		return _getRawParam<AlignmentParameter>().metricAlign();
	}
	const std::shared_ptr<Alignment> RunParameters::csmAlign()
	{
		return _getRawParam<CsmParameter>().csmAlign();
	}
	const std::shared_ptr<Alignment> RunParameters::fineIntAlign() {
		return _getRawParam<FineIntParameter>().fineIntAlign();
	}
	std::shared_ptr<Raster<bool>> RunParameters::layout()
	{
		return _layout;
	}

	std::string RunParameters::layoutTileName(cell_t tile)
	{
		auto insertZeroes = [](int value, int maxvalue)->std::string
		{
			size_t nDigits = (size_t)(std::log10(maxvalue) + 1);
			size_t thisDigitCount = std::max(1ull, (size_t)(std::log10(value) + 1));
			return std::string(nDigits - thisDigitCount, '0') + std::to_string(value);
		};

		return "Col" + insertZeroes(layout()->colFromCell(tile) + 1, layout()->ncol()) +
			"_Row" + insertZeroes(layout()->rowFromCell(tile) + 1, layout()->nrow());
	}

	std::mutex& RunParameters::cellMutex(cell_t cell)
	{
		return (*_cellMuts)[cell % _cellMutCount];
	}
	std::mutex& RunParameters::globalMutex()
	{
		return _globalMut;
	}


	const std::vector<std::shared_ptr<LasFilter>>& RunParameters::filters()
	{
		return _getRawParam<FilterParameter>().filters();
	}
	coord_t RunParameters::minHt() 
	{
		return _getRawParam<FilterParameter>().minht();
	}
	coord_t RunParameters::maxHt() 
	{
		return _getRawParam<FilterParameter>().maxht();
	}

	CsmAlgorithm* RunParameters::csmAlgorithm()
	{
		return _getRawParam<CsmParameter>().csmAlgorithm();
	}

	CsmPostProcessor* RunParameters::csmPostProcessAlgorithm()
	{
		return _getRawParam<CsmParameter>().csmPostProcessor();
	}

	const std::vector<Extent>& RunParameters::lasExtents()
	{
		return _getRawParam<LasFileParameter>().sortedLasExtents();
	}
	LasReader RunParameters::getLas(size_t i)
	{
		return _getRawParam<LasFileParameter>().getLas(i);
	}
	DemAlgorithm* RunParameters::demAlgorithm() 
	{
		return _getRawParam<DemParameter>().demAlgorithm();
	}
	int RunParameters::nThread() 
	{
		return _getRawParam<ComputerParameter>().nThread();
	}
	coord_t RunParameters::binSize()
	{
		return convertUnits(0.01, LinearUnitDefs::meter, outUnits());
	}
	size_t RunParameters::tileFileSize() 
	{
		return 500ll * 1024 * 1024; //500 MB
	}
	coord_t RunParameters::canopyCutoff() 
	{
		return _getRawParam<PointMetricParameter>().canopyCutoff();
	}
	const std::vector<coord_t>& RunParameters::strataBreaks() 
	{
		static std::vector<coord_t> empty;
		return doStratumMetrics() ? _getRawParam<PointMetricParameter>().strata() : empty;
	}
	const std::vector<std::string>& RunParameters::strataNames()
	{
		return _getRawParam<PointMetricParameter>().strataNames();
	}
	TaoIdAlgorithm* RunParameters::taoIdAlgorithm()
	{
		return _getRawParam<TaoParameter>().taoIdAlgo();
	}
	TaoSegmentAlgorithm* RunParameters::taoSegAlgorithm()
	{
		return _getRawParam<TaoParameter>().taoSegAlgo();
	}
	const std::filesystem::path& RunParameters::outFolder()
	{
		return _getRawParam<OutputParameter>().path();
	}
	const std::string& RunParameters::name()
	{
		return _getRawParam<NameParameter>().name();
	}
	coord_t RunParameters::fineIntCanopyCutoff() {
		return _getRawParam<FineIntParameter>().fineIntCutoff();
	}

	bool RunParameters::doPointMetrics() {
		return _getRawParam<WhichProductsParameter>().doPointMetrics();
	}
	bool RunParameters::doFirstReturnMetrics()
	{
		return _getRawParam<PointMetricParameter>().doFirstReturns();
	}
	bool RunParameters::doAllReturnMetrics()
	{
		return _getRawParam<PointMetricParameter>().doAllReturns();
	}
	bool RunParameters::doAdvancedPointMetrics()
	{
		return _getRawParam<PointMetricParameter>().doAdvancedPointMetrics();
	}
	bool RunParameters::doCsm()
	{
		return _getRawParam<WhichProductsParameter>().doCsm();
	}
	bool RunParameters::doCsmMetrics()
	{
		return _getRawParam<CsmParameter>().doCsmMetrics();
	}
	bool RunParameters::doTaos()
	{
		return doCsm() && _getRawParam<WhichProductsParameter>().doTao();
	}
	bool RunParameters::doFineInt()
	{
		return _getRawParam<WhichProductsParameter>().doFineInt();
	}
	bool RunParameters::doTopo()
	{
		return _getRawParam<WhichProductsParameter>().doTopo();
	}
	bool RunParameters::doStratumMetrics()
	{
		return _getRawParam<PointMetricParameter>().doStratumMetrics();
	}

	bool RunParameters::isDebugNoAlign()
	{
		return _getRawParam<AlignmentParameter>().isDebug();
	}
	bool RunParameters::isDebugNoOutput()
	{
		return _getRawParam<OutputParameter>().isDebugNoOutput();
	}
	bool RunParameters::isAnyDebug()
	{
		return isDebugNoAlign() || isDebugNoOutput();
	}

	RunParameters::ParseResults RunParameters::parseArgs(const std::vector<std::string>& args)
	{
		namespace po = boost::program_options;
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
	RunParameters::ParseResults RunParameters::parseIni(const std::string& path)
	{
		namespace po = boost::program_options;
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
	std::ostream& RunParameters::writeOptions(std::ostream& out, ParamCategory cat) const
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

	void RunParameters::logGDALErrors(CPLErr eErrClass, CPLErrorNum nError, const char* pszErrorMsg) {
		LapisLogger::getLogger().logMessage(pszErrorMsg);
	}

	void RunParameters::logProjErrors(void* v, int i, const char* c) {
		LapisLogger::getLogger().logMessage(c);
	}

	//these functions used to belong here but is going to be moved; until they find their forever home, I don't want to delete the implementation
	/*
	RunParameters::DataIssues RunParameters::checkForDataIssues() {
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
	

	void RunParameters::_checkLasDemOverlap(DataIssues& di, const std::set<LasFileExtent>& las, const std::set<DemFileAlignment>& dem)
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
			fullExtent = extendExtent(fullExtent, lasExtents[i]);
		}
		coord_t cellsize = convertUnits(30, LinearUnitDefs::meter, outCrs.getXYUnits());
		Alignment fullAlign = Alignment(fullExtent, 0, 0, cellsize, cellsize);
		Raster<bool> lasCovers{ fullAlign };
		Raster<bool> demCovers{ fullAlign };

		for (auto& e : lasExtents) {
			Extent projE = QuadExtent(e, lasCovers.crs()).outerExtent();
			for (auto& cell : fullAlign.cellsFromExtent(projE, SnapType::out)) {
				lasCovers[cell].value() = true;
			}
		}
		for (auto& e : demExtents) {
			Extent projE = QuadExtent(e, lasCovers.crs()).outerExtent();
			for (auto& cell : fullAlign.cellsFromExtent(projE, SnapType::out)) {
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
					Extent cr = cropExtent(thisExtent, compareExtent);
					di.overlapArea += (cr.ymax() - cr.ymin()) * (cr.xmax() - cr.xmin());
				}
			}
		}
	}

	void RunParameters::_checkSampleLas(DataIssues& di, const std::string& filename, const std::set<DemFileAlignment>& demSet) {
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
	*/
}