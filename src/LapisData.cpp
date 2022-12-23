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
	LapisData::LapisData() : _needAbort(false)
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
	const Unit& LapisData::outUnits() 
	{
		return _getRawParam<ParamName::outUnits>().unit();
	}
	const Unit& LapisData::prevUnits() 
	{
		return _prevUnits;
	}
	const std::string& LapisData::unitSingular() 
	{
		return _getRawParam<ParamName::outUnits>().unitSingularName();
	}
	const std::string& LapisData::unitPlural() 
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
		setPrevUnits(outUnits());
	}
	void LapisData::prepareForRun()
	{
		_needAbort = false;
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->prepareForRun();
			if (getNeedAbort()) {
				return;
			}
		}
		_cellMuts = std::make_unique<std::vector<std::mutex>>(_cellMutCount);

		if (isAnyDebug()) {
			setNeedAbortTrue();
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
			layoutAlign = extendAlignment(layoutAlign, *fineIntAlign(), SnapType::out);
		}
		_layout = std::make_shared<Raster<bool>>(layoutAlign);
	}
	void LapisData::cleanAfterRun()
	{
		for (size_t i = 0; i < _params.size(); ++i) {
			_params[i]->cleanAfterRun();
		}
		_cellMuts.reset();
		_needAbort = false;
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
		_needAbort = false;
	}

	const Extent& LapisData::fullExtent()
	{
		return _getRawParam<ParamName::las>().getFullExtent();
	}

	const CoordRef& LapisData::userCrs() 
	{
		return _getRawParam<ParamName::alignment>().getCurrentOutCrs();
	}

	const std::shared_ptr<Alignment> LapisData::metricAlign()
	{
		return _getRawParam<ParamName::alignment>().metricAlign();
	}
	const std::shared_ptr<Alignment> LapisData::csmAlign()
	{
		return _getRawParam<ParamName::csmOptions>().csmAlign();
	}
	const std::shared_ptr<Alignment> LapisData::fineIntAlign() {
		return _getRawParam<ParamName::fineIntOptions>().fineIntAlign();
	}
	shared_raster<bool> LapisData::layout()
	{
		return _layout;
	}

	std::mutex& LapisData::cellMutex(cell_t cell)
	{
		return (*_cellMuts)[cell % _cellMutCount];
	}
	std::mutex& LapisData::globalMutex()
	{
		return _globalMut;
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
	const std::vector<Extent>& LapisData::lasExtents()
	{
		return _getRawParam<ParamName::las>().sortedLasExtents();
	}
	LasReader LapisData::getLas(size_t i)
	{
		LasReader lr{ _getRawParam<ParamName::las>().sortedLasFileNames()[i] };
		if (!lasCrsOverride().isEmpty()) {
			lr.defineCRS(lasCrsOverride());
		}
		if (!lasUnitOverride().isUnknown()) {
			lr.setZUnits(lasUnitOverride());
		}
		for (auto& filter : filters()) {
			lr.addFilter(filter);
		}
		return lr;
	}
	const std::vector<Alignment>& LapisData::demAligns()
	{
		return _getRawParam<ParamName::dem>().demAligns();
	}
	Raster<coord_t> LapisData::getDem(size_t i)
	{
		Raster<coord_t> dem{ _getRawParam<ParamName::dem>().demFileNames()[i] };
		if (!demCrsOverride().isEmpty()) {
			dem.defineCRS(demCrsOverride());
		}
		if (!demUnitOverride().isUnknown()) {
			dem.setZUnits(demUnitOverride());
		}
		return dem;
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
	const std::shared_ptr<DemAlgorithm> LapisData::demAlgorithm() 
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
		return convertUnits(0.01, LinearUnitDefs::meter, outUnits());
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
	IdAlgo::IdAlgo LapisData::taoIdAlgo()
	{
		return _getRawParam<ParamName::taoOptions>().taoIdAlgo();
	}
	SegAlgo::SegAlgo LapisData::taoSegAlgo()
	{
		return _getRawParam<ParamName::taoOptions>().taoSegAlgo();
	}
	const std::vector<coord_t>& LapisData::strataBreaks() 
	{
		static std::vector<coord_t> empty;
		return doStratumMetrics() ? _getRawParam<ParamName::pointMetricOptions>().strata() : empty;
	}
	const std::vector<std::string>& LapisData::strataNames()
	{
		return _getRawParam<ParamName::pointMetricOptions>().strataNames();
	}
	const std::filesystem::path& LapisData::outFolder()
	{
		return _getRawParam<ParamName::output>().path();
	}
	const std::string& LapisData::name()
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
	bool LapisData::doAdvancedPointMetrics()
	{
		return _getRawParam<ParamName::pointMetricOptions>().doAdvancedPointMetrics();
	}
	bool LapisData::doCsm()
	{
		return _getRawParam<ParamName::whichProducts>().doCsm();
	}
	bool LapisData::doCsmMetrics()
	{
		return _getRawParam<ParamName::csmOptions>().doCsmMetrics();
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
		return isDebugNoAlign() || isDebugNoOutput();
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
	std::ostream& LapisData::writeOptions(std::ostream& out, ParamCategory cat) const
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

	void LapisData::setNeedAbortTrue()
	{
		_needAbort = true;
	}

	bool LapisData::getNeedAbort() const
	{
		return _needAbort;
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

	void LapisData::reportFailedLas(const std::string& s)
	{
		_failedLas.push_back(s);
	}
}