#include "app_pch.hpp"
#include "LapisParameters.hpp"
#include"LapisUtils.hpp"
#include"lapisdata.hpp"

namespace lapis {
	void lasDemUnifiedGui(LasDemSpecifics& spec)
	{
		std::set<std::string>& fileSpecSet = spec.getFileSpecsSet();
		std::string buttonText = "Add " + spec.name + " Files";
		if (ImGui::Button(buttonText.c_str())) {
			NFD::OpenDialogMultiple(spec.nfdFiles, spec.fileFilter.get(), 1, (const nfdnchar_t*)nullptr);
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
	std::string getDisplayCrsString(const char* inStr, const char* defaultMessage) {
		std::string out;
		CoordRef crs;
		try {
			crs = CoordRef(inStr);
			if (crs.isEmpty()) {
				out = defaultMessage;
			}
			else {
				out = crs.getSingleLineWKT();
			}
		}
		catch (UnableToDeduceCRSException e) {
			out = "Error reading CRS";
		}
		return out;
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
					spec.crsDisplayString = getDisplayCrsString(spec.nfdFile.get(), "No CRS in File");
					spec.nfdFile.reset();
					spec.crsBuffer[0] = 0;
				}
				ImGui::SameLine();
				if (ImGui::Button("OK")) {
					spec.crsDisplayString = getDisplayCrsString(spec.crsBuffer.data(), "Infer From Files");
					spec.displayManualCrsWindow = false;
				}
				ImGui::End();
			}
		}
	}
	void LasDemOverrideSpecifics::importFromBoost() {
		if (crsBoostString.size()) {
			crsDisplayString = getDisplayCrsString(crsBoostString.c_str(), "Infer From Files");
			crsBoostString.clear();
		}
		if (unitBoostString.size()) {
			unitRadio = unitRadioFromString(unitBoostString);
			unitBoostString.clear();
		}
	}
	void updateUnitsBuffer(ParamBase::FloatBuffer& buff)
	{
		
		const Unit& src = getUnitsLastFrame();
		const Unit& dst = getCurrentUnits();
		try {
			double v = atof(buff.data());
			v = convertUnits(v, src, dst);
			std::string s = std::to_string(v);
			s.erase(s.find_last_not_of('0') + 1, std::string::npos);
			s.erase(s.find_last_not_of('.') + 1, std::string::npos);
			strncpy_s(buff.data(), buff.size(), s.c_str(), buff.size());
		}
		catch (...) {
			const char* msg = "Error";
			strncpy_s(buff.data(), buff.size(), msg, buff.size());
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
		return d.getUserUnits();
	}
	const Unit& getUnitsLastFrame() {
		auto& d = LapisData::getDataObject();
		return d.getPrevUnits();
	}
	const std::string& getUnitsPluralName() {
		return Parameter<ParamName::outUnits>::getUnitsPluralName();
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

	Parameter<ParamName::output>::Parameter() {}
	void Parameter<ParamName::output>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()((_cmdName + ",O").c_str(), po::value<std::string>(&_string),
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

		ImGui::InputText("", _buffer.data(), _buffer.size());

		if (ImGui::Button("Browse")) {
			NFD::PickFolder(_nfdFolder);
		}
		if (_nfdFolder) {
			strncpy_s(_buffer.data(), _buffer.size(), _nfdFolder.get(), _buffer.size());
			_nfdFolder.reset();
		}
	}
	void Parameter<ParamName::output>::importFromBoost() {
		if (_string.size()) {
			strncpy_s(_buffer.data(), _buffer.size(), _string.c_str(), _buffer.size());
			_string.clear();
		}
	}
	void Parameter<ParamName::output>::updateUnits() {}

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

	Parameter<ParamName::outUnits>::Parameter() {}
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
		return unitPluralNames[_radio];
	}
	void Parameter<ParamName::outUnits>::importFromBoost() {
		if (_boostString.size()) {
			_radio = unitRadioFromString(_boostString);
			_boostString.clear();
		}
	}
	void Parameter<ParamName::outUnits>::updateUnits() {}

	Parameter<ParamName::alignment>::Parameter() : _cellsizeBoost(30), _xresBoost(30), _yresBoost(30), _xoriginBoost(0), _yoriginBoost(0) {
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
			(_crsBoostString.c_str(), po::value<std::string>(&_crsBoostString),
				"The desired CRS for the output layers\n"
				"Can be a filename, or a manual specification (usually EPSG)");

		hidden.add_options()(_xresCmd.c_str(), po::value<coord_t>(&_xresBoost), "")
			(_yresCmd.c_str(), po::value<coord_t>(&_yresBoost), "")
			(_xoriginCmd.c_str(), po::value<coord_t>(&_xoriginBoost), "")
			(_yoriginCmd.c_str(), po::value<coord_t>(&_yoriginBoost), "");
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
		ImGui::Text(_cellsizeBuffer.data());
		ImGui::SameLine();
		if (atof(_cellsizeBuffer.data()) != 0) {
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
				else {
					strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), "Diff X/Y");
				}
			}
		}
		else {
			cellSizeUpdate = input("X/Y Resolution:", _cellsizeBuffer);
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
		if (std::atof(_cellsizeBuffer.data()) > 0) {
			updateUnitsBuffer(_cellsizeBuffer);
		}
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
				if (a.xres() == a.yres()) {
					convertAndCopy(_cellsizeBuffer, a.xres());
				}
				else {
					strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), "Diff X/Y");
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
				_crsDisplayString = err;
				_crsBuffer[0] = 0;
			}

		}
		_alignFileBoostString.clear();
		if (!std::isnan(_cellsizeBoost)) {
			copyToBuffer(_cellsizeBuffer, _cellsizeBoost);
			copyToBuffer(_xresBuffer, _cellsizeBoost);
			copyToBuffer(_yresBuffer, _cellsizeBoost);
		}
		_cellsizeBoost = std::nan("");

		if (!std::isnan(_xresBoost)) {
			copyToBuffer(_xresBuffer, _xresBoost);
		}
		_xresBoost = std::nan("");
		if (!std::isnan(_yresBoost)) {
			copyToBuffer(_yresBuffer, _yresBoost);
		}
		_yresBoost = std::nan("");
		if (strcmp(_xresBuffer.data(), _yresBuffer.data()) != 0) {
			strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), _xresBuffer.data());
		}
		else {
			strcpy_s(_cellsizeBuffer.data(), _cellsizeBuffer.size(), "Diff X/Y");
		}

		if (!std::isnan(_xoriginBoost)) {
			copyToBuffer(_xoriginBuffer, _xoriginBoost);
		}
		_xoriginBoost = std::nan("");

		if (!std::isnan(_yoriginBoost)) {
			copyToBuffer(_yoriginBuffer, _yoriginBoost);
		}
		_yoriginBoost = std::nan("");

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
		//TODO: log the errors
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
			copyToBuffer(_footprintDiamBuffer, _footprintDiamBoost);
			_footprintDiamBoost = std::nan("");
		}
	}

	Parameter<ParamName::metricOptions>::Parameter() : _canopyBoost(2) {}
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
		}
		if (ImGui::Button("Remove All")) {
			_strataBuffers.clear();
			_strataBuffers.emplace_back();
		}
		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			std::set<double> numericBreaks;
			for (size_t i = 0; i < _strataBuffers.size(); ++i) {
				try {
					numericBreaks.insert(std::stod(_strataBuffers[i].data()));
				}
				catch (std::runtime_error e) { /*TODO: log the error*/ }
			}
			_strataBuffers.clear();
			for (double v : numericBreaks) {
				if (v > 0) {
					_strataBuffers.emplace_back();
					std::string s = std::to_string(v);
					s.erase(s.find_last_not_of('0') + 1, std::string::npos);
					s.erase(s.find_last_not_of('.') + 1, std::string::npos);
					strcpy_s(_strataBuffers[_strataBuffers.size() - 1].data(),
						_strataBuffers[_strataBuffers.size() - 1].size(), s.c_str());
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
				//TODO: log the error
				numericStrata.clear();
				break;
			}
		}
		if (numericStrata.size()) {
			std::sort(numericStrata.begin(), numericStrata.end());
			_strataBuffers.clear();
			_strataBuffers.resize(numericStrata.size());
			for (size_t i = 0; i < numericStrata.size(); ++i) {
				_strataBuffers.emplace_back();
				copyToBuffer(_strataBuffers[i], numericStrata[i]);
			}
		}
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
			//TODO: log error
			_classBoost.clear();
			return;
		}

		std::stringstream tokenizer{ _classBoost };
		std::string temp;
		std::unordered_set<int> set;
		while (std::getline(tokenizer, temp, ',')) {
			int cl = std::stoi(temp);
			if (cl > _classChecks.size() || cl < 0) {
				//TODO: log error
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

	Parameter<ParamName::optionalMetrics>::Parameter() {}
	void Parameter<ParamName::optionalMetrics>::addToCmd(po::options_description& visible,
		po::options_description& hidden){
		visible.add_options()(_advPointCmd.c_str(), po::bool_switch(&_advPointCheck),
			"Calculate a larger suite of point metrics.")
			(_fineIntCmd.c_str(), po::bool_switch(&_fineIntCheck),
				"Create a canopy mean intensity raster with the same resolution as the CSM");
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

	Parameter<ParamName::computerOptions>::Parameter() : _threadBoost(defaultNThread()), _gdalprojCheck(false), _perfCheck(false) {}
	void Parameter<ParamName::computerOptions>::addToCmd(po::options_description& visible,
		po::options_description& hidden) {
		visible.add_options()(_threadCmd.c_str(), po::value<int>(&_threadBoost),
			"The number of threads to run Lapis on. Defaults to the number of cores on the computer");
		hidden.add_options()(_perfCmd.c_str(), po::bool_switch(&_perfCheck), "")
			(_displayGdalProjCmd.c_str(), po::bool_switch(&_gdalprojCheck), "");
	}
	std::ostream& Parameter<ParamName::computerOptions>::printToIni(std::ostream& o) {
		o << _threadCmd << "=" << _threadBuffer.data() << "\n";
		if (_perfCheck) {
			o << _perfCmd << "=\n";
		}
		if (_gdalprojCheck) {
			o << _displayGdalProjCmd << "=\n";
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
		ImGui::Checkbox("Display GDAL and Proj warnings", &_gdalprojCheck);
	}
	void Parameter<ParamName::computerOptions>::importFromBoost() {
		if (_threadBoost > 0) {
			int x = _threadBoost < 999 ? _threadBoost : 999;
			_threadBoost = 0;
		}
	}
	void Parameter<ParamName::computerOptions>::updateUnits() {}

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
}
