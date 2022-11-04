#include "app_pch.hpp"
#include "LapisParameters.hpp"
#include"LapisUtils.hpp"
#include"lapisdata.hpp"
#include"LapisLogger.hpp"

namespace lapis {
	void lasDemUnifiedGui(LasDemSpecifics& spec)
	{
		std::set<std::string>& fileSpecSet = spec.getFileSpecsSet();
		std::string buttonText = "Add " + spec.name + " Files";
		auto filterptr = spec.fileFilter ? spec.fileFilter.get() : nullptr;
		int filtercnt = spec.fileFilter ? 1 : 0;
		if (ImGui::Button(buttonText.c_str())) {
			NFD::OpenDialogMultiple(spec.nfdFiles, filterptr, filtercnt, (const nfdnchar_t*)nullptr);
		}
		nfdpathsetsize_t selectedFileCount = 0;
		if (spec.nfdFiles) {
			NFD::PathSet::Count(spec.nfdFiles, selectedFileCount);
			for (nfdpathsetsize_t i = 0; i < selectedFileCount; ++i) {
				NFD::UniquePathSetPathU8 path;
				NFD::PathSet::GetPath(spec.nfdFiles, i, path);
				fileSpecSet.insert(path.get());
			}
			spec.nfdFiles.reset();
		}

		ImGui::SameLine();
		buttonText = "Add " + spec.name + " Folder";
		if (ImGui::Button(buttonText.c_str())) {
			NFD::PickFolder(spec.nfdFolder);
		}
		if (spec.nfdFolder) {
			if (spec.recursiveCheckbox) {
				fileSpecSet.insert(spec.nfdFolder.get());
			}
			else {
				for (const std::string& s : spec.wildcards) {
					fileSpecSet.insert(spec.nfdFolder.get() + std::string("\\") + s);
				}
			}
			spec.nfdFolder.reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			fileSpecSet.clear();
		}
		//ImGui::SameLine();
		ImGui::Checkbox("Add subfolders", &spec.recursiveCheckbox);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		std::string childname = "##" + spec.name;
		ImGui::BeginChild(childname.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, window_flags);
		for (const std::string& s : fileSpecSet) {
			ImGui::Text(s.c_str());
		}
		ImGui::EndChild();
	}
	void updateCrsAndDisplayString(const char* inStr, const char* defaultMessage, std::string* display, CoordRef* crs) {
		try {
			*crs = CoordRef(inStr);
			if (crs->isEmpty()) {
				*display = defaultMessage;
			}
			else {
				*display = crs->getSingleLineWKT();
			}
		}
		catch (UnableToDeduceCRSException e) {
			*display = "Error reading CRS";
		}
	}
	int unitRadioFromString(const std::string& s) {
		std::regex meterregex{ ".*m.*",std::regex::icase };
		if (std::regex_match(s, meterregex)) {
			return 1;
		}
		std::regex surveyregex{ ".*us.*",std::regex::icase };
		if (std::regex_match(s, surveyregex)) {
			return 3;
		}
		std::regex footregex{ ".*f.*",std::regex::icase };
		if (std::regex_match(s, footregex)) {
			return 2;
		}
		return 0;
	}
	void lasDemOverrideUnifiedGui(LasDemOverrideSpecifics& spec) {
		ImGui::Text((spec.name + " CRS:").c_str());
		ImGui::BeginChild((spec.name + std::string("Override")).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(spec.crsDisplayString.c_str());
		ImGui::EndChild();
		if (ImGui::Button("Specify CRS")) {
			spec.displayManualCrsWindow = true;
			spec.textSiezeFocus = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			spec.crsDisplayString = "Infer From Files";
		}
		ImGui::Text((spec.name + " Elevation Units").c_str());
		ImGui::RadioButton("Infer from files", &spec.unitRadio, 0);
		ImGui::RadioButton("Meters", &spec.unitRadio, 1);
		ImGui::RadioButton("International Feet", &spec.unitRadio, 2);
		ImGui::RadioButton("US Survey Feet", &spec.unitRadio, 3);


		if (spec.displayManualCrsWindow) {
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
			ImGui::SetNextWindowSize(ImVec2(400, 220));
			if (!ImGui::Begin((spec.name + " CRS Override").c_str(), &spec.displayManualCrsWindow, flags)) {
				ImGui::End();
			}
			else {
				if (spec.textSiezeFocus) {
					ImGui::SetKeyboardFocusHere();
					spec.textSiezeFocus = false;
				}
				ImGui::InputTextMultiline("", spec.crsBuffer.data(), spec.crsBuffer.size(), ImVec2(350, 100), 0);
				ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
				if (ImGui::Button("Specify from file")) {
					NFD::OpenDialog(spec.nfdFile);
				}
				if (spec.nfdFile) {
					updateCrsAndDisplayString(spec.nfdFile.get(), "No CRS in File", &spec.crsDisplayString, &spec.crs);
					spec.nfdFile.reset();
					spec.crsBuffer[0] = 0;
				}
				ImGui::SameLine();
				if (ImGui::Button("OK")) {
					updateCrsAndDisplayString(spec.crsBuffer.data(), "Infer From Files", &spec.crsDisplayString, &spec.crs);
					spec.displayManualCrsWindow = false;
				}
				ImGui::End();
			}
		}
	}
	void LasDemOverrideSpecifics::importFromBoost() {
		if (crsBoostString.size()) {
			updateCrsAndDisplayString(crsBoostString.c_str(), "Infer From Files", &crsDisplayString, &crs);
			crsBoostString.clear();
		}
		if (unitBoostString.size()) {
			unitRadio = unitRadioFromString(unitBoostString);
			unitBoostString.clear();
		}
		if (unitRadio == 0 && !crs.getZUnits().isUnknown()) {
			unitRadio = unitRadioFromString(crs.getZUnits().name);
		}
	}
	void updateUnitsBuffer(ParamBase::FloatBuffer& buff)
	{
		const Unit& src = getUnitsLastFrame();
		const Unit& dst = getCurrentUnits();
		try {
			double v = std::stod(buff.data());
			v = convertUnits(v, src, dst);
			std::string s = std::to_string(v);
			s.erase(s.find_last_not_of('0') + 1, std::string::npos);
			s.erase(s.find_last_not_of('.') + 1, std::string::npos);
			strncpy_s(buff.data(), buff.size(), s.c_str(), buff.size());
		}
		catch (...) {
			if (strlen(buff.data())) {
				const char* msg = "Error";
				strncpy_s(buff.data(), buff.size(), msg, buff.size());
			}
		}
	}
	void copyToBuffer(ParamBase::FloatBuffer& buff, coord_t x) {
		std::string s = std::to_string(x);
		s.erase(s.find_last_not_of('0') + 1, std::string::npos);
		s.erase(s.find_last_not_of('.') + 1, std::string::npos);
		strncpy_s(buff.data(), buff.size(), s.c_str(), s.size());
	}
	const Unit& getCurrentUnits() {
		auto& d = LapisData::getDataObject();
		return d.getCurrentUnits();
	}
	const Unit& getUnitsLastFrame() {
		auto& d = LapisData::getDataObject();
		return d.getPrevUnits();
	}
	const std::string& getUnitsPluralName() {
		return Parameter<ParamName::outUnits>::getUnitsPluralName();
	}

	Parameter<ParamName::name>::Parameter() {
		_nameBuffer[0] = 0;
	}
	void Parameter<ParamName::name>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {

		visible.add_options()((_nameCmd + ",N").c_str(), po::value<std::string>(&_nameBoost),
			"The name of the acquisition. If specified, will be included in the filenames and metadata.");
	}
	std::ostream& Parameter<ParamName::name>::printToIni(std::ostream& o) {
		o << _nameCmd << "=" << _nameBuffer.data() << "\n";
		return o;
	}
	ParamCategory Parameter<ParamName::name>::getCategory() const {
		return ParamCategory::data;
	}
	void Parameter<ParamName::name>::renderGui() {
		ImGui::Text("Run Name:");
		ImGui::SameLine();
		ImGui::InputText("##runname", _nameBuffer.data(), _nameBuffer.size());
	}
	void Parameter<ParamName::name>::importFromBoost() {
		if (_nameBoost.size()) {
			strncpy_s(_nameBuffer.data(), _nameBuffer.size(), _nameBoost.c_str(), _nameBuffer.size());
			_nameBoost.clear();
		}
	}
	void Parameter<ParamName::name>::updateUnits() {}
	void Parameter<ParamName::name>::prepareForRun() {
		_runString = _nameBuffer.data();
	}
	void Parameter<ParamName::name>::cleanAfterRun() {
		_runString.clear();
	}

	const std::string& Parameter<ParamName::name>::getName()
	{
		prepareForRun();
		return _runString;
	}

	Parameter<ParamName::las>::Parameter() {
		_spec.fileFilter = std::make_unique<nfdnfilteritem_t>(L"LAS Files", L"las,laz");
		_spec.name = "Laz";
		_spec.wildcards.push_back(".las");
		_spec.wildcards.push_back(".laz");
	}
	void Parameter<ParamName::las>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		visible.add_options()((_cmdName + ",L").c_str(), po::value<std::vector<std::string>>(&_spec.fileSpecsVector),
			"Specify input point cloud (las/laz) files in one of three ways:\n"
			"\tAs a file name pointing to a point cloud file\n"
			"As a folder name, which will haves it and its subfolders searched for .las or .laz files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.laz\n"
			"This option can be specified multiple times");
	}
	std::ostream& Parameter<ParamName::las>::printToIni(std::ostream& o) {
		for (const std::string& s : _spec.getFileSpecsSet()) {
			o << _cmdName << "=" << s << "\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::las>::getCategory() const {
		return ParamCategory::data;
	}
	void Parameter<ParamName::las>::renderGui() {
		lasDemUnifiedGui(_spec);
	}
	void Parameter<ParamName::las>::importFromBoost() {
		_spec.importFromBoost();
	}
	void Parameter<ParamName::las>::updateUnits() {}
	void Parameter<ParamName::las>::prepareForRun() {
		if (_fileExtents.size()) {
			return;
		}
		auto& d = LapisData::getDataObject();
		auto& lasOverride = d._getRawParam<ParamName::lasOverride>();
		auto& alignParam = d._getRawParam<ParamName::alignment>();

		if (alignParam.isDebug()) {
			return;
		}
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::findingLasFiles);

		CoordRef crsOver = lasOverride.getCurrentCrs();
		auto s = iterateOverFileSpecifiers(_spec.getFileSpecsSet(), &tryLasFile, crsOver, crsOver.getZUnits());

		
		CoordRef outCrs = alignParam.getCurrentOutCrs();
		if (outCrs.isEmpty()) {
			for (auto& v : s) {
				outCrs = v.ext.crs();
				if (!outCrs.isEmpty()) {
					break;
				}
			}
		}
		
		for (auto& v : s) {
			LasExtent e = { QuadExtent(v.ext, outCrs).outerExtent(), v.ext.nPoints() };
			_fileExtents.emplace_back(v.filename,e);
		}
		log.logMessage(std::to_string(_fileExtents.size()) + " Las Files Found");
		if (_fileExtents.size() == 0) {
			d.needAbort = true;
			log.logMessage("Aborting");
		}
	}
	void Parameter<ParamName::las>::cleanAfterRun() {
		_fileExtents.clear();
	}
	Extent Parameter<ParamName::las>::getFullExtent() {
		prepareForRun();
		auto& d = LapisData::getDataObject();
		auto& alignParam = d._getRawParam<ParamName::alignment>();
		auto& lasOverride = d._getRawParam<ParamName::lasOverride>();
		CoordRef crsOut = alignParam.getCurrentOutCrs();
		coord_t xmin = std::numeric_limits<coord_t>::max();
		coord_t ymin = std::numeric_limits<coord_t>::max();
		coord_t xmax = std::numeric_limits<coord_t>::lowest();
		coord_t ymax = std::numeric_limits<coord_t>::lowest();
		for (auto& l : _fileExtents) {
			if (!crsOut.isConsistentHoriz(l.ext.crs())) {
				Extent e = QuadExtent(l.ext, crsOut).outerExtent();
				xmin = std::min(xmin, e.xmin());
				xmax = std::max(xmax, e.xmax());
				ymin = std::min(ymin, e.ymin());
				ymax = std::max(ymax, e.ymax());
			}
			else {
				if (!l.ext.crs().isEmpty()) {
					crsOut = l.ext.crs();
					const Unit& u = lasOverride.getCurrentCrs().getZUnits();
					if (!u.isUnknown()) {
						crsOut.setZUnits(u);
					}
				}
				xmin = std::min(xmin, l.ext.xmin());
				xmax = std::max(xmax, l.ext.xmax());
				ymin = std::min(ymin, l.ext.ymin());
				ymax = std::max(ymax, l.ext.ymax());
			}
		}
		return Extent(xmin, xmax, ymin, ymax, crsOut);
	}
	const std::vector<LasFileExtent> Parameter<ParamName::las>::getAllExtents()
	{
		prepareForRun();
		return _fileExtents;
	}

	Parameter<ParamName::dem>::Parameter() {
		_spec.name = "Dem";
		_spec.wildcards.push_back("*.*");
	}
	void Parameter<ParamName::dem>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()((_cmdName + ",D").c_str(), po::value<std::vector<std::string>>(&_spec.fileSpecsVector),
			"Specify input raster elevation models in one of three ways:\n"
			"\tAs a file name pointing to a raster file\n"
			"As a folder name, which will haves it and its subfolders searched for raster files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.tif\n"
			"This option can be specified multiple times\n"
			"Most raster formats are supported, but arcGIS .gdb geodatabases are not");
	}
	std::ostream& Parameter<ParamName::dem>::printToIni(std::ostream& o){
		for (const std::string& s : _spec.getFileSpecsSet()) {
			o << _cmdName << "=" << s << "\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::dem>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::dem>::renderGui(){
		lasDemUnifiedGui(_spec);
	}
	void Parameter<ParamName::dem>::importFromBoost() {
		_spec.importFromBoost();
	}
	void Parameter<ParamName::dem>::updateUnits() {}
	void Parameter<ParamName::dem>::prepareForRun() {
		if (_fileAligns.size()) {
			return;
		}
		if (!_spec.getFileSpecsSet().size()) {
			return;
		}
		auto& d = LapisData::getDataObject();
		auto& demOverride = d._getRawParam<ParamName::demOverride>();
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::findingDemFiles);

		CoordRef crsOver = demOverride.getCurrentCrs();
		_fileAligns = iterateOverFileSpecifiers(_spec.getFileSpecsSet(), &tryDtmFile, crsOver, crsOver.getZUnits());
		log.logMessage(std::to_string(_fileAligns.size()) + " Dem Files Found");
	}
	void Parameter<ParamName::dem>::cleanAfterRun() {
		_fileAligns.clear();
	}

	Parameter<ParamName::output>::Parameter() {}
	void Parameter<ParamName::output>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()((_cmdName + ",O").c_str(), po::value<std::string>(&_boostString),
			"The output folder to store results in");
	}
	std::ostream& Parameter<ParamName::output>::printToIni(std::ostream& o){
		o << _cmdName << "=" << _buffer.data() << "\n";
		return o;
	}
	ParamCategory Parameter<ParamName::output>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::output>::renderGui(){
		ImGui::Text("Output Folder:");
		ImGui::SameLine();
		ImGui::InputText("", _buffer.data(), _buffer.size());
		ImGui::SameLine();
		if (ImGui::Button("Browse")) {
			NFD::PickFolder(_nfdFolder);
		}
		if (_nfdFolder) {
			strncpy_s(_buffer.data(), _buffer.size(), _nfdFolder.get(), _buffer.size());
			_nfdFolder.reset();
		}
	}
	void Parameter<ParamName::output>::importFromBoost() {
		if (_boostString.size()) {
			strncpy_s(_buffer.data(), _buffer.size(), _boostString.c_str(), _buffer.size());
			_boostString.clear();
		}
	}
	void Parameter<ParamName::output>::updateUnits() {}
	void Parameter<ParamName::output>::prepareForRun() {
		_path = _buffer.data();
		LapisData& d = LapisData::getDataObject();
		auto& nameParam = d._getRawParam<ParamName::name>();
		size_t maxFileLength = d.maxLapisFileName + _path.string().size() + nameParam.getName().size();
		LapisLogger& log = LapisLogger::getLogger();
		if (maxFileLength > d.maxTotalFilePath) {
			log.logMessage("Total file path is too long");
			log.logMessage("Aborting");
			d.needAbort = true;
		}
		namespace fs = std::filesystem;
		if (fs::is_directory(_path) || _path == "debug-test") { //so the tests can feed a dummy value that doesn't trip the error detection
			return;
		}
		try {
			fs::create_directory(_path);
		}
		catch (fs::filesystem_error e) {
			log.logMessage("Unable to create output directory");
			log.logMessage("Aborting");
			d.needAbort = true;
		}
	}
	void Parameter<ParamName::output>::cleanAfterRun() {
		_path.clear();
	}

	Parameter<ParamName::lasOverride>::Parameter() {
		_spec.crsBuffer = std::vector<char>(10000);
		_spec.crsBuffer[0] = 0; //this should be enough to make whatever garbage is in the other 9999 bytes invisible
		_spec.name = "Laz";
	}
	void Parameter<ParamName::lasOverride>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		hidden.add_options()(_lasUnitCmd.c_str(), po::value<std::string>(&_spec.unitBoostString))
			(_lasCrsCmd.c_str(), po::value<std::string>(&_spec.crsBoostString));
	}
	std::ostream& Parameter<ParamName::lasOverride>::printToIni(std::ostream& o){
		const Unit& u = unitRadioOrder[_spec.unitRadio];
		if (!u.isUnknown()) {
			o << _lasUnitCmd << "=" << u.name << "\n";
		}
		CoordRef crs;
		try {
			crs = CoordRef{ _spec.crsDisplayString };
		}
		catch (UnableToDeduceCRSException e) {}
		if (!crs.isEmpty()) {
			o << _lasCrsCmd << "=" << crs.getSingleLineWKT() << "\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::lasOverride>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::lasOverride>::renderGui(){
		lasDemOverrideUnifiedGui(_spec);
	}
	void Parameter<ParamName::lasOverride>::importFromBoost() {
		_spec.importFromBoost();
	}
	void Parameter<ParamName::lasOverride>::updateUnits() {}
	const CoordRef& Parameter<ParamName::lasOverride>::getCurrentCrs() {
		prepareForRun();
		return _spec.crs;
	}
	void Parameter<ParamName::lasOverride>::prepareForRun() {
		const Unit& u = unitRadioOrder[_spec.unitRadio];
		if (!u.isUnknown()) {
			_spec.crs.setZUnits(u);
		}
	}
	void Parameter<ParamName::lasOverride>::cleanAfterRun() {}

	Parameter<ParamName::demOverride>::Parameter() {
		_spec.crsBuffer = std::vector<char>(10000);
		_spec.crsBuffer[0] = 0; //this should be enough to make whatever garbage is in the other 9999 bytes invisible
		_spec.name = "Dem";
	}
	void Parameter<ParamName::demOverride>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		hidden.add_options()(_demUnitCmd.c_str(), po::value<std::string>(&_spec.unitBoostString))
			(_demCrsCmd.c_str(), po::value<std::string>(&_spec.crsBoostString));
	}
	std::ostream& Parameter<ParamName::demOverride>::printToIni(std::ostream& o){
		const Unit& u = unitRadioOrder[_spec.unitRadio];
		if (!u.isUnknown()) {
			o << _demUnitCmd << "=" << u.name << "\n";
		}
		CoordRef crs;
		try {
			crs = CoordRef{ _spec.crsDisplayString };
		}
		catch (UnableToDeduceCRSException e) {}
		if (!crs.isEmpty()) {
			o << _demCrsCmd << "=" << crs.getSingleLineWKT() << "\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::demOverride>::getCategory() const{
		return ParamCategory::data;
	}
	void Parameter<ParamName::demOverride>::renderGui(){
		lasDemOverrideUnifiedGui(_spec);
	}
	void Parameter<ParamName::demOverride>::importFromBoost() {
		_spec.importFromBoost();
	}
	void Parameter<ParamName::demOverride>::updateUnits() {}
	const CoordRef& Parameter<ParamName::demOverride>::getCurrentCrs() {
		prepareForRun();
		return _spec.crs;
	}
	void Parameter<ParamName::demOverride>::prepareForRun() {
		const Unit& u = unitRadioOrder[_spec.unitRadio];
		if (!u.isUnknown()) {
			_spec.crs.setZUnits(u);
		}
	}
	void Parameter<ParamName::demOverride>::cleanAfterRun() {}

	Parameter<ParamName::outUnits>::Parameter() : _radio(1) {}
	void Parameter<ParamName::outUnits>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_cmdName.c_str(), po::value<std::string>(&_boostString),
			"The units you want output to be in. All options will be interpretted as these units\n"
			"\tValues: m (for meters), f (for international feet), usft (for us survey feet)\n"
			"\tDefault is meters");
	}
	std::ostream& Parameter<ParamName::outUnits>::printToIni(std::ostream& o){
		const Unit& u = unitRadioOrder[_radio];
		if (!u.isUnknown()) {
			o << _cmdName << "=" << u.name << "\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::outUnits>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::outUnits>::renderGui(){
		ImGui::Text("Output Units");
		ImGui::RadioButton("Meters", &_radio, 1);
		ImGui::RadioButton("International Feet", &_radio, 2);
		ImGui::RadioButton("US Survey Feet", &_radio, 3);
	}
	const std::string& Parameter<ParamName::outUnits>::getUnitsPluralName() {
		auto& d = LapisData::getDataObject();
		auto& p = d._getRawParam<ParamName::outUnits>();
		return unitPluralNames[p._radio];
	}
	void Parameter<ParamName::outUnits>::importFromBoost() {
		if (_boostString.size()) {
			_radio = unitRadioFromString(_boostString);
			_boostString.clear();
		}
	}
	void Parameter<ParamName::outUnits>::updateUnits() {}
	void Parameter<ParamName::outUnits>::prepareForRun() {}
	void Parameter<ParamName::outUnits>::cleanAfterRun() {}

	Parameter<ParamName::alignment>::Parameter() : _cellsizeBoost(30), _xresBoost(30), _yresBoost(30), _xoriginBoost(0), _yoriginBoost(0),
	_debugBool(false) {
		_crsBuffer = std::vector<char>(10000);
		_crsBuffer[0] = 0;
	}
	void Parameter<ParamName::alignment>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()((_alignCmd + ",A").c_str(), po::value<std::string>(&_alignFileBoostString),
			"A raster file you want the output metrics to align with\n"
			"Overwritten by --cellsize and --out-crs options")
			(_cellsizeCmd.c_str(), po::value<coord_t>(&_cellsizeBoost),
				"The desired cellsize of the output metric rasters\n"
				"Defaults to 30 meters")
			(_crsCmd.c_str(), po::value<std::string>(&_crsBoostString),
				"The desired CRS for the output layers\n"
				"Can be a filename, or a manual specification (usually EPSG)");

		hidden.add_options()(_xresCmd.c_str(), po::value<coord_t>(&_xresBoost), "")
			(_yresCmd.c_str(), po::value<coord_t>(&_yresBoost), "")
			(_xoriginCmd.c_str(), po::value<coord_t>(&_xoriginBoost), "")
			(_yoriginCmd.c_str(), po::value<coord_t>(&_yoriginBoost), "")
			(_debugCmd.c_str(), po::bool_switch(&_debugBool), "")
			;
	}
	std::ostream& Parameter<ParamName::alignment>::printToIni(std::ostream& o){
		CoordRef crs;
		try {
			crs = CoordRef{ _crsDisplayString };
		}
		catch (UnableToDeduceCRSException e) {}

		o << _xresCmd << "=" << _xresBuffer.data() << "\n";
		o << _yresCmd << "=" << _yresBuffer.data() << "\n";
		o << _xoriginCmd << "=" << _xoriginBuffer.data() << "\n";
		o << _yoriginCmd << "=" << _yoriginBuffer.data() << "\n";
		if (!crs.isEmpty()) {
			o << _crsCmd << "=" << crs.getSingleLineWKT() << "\n";
		}

		return o;
	}
	ParamCategory Parameter<ParamName::alignment>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::alignment>::renderGui(){
		ImGui::Text("Output Alignment");
		if (ImGui::Button("Specify From File")) {
			NFD::OpenDialog(_nfdAlignFile);
		}

		if (_nfdAlignFile) {
			_alignFileBoostString = _nfdAlignFile.get();
			_nfdAlignFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Specify Manually")) {
			_displayManualWindow = true;
		}
		ImGui::Text("Output CRS:");
		ImGui::BeginChild("OutCRSText", ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(_crsDisplayString.c_str());
		ImGui::EndChild();
		ImGui::Text("Cellsize: ");
		ImGui::SameLine();
		if (_xyResDiffCheck) {
			ImGui::Text("Diff X/Y");
		}
		else {
			ImGui::Text(_cellsizeBuffer.data());
			ImGui::SameLine();
			ImGui::Text(getUnitsPluralName().c_str());
		}


		if (!_displayManualWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("Manual Alignment Window", &_displayManualWindow, flags)) {
			ImGui::End();
		}

		ImGui::Checkbox("Different X/Y Resolution", &_xyResDiffCheck);
		ImGui::Checkbox("Different X/Y Origin", &_xyOriginDiffCheck);

		auto input = [&](const char* name, FloatBuffer& buff) {
			bool edited = false;
			ImGui::Text(name);
			ImGui::SameLine();
			std::string label = "##" + std::string(name);
			edited = ImGui::InputText(label.c_str(), buff.data(), buff.size(), ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::Text(getUnitsPluralName().c_str());
			return edited;
		};

		bool cellSizeUpdate = false;
		if (_xyResDiffCheck) {
			cellSizeUpdate = input("X Resolution:", _xresBuffer) || cellSizeUpdate;
			cellSizeUpdate = input("Y Resolution:", _yresBuffer) || cellSizeUpdate;
			if (cellSizeUpdate) {
				if (atof(_xresBuffer.data()) == atof(_yresBuffer.data())) {
					strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), _xresBuffer.data());
				}
			}
		}
		else {
			cellSizeUpdate = input("Cellsize:", _cellsizeBuffer);
			if (cellSizeUpdate) {
				strcpy_s(_xresBuffer.data(), _xresBuffer.size(), _cellsizeBuffer.data());
				strcpy_s(_yresBuffer.data(), _yresBuffer.size(), _cellsizeBuffer.data());
			}
		}

		if (_xyOriginDiffCheck) {
			input("X Origin:", _xoriginBuffer);
			input("Y Origin:", _yoriginBuffer);
		}
		else {
			if (input("X/Y Origin:", _xoriginBuffer)) {
				strcpy_s(_yoriginBuffer.data(), _yoriginBuffer.size(), _xoriginBuffer.data());
			}
		}

		ImGui::Text("CRS Input:");
		ImGui::InputTextMultiline("", _crsBuffer.data(), _crsBuffer.size(),
			ImVec2(ImGui::GetContentRegionAvail().x, 300));
		ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
		if (ImGui::Button("Specify CRS From File")) {
			NFD::OpenDialog(_nfdCrsFile);
		}
		if (_nfdCrsFile) {
			try {
				CoordRef crs{ _nfdCrsFile.get() };
				strcpy_s(_crsBuffer.data(), _crsBuffer.size(), crs.getCompleteWKT().c_str());
			}
			catch (UnableToDeduceCRSException e) {
				_crsDisplayString = "Error";
				strcpy_s(_crsBuffer.data(), _crsBuffer.size(), "Error");
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crsDisplayString = "Same as LAS Files";
			_crsBuffer[0] = 0;
		}
		if (ImGui::Button("OK")) {
			_displayManualWindow = false;
			try {
				CoordRef crs{ _crsBuffer.data() };
				_crsDisplayString = crs.getSingleLineWKT();
			}
			catch (UnableToDeduceCRSException e) {
				_crsDisplayString = "Error inferring CRS from user input";
			}
		}

		ImGui::End();
	}
	void Parameter<ParamName::alignment>::updateUnits() {
		updateUnitsBuffer(_xresBuffer);
		updateUnitsBuffer(_yresBuffer);
		updateUnitsBuffer(_xoriginBuffer);
		updateUnitsBuffer(_yoriginBuffer);
		updateUnitsBuffer(_cellsizeBuffer);
	}
	void Parameter<ParamName::alignment>::importFromBoost() {
		if (_alignFileBoostString.size()) {
			try {
				Alignment a{ _alignFileBoostString };

				auto convertAndCopy = [&](FloatBuffer& buff, coord_t x) {
					const Unit& src = a.crs().getXYUnits();
					const Unit& dst = getCurrentUnits();
					x = convertUnits(x, src, dst);
					copyToBuffer(buff, x);
				};

				convertAndCopy(_xresBuffer, a.xres());
				convertAndCopy(_yresBuffer, a.yres());
				convertAndCopy(_xoriginBuffer, a.xOrigin());
				convertAndCopy(_yoriginBuffer, a.yOrigin());
				_xyOriginDiffCheck = (a.xOrigin() == a.yOrigin());
				if (a.xres() == a.yres()) {
					_xyResDiffCheck = false;
					convertAndCopy(_cellsizeBuffer, a.xres());
				}
				else {
					_xyResDiffCheck = true;
				}
				if (!a.crs().isEmpty()) {
					_crsDisplayString = a.crs().getSingleLineWKT();
				}
				else {
					_crsDisplayString = "Same as LAZ Files";
				}
				_crsBuffer[0] = 0;
			}
			catch (InvalidRasterFileException e) {
				std::string err = "Error";
				auto setError = [&](FloatBuffer& buff) {
					strcpy_s(buff.data(), buff.size(), err.c_str());
				};
				setError(_xresBuffer);
				setError(_yresBuffer);
				setError(_xoriginBuffer);
				setError(_yoriginBuffer);
				setError(_cellsizeBuffer);
				_crsBuffer[0] = 0;
			}

		}
		_alignFileBoostString.clear();
		if (!std::isnan(_cellsizeBoost)) {
			copyToBuffer(_cellsizeBuffer, _cellsizeBoost);
			copyToBuffer(_xresBuffer, _cellsizeBoost);
			copyToBuffer(_yresBuffer, _cellsizeBoost);
			_xyResDiffCheck = false;
		}
		_cellsizeBoost = std::nan("");

		bool xory = !std::isnan(_xresBoost) || !std::isnan(_yresBoost);
		if (!std::isnan(_xresBoost)) {
			copyToBuffer(_xresBuffer, _xresBoost);
		}
		_xresBoost = std::nan("");
		if (!std::isnan(_yresBoost)) {
			copyToBuffer(_yresBuffer, _yresBoost);
		}
		_yresBoost = std::nan("");
		if (xory) {
			if (strcmp(_xresBuffer.data(), _yresBuffer.data()) == 0) {
				strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), _xresBuffer.data());
				_xyResDiffCheck = false;
			}
			else {
				_xyResDiffCheck = true;
			}
		}

		xory = !std::isnan(_xoriginBoost) || !std::isnan(_yoriginBoost);
		if (!std::isnan(_xoriginBoost)) {
			copyToBuffer(_xoriginBuffer, _xoriginBoost);
		}
		_xoriginBoost = std::nan("");

		if (!std::isnan(_yoriginBoost)) {
			copyToBuffer(_yoriginBuffer, _yoriginBoost);
		}
		_yoriginBoost = std::nan("");
		if (xory) {
			_xyOriginDiffCheck = strcmp(_xoriginBuffer.data(), _yoriginBuffer.data());
		}

		if (_crsBoostString.size()) {
			CoordRef crs;
			try {
				crs = CoordRef{ _crsBoostString };
				_crsDisplayString = crs.getSingleLineWKT();
				_crsBuffer[0] = 0;
			}
			catch (UnableToDeduceCRSException e) {
				_crsDisplayString = "Error reading CRS";
			}
		}
		_crsBoostString.clear();
	}
	CoordRef Parameter<ParamName::alignment>::getCurrentOutCrs() const {
		CoordRef crs;
		try {
			crs = CoordRef(_crsDisplayString);
		}
		catch (UnableToDeduceCRSException e) {}
		return crs;
	}
	const Alignment& Parameter<ParamName::alignment>::getFullAlignment()
	{
		prepareForRun();
		return *_align;
	}
	void Parameter<ParamName::alignment>::prepareForRun() {
		if (_align) {
			return;
		}
		if (_debugBool) {
			//something intentionally weird so nothing will match it unless it copied from it
			_align = std::make_shared<Alignment>(Extent(0, 90, 10, 80), 8, 6, 13, 19);
			return;
		}
		auto& d = LapisData::getDataObject();
		auto& las = d._getRawParam<ParamName::las>();
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::settingUp);

		Extent e = las.getFullExtent();

		coord_t xres = getValueWithError(_xresBuffer, "cellsize");
		coord_t yres = getValueWithError(_yresBuffer, "cellsize");
		coord_t xorigin = getValueWithError(_xoriginBuffer, "origin");
		coord_t yorigin = getValueWithError(_yoriginBuffer, "origin");
		const Unit& src = d.getCurrentUnits();
		Unit dst = e.crs().getXYUnits();
		xres = convertUnits(xres, src, dst);
		yres = convertUnits(yres, src, dst);
		xorigin = convertUnits(xorigin, src, dst);
		yorigin = convertUnits(yorigin, src, dst);

		if (xres <= 0 || yres <= 0) {
			log.logMessage("Celsize must be positive");
			log.logMessage("Aborting");
			d.needAbort = true;
			xres = 30; yres = 30; //just so this function can complete without further errors before LapisData handles the abort
		}

		_align.reset();
		_align = std::make_shared<Alignment>(e, xorigin, yorigin, xres, yres);

		_nLaz = std::make_shared<Raster<int>>(*_align);
		auto& exts = las.getAllExtents();
		for (auto& le : exts) {
			std::vector<cell_t> cells = _nLaz->cellsFromExtent(le.ext, SnapType::out);
			for (cell_t cell : cells) {
				_nLaz->atCellUnsafe(cell).value()++;
				_nLaz->atCellUnsafe(cell).has_value() = true;
			}
		}
	}
	void Parameter<ParamName::alignment>::cleanAfterRun() {
		_align.reset();
		_nLaz.reset();
	}
	bool Parameter<ParamName::alignment>::isDebug() const
	{
		return _debugBool;
	}

	Parameter<ParamName::csmOptions>::Parameter() : _smooth(3), _footprintDiamBoost(0.4), _csmCellsizeBoost(1) {}
	void Parameter<ParamName::csmOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_csmCellsizeCmd.c_str(), po::value<coord_t>(&_csmCellsizeBoost),
			"The desired cellsize of the output canopy surface model\n"
			"Defaults to 1 meter");
		hidden.add_options()(_smoothCmd.c_str(), po::value<int>(&_smooth), "")
			(_footprintDiamCmd.c_str(), po::value<coord_t>(&_footprintDiamBoost), "");
	}
	std::ostream& Parameter<ParamName::csmOptions>::printToIni(std::ostream& o){

		o << _csmCellsizeCmd << "=" << _csmCellsizeBuffer.data() << "\n";
		o << _smoothCmd << "=" << _smooth << "\n";
		o << _footprintDiamCmd << "=" << _footprintDiamBuffer.data() << "\n";

		return o;
	}
	ParamCategory Parameter<ParamName::csmOptions>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::csmOptions>::renderGui(){
		ImGui::Text("Canopy Surface Model Options");

		ImGui::Text("Cellsize:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##CsmCellsize", _csmCellsizeBuffer.data(), _csmCellsizeBuffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(getUnitsPluralName().c_str());

		ImGui::Text("Smoothing:");
		ImGui::SameLine();
		ImGui::RadioButton("None", &_smooth, 1);
		ImGui::SameLine();
		ImGui::RadioButton("3x3", &_smooth, 3);
		ImGui::SameLine();
		ImGui::RadioButton("5x5", &_smooth, 5);

		ImGui::Checkbox("Show Advanced Options", &_showAdvanced);
		if (_showAdvanced) {
			ImGui::Text("Footprint Diameter:");
			ImGui::SameLine();
			ImGui::PushItemWidth(100);
			ImGui::InputText("##Footprint", _footprintDiamBuffer.data(), _footprintDiamBuffer.size());
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Text(getUnitsPluralName().c_str());
		}
	}
	void Parameter<ParamName::csmOptions>::updateUnits() {
		updateUnitsBuffer(_footprintDiamBuffer);
		updateUnitsBuffer(_csmCellsizeBuffer);
	}
	void Parameter<ParamName::csmOptions>::importFromBoost() {
		LapisLogger& log = LapisLogger::getLogger();
		if (_smooth % 2 == 0 || _smooth <= 0) {
			log.logMessage("Smooth factor must be a postive, odd integer");
		}
		if (_smooth % 2 == 0) {
			_smooth--;
		}
		if (_smooth <= 0) {
			_smooth = 1;
		}
		if (!std::isnan(_footprintDiamBoost)) {
			copyToBuffer(_footprintDiamBuffer, _footprintDiamBoost);
			_footprintDiamBoost = std::nan("");
		}
		if (!std::isnan(_csmCellsizeBoost)) {
			copyToBuffer(_csmCellsizeBuffer, _csmCellsizeBoost);
			_csmCellsizeBoost = std::nan("");
		}
	}
	void Parameter<ParamName::csmOptions>::prepareForRun() {
		if (_csmAlign) {
			return;
		}

		auto& d = LapisData::getDataObject();
		auto& alignParam = d._getRawParam<ParamName::alignment>();
		auto& log = LapisLogger::getLogger();
		
		_footprintDiamCache = getValueWithError(_footprintDiamBuffer, "footprint");
		coord_t cellsize = getValueWithError(_csmCellsizeBuffer, "csm cellsize");

		if (_footprintDiamCache < 0) {
			log.logMessage("Pulse diameter must be non-negative");
			log.logMessage("Aborting");
			d.needAbort = true;
		}
		if (cellsize <= 0) {
			log.logMessage("CSM Cellsize must be positive");
			log.logMessage("Aborting");
			d.needAbort = true;
			cellsize = 1;
		}

		const Alignment& metricAlign = alignParam.getFullAlignment();
		const Unit& u = d.getCurrentUnits();
		cellsize = convertUnits(cellsize, u, metricAlign.crs().getXYUnits());
		_csmAlign = std::make_shared<Alignment>(metricAlign, metricAlign.xOrigin(), metricAlign.yOrigin(), cellsize, cellsize);
	}
	void Parameter<ParamName::csmOptions>::cleanAfterRun() {
		_footprintDiamCache = std::nan("");
		_csmAlign.reset();
	}

	Parameter<ParamName::metricOptions>::Parameter() : _canopyBoost(2) {
		_strataBoost = "0.5,1,2,4,8,16,32,48,64";
	}
	void Parameter<ParamName::metricOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_canopyCmd.c_str(), po::value<coord_t>(&_canopyBoost),
			"The height threshold for a point to be considered canopy.")
			(_strataCmd.c_str(), po::value<std::string>(&_strataBoost),
				"A comma-separated list of strata breaks on which to calculate strata metrics.");
	}
	std::ostream& Parameter<ParamName::metricOptions>::printToIni(std::ostream& o){
		o << _canopyCmd << "=" << _canopyBuffer.data() << "\n";

		if (_strataBuffers.size()) {
			bool needComma = false;
			o << _strataCmd << "=";
			for (size_t i = 0; i < _strataBuffers.size(); ++i) {
				if (needComma) {
					o << ",";
				}
				o << _strataBuffers[i].data();
				needComma = true;
			}
			o << "\n";
		}

		return o;
	}
	ParamCategory Parameter<ParamName::metricOptions>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::metricOptions>::renderGui(){
		ImGui::Text("Metric Options");
		ImGui::Text("Canopy Cutoff:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##canopy", _canopyBuffer.data(), _canopyBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(getUnitsPluralName().c_str());

		ImGui::BeginChild("stratumleft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, ImGui::GetContentRegionAvail().y), false, 0);
		ImGui::Text("Stratum Breaks:");
		if (ImGui::Button("Change")) {
			_displayStratumWindow = true;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("stratumright", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
		for (size_t i = 0; i < _strataBuffers.size(); ++i) {
			ImGui::Text(_strataBuffers[i].data());
			ImGui::SameLine();
			ImGui::Text(getUnitsPluralName().c_str());
		}
		ImGui::EndChild();

		if (!_displayStratumWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("stratumwindow", &_displayStratumWindow, flags)) {
			ImGui::End();
		}
		if (ImGui::Button("Add")) {
			_strataBuffers.emplace_back();
			_strataBuffers[_strataBuffers.size() - 1][0] = 0;
		}
		if (ImGui::Button("Remove All")) {
			_strataBuffers.clear();
			_strataBuffers.emplace_back();
			_strataBuffers[_strataBuffers.size() - 1][0] = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			std::set<double> numericBreaks;
			for (size_t i = 0; i < _strataBuffers.size(); ++i) {
				try {
					numericBreaks.insert(std::stod(_strataBuffers[i].data()));
				}
				catch (std::invalid_argument e) { 
					LapisLogger::getLogger().logMessage("Unable to interpret " + std::string(_strataBuffers[i].data()) + " as a number");
				}
			}
			_strataBuffers.clear();
			for (double v : numericBreaks) {
				if (v != 0) {
					_strataBuffers.emplace_back();
					std::string s = std::to_string(v);
					s.erase(s.find_last_not_of('0') + 1, std::string::npos);
					s.erase(s.find_last_not_of('.') + 1, std::string::npos);
					strcpy_s(_strataBuffers[_strataBuffers.size() - 1].data(),
						_strataBuffers[_strataBuffers.size() - 1].size(), s.c_str());
				}
				else {
					_strataBuffers.emplace_back();
					strcpy_s(_strataBuffers[_strataBuffers.size() - 1].data(),
						_strataBuffers[_strataBuffers.size() - 1].size(), "0");
				}
			}
			_displayStratumWindow = false;
		}

		for (size_t i = 0; i < _strataBuffers.size(); ++i) {
			ImGui::InputText(("##" + std::to_string(i)).c_str(), _strataBuffers[i].data(), _strataBuffers[i].size());
			ImGui::SameLine();
			ImGui::Text(getUnitsPluralName().c_str());
		}

		ImGui::End();
	}
	void Parameter<ParamName::metricOptions>::updateUnits() {
		updateUnitsBuffer(_canopyBuffer);
		for (size_t i = 0; i < _strataBuffers.size(); ++i) {
			updateUnitsBuffer(_strataBuffers[i]);
		}
	}
	void Parameter<ParamName::metricOptions>::importFromBoost() {
		if (!std::isnan(_canopyBoost)) {
			copyToBuffer(_canopyBuffer, _canopyBoost);
			_canopyBoost = std::nan("");
		}

		if (!_strataBoost.size()) {
			return;
		}
		std::stringstream tokenizer{ _strataBoost };
		std::string temp;
		std::vector<double> numericStrata;
		while (std::getline(tokenizer, temp, ',')) {
			try {
				numericStrata.push_back(std::stod(temp));
			}
			catch (std::invalid_argument e) {
				LapisLogger::getLogger().logMessage("Unable to interpret " + temp + " as a number");
				numericStrata.clear();
				break;
			}
		}
		if (numericStrata.size()) {
			std::sort(numericStrata.begin(), numericStrata.end());
			_strataBuffers.clear();
			for (size_t i = 0; i < numericStrata.size(); ++i) {
				_strataBuffers.emplace_back();
				copyToBuffer(_strataBuffers[i], numericStrata[i]);
			}
		}
		_strataBoost.clear();
	}
	void Parameter<ParamName::metricOptions>::prepareForRun() {
		LapisLogger& log = LapisLogger::getLogger();
		_canopyCache = getValueWithError(_canopyBuffer, "canopy cutoff");
		_strataCache.resize(_strataBuffers.size());
		for (size_t i = 0; i < _strataCache.size(); ++i) {
			_strataCache[i] = std::stod(_strataBuffers[i].data());
		}
		if (_canopyCache < 0) {
			log.logMessage("Canopy cutoff is negative. Is this intentional?");
		}
	}
	void Parameter<ParamName::metricOptions>::cleanAfterRun() {
		_strataCache.clear();
		_strataCache.shrink_to_fit();
	}
	const std::vector<coord_t>& Parameter<ParamName::metricOptions>::getStrataBreaks()
	{
		prepareForRun();
		return _strataCache;
	}

	Parameter<ParamName::filterOptions>::Parameter() : _minhtBoost(-8), _maxhtBoost(100) {
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			_classChecks[i] = true;
		}
		_classChecks[7] = false;
		_classChecks[9] = false;
		_classChecks[18] = false;
		_updateClassListString();
	}
	void Parameter<ParamName::filterOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_minhtCmd.c_str(), po::value<coord_t>(&_minhtBoost),
			"The threshold for low outliers. Points with heights below this value will be excluded.")
			(_maxhtCmd.c_str(), po::value<coord_t>(&_maxhtBoost),
				"The threshold for high outliers. Points with heights above this value will be excluded.")
			(_classCmd.c_str(), po::value<std::string>(&_classBoost),
				"A comma-separated list of LAS point classifications to use for this run.\n"
				"Alternatively, preface the list with ~ to specify a blacklist.");
		hidden.add_options()(_useWithheldCmd.c_str(), po::bool_switch(&_useWithheld), "");
	}
	std::ostream& Parameter<ParamName::filterOptions>::printToIni(std::ostream& o){
		o << _minhtCmd << "=" << _minhtBuffer.data() << "\n";
		o << _maxhtCmd << "=" << _maxhtBuffer.data() << "\n";
		if (!_filterWithheldCheck) {
			o << _useWithheldCmd << "=\n";
		}

		int nallowed = 0;
		int nblocked = 0;
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			if (_classChecks[i]) {
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

			for (size_t i = 0; i < _classChecks.size(); ++i) {
				if (_classChecks[i] && (nblocked >= nallowed)) {
					if (comma) {
						classList += ",";
					}
					else {
						comma = true;
					}
					classList += std::to_string(i);
				}
				else if (!_classChecks[i] && (nblocked < nallowed)) {
					if (comma) {
						classList += ",";
					}
					else {
						comma = true;
					}
					classList += std::to_string(i);
				}
			}
			o << _classCmd << "=" << classList << "\n";
		}

		return o;
	}
	ParamCategory Parameter<ParamName::filterOptions>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::filterOptions>::renderGui(){
		ImGui::Text("Filter Options");
		if (ImGui::Checkbox("Exclude Withheld Points", &_filterWithheldCheck)) {
			_useWithheld = !_filterWithheldCheck;
		}

		ImGui::Text("Low Outlier:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##minht", _minhtBuffer.data(), _minhtBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(getUnitsPluralName().c_str());

		ImGui::Text("High Outlier:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputText("##maxht", _maxhtBuffer.data(), _maxhtBuffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text(getUnitsPluralName().c_str());

		ImGui::Text("Use Classes:");
		ImGui::SameLine();
		ImGui::BeginChild("##classlist", ImVec2(120, 50), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(_classDisplayString.c_str());
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::Button("Change")) {
			_displayClassWindow = true;
		}

		if (!_displayClassWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(300, 400));
		if (!ImGui::Begin("##classwindow", &_displayClassWindow, flags)) {
			ImGui::End();
		}
		else {
			if (ImGui::Button("Select All")) {
				for (size_t i = 0; i < _classChecks.size(); ++i) {
					_classChecks[i] = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Deselect All")) {
				for (size_t i = 0; i < _classChecks.size(); ++i) {
					_classChecks[i] = false;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("OK")) {
				_displayClassWindow = false;
				_updateClassListString();
			}

			ImGui::BeginChild("##classcheckboxchild", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 60), true, 0);
			for (size_t i = 0; i < _classChecks.size(); ++i) {
				std::string label = std::to_string(i);
				if (i < _classNames.size()) {
					label += " " + _classNames[i];
				}
				ImGui::Checkbox(label.c_str(), &_classChecks[i]);
			}
			ImGui::EndChild();

			ImGui::Text("Lapis does not classify points.\nThis filter only applies to\nclassification already performed\non the data.");
		}

		ImGui::End();
	}
	void Parameter<ParamName::filterOptions>::updateUnits() {
		updateUnitsBuffer(_minhtBuffer);
		updateUnitsBuffer(_maxhtBuffer);
	}
	void Parameter<ParamName::filterOptions>::importFromBoost() {
		if (!std::isnan(_minhtBoost)) {
			copyToBuffer(_minhtBuffer, _minhtBoost);
			_minhtBoost = std::nan("");
		}
		if (!std::isnan(_maxhtBoost)) {
			copyToBuffer(_maxhtBuffer, _maxhtBoost);
			_maxhtBoost = std::nan("");
		}
		_filterWithheldCheck = !_useWithheld;

		if (!_classBoost.size()) {
			return;
		}
		std::regex whitelistregex{ "[0-9]+(,[0-9]+)*" };
		std::regex blacklistregex{ "~[0-9]+(,[0-9]+)*" };

		bool isWhiteList = true;

		if (std::regex_match(_classBoost, whitelistregex)) {
			isWhiteList = true;
		}
		else if (std::regex_match(_classBoost, blacklistregex)) {
			isWhiteList = false;
			_classBoost = _classBoost.substr(1, _classBoost.size());
		}
		else {
			LapisLogger::getLogger().logMessage("Incorrect formatting for class filter");
			_classBoost.clear();
			return;
		}

		std::stringstream tokenizer{ _classBoost };
		std::string temp;
		std::unordered_set<int> set;
		while (std::getline(tokenizer, temp, ',')) {
			int cl = std::stoi(temp);
			if (cl > _classChecks.size() || cl < 0) {
				LapisLogger::getLogger().logMessage("Class values must be between 0 and 255");
			}
			else {
				set.insert(cl);
			}
		}
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			bool inset = set.contains((int)i);
			if (inset) {
				_classChecks[i] = isWhiteList;
			}
			else {
				_classChecks[i] = !isWhiteList;
			}
		}
		_classBoost.clear();
	}
	void Parameter<ParamName::filterOptions>::_updateClassListString() {
		_classDisplayString = "";
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			if (_classChecks[i]) {
				if (_classDisplayString.size()) {
					_classDisplayString += ",";
				}
				_classDisplayString += std::to_string(i);
			}
		}
	}
	void Parameter<ParamName::filterOptions>::prepareForRun() {
		if (_filters.size()) {
			return;
		}

		_minhtCache = getValueWithError(_minhtBuffer, "min height");
		_maxhtCache = getValueWithError(_maxhtBuffer, "max height");

		if (_minhtCache > _maxhtCache) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Low outlier must be less than high outlier");
			log.logMessage("Aborting");
			LapisData::getDataObject().needAbort = true;
		}

		if (_filterWithheldCheck) {
			_filters.emplace_back(new LasFilterWithheld());
		}
		int nallowed = 0;
		int nblocked = 0;
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			if (_classChecks[i]) {
				nallowed++;
			}
			else {
				nblocked++;
			}
		}

		if (nblocked == 0) {
			return;
		}
		std::unordered_set<uint8_t> classes;
		for (size_t i = 0; i < _classChecks.size(); ++i) {
			if (nallowed < nblocked && _classChecks[i]) {
				classes.insert((uint8_t)i);
			}
			if (nallowed >= nblocked && !_classChecks[i]) {
				classes.insert((uint8_t)i);
			}
		}
		if (nallowed < nblocked) {
			_filters.emplace_back(new LasFilterClassWhitelist(classes));
		}
		else {
			_filters.emplace_back(new LasFilterClassBlacklist(classes));
		}
	}
	void Parameter<ParamName::filterOptions>::cleanAfterRun() {
		_filters.clear();
		_filters.shrink_to_fit();
	}

	Parameter<ParamName::optionalMetrics>::Parameter() : _debugBool(false) {}
	void Parameter<ParamName::optionalMetrics>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_advPointCmd.c_str(), po::bool_switch(&_advPointCheck),
			"Calculate a larger suite of point metrics.")
			(_fineIntCmd.c_str(), po::bool_switch(&_fineIntCheck),
				"Create a canopy mean intensity raster with the same resolution as the CSM");

		hidden.add_options()(_debugNoAllocRaster.c_str(), po::bool_switch(&_debugBool), "");
	}
	std::ostream& Parameter<ParamName::optionalMetrics>::printToIni(std::ostream& o){
		if (_fineIntCheck) {
			o << _fineIntCmd << "=\n";
		}
		if (_advPointCheck) {
			o << _advPointCmd << "=\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::optionalMetrics>::getCategory() const{
		return ParamCategory::process;
	}
	void Parameter<ParamName::optionalMetrics>::renderGui(){
		ImGui::Text("Optional Product Options");
		ImGui::Checkbox("Advanced Point Metrics", &_advPointCheck);
		ImGui::Checkbox("Fine-scale Canopy Intensity", &_fineIntCheck);
	}
	void Parameter<ParamName::optionalMetrics>::importFromBoost() {}
	void Parameter<ParamName::optionalMetrics>::updateUnits() {}
	void Parameter<ParamName::optionalMetrics>::prepareForRun() {
		if (_debugBool) {
			return;
		}

		auto& d = LapisData::getDataObject();
		const Alignment& a = d._getRawParam<ParamName::alignment>().getFullAlignment();

		using oul = OutputUnitLabel;
		using pmc = PointMetricCalculator;

		auto addMetric = [&](const std::string& name, MetricFunc f, oul u) {
			_allReturnPointMetrics.emplace_back(name, f, a, u);
			_firstReturnPointMetrics.emplace_back(name, f, a, u);
		};

		addMetric("Mean_CanopyHeight", &pmc::meanCanopy, oul::Default);
		addMetric("StdDev_CanopyHeight", &pmc::stdDevCanopy, oul::Default);
		addMetric("25thPercentile_CanopyHeight", &pmc::p25Canopy, oul::Default);
		addMetric("50thPercentile_CanopyHeight", &pmc::p50Canopy, oul::Default);
		addMetric("75thPercentile_CanopyHeight", &pmc::p75Canopy, oul::Default);
		addMetric("95thPercentile_CanopyHeight", &pmc::p95Canopy, oul::Default);
		addMetric("TotalReturnCount", &pmc::returnCount, oul::Unitless);
		addMetric("CanopyCover", &pmc::canopyCover, oul::Percent);

		if (_advPointCheck) {
			addMetric("CoverAboveMean", &pmc::coverAboveMean, oul::Percent);
			addMetric("CanopyReliefRatio", &pmc::canopyReliefRatio, oul::Unitless);
			addMetric("CanopySkewness", &pmc::skewnessCanopy, oul::Unitless);
			addMetric("CanopyKurtosis", &pmc::kurtosisCanopy, oul::Unitless);
			addMetric("05thPercentile_CanopyHeight", &pmc::p05Canopy, oul::Default);
			addMetric("10thPercentile_CanopyHeight", &pmc::p10Canopy, oul::Default);
			addMetric("15thPercentile_CanopyHeight", &pmc::p15Canopy, oul::Default);
			addMetric("20thPercentile_CanopyHeight", &pmc::p20Canopy, oul::Default);
			addMetric("30thPercentile_CanopyHeight", &pmc::p30Canopy, oul::Default);
			addMetric("35thPercentile_CanopyHeight", &pmc::p35Canopy, oul::Default);
			addMetric("40thPercentile_CanopyHeight", &pmc::p40Canopy, oul::Default);
			addMetric("45thPercentile_CanopyHeight", &pmc::p45Canopy, oul::Default);
			addMetric("55thPercentile_CanopyHeight", &pmc::p55Canopy, oul::Default);
			addMetric("60thPercentile_CanopyHeight", &pmc::p60Canopy, oul::Default);
			addMetric("65thPercentile_CanopyHeight", &pmc::p65Canopy, oul::Default);
			addMetric("70thPercentile_CanopyHeight", &pmc::p70Canopy, oul::Default);
			addMetric("80thPercentile_CanopyHeight", &pmc::p80Canopy, oul::Default);
			addMetric("85thPercentile_CanopyHeight", &pmc::p85Canopy, oul::Default);
			addMetric("90thPercentile_CanopyHeight", &pmc::p90Canopy, oul::Default);
			addMetric("99thPercentile_CanopyHeight", &pmc::p99Canopy, oul::Default);
		}

		const std::vector<coord_t>& strataBreaks = d._getRawParam<ParamName::metricOptions>().getStrataBreaks();
		if (strataBreaks.size()) {
			auto to_string_with_precision = [](coord_t v)->std::string {
				std::ostringstream out;
				out.precision(2);
				out << v;
				return out.str();
			};
			std::vector<std::string> stratumNames;
			stratumNames.push_back("LessThan" + to_string_with_precision(strataBreaks[0]));
			for (size_t i = 1; i < strataBreaks.size(); ++i) {
				stratumNames.push_back(to_string_with_precision(strataBreaks[i - 1]) + "To" + to_string_with_precision(strataBreaks[i]));
			}
			stratumNames.push_back("GreaterThan" + to_string_with_precision(strataBreaks[strataBreaks.size() - 1]));

			_allReturnStratumMetrics.emplace_back("StratumCover_", stratumNames, &pmc::stratumCover, a, oul::Percent);
			_allReturnStratumMetrics.emplace_back("StratumReturnProportion_", stratumNames, &pmc::stratumPercent, a, oul::Percent);
			_firstReturnStratumMetrics.emplace_back("StratumCover_", stratumNames, &pmc::stratumCover, a, oul::Percent);
			_firstReturnStratumMetrics.emplace_back("StratumReturnProportion_", stratumNames, &pmc::stratumPercent, a, oul::Percent);
		}

		_elevNumerator = std::make_shared<Raster<coord_t>>(a);
		_elevDenominator = std::make_shared<Raster<coord_t>>(a);
		_topoMetrics.push_back({ viewSlope<coord_t,metric_t>,"Slope",oul::Radian });
		_topoMetrics.push_back({ viewAspect<coord_t,metric_t>,"Aspect",oul::Radian });

		_csmMetrics.push_back({ &viewMax<csm_t>, "MaxCSM", a,oul::Default });
		_csmMetrics.push_back({ &viewMean<csm_t>, "MeanCSM", a,oul::Default });
		_csmMetrics.push_back({ &viewStdDev<csm_t>, "StdDevCSM", a,oul::Default });
		_csmMetrics.push_back({ &viewRumple<csm_t>, "RumpleCSM", a,oul::Unitless });

		_allReturnCalculators = std::make_shared<Raster<PointMetricCalculator>>(a);
		_firstReturnCalculators = std::make_shared<Raster<PointMetricCalculator>>(a);
	}
	void Parameter<ParamName::optionalMetrics>::cleanAfterRun() {
		_allReturnPointMetrics.clear();
		_firstReturnPointMetrics.clear();
		_allReturnStratumMetrics.clear();
		_firstReturnStratumMetrics.clear();
		_csmMetrics.clear();
		_topoMetrics.clear();
		_elevNumerator.reset();
		_elevDenominator.reset();
		_allReturnCalculators.reset();
		_firstReturnCalculators.reset();
	}

	Parameter<ParamName::computerOptions>::Parameter() : _threadBoost(defaultNThread()), _perfCheck(false) {}
	void Parameter<ParamName::computerOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		visible.add_options()(_threadCmd.c_str(), po::value<int>(&_threadBoost),
			"The number of threads to run Lapis on. Defaults to the number of cores on the computer");
		hidden.add_options()(_perfCmd.c_str(), po::bool_switch(&_perfCheck), "")
			;
	}
	std::ostream& Parameter<ParamName::computerOptions>::printToIni(std::ostream& o) {
		o << _threadCmd << "=" << _threadBuffer.data() << "\n";
		if (_perfCheck) {
			o << _perfCmd << "=\n";
		}
		return o;
	}
	ParamCategory Parameter<ParamName::computerOptions>::getCategory() const {
		return ParamCategory::computer;
	}
	void Parameter<ParamName::computerOptions>::renderGui() {
		ImGui::Text("Number of threads:");
		ImGui::SameLine();
		ImGui::InputText("", _threadBuffer.data(), _threadBuffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::Checkbox("Performance Mode", &_perfCheck);
	}
	void Parameter<ParamName::computerOptions>::importFromBoost() {
		if (_threadBoost > 0) {
			int x = _threadBoost < 999 ? _threadBoost : 999;
			x = x > 0 ? x : 1;
			copyToBuffer(_threadBuffer, x);
			_threadBoost = 0;
		}
	}
	void Parameter<ParamName::computerOptions>::updateUnits() {}
	void Parameter<ParamName::computerOptions>::prepareForRun() {
		_threadCache = std::stoi(_threadBuffer.data());
	}
	void Parameter<ParamName::computerOptions>::cleanAfterRun() {}

	std::set<std::string>& LasDemSpecifics::getFileSpecsSet() {
		return fileSpecsSet;
	}
	const std::set<std::string>& LasDemSpecifics::getFileSpecsSet() const
	{
		return fileSpecsSet;
	}
	void LasDemSpecifics::importFromBoost()
	{
		if (!fileSpecsVector.size()) {
			return;
		}
		fileSpecsSet.clear();
		for (auto& s : fileSpecsVector) {
			fileSpecsSet.insert(s);
		}
		fileSpecsVector.clear();
	}


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
	coord_t getValueWithError(const ParamBase::FloatBuffer& buff, const std::string& name) {
		LapisLogger& log = LapisLogger::getLogger();
		coord_t out = 30;
		try {
			out = std::stod(buff.data());
		}
		catch (std::invalid_argument e) {
			log.logMessage("Error reading value of " + name);
			log.logMessage("Aborting");
			LapisData::getDataObject().needAbort = true;
		}
		return out;
	}
}
