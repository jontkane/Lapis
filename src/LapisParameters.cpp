#include "app_pch.hpp"
#include "LapisParameters.hpp"
#include"LapisUtils.hpp"
#include"lapisdata.hpp"
#include"LapisLogger.hpp"

namespace lapis {

	Parameter<ParamName::name>::Parameter() {
		_name.addShortCmdAlias('N');
		_name.addHelpText("If you supply a name for the run, it will be included in the filenames of the output.");
	}
	void Parameter<ParamName::name>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {

		_name.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::name>::printToIni(std::ostream& o) {
		_name.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::name>::getCategory() const {
		return ParamCategory::data;
	}
	void Parameter<ParamName::name>::renderGui() {
		_name.renderGui();
	}
	void Parameter<ParamName::name>::importFromBoost() {
		_name.importFromBoost();
	}
	void Parameter<ParamName::name>::updateUnits() {}
	void Parameter<ParamName::name>::prepareForRun() {
		if (_runPrepared) {
			return;
		}
		_nameString = _name.getValue();
		_runPrepared = true;
	}
	void Parameter<ParamName::name>::cleanAfterRun() {
		_nameString.clear();
		_runPrepared = false;
	}
	const std::string& Parameter<ParamName::name>::name()
	{
		prepareForRun();
		return _nameString;
	}

	Parameter<ParamName::las>::Parameter() {
		_specifiers.addShortCmdAlias('L');
		_specifiers.addHelpText("Use these buttons to specify the las/laz format lidar point clouds you want to process.\n\n"
			"If you select a folder, it will include all las/laz files in that folder.\n\n"
			"If the 'recursive' box is checked, it will also include any las/laz files in subfolders.");
	}
	void Parameter<ParamName::las>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		_specifiers.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::las>::printToIni(std::ostream& o) {
		_specifiers.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::las>::getCategory() const {
		return ParamCategory::data;
	}
	void Parameter<ParamName::las>::renderGui() {
		_specifiers.renderGui();
	}
	void Parameter<ParamName::las>::importFromBoost() {
		_specifiers.importFromBoost();
	}
	void Parameter<ParamName::las>::updateUnits() {}
	void Parameter<ParamName::las>::prepareForRun() {
		
		if (_runPrepared) {
			return;
		}

		auto& d = LapisData::getDataObject();

		if (d.isDebugNoAlign()) {
			_runPrepared = true;
			return;
		}
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::findingLasFiles);

		CoordRef crsOver = d.lasCrsOverride();
		auto s = iterateOverFileSpecifiers(_specifiers.getSpecifiers(), &tryLasFile, crsOver, crsOver.getZUnits());

		
		CoordRef outCrs = d.userCrs();
		if (outCrs.isEmpty()) {
			for (const LasFileExtent& e : s) {
				outCrs = e.ext.crs();
				if (!outCrs.isEmpty()) {
					break;
				}
			}
		}
		
		std::vector<LasFileExtent> fileExtentVector;

		for (const LasFileExtent& l : s) {
			LasExtent e = { QuadExtent(l.ext, outCrs).outerExtent(), l.ext.nPoints() };
			fileExtentVector.emplace_back(l.filename,e);
		}
		std::sort(fileExtentVector.begin(), fileExtentVector.end());
		log.logMessage(std::to_string(fileExtentVector.size()) + " Las Files Found");
		if (fileExtentVector.size() == 0) {
			d.setNeedAbortTrue();
			log.logMessage("Aborting");
		}

		bool init = false;

		for (const LasFileExtent& l : fileExtentVector) {
			_lasFileNames.push_back(l.filename);
			_lasExtents.push_back(l.ext);
			if (!init) {
				_fullExtent = l.ext;
				init = true;
			}
			else {
				_fullExtent = extend(_fullExtent, l.ext);
			}
		}

		_runPrepared = true;
	}
	void Parameter<ParamName::las>::cleanAfterRun() {
		_lasExtents.clear();
		_lasFileNames.clear();

		_fullExtent = Extent();

		_runPrepared = false;
	}
	const Extent& Parameter<ParamName::las>::getFullExtent() {
		prepareForRun();
		return _fullExtent;
	}
	const std::vector<Extent>& Parameter<ParamName::las>::sortedLasExtents()
	{
		prepareForRun();
		return _lasExtents;
	}
	const std::set<std::string>& Parameter<ParamName::las>::currentFileSpecifiers() const
	{
		return _specifiers.getSpecifiers();
	}

	const std::vector<std::string>& Parameter<ParamName::las>::sortedLasFileNames()
	{
		return _lasFileNames;
	}

	Parameter<ParamName::dem>::Parameter() {
		_specifiers.addShortCmdAlias('D');
		_specifiers.addHelpText("Use these buttons to specify the vendor-provided DEM rasters you want to use for this run.\n\n"
			"If you select a folder, it will assume all rasters in that folder are DEMs.\n\n"
			"If the 'recursive' box is checked, it will also include any las/laz files in subfolders.\n\n"
		"Lapis cannot currently create its own ground model.\n\n"
		"If your rasters are in ESRI grid format, select the folder itself.");
	}
	void Parameter<ParamName::dem>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_specifiers.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::dem>::printToIni(std::ostream& o){
		_specifiers.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::dem>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::dem>::renderGui(){
		_specifiers.renderGui();
	}
	void Parameter<ParamName::dem>::importFromBoost() {
		_specifiers.importFromBoost();
	}
	void Parameter<ParamName::dem>::updateUnits() {}
	void Parameter<ParamName::dem>::prepareForRun() {
		if (_runPrepared) {
			return;
		}
		if (!_specifiers.getSpecifiers().size()) {
			return;
		}
		auto& d = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::findingDemFiles);

		CoordRef crsOver = d.demCrsOverride();
		std::set<DemFileAlignment> fileAligns = iterateOverFileSpecifiers(_specifiers.getSpecifiers(), &tryDtmFile, crsOver, crsOver.getZUnits());
		log.logMessage(std::to_string(fileAligns.size()) + " Dem Files Found");

		for (const DemFileAlignment& d : fileAligns) {
			_demAligns.push_back(d.align);
			_demFileNames.push_back(d.filename);
		}

		_runPrepared = true;
	}
	void Parameter<ParamName::dem>::cleanAfterRun() {
		_demFileNames.clear();
		_demAligns.clear();
		_runPrepared = false;
	}
	const std::vector<Alignment>& Parameter<ParamName::dem>::demAligns()
	{
		prepareForRun();
		return _demAligns;
	}
	const std::vector<std::string>& Parameter<ParamName::dem>::demFileNames()
	{
		prepareForRun();
		return _demFileNames;
	}
	const std::set<std::string>& Parameter<ParamName::dem>::currentFileSpecifiers() const
	{
		return _specifiers.getSpecifiers();
	}

	Parameter<ParamName::output>::Parameter() {
		_output.addShortCmdAlias('O');
		_debugNoOutput.addHelpText("This checkbox should only be displayed in debug mode. If you see it in a public release, please contact the developer.");
	}
	void Parameter<ParamName::output>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_output.addToCmd(visible, hidden);
		_debugNoOutput.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::output>::printToIni(std::ostream& o){
		_output.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::output>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::output>::renderGui(){
		if (_debugNoOutput.currentState()) {
			ImGui::BeginDisabled();
		}
		_output.renderGui();
		if (_debugNoOutput.currentState()) {
			ImGui::EndDisabled();
		}
#ifndef NDEBUG
		ImGui::SameLine();
		_debugNoOutput.renderGui();
#endif
	}
	void Parameter<ParamName::output>::importFromBoost() {
		_output.importFromBoost();
		_debugNoOutput.importFromBoost();
	}
	void Parameter<ParamName::output>::updateUnits() {}
	void Parameter<ParamName::output>::prepareForRun() {

		if (_runPrepared) {
			return;
		}

		_outPath = _output.path();

		LapisData& d = LapisData::getDataObject();
		size_t maxFileLength = d.maxLapisFileName + _outPath.string().size() + d.name().size();
		LapisLogger& log = LapisLogger::getLogger();
		if (maxFileLength > d.maxTotalFilePath) {
			log.logMessage("Total file path is too long");
			log.logMessage("Aborting");
			d.setNeedAbortTrue();
		}
		namespace fs = std::filesystem;
		if (fs::is_directory(_outPath)
			|| _debugNoOutput.currentState()) { //so the tests can feed a dummy value that doesn't trip the error detection
			return;
		}
		try {
			fs::create_directory(_outPath);
		}
		catch (fs::filesystem_error e) {
			log.logMessage("Unable to create output directory");
			log.logMessage("Aborting");
			d.setNeedAbortTrue();
		}
		_runPrepared = true;
	}
	void Parameter<ParamName::output>::cleanAfterRun() {
		_runPrepared = false;
	}
	const std::filesystem::path& Parameter<ParamName::output>::path()
	{
		prepareForRun();
		return _outPath;
	}
	bool Parameter<ParamName::output>::isDebugNoOutput() const
	{
		return _debugNoOutput.currentState();
	}

	Parameter<ParamName::lasOverride>::Parameter() {
		_unit.addOption("Infer from files", UnitRadio::UNKNOWN, LinearUnitDefs::unkLinear);
		_unit.addOption("Meters", UnitRadio::METERS, LinearUnitDefs::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, LinearUnitDefs::foot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, LinearUnitDefs::surveyFoot);
	}
	void Parameter<ParamName::lasOverride>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_unit.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::lasOverride>::printToIni(std::ostream& o){
		_unit.printToIni(o);
		_crs.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::lasOverride>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::lasOverride>::renderGui(){
		ImGui::Text("CRS of Laz Files:");
		_crs.renderDisplayString();
		if (ImGui::Button("Specify CRS")) {
			_displayWindow = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}

		ImGui::SameLine();
		ImGuiHelpMarker("Most of the time, Lapis can tell the projection and units of the laz files.\n\n"
			"However, especially with very old (pre-2013) laz files, it can sometimes be wrong.\n\n"
			"You can specify the correct information here.\n\n"
			"Note that this will be applied to ALL las/laz files. You cannot specify one projection for some and another for others.\n\n");

		_unit.renderGui();

		if (_displayWindow) {
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
			ImGui::SetNextWindowSize(ImVec2(400, 220));
			if (!ImGui::Begin("Laz CRS window", &_displayWindow, flags)) {
				ImGui::End();
			}
			else {
				_crs.renderGui();
				ImGui::SameLine();
				if (ImGui::Button("OK")) {
					_displayWindow = false;
				}
				ImGui::End();
			}
		}
	}
	void Parameter<ParamName::lasOverride>::importFromBoost() {
		_unit.importFromBoost();
		_crs.importFromBoost();
	}
	void Parameter<ParamName::lasOverride>::updateUnits() {}
	void Parameter<ParamName::lasOverride>::prepareForRun() {
	}
	void Parameter<ParamName::lasOverride>::cleanAfterRun() {
	}
	const CoordRef& Parameter<ParamName::lasOverride>::crs()
	{
		_crs.setZUnits(zUnits());
		return _crs.cachedCrs();
	}
	const Unit& Parameter<ParamName::lasOverride>::zUnits() const
	{
		return _unit.currentSelection();
	}

	Parameter<ParamName::demOverride>::Parameter() {
		_unit.addOption("Infer from files", UnitRadio::UNKNOWN, LinearUnitDefs::unkLinear);
		_unit.addOption("Meters", UnitRadio::METERS, LinearUnitDefs::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, LinearUnitDefs::foot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, LinearUnitDefs::surveyFoot);
	}
	void Parameter<ParamName::demOverride>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_unit.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::demOverride>::printToIni(std::ostream& o) {
		_unit.printToIni(o);
		_crs.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::demOverride>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::demOverride>::renderGui(){
		ImGui::Text("CRS of DEM Files:");
		_crs.renderDisplayString();
		if (ImGui::Button("Specify CRS")) {
			_displayWindow = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}

		ImGui::SameLine();
		ImGuiHelpMarker("Most of the time, Lapis can telln the projection and units of the dem raster files.\n\n"
			"However, sometimes the units are unexpected, or the projection isn't defined.\n\n"
			"You can specify the correct information here.\n\n"
			"Note that this will be applied to ALL DEM rasters. You cannot yet specify one projection for some and another for others.\n\n");

		_unit.renderGui();

		if (_displayWindow) {
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
			ImGui::SetNextWindowSize(ImVec2(400, 220));
			if (!ImGui::Begin("DEM CRS window", &_displayWindow, flags)) {
				ImGui::End();
			}
			else {
				_crs.renderGui();
				ImGui::SameLine();
				if (ImGui::Button("OK")) {
					_displayWindow = false;
				}
				ImGui::End();
			}
		}
	}
	void Parameter<ParamName::demOverride>::importFromBoost() {
		_unit.importFromBoost();
		_crs.importFromBoost();
	}
	void Parameter<ParamName::demOverride>::updateUnits() {}
	void Parameter<ParamName::demOverride>::prepareForRun() {
	}
	void Parameter<ParamName::demOverride>::cleanAfterRun() {
	}
	const CoordRef& Parameter<ParamName::demOverride>::crs()
	{
		_crs.setZUnits(zUnits());
		return _crs.cachedCrs();
	}
	const Unit& Parameter<ParamName::demOverride>::zUnits() const
	{
		return _unit.currentSelection();
	}

	Parameter<ParamName::outUnits>::Parameter()
	{
		_unit.addOption("Meters", UnitRadio::METERS, LinearUnitDefs::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, LinearUnitDefs::foot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, LinearUnitDefs::surveyFoot);
	}
	void Parameter<ParamName::outUnits>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_unit.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::outUnits>::printToIni(std::ostream& o){
		_unit.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::outUnits>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::outUnits>::renderGui(){
		_unit.renderGui();
	}
	void Parameter<ParamName::outUnits>::importFromBoost() {
		_unit.importFromBoost();
	}
	void Parameter<ParamName::outUnits>::updateUnits() {}
	void Parameter<ParamName::outUnits>::prepareForRun() {
	}
	void Parameter<ParamName::outUnits>::cleanAfterRun() {
	}
	const std::string& Parameter<ParamName::outUnits>::unitPluralName() const
	{
		return _pluralNames.at(unit());
	}
	const std::string& Parameter<ParamName::outUnits>::unitSingularName() const
	{
		return _singularNames.at(unit());
	}
	const Unit& Parameter<ParamName::outUnits>::unit() const
	{
		return _unit.currentSelection();
	}

	Parameter<ParamName::alignment>::Parameter() 
	{
		_xorigin.addHelpText("The origin is what sometimes causes rasters with the same cellsize and CRS to not line up.\n\n"
			"Most of the time, when you specify this, it's to ensure a match with some other specific raster.\n\n"
			"You can specify the coordinates of the lower-left corner of any cell in the desired output grid.\n\n");
		_yorigin.addHelpText("");
		_origin.addHelpText("The origin is what sometimes causes rasters with the same cellsize and CRS to not line up.\n\n"
			"Most of the time, when you specify this, it's to ensure a match with some other specific raster.\n\n"
			"You can specify the coordinates of the lower-left corner of any cell in the desired output grid.\n\n");
		_xres.addHelpText("The cellsize is the size of each pixel on a side.\n\n"
			"This cellsize will be used for coarse output rasters, like point metrics, but not for fine output rasters, like the canopy surface model.\n\n");
		_yres.addHelpText("");
		_cellsize.addHelpText("The cellsize is the size of each pixel on a side.\n\n"
			"This cellsize will be used for coarse output rasters, like point metrics, but not for fine output rasters, like the canopy surface model.\n\n");

		_debugNoAlign.addHelpText("This checkbox should only be displayed in debug mode. If you see it in a public release, please contact the developer.");
	}
	void Parameter<ParamName::alignment>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()
			((_alignCmd + ",A").c_str(), po::value<std::string>(&_alignFileBoostString),
			"A raster file you want the output metrics to align with\n"
			"Overwritten by --cellsize and --out-crs options");

		_cellsize.addToCmd(visible, hidden);
		_xres.addToCmd(visible, hidden);
		_yres.addToCmd(visible, hidden);
		_origin.addToCmd(visible, hidden);
		_xorigin.addToCmd(visible, hidden);
		_yorigin.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);

		_debugNoAlign.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::alignment>::printToIni(std::ostream& o){
		
		_xres.printToIni(o);
		_yres.printToIni(o);
		_xorigin.printToIni(o);
		_yorigin.printToIni(o);
		_crs.printToIni(o);

		return o;
	}
	ParamCategory Parameter<ParamName::alignment>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::alignment>::renderGui(){

		_manualWindow();
		_errorWindow();
		ImGui::Text("Output Alignment");
		if (ImGui::Button("Specify From File")) {
			NFD::OpenDialog(_nfdAlignFile);
		}

		if (_nfdAlignFile) {
			_alignFileBoostString = _nfdAlignFile.get();
			importFromBoost();
			_nfdAlignFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Specify Manually")) {
			_displayManualWindow = true;
		}

		ImGui::SameLine();
		ImGuiHelpMarker("This is how you specify a snap raster for the output of the run.\n\n"
			"If you specify a raster file, the output will have the same cellsize, projection, and alignment as that file.\n\n"
			"If you don't have a file with the alignment you want, you can also specify it manually.");

		ImGui::Text("Output CRS:");
		_crs.renderDisplayString();
		ImGui::Text("Cellsize: ");
		ImGui::SameLine();
		if (_xyResDiffCheck) {
			ImGui::Text("Diff X/Y");
		}
		else {
			ImGui::Text(_cellsize.asText());
			ImGui::SameLine();
			ImGui::Text(LapisData::getDataObject().unitPlural().c_str());
		}

#ifndef NDEBUG
		_debugNoAlign.renderGui();
#endif
	}
	void Parameter<ParamName::alignment>::updateUnits() {
		_cellsize.updateUnits();
		_xres.updateUnits();
		_yres.updateUnits();
		_origin.updateUnits();
		_xorigin.updateUnits();
		_yorigin.updateUnits();
	}
	void Parameter<ParamName::alignment>::importFromBoost() {
		if (_alignFileBoostString.size()) {
			try {
				Alignment a{ _alignFileBoostString };

				const Unit& src = a.crs().getXYUnits();
				const Unit& dst = LapisData::getDataObject().outUnits();

				_xres.setValue(convertUnits(a.xres(), src, dst));
				_yres.setValue(convertUnits(a.yres(), src, dst));
				if (a.xres() == a.yres()) {
					_xyResDiffCheck = false;
					_cellsize.setValue(convertUnits(a.xres(), src, dst));
				}
				else {
					_xyResDiffCheck = true;
				}

				_xorigin.setValue(convertUnits(a.xOrigin(), src, dst));
				_yorigin.setValue(convertUnits(a.yOrigin(), src, dst));
				if (a.xOrigin() == a.yOrigin()) {
					_xyOriginDiffCheck = true;
					_origin.setValue(convertUnits(a.xOrigin(), src, dst));
				}
				else {
					_xyOriginDiffCheck = false;
				}

				if (!a.crs().isEmpty()) {
					_crs.setCrs(a.crs());
				}
				else {
					_crs.reset();
				}
			}
			catch (InvalidRasterFileException e) {
				_displayErrorWindow = true;
			}

		}
		_alignFileBoostString.clear();
		if (_cellsize.importFromBoost()) {
			_xres.copyFrom(_cellsize);
			_yres.copyFrom(_cellsize);
			_xyResDiffCheck = false;
		}
		if (_origin.importFromBoost()) {
			_xorigin.copyFrom(_origin);
			_yorigin.copyFrom(_origin);
			_xyOriginDiffCheck = false;
		}

		bool xory = _xres.importFromBoost() || _yres.importFromBoost();
		if (xory) {
			if (strcmp(_xres.asText(), _yres.asText()) == 0) {
				_cellsize.copyFrom(_xres);
				_xyResDiffCheck = false;
			}
			else {
				_xyResDiffCheck = true;
			}
		}

		xory = _xorigin.importFromBoost() || _yorigin.importFromBoost();
		if (xory) {
			if (strcmp(_xorigin.asText(), _yres.asText()) == 0) {
				_origin.copyFrom(_xorigin);
				_xyOriginDiffCheck = false;
			}
			else {
				_xyOriginDiffCheck = true;
			}
		}

		_crs.importFromBoost();
		_debugNoAlign.importFromBoost();
	}
	const CoordRef& Parameter<ParamName::alignment>::getCurrentOutCrs() const {
		return _crs.cachedCrs();
	}
	const Alignment& Parameter<ParamName::alignment>::getFullAlignment()
	{
		prepareForRun();
		return *_align;
	}
	void Parameter<ParamName::alignment>::prepareForRun() {
		if (_runPrepared) {
			return;
		}
		if (_debugNoAlign.currentState()) {
			//something intentionally weird so nothing will match it unless it copied from it
			_align = std::make_shared<Alignment>(Extent(0, 50, 10, 60), 8, 6, 13, 19);
			_runPrepared = true;
			return;
		}
		auto& d = LapisData::getDataObject();
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::settingUp);

		Extent e = d.fullExtent();

		coord_t xres, yres, xorigin, yorigin;

		if (_xyResDiffCheck) {
			xres = _xres.getValueLogErrors();
			yres = _yres.getValueLogErrors();
		}
		else {
			xres = _cellsize.getValueLogErrors();
			yres = _cellsize.getValueLogErrors();
		}

		if (_xyOriginDiffCheck) {
			xorigin = _xorigin.getValueLogErrors();
			yorigin = _yorigin.getValueLogErrors();
		}
		else {
			xorigin = _origin.getValueLogErrors();
			yorigin = _origin.getValueLogErrors();
		}

		const Unit& src = d.outUnits();
		Unit dst = e.crs().getXYUnits();
		xres = convertUnits(xres, src, dst);
		yres = convertUnits(yres, src, dst);
		xorigin = convertUnits(xorigin, src, dst);
		yorigin = convertUnits(yorigin, src, dst);

		if (xres <= 0 || yres <= 0) {
			log.logMessage("Cellsize must be positive");
			log.logMessage("Aborting");
			d.setNeedAbortTrue();
		}

		if (d.getNeedAbort()) {
			return;
		}

		_align.reset();
		_align = std::make_shared<Alignment>(e, xorigin, yorigin, xres, yres);

#ifndef NDEBUG
		log.logMessage("Out CRS: " + _align->crs().getShortName());
#endif
		_runPrepared = true;
	}
	void Parameter<ParamName::alignment>::cleanAfterRun() {
		_align.reset();
		_runPrepared = false;
	}
	std::shared_ptr<Alignment> Parameter<ParamName::alignment>::metricAlign()
	{
		prepareForRun();
		return _align;
	}
	bool Parameter<ParamName::alignment>::isDebug() const
	{
		return _debugNoAlign.currentState();
	}
	void Parameter<ParamName::alignment>::_manualWindow()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!_displayManualWindow) {
			return;
		}
		if (!ImGui::Begin("Manual Alignment Window", &_displayManualWindow, flags)) {
			ImGui::End();
			return;
		}

		ImGui::Checkbox("Show Advanced Options", &_displayAdvanced);

		if (_displayAdvanced) {
			ImGui::Checkbox("Different X/Y Resolution", &_xyResDiffCheck);
			ImGui::Checkbox("Different X/Y Origin", &_xyOriginDiffCheck);
		}

		if (_displayAdvanced && _xyResDiffCheck) {
			_xres.renderGui();
			_yres.renderGui();
		}
		else {
			if (_cellsize.renderGui()) {
				_xres.copyFrom(_cellsize);
				_yres.copyFrom(_cellsize);
			}
		}

		if (_displayAdvanced && _xyOriginDiffCheck) {
			_xorigin.renderGui();
			_yorigin.renderGui();
		}
		else if (_displayAdvanced) {
			if (_origin.renderGui()) {
				_xorigin.copyFrom(_origin);
				_yorigin.copyFrom(_origin);
			}
		}

		_crs.renderGui();
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}
		if (ImGui::Button("OK")) {
			_displayManualWindow = false;
		}

		ImGui::End();
	}
	void Parameter<ParamName::alignment>::_errorWindow()
	{
		if (!_displayErrorWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("Alignment Error Window", &_displayManualWindow, flags)) {
			ImGui::End();
			return;
		}

		ImGui::Text("Error Reading that Raster File");
		if (ImGui::Button("OK")) {
			_displayErrorWindow = false;
		}

		ImGui::End();
	}

	Parameter<ParamName::csmOptions>::Parameter() {
		_smooth.addOption("3x3", 3, 3);
		_smooth.addOption("No Smoothing", 1, 1);
		_smooth.addOption("5x5", 5, 5);
		_smooth.setSingleLine();

		_footprint.addHelpText("This value indicates the estimated diameter of a lidar pulse as it strikes the canopy.\n\n"
			"Each return will be considered a circle with this diameter for the purpose of constructing the canopy surface model.\n\n"
			"If this behavior is undesirable, set the value to 0.");
		_smooth.addHelpText("The desired smoothing for the output CSM.\n\n"
			"If set to 3x3, each cell in the final raster will be equal to the average of that cell and its 8 neighbors in the pre-smoothing CSM.\n\n"
			"If set to 5x5, it will use the average of those 9 cells and the 16 cells surrounding them.");
		_doMetrics.addHelpText("If this is checked, Lapis will calculate summary metrics on the CSM itself, such as the mean and standard deviation of values.");
	}
	void Parameter<ParamName::csmOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_cellsize.addToCmd(visible, hidden);
		_footprint.addToCmd(visible, hidden);
		_doMetrics.addToCmd(visible, hidden);
		_smooth.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::csmOptions>::printToIni(std::ostream& o){
		_cellsize.printToIni(o);
		_footprint.printToIni(o);
		_doMetrics.printToIni(o);
		_smooth.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::csmOptions>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::csmOptions>::renderGui(){
		ImGui::Text("Canopy Surface Model Options");

		_cellsize.renderGui();

		_smooth.renderGui();

		_doMetrics.renderGui();

		ImGui::Checkbox("Show Advanced Options", &_showAdvanced);
		if (_showAdvanced) {
			_footprint.renderGui();
		}
	}
	void Parameter<ParamName::csmOptions>::updateUnits() {
		_cellsize.updateUnits();
		_footprint.updateUnits();
	}
	void Parameter<ParamName::csmOptions>::importFromBoost() {
		_smooth.importFromBoost();
		_footprint.importFromBoost();
		_cellsize.importFromBoost();
		_doMetrics.importFromBoost();
	}
	void Parameter<ParamName::csmOptions>::prepareForRun() {
		if (_runPrepared) {
			return;
		}

		auto& d = LapisData::getDataObject();
		auto& log = LapisLogger::getLogger();
		const Alignment& metricAlign = *d.metricAlign();
		const Unit& u = d.outUnits();


		if (!d.doCsm()) {
			//there's a few random places in the code that use the csm alignment even if the csm isn't being calculated
			//if the csm is turned off, so we don't have its parameters, just default to 1 meter cellsize
			coord_t cellsize = convertUnits(1, u, metricAlign.crs().getXYUnits());
			_csmAlign = std::make_shared<Alignment>(metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), cellsize, cellsize);
			_runPrepared = true;
			return;
		}

		if (_footprint.getValueLogErrors() < 0) {
			log.logMessage("Pulse diameter must be non-negative");
			log.logMessage("Aborting");
			d.setNeedAbortTrue();
		}
		if (_cellsize.getValueLogErrors() <= 0) {
			log.logMessage("CSM Cellsize must be positive");
			log.logMessage("Aborting");
			d.setNeedAbortTrue();
		}

		if (d.getNeedAbort()) {
			return;
		}
		
		coord_t cellsize = convertUnits(_cellsize.getValueLogErrors(), u, metricAlign.crs().getXYUnits());
		_csmAlign = std::make_shared<Alignment>(metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), cellsize, cellsize);

		_runPrepared = true;
	}
	void Parameter<ParamName::csmOptions>::cleanAfterRun() {
		_csmAlign.reset();
		_runPrepared = false;
	}
	int Parameter<ParamName::csmOptions>::smooth() const
	{
		return _smooth.currentSelection();
	}
	std::shared_ptr<Alignment> Parameter<ParamName::csmOptions>::csmAlign()
	{
		prepareForRun();
		return _csmAlign;
	}

	coord_t Parameter<ParamName::csmOptions>::footprintDiameter() const
	{
		return _footprint.getValueLogErrors();
	}

	bool Parameter<ParamName::csmOptions>::doCsmMetrics() const
	{
		return _doMetrics.currentState();
	}

	Parameter<ParamName::pointMetricOptions>::Parameter() {
		_advMetrics.displayFalseFirst();

		_whichReturns.addCombo(DONT_SKIP, DONT_SKIP, 2, "Both");
		_whichReturns.addCombo(DO_SKIP, DONT_SKIP, 0, "All Returns");
		_whichReturns.addCombo(DONT_SKIP, DO_SKIP, 1, "First Returns");

		_canopyCutoff.addHelpText("Many point metrics are either calculated only on canopy points, or treat canopy points specially in some other way.\n\n"
			"A canopy point is defined as being above some specific height, which you specify here.");
		_doStrata.addHelpText("Some metrics are calculated on specific height bands; for example, on all returns between 8 and 16 meters above the ground."
			"These numbers specify those bands.");
		_advMetrics.addHelpText("There are a very large number of point metrics that have been proposed over the years.\n\n"
			"However, a much smaller subset suffice for the vast majority of applications.\n\n"
			"This checkbox controls whether Lapis calculates the more limited set, or the full set.");
		_whichReturns.addHelpText("Depending on the application, you may want point metrics to be calculated either on first returns only, or on all returns.\n\n"
			"You can control that with this selection.");
	}
	void Parameter<ParamName::pointMetricOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_canopyCutoff.addToCmd(visible, hidden);
		_strata.addToCmd(visible, hidden);
		_advMetrics.addToCmd(visible, hidden);
		_whichReturns.addToCmd(visible, hidden);
		_doStrata.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::pointMetricOptions>::printToIni(std::ostream& o){
		_canopyCutoff.printToIni(o);
		_strata.printToIni(o);
		_advMetrics.printToIni(o);
		_whichReturns.printToIni(o);
		_doStrata.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::pointMetricOptions>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::pointMetricOptions>::renderGui(){
		ImGui::Text("Metric Options");

		ImGui::BeginChild("stratumleft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, ImGui::GetContentRegionAvail().y), false, 0);
		_canopyCutoff.renderGui();
		_advMetrics.renderGui();
		ImGui::Text("Calculate Metrics Using:");
		_whichReturns.renderGui();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("stratumright", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
		_doStrata.renderGui();
		if (_doStrata.currentState()) {
			_strata.renderGui();
		}
		ImGui::EndChild();
	}
	void Parameter<ParamName::pointMetricOptions>::updateUnits() {
		_canopyCutoff.updateUnits();
		_strata.updateUnits();
	}
	void Parameter<ParamName::pointMetricOptions>::importFromBoost() {
		
		_canopyCutoff.importFromBoost();
		_strata.importFromBoost();
		_advMetrics.importFromBoost();
		_whichReturns.importFromBoost();
		_doStrata.importFromBoost();
	}
	void Parameter<ParamName::pointMetricOptions>::prepareForRun() {

		if (_runPrepared) {
			return;
		}

		auto& d = LapisData::getDataObject();

		LapisLogger& log = LapisLogger::getLogger();
		if (_canopyCutoff.getValueLogErrors() < 0) {
			log.logMessage("Canopy cutoff is negative. Is this intentional?");
		}

		if (!d.doPointMetrics()) {
			_runPrepared = true;
			return;
		}

		if (_doStrata.currentState()) {
			if (_strata.cachedValues().size()) {
				auto to_string_with_precision = [](coord_t v)->std::string {
					//all this nonsense should get a non-scientific notation representation with at most two places after the decimal point, but trailing 0s removed
					std::ostringstream out;
					out.precision(2);
					out << std::fixed;
					out << v;
					std::string s = out.str();
					s.erase(s.find_last_not_of('0') + 1, std::string::npos);
					s.erase(s.find_last_not_of('.') + 1, std::string::npos);
					return s;
				};
				const std::vector<coord_t>& strata = _strata.cachedValues();
				_strataNames.push_back("LessThan" + to_string_with_precision(strata[0]));
				for (size_t i = 1; i < strata.size(); ++i) {
					_strataNames.push_back(to_string_with_precision(strata[i - 1]) + "To" + to_string_with_precision(strata[i]));
				}
				_strataNames.push_back("GreaterThan" + to_string_with_precision(strata[strata.size() - 1]));
			}
		}

		_runPrepared = true;
	}
	void Parameter<ParamName::pointMetricOptions>::cleanAfterRun() {
		_runPrepared = false;
	}
	bool Parameter<ParamName::pointMetricOptions>::doAllReturns() const
	{
		return _whichReturns.currentState(ALL_RETURNS) == DONT_SKIP;
	}
	bool Parameter<ParamName::pointMetricOptions>::doFirstReturns() const
	{
		return _whichReturns.currentState(FIRST_RETURNS) == DONT_SKIP;
	}
	const std::vector<coord_t>& Parameter<ParamName::pointMetricOptions>::strata() const
	{
		return _strata.cachedValues();
	}
	const std::vector<std::string>& Parameter<ParamName::pointMetricOptions>::strataNames() 
	{
		prepareForRun();
		return _strataNames;
	}
	coord_t Parameter<ParamName::pointMetricOptions>::canopyCutoff() const
	{
		return _canopyCutoff.getValueLogErrors();
	}
	bool Parameter<ParamName::pointMetricOptions>::doStratumMetrics() const
	{
		return _doStrata.currentState();
	}

	bool Parameter<ParamName::pointMetricOptions>::doAdvancedPointMetrics() const
	{
		return _advMetrics.currentState();
	}

	Parameter<ParamName::filters>::Parameter() {
		for (size_t i = 0; i < 256; ++i) {
			_class.setState(i, true);
		}
		_class.setState(7, false);
		_class.setState(9, false);
		_class.setState(18, false);

		_minht.addHelpText("Ocassionally, points in a laz file will be so far above or below the ground as to be obvious errors.\n\n"
		"You can specify those cutoffs here.");
		_class.addHelpText("Your laz files may have classification information indicating a best guess at what the lidar pulse struck.\n\n"
			"You can choose to exclude certain classifications from the analysis.");
		_filterWithheld.addHelpText("Some points in laz files are marked as withheld, indicating that they should not be used.\n\n"
			"For the vast majority of applications, they should be excluded, but you can choose to include them with this checkbox.");
	}
	void Parameter<ParamName::filters>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_minht.addToCmd(visible, hidden);
		_maxht.addToCmd(visible, hidden);
		_class.addToCmd(visible, hidden);
		_filterWithheld.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::filters>::printToIni(std::ostream& o){
		_minht.printToIni(o);
		_maxht.printToIni(o);
		_filterWithheld.printToIni(o);
		_class.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::filters>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::filters>::renderGui(){
		_filterWithheld.renderGui();
		_minht.renderGui();
		_maxht.renderGui();

		ImGui::Text("Use Classes:");
		ImGui::SameLine();
		_class.renderGui();
	}
	void Parameter<ParamName::filters>::updateUnits() {
		_minht.updateUnits();
		_maxht.updateUnits();
	}
	void Parameter<ParamName::filters>::importFromBoost() {
		_minht.importFromBoost();
		_maxht.importFromBoost();
		_filterWithheld.importFromBoost();
		_class.importFromBoost();
	}
	void Parameter<ParamName::filters>::prepareForRun() {
		if (_runPrepared) {
			return;
		}

		if (_minht.getValueLogErrors() > _maxht.getValueLogErrors()) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Low outlier must be less than high outlier");
			log.logMessage("Aborting");
			LapisData::getDataObject().setNeedAbortTrue();
			return;
		}
		if (_filterWithheld.currentState()) {
			_filters.emplace_back(new LasFilterWithheld());
		}
		_filters.push_back(_class.getFilter());
		_runPrepared = true;
	}
	void Parameter<ParamName::filters>::cleanAfterRun() {
		_filters.clear();
		_filters.shrink_to_fit();
		_runPrepared = false;
	}
	const std::vector<std::shared_ptr<LasFilter>>& Parameter<ParamName::filters>::filters()
	{
		prepareForRun();
		return _filters;
	}
	coord_t Parameter<ParamName::filters>::minht() const
	{
		return _minht.getValueLogErrors();
	}
	coord_t Parameter<ParamName::filters>::maxht() const
	{
		return _maxht.getValueLogErrors();
	}

	Parameter<ParamName::whichProducts>::Parameter() {
		_doCsm.addHelpText("Calculate a canopy surface model.\n\n"
			"A canopy surface model is a rasterized product usually produced at a fine (2m or smaller) resolution.\n\n"
			"It indicates a best guess as to the height of the canopy in each pixel.");
		_doPointMetrics.addHelpText("Calculate point metrics.\n\n"
			"Point metrics are a rasterized product usually produced at a coarse (10m or large) resolution.\n\n"
			"They represent summaries (such as the mean, standard deviation, etc) of the heights of the returns in each pixel.\n\n"
			"They can give a good idea of the structure of the forest within the pixel.");
		_doTao.addHelpText("Identify Tree-Approximate Objects (TAOs)\n\n"
			"A TAO represents something that the algorithm believes to be an overstory tree.\n\n"
			"No attempt is made to identify understory trees, and there is considerable inaccuracy even in the overstory.");
		_doTopo.addHelpText("Calculate topographic metrics such as slope and aspect off of the DEM.");
		_doFineInt.addHelpText("Calculate a fine-scale mean intensity image\n\n"
			"Intensity is a measure of how bright a lidar pulse is. Chlorophyll is very bright in near infrared, so high intensity can indicate the presence of live vegetation.");
	}
	void Parameter<ParamName::whichProducts>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		_doFineInt.addToCmd(visible, hidden);
		_doCsm.addToCmd(visible, hidden);
		_doPointMetrics.addToCmd(visible, hidden);
		_doTao.addToCmd(visible, hidden);
		_doTopo.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::whichProducts>::printToIni(std::ostream& o){
		_doFineInt.printToIni(o);
		_doCsm.printToIni(o);
		_doPointMetrics.printToIni(o);
		_doTao.printToIni(o);
		_doTopo.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::whichProducts>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::whichProducts>::renderGui(){
		ImGui::Text("Optional Product Options");
		_doPointMetrics.renderGui();
		_doCsm.renderGui();
		if (!_doCsm.currentState()) {
			ImGui::BeginDisabled();
			_doTao.setState(false);
		}
		_doTao.renderGui();
		if (!_doCsm.currentState()) {
			ImGui::EndDisabled();
		}
		_doTopo.renderGui();
		_doFineInt.renderGui();
	}
	void Parameter<ParamName::whichProducts>::importFromBoost() {
		_doFineInt.importFromBoost();
		_doCsm.importFromBoost();
		_doPointMetrics.importFromBoost();
		_doTao.importFromBoost();
		_doTopo.importFromBoost();
	}
	void Parameter<ParamName::whichProducts>::updateUnits() {}
	void Parameter<ParamName::whichProducts>::prepareForRun() {}
	void Parameter<ParamName::whichProducts>::cleanAfterRun() {}
	bool Parameter<ParamName::whichProducts>::doCsm() const
	{
		return _doCsm.currentState();
	}
	bool Parameter<ParamName::whichProducts>::doPointMetrics() const
	{
		return _doPointMetrics.currentState();
	}
	bool Parameter<ParamName::whichProducts>::doTao() const
	{
		return _doTao.currentState();
	}
	bool Parameter<ParamName::whichProducts>::doTopo() const
	{
		return _doTopo.currentState();
	}
	bool Parameter<ParamName::whichProducts>::doFineInt() const
	{
		return _doFineInt.currentState();
	}

	Parameter<ParamName::computerOptions>::Parameter() {
		_thread.addHelpText("This controls how many independent threads to run the Lapis process on.\n\n"
			"On most computers, this should be set to 2 or 3 below the number of logical cores on the machine.\n\n"
			"If Lapis is causing your computer to slow down, considering lowering this.");
	}
	void Parameter<ParamName::computerOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		_thread.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::computerOptions>::printToIni(std::ostream& o) {
		_thread.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::computerOptions>::getCategory() const {
		return ParamCategory::computer;
	}
	void Parameter<ParamName::computerOptions>::renderGui() {
		_thread.renderGui();
	}
	void Parameter<ParamName::computerOptions>::importFromBoost() {
		_thread.importFromBoost();
	}
	void Parameter<ParamName::computerOptions>::updateUnits() {}
	void Parameter<ParamName::computerOptions>::prepareForRun() {
		if (_thread.getValueLogErrors() <= 0) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Number of threads must be positive");
			log.logMessage("Aborting");
			LapisData::getDataObject().setNeedAbortTrue();
		}
	}
	void Parameter<ParamName::computerOptions>::cleanAfterRun() {}
	int Parameter<ParamName::computerOptions>::nThread() const
	{
		return _thread.getValueLogErrors();
	}

	Parameter<ParamName::taoOptions>::Parameter() {
		_idAlgo.addOption("High Points", IdAlgo::HIGHPOINT, IdAlgo::HIGHPOINT);
		_idAlgo.setSingleLine();
		_segAlgo.addOption("Watershed", SegAlgo::WATERSHED, SegAlgo::WATERSHED);
		_segAlgo.setSingleLine();

		_sameMinHt.setState(true);

		_idAlgo.addHelpText("The algorithm for identifying trees. Only the 'high points' algorithm is supported currently.\n\n"
			"High Points: This is a CSM-based algorith. A CSM pixel is considered a candidate for being the stem of a tree if it's higher than all 8 of its neighbors.\n"
			"It is good for trees such as conifers with a well-defined tops.");
		_segAlgo.addHelpText("The algorithm for segmenting the canopy between the identified trees. Only the watershed algorith is supported currently.\n\n"
			"Watershed: expand each identified tree downwards from the center until it would have to go upwards again.\n"
			"The watershed algorithm requires no parameterization");

	}
	void Parameter<ParamName::taoOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		_minht.addToCmd(visible, hidden);
		_mindist.addToCmd(visible, hidden);
		_idAlgo.addToCmd(visible, hidden);
		_segAlgo.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::taoOptions>::printToIni(std::ostream& o) {
		if (!_sameMinHt.currentState()) {
			_minht.printToIni(o);
		}
		_mindist.printToIni(o);
		
		_idAlgo.printToIni(o);
		_segAlgo.printToIni(o);
		return o;
	}
	ParamCategory Parameter<ParamName::taoOptions>::getCategory() const {
		return ParamCategory::process;
	}
	void Parameter<ParamName::taoOptions>::renderGui() {
		if (!LapisData::getDataObject().doCsm()) {
			ImGui::Text("Tree identification requires a CSM");
			return;
		}

		_idAlgo.renderGui();
		_segAlgo.renderGui();

		_mindist.renderGui();

		ImGui::Text("Minimum Tree Height:");
		_sameMinHt.renderGui();
		ImGui::SameLine();
		if (_sameMinHt.currentState()) {
			ImGui::BeginDisabled();
		}
		_minht.renderGui();
		if (_sameMinHt.currentState()) {
			ImGui::EndDisabled();
		}
	}
	void Parameter<ParamName::taoOptions>::importFromBoost() {
		
		//the first time this function is called, the default value for minHt will be copied into the buffer
		//this should *not* cause the checkbox to flip to having a unique value
		//However, any subsequent imports from boost indicate intentionality, and if minht is specified, that should be respected
		if (_minht.importFromBoost()) {
			_sameMinHt.setState(false);
		}
		_mindist.importFromBoost();
		_idAlgo.importFromBoost();
		_segAlgo.importFromBoost();
	}
	void Parameter<ParamName::taoOptions>::updateUnits() {
		_minht.updateUnits();
		_mindist.updateUnits();
	}
	void Parameter<ParamName::taoOptions>::prepareForRun() {
		
		if (!LapisData::getDataObject().doTaos()) {
			return;
		}
		
		LapisLogger& log = LapisLogger::getLogger();
		
		if (!_sameMinHt.currentState()) {
			if (_minht.getValueLogErrors() < 0) {
				log.logMessage("Minimum tree height is negative. Is this intentional?");
			}
		}

		if (_mindist.getValueLogErrors() < 0) {
			log.logMessage("Minimum TAO Distance cannot be negative");
			log.logMessage("Aborting");
			LapisData::getDataObject().setNeedAbortTrue();
			return;
		}
	}
	void Parameter<ParamName::taoOptions>::cleanAfterRun() {
	}
	coord_t Parameter<ParamName::taoOptions>::minTaoHt() const
	{
		return _sameMinHt.currentState() ? LapisData::getDataObject().canopyCutoff() : _minht.getValueLogErrors();
	}
	coord_t Parameter<ParamName::taoOptions>::minTaoDist() const
	{
		return _mindist.getValueLogErrors();
	}

	IdAlgo::IdAlgo Parameter<ParamName::taoOptions>::taoIdAlgo() const
	{
		return _idAlgo.currentSelection();
	}

	SegAlgo::SegAlgo Parameter<ParamName::taoOptions>::taoSegAlgo() const
	{
		return _segAlgo.currentSelection();
	}

	Parameter<ParamName::fineIntOptions>::Parameter() {
		_sameCellsize.setState(true);
		_sameCutoff.setState(true);

		_useCutoff.addHelpText("Depending on your application, you may want the mean intensity to include all returns, or just returns from trees.\n\b"
			"As with point metrics, the canopy is defined as being returns that are at least a certain height above the ground.");
	}
	void Parameter<ParamName::fineIntOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		_cellsize.addToCmd(visible, hidden);
		_useCutoff.addToCmd(visible, hidden);
		_cutoff.addToCmd(visible, hidden);
	}
	std::ostream& Parameter<ParamName::fineIntOptions>::printToIni(std::ostream& o) {
		if (!_sameCellsize.currentState()) {
			_cellsize.printToIni(o);
		}
		_useCutoff.printToIni(o);
		if (_useCutoff.currentState()) {
			if (!_sameCutoff.currentState()) {
				_cutoff.printToIni(o);
			}
		}
		return o;
	}
	ParamCategory Parameter<ParamName::fineIntOptions>::getCategory() const {
		return ParamCategory::process;
	}
	void Parameter<ParamName::fineIntOptions>::renderGui() {
		ImGui::Text("Cellsize:");
		_sameCellsize.renderGui();
		ImGui::SameLine();
		if (_sameCellsize.currentState()) {
			ImGui::BeginDisabled();
		}
		_cellsize.renderGui();
		if (_sameCellsize.currentState()) {
			ImGui::EndDisabled();
		}

		ImGui::Dummy(ImVec2(0,10));
		ImGui::Text("Height Cutoff:");
		_useCutoff.renderGui();
		if (_useCutoff.currentState()) {
			_sameCutoff.renderGui();
			ImGui::SameLine();
			if (_sameCutoff.currentState()) {
				ImGui::BeginDisabled();
			}
			_cutoff.renderGui();
			if (_sameCutoff.currentState()) {
				ImGui::EndDisabled();
			}
		}
	}
	void Parameter<ParamName::fineIntOptions>::importFromBoost() {
		
		_useCutoff.importFromBoost();
		if (_cutoff.importFromBoost() && _useCutoff.currentState()) {
			if (!_initialSetup) {
				_sameCutoff.setState(false);
			}
		}
		if (_cellsize.importFromBoost()) {
			if (!_initialSetup) {
				_sameCellsize.setState(false);
			}
		}
		_initialSetup = false;
	}
	void Parameter<ParamName::fineIntOptions>::updateUnits() {
		_cellsize.updateUnits();
		_cutoff.updateUnits();
	}
	void Parameter<ParamName::fineIntOptions>::prepareForRun() {
		if (_runPrepared) {
			return;
		}
		LapisLogger& log = LapisLogger::getLogger();
		LapisData& d = LapisData::getDataObject();

		if (!d.doFineInt()) {
			_runPrepared = true;
			return;
		}

		if (_useCutoff.currentState()) {
			if (_cutoff.getValueLogErrors() < 0) {
				log.logMessage("Fine Intensity cutoff is negative. Is this intentional?");
			}
		}

		coord_t cellsize = 1;
		if (_sameCellsize.currentState()) {
			const Alignment& a = *d.csmAlign();
			cellsize = a.xres();
		}
		else {
			cellsize = _cellsize.getValueLogErrors();
			if (cellsize <= 0) {
				log.logMessage("Fine Intensity Cellsize is Negative");
				log.logMessage("Aborting");
				d.setNeedAbortTrue();
				return;
			}
		}

		const Alignment& a = *d.metricAlign();
		_fineIntAlign = std::make_unique<Alignment>((Extent)a, a.xOrigin(), a.yOrigin(), cellsize, cellsize);

		_runPrepared = true;
	}
	void Parameter<ParamName::fineIntOptions>::cleanAfterRun() {
		_fineIntAlign.reset();
		_runPrepared = false;
	}
	std::shared_ptr<Alignment> Parameter<ParamName::fineIntOptions>::fineIntAlign()
	{
		prepareForRun();
		return _fineIntAlign;
	}
	coord_t Parameter<ParamName::fineIntOptions>::fineIntCutoff() const
	{
		if (!_useCutoff.currentState()) {
			return std::numeric_limits<coord_t>::lowest();
		}
		return _sameCutoff.currentState() ? LapisData::getDataObject().canopyCutoff() : _cutoff.getValueLogErrors();
	}

	Parameter<ParamName::topoOptions>::Parameter() {}
	void Parameter<ParamName::topoOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {}
	std::ostream& Parameter<ParamName::topoOptions>::printToIni(std::ostream& o) {
		return o;
	}
	ParamCategory Parameter<ParamName::topoOptions>::getCategory() const {
		return ParamCategory::process;
	}
	void Parameter<ParamName::topoOptions>::renderGui() {}
	void Parameter<ParamName::topoOptions>::importFromBoost() {}
	void Parameter<ParamName::topoOptions>::updateUnits() {}
	void Parameter<ParamName::topoOptions>::prepareForRun() {}
	void Parameter<ParamName::topoOptions>::cleanAfterRun() {}

	template<class T>
	std::set<T> iterateOverFileSpecifiers(const std::set<std::string>& specifiers, fileSpecOpen<T> openFunc,
		const CoordRef& crs, const Unit& u)
	{

		namespace fs = std::filesystem;

		std::set<T> fileList;

		std::queue<std::string> toCheck;

		for (const std::string& spec : specifiers) {
			toCheck.push(spec);
		}

		while (toCheck.size()) {
			fs::path specPath{ toCheck.front() };
			toCheck.pop();

			//specified directories get searched recursively
			if (fs::is_directory(specPath)) {
				for (auto& subpath : fs::directory_iterator(specPath)) {
					toCheck.push(subpath.path().string());
				}
				//because of the dumbass ESRI grid format, I have to try to open folders as rasters as well
				(*openFunc)(specPath, fileList, crs, u);
			}

			if (fs::is_regular_file(specPath)) {
				//if it's a file, try to add it to the map
				(*openFunc)(specPath, fileList, crs, u);
			}

			//wildcard specifiers (e.g. C:\data\*.laz) are basically a non-recursive directory check with an extension
			if (specPath.has_filename()) {
				if (fs::is_directory(specPath.parent_path())) {
					std::regex wildcard{ "^\\*\\..+" };
					std::string ext = "";
					if (std::regex_match(specPath.filename().string(), wildcard)) {
						ext = specPath.extension().string();
					}

					if (ext.size()) {
						for (auto& subpath : fs::directory_iterator(specPath.parent_path())) {
							if (subpath.path().has_extension() && subpath.path().extension() == ext || ext == ".*") {
								toCheck.push(subpath.path().string());
							}
						}
					}
				}
			}
		}
		return fileList;
	}

	template std::set<LasFileExtent> iterateOverFileSpecifiers<LasFileExtent>(const std::set<std::string>& specifiers,
		fileSpecOpen<LasFileExtent> openFunc, const CoordRef& crs, const Unit& u);
	template std::set<DemFileAlignment> iterateOverFileSpecifiers<DemFileAlignment>(const std::set<std::string>& specifiers,
		fileSpecOpen<DemFileAlignment> openFunc, const CoordRef& crs, const Unit& u);

	void tryLasFile(const std::filesystem::path& file, std::set<LasFileExtent>& data,
		const CoordRef& crs, const Unit& u)
	{
		LapisLogger& log = LapisLogger::getLogger();
		if (!file.has_extension()) {
			return;
		}
		if (file.extension() != ".laz" && file.extension() != ".las") {
			return;
		}
		try {
			LasExtent e{ file.string() };
			if (!crs.isEmpty()) {
				e.defineCRS(crs);
			}
			if (!u.isUnknown()) {
				e.setZUnits(u);
			}
			data.insert({ file.string(), e });
		}
		catch (...) {
			log.logMessage("Error reading " + file.string());
			LapisData::getDataObject().reportFailedLas(file.string());

		}
	}

	void tryDtmFile(const std::filesystem::path& file, std::set<DemFileAlignment>& data,
		const CoordRef& crs, const Unit& u)
	{
		if (file.extension() == ".aux" || file.extension() == ".ovr" || file.extension() == ".adf"
			|| file.extension() == ".xml") { //excluding commonly-found non-raster files to prevent slow calls to GDAL
			return;
		}
		try {
			Alignment a{ file.string() };
			if (!crs.isEmpty()) {
				a.defineCRS(crs);
			}
			if (!u.isUnknown()) {
				a.setZUnits(u);
			}
			data.insert({ file.string(), a });
		}
		catch (...) {}
	}
}
