#include"app_pch.hpp"
#include"CommonGuiElements.hpp"
#include"LapisData.hpp"
#include"LapisLogger.hpp"

namespace lapis {


	CmdGuiElement::CmdGuiElement(const std::string& guiDesc, const std::string& cmdName)
		: _cmdName(cmdName), _guiDesc(guiDesc), _hidden(true), _cmdDesc()
	{
	}
	CmdGuiElement::CmdGuiElement(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _guiDesc(guiDesc), _cmdName(cmdName), _cmdDesc(cmdDescription), _hidden(false)
	{
	}

	void CmdGuiElement::addShortCmdAlias(char alias)
	{
		_alias = alias;
	}

	CheckBox::CheckBox(const std::string& guiDesc, const std::string& cmdName)
		: _state(false), _boostState(false), CmdGuiElement(guiDesc, cmdName)
	{
	}
	CheckBox::CheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _state(false), _boostState(false), CmdGuiElement(guiDesc, cmdName, cmdDescription)
	{
	}
	void CheckBox::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase<bool>(visible, hidden, &_boostState);
	}
	std::ostream& CheckBox::printToIni(std::ostream& o) const
	{
		if (_state) {
			o << _cmdName << "=\n";
		}
		return o;
	}
	bool CheckBox::renderGui()
	{
		std::string label = _guiDesc + "##" + _cmdName;
		return ImGui::Checkbox(label.c_str(), &_state);
	}
	bool CheckBox::importFromBoost()
	{
		_state = _boostState;
		return true;
	}
	bool CheckBox::currentState() const
	{
		return _state;
	}
	void CheckBox::setState(bool b)
	{
		_state = b;
	}

	InvertedCheckBox::InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName)
		: _state(true), _boostState(false), CmdGuiElement(guiDesc, cmdName)
	{
	}
	InvertedCheckBox::InvertedCheckBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: _state(true), _boostState(false), CmdGuiElement(guiDesc, cmdName, cmdDescription)
	{
	}
	void InvertedCheckBox::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase<bool>(visible, hidden, &_boostState);
	}
	std::ostream& InvertedCheckBox::printToIni(std::ostream& o) const
	{
		if (!_state) {
			o << _cmdName << "=\n";
		}
		return o;
	}
	bool InvertedCheckBox::renderGui()
	{
		std::string label = _guiDesc + "##" + _cmdName;
		return ImGui::Checkbox(label.c_str(), &_state);
	}
	bool InvertedCheckBox::importFromBoost()
	{
		_state = !_boostState;
		return true;
	}
	bool InvertedCheckBox::currentState() const
	{
		return _state;
	}
	void InvertedCheckBox::setState(bool b)
	{
		_state = b;
	}

	RadioBoolean::RadioBoolean(const std::string& cmdName, const std::string& trueDisplayString, const std::string& falseDisplayString)
		: CmdGuiElement("", cmdName), _trueDisplayString(trueDisplayString + "##" + _cmdName), _falseDisplayString(falseDisplayString + "##" + _cmdName)
	{
	}
	RadioBoolean::RadioBoolean(const std::string& cmdName, const std::string& trueDisplayString, const std::string& falseDisplayString, const std::string& cmdDescription)
		: CmdGuiElement("",cmdName,cmdDescription), _trueDisplayString(trueDisplayString), _falseDisplayString(falseDisplayString)
	{
	}
	void RadioBoolean::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostState);
	}
	std::ostream& RadioBoolean::printToIni(std::ostream& o) const
	{
		if (_radio == TRUE_RADIO) {
			o << _cmdName << "=\n";
		}

		return o;
	}
	bool RadioBoolean::renderGui()
	{
		bool changed = false;
		if (_trueFirst) {
			changed = ImGui::RadioButton(_trueDisplayString.c_str(), &_radio, TRUE_RADIO) || changed;
			ImGui::SameLine();
			changed = ImGui::RadioButton(_falseDisplayString.c_str(), &_radio, FALSE_RADIO) || changed;
		}
		else {
			changed = ImGui::RadioButton(_falseDisplayString.c_str(), &_radio, FALSE_RADIO) || changed;
			ImGui::SameLine();
			changed = ImGui::RadioButton(_trueDisplayString.c_str(), &_radio, TRUE_RADIO) || changed;
		}
		return changed;
	}
	bool RadioBoolean::importFromBoost()
	{
		if (_boostState) {
			_radio = TRUE_RADIO;
		}
		else {
			_radio = FALSE_RADIO;
		}
		return true;
	}
	bool RadioBoolean::currentState() const
	{
		return _radio == TRUE_RADIO;
	}
	void RadioBoolean::setState(bool b)
	{
		if (b) {
			_radio = TRUE_RADIO;
		}
		else {
			_radio = FALSE_RADIO;
		}
	}
	void RadioBoolean::displayFalseFirst()
	{
		_trueFirst = false;
	}

	RadioDoubleBoolean::RadioDoubleBoolean(const std::string& firstCmdName, const std::string& secondCmdName)
		: _firstCmdName(firstCmdName), _secondCmdName(secondCmdName), CmdGuiElement("", "")
	{
	}
	void RadioDoubleBoolean::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		hidden.add_options()(_firstCmdName.c_str(), po::bool_switch(&_firstBoost), "")
			(_secondCmdName.c_str(), po::bool_switch(&_secondBoost), "");
	}
	std::ostream& RadioDoubleBoolean::printToIni(std::ostream& o) const
	{
		if (currentState(FIRST)) {
			o << _firstCmdName << "=\n";
		}
		if (currentState(SECOND)) {
			o << _secondCmdName << "=\n";
		}
		return o;
	}
	bool RadioDoubleBoolean::renderGui()
	{
		bool changed = false;

		bool init = false;
		for (auto& v : _buttons) {
			if (!init) {
				init = true;
			}
			else {
				ImGui::SameLine();
			}
			std::string label = v.second.displayString + "##" + _firstCmdName + _secondCmdName;
			changed = ImGui::RadioButton(label.c_str(), &_radio, v.first) || changed;
		}
		return changed;
	}
	bool RadioDoubleBoolean::importFromBoost()
	{
		for (auto& v : _buttons) {
			if (v.second.first == _firstBoost && v.second.second == _secondBoost) {
				_radio = v.first;
			}
		}
		return true;
	}
	bool RadioDoubleBoolean::currentState(int which) const
	{
		if (which == FIRST) {
			return _buttons.at(_radio).first;
		}
		else {
			return _buttons.at(_radio).second;
		}
	}
	void RadioDoubleBoolean::addCombo(bool firstState, bool secondState, int ordering, const std::string& displayString)
	{
		_buttons[ordering] = { firstState, secondState, displayString };
		if (!_defaultSet) {
			_radio = ordering;
			_defaultSet = true;
		}
	}

	NumericTextBoxWithUnits::NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue)
		: CmdGuiElement(guiDesc, cmdName), _boostValue(defaultValue), _buffer()
	{
		importFromBoost();
	}
	NumericTextBoxWithUnits::NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription), _boostValue(defaultValue)
	{
		importFromBoost();
	}
	void NumericTextBoxWithUnits::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase<coord_t>(visible, hidden, &_boostValue);
	}
	std::ostream& NumericTextBoxWithUnits::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=" << _buffer.data() << "\n";

		return o;
	}
	bool NumericTextBoxWithUnits::renderGui()
	{
		bool changed;

		ImGui::Text(_guiDesc.c_str());

		ImGui::SameLine();
		static constexpr int pixelWidth = 100;
		ImGui::PushItemWidth(pixelWidth);
		const std::string name = "##" + _cmdName;
		changed = ImGui::InputText(name.c_str(), _buffer.data(), _buffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();

		ImGui::SameLine();
		LapisData& data = LapisData::getDataObject();
		if (std::atof(_buffer.data()) == 1.f) {
			ImGui::Text(data.getUnitSingular().c_str());
		}
		else {
			ImGui::Text(data.getUnitPlural().c_str());
		}

		return changed;
	}
	bool NumericTextBoxWithUnits::importFromBoost()
	{
		if (!std::isnan(_boostValue)) {
			std::string s = truncDtoS(_boostValue);
			strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), s.size());
			_boostValue = std::nan("");
			return true;
		}
		return false;
	}
	void NumericTextBoxWithUnits::updateUnits()
	{
		LapisData& data = LapisData::getDataObject();
		const Unit& src = data.getPrevUnits();
		const Unit& dst = data.getCurrentUnits();
		try {
			double v = std::stod(_buffer.data());
			v = convertUnits(v, src, dst);
			std::string s = truncDtoS(v);
			strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
		}
		catch (...) {
			if (strlen(_buffer.data())) {
				const char* msg = "Error";
				strncpy_s(_buffer.data(), _buffer.size(), msg, _buffer.size());
			}
		}
	}
	const char* NumericTextBoxWithUnits::asText() const
	{
		return _buffer.data();
	}
	coord_t NumericTextBoxWithUnits::getValueLogErrors() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		try {
			return std::stod(_buffer.data());
		}
		catch (std::invalid_argument e) {
			log.logMessage("Error reading value of " + _guiDesc);
			log.logMessage("Aborting");
			LapisData::getDataObject().needAbort = true;
			return std::nan("");
		}
	}
	void NumericTextBoxWithUnits::copyFrom(const NumericTextBoxWithUnits& other)
	{
		strncpy_s(_buffer.data(), _buffer.size(), other._buffer.data(), other._buffer.size());
	}
	void NumericTextBoxWithUnits::setValue(coord_t v)
	{
		std::string s = truncDtoS(v);
		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
	}
	void NumericTextBoxWithUnits::setError()
	{
		strncpy_s(_buffer.data(), _buffer.size(), "Error", _buffer.size());
	}
	std::string NumericTextBoxWithUnits::truncDtoS(coord_t v) const
	{
		std::string s = std::to_string(v);
		s.erase(s.find_last_not_of('0') + 1, std::string::npos);
		s.erase(s.find_last_not_of('.') + 1, std::string::npos);
		if (s.size() > _buffer.size() - 1) {
			s.erase(_buffer.size() - 1, std::string::npos);
		}
		return s;
	}

	MultiNumericTextBoxWithUnits::MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue)
		: CmdGuiElement(guiDesc, cmdName), _boostString(defaultValue)
	{
		_buffers.reserve(LAPIS_MAX_STRATUM_COUNT);
		_cachedValues.reserve(LAPIS_MAX_STRATUM_COUNT);
		importFromBoost();
	}
	MultiNumericTextBoxWithUnits::MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName,
		const std::string& defaultValue, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription), _boostString(defaultValue)
	{
		importFromBoost();
	}
	void MultiNumericTextBoxWithUnits::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& MultiNumericTextBoxWithUnits::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=";
		for (size_t i = 0; i < _buffers.size(); ++i) {
			o << _buffers[i].asText() << ",";
		}
		o << "\n";
		return o;
	}
	bool MultiNumericTextBoxWithUnits::renderGui()
	{
		bool changed = _window();
		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		if (ImGui::Button(("Change##" + _cmdName).c_str())) {
			_displayWindow = true;
		}
		for (size_t i = 0; i < _buffers.size(); ++i) {
			ImGui::Text(_buffers[i].asText());
			ImGui::SameLine();
			ImGui::Text(LapisData::getDataObject().getUnitPlural().c_str());
		}
		return changed;
	}
	bool MultiNumericTextBoxWithUnits::importFromBoost()
	{
		if (!_boostString.size()) {
			return false;
		}
		std::stringstream tokenizer{ _boostString };
		std::string temp;
		std::vector<double> numericStrata;
		while (std::getline(tokenizer, temp, ',')) {
			if (temp.size() == 0) {
				continue;
			}
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
			_buffers.clear();
			for (size_t i = 0; i < numericStrata.size(); ++i) {
				_addBuffer();
				_buffers[i].setValue(numericStrata[i]);
			}
		}
		_boostString.clear();
		_cache();
		return true;
	}
	void MultiNumericTextBoxWithUnits::updateUnits()
	{
		LapisData& d = LapisData::getDataObject();
		if (d.getPrevUnits().convFactor != d.getCurrentUnits().convFactor) {
			for (size_t i = 0; i < _buffers.size(); ++i) {
				_buffers[i].updateUnits();
			}
			_cache();
		}
	}
	const std::vector<coord_t>& MultiNumericTextBoxWithUnits::cachedValues() const
	{
		return _cachedValues;
	}
	void MultiNumericTextBoxWithUnits::_cache()
	{
		_cachedValues.clear();
		std::set<coord_t> uniqueValues;
		for (size_t i = 0; i < _buffers.size(); ++i) {
			coord_t v;
			try {
				v = std::stod(_buffers[i].asText());
				uniqueValues.insert(v);
			}
			catch (std::invalid_argument e) {}
		}
		for (coord_t v : uniqueValues) {
			_cachedValues.push_back(v);
		}

		_buffers.clear();
		for (size_t i = 0; i < _cachedValues.size(); ++i) {
			_addBuffer();
			_buffers[i].setValue(_cachedValues[i]);
		}
	}
	void MultiNumericTextBoxWithUnits::_addBuffer()
	{
		std::string label = "##" + _cmdName + std::to_string(_buffers.size());
		_buffers.emplace_back("", label, 0);
	}
	bool MultiNumericTextBoxWithUnits::_window()
	{
		if (!_displayWindow) {
			return false;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		std::string label = "##" + _cmdName + "window";
		if (!ImGui::Begin(label.c_str(), &_displayWindow, flags)) {
			ImGui::End();
			return false;
		}
		bool needenddisabled = false;
		if (_buffers.size() >= LAPIS_MAX_STRATUM_COUNT) {
			ImGui::BeginDisabled();
			needenddisabled = true;
		}
		if (ImGui::Button("Add")) {
			_addBuffer();
		}
		if (needenddisabled) {
			ImGui::EndDisabled();
		}
		if (ImGui::Button("Remove All")) {
			_buffers.clear();
			_addBuffer();
		}
		ImGui::SameLine();
		if (ImGui::Button("Done")) {
			_cache();
			_displayWindow = false;
		}

		bool changed = false;
		for (size_t i = 0; i < _buffers.size(); ++i) {
			changed = _buffers[i].renderGui() || changed;
		}

		ImGui::End();

		return changed;
	}

	IntegerTextBox::IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription), _boostValue(defaultValue)
	{
		importFromBoost();
	}
	void IntegerTextBox::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostValue);
	}
	std::ostream& IntegerTextBox::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=" << _buffer.data() << "\n";
		return o;
	}
	bool IntegerTextBox::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		std::string label = "##" + _cmdName;
		return ImGui::InputText(label.c_str(), _buffer.data(), _buffer.size(), ImGuiInputTextFlags_CharsDecimal);
	}
	bool IntegerTextBox::importFromBoost()
	{
		if (_boostValue <=0) {
			return false;
		}
		std::string s = std::to_string(_boostValue);
		s.erase(s.find_last_not_of('0') + 1, std::string::npos);
		s.erase(s.find_last_not_of('.') + 1, std::string::npos);
		if (s.size() > _buffer.size() - 1) {
			s.erase(_buffer.size() - 1, std::string::npos);
		}
		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
		_boostValue = -1;
		return true;
	}
	int IntegerTextBox::getValueLogErrors() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		try {
			return std::stoi(_buffer.data());
		}
		catch (std::invalid_argument e) {
			log.logMessage("Error reading value of " + _guiDesc);
			log.logMessage("Aborting");
			LapisData::getDataObject().needAbort = true;
			return 1;
		}
	}

	TextBox::TextBox(const std::string& guiDesc, const std::string& cmdName)
		: CmdGuiElement(guiDesc, cmdName)
	{
		_buffer[0] = '\0';
	}
	TextBox::TextBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription)
	{
		_buffer[0] = '\0';
	}
	void TextBox::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase<std::string>(visible, hidden, &_boost);
	}
	std::ostream& TextBox::printToIni(std::ostream& o) const
	{
		if (strlen(_buffer.data())) {
			o << _cmdName << "=" << _buffer.data() << "\n";
		}
		return o;
	}
	bool TextBox::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		std::string label = "##" + _cmdName;
		bool changed = ImGui::InputText(label.c_str(), _buffer.data(), _buffer.size());
		ImGui::PopItemWidth();
		return changed;
	}
	bool TextBox::importFromBoost()
	{
		if (!_boost.size()) {
			return false;
		}
		if (_boost.size() > _buffer.size() - 1) {
			_boost.erase(_buffer.size() - 1, std::string::npos);
		}
		strncpy_s(_buffer.data(), _buffer.size(), _boost.c_str(), _buffer.size());
		_boost.clear();
		return true;
	}
	std::string TextBox::getValue() const
	{
		return _buffer.data();
	}

	FileSpecifierSet::FileSpecifierSet(const std::string& guiDesc, const std::string& cmdName,
		const std::string& cmdDescription, 
		const std::vector<std::string>& wildcards, std::unique_ptr<nfdnfilteritem_t>&& fileFilter)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription), _fileFilter(std::move(fileFilter)), _wildcards(wildcards)
	{
	}
	void FileSpecifierSet::addToCmd(po::options_description& visible,
		po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_fileSpecsBoost);
	}
	std::ostream& FileSpecifierSet::printToIni(std::ostream& o) const
	{
		for (const std::string& s : _fileSpecsSet) {
			o << _cmdName << "=" << s << "\n";
		}

		return o;
	}
	bool FileSpecifierSet::renderGui() {

		bool changed = false;

		std::string buttonText = "Add " + _guiDesc + " Files";
		auto filterptr = _fileFilter ? _fileFilter.get() : nullptr;
		int filtercnt = _fileFilter ? 1 : 0;
		if (ImGui::Button(buttonText.c_str())) {
			NFD::OpenDialogMultiple(_nfdFiles, filterptr, filtercnt, (const nfdnchar_t*)nullptr);
		}
		nfdpathsetsize_t selectedFileCount = 0;
		if (_nfdFiles) {
			NFD::PathSet::Count(_nfdFiles, selectedFileCount);
			for (nfdpathsetsize_t i = 0; i < selectedFileCount; ++i) {
				NFD::UniquePathSetPathU8 path;
				NFD::PathSet::GetPath(_nfdFiles, i, path);
				_fileSpecsSet.insert(path.get());
			}
			_nfdFiles.reset();
			changed = true;
		}

		ImGui::SameLine();
		buttonText = "Add " + _guiDesc + " Folder";
		if (ImGui::Button(buttonText.c_str())) {
			NFD::PickFolder(_nfdFolder);
		}
		if (_nfdFolder) {
			if (_recursiveCheck) {
				_fileSpecsSet.insert(_nfdFolder.get());
			}
			else {
				for (const std::string& s : _wildcards) {
					_fileSpecsSet.insert(_nfdFolder.get() + std::string("\\") + s);
				}
			}
			_nfdFolder.reset();
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			_fileSpecsSet.clear();
		}
		//ImGui::SameLine();
		ImGui::Checkbox("Add subfolders", &_recursiveCheck);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		std::string childname = "##" + _cmdName;
		ImGui::BeginChild(childname.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, window_flags);
		for (const std::string& s : _fileSpecsSet) {
			ImGui::Text(s.c_str());
		}
		ImGui::EndChild();

		return changed;
	}
	bool FileSpecifierSet::importFromBoost()
	{
		bool changed = _fileSpecsBoost.size();
		for (const std::string& s : _fileSpecsBoost) {
			_fileSpecsSet.insert(s);
		}
		_fileSpecsBoost.clear();
		return changed;
	}
	const std::set<std::string>& FileSpecifierSet::getSpecifiers() const
	{
		return _fileSpecsSet;
	}

	FolderTextInput::FolderTextInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription)
	{
		_buffer[0] = '\0';
	}
	void FolderTextInput::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& FolderTextInput::printToIni(std::ostream& o) const
	{
		std::string s{ _buffer.data() };
		if (s.size()) {
			o << _cmdName << "=" << s << "\n";
		}
		return o;
	}
	bool FolderTextInput::renderGui()
	{
		bool changed;
		
		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		std::string label = "##" + _cmdName;
		changed = ImGui::InputText(label.c_str(), _buffer.data(), _buffer.size());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Browse")) {
			NFD::PickFolder(_nfdFolder);
		}
		if (_nfdFolder) {
			strncpy_s(_buffer.data(), _buffer.size(), _nfdFolder.get(), _buffer.size());
			_nfdFolder.reset();
			changed = true;
		}

		return changed;
	}
	bool FolderTextInput::importFromBoost()
	{
		if (!_boostString.size()) {
			return false;
		}
		if (_boostString.size() > _buffer.size() - 1) {
			_boostString.erase(_buffer.size() - 1, std::string::npos);
		}
		strncpy_s(_buffer.data(), _buffer.size(), _boostString.c_str(), _buffer.size());
		return true;
	}
	std::filesystem::path FolderTextInput::path() const
	{
		return _buffer.data();
	}

	CRSInput::CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultDisplay)
		: CmdGuiElement(guiDesc, cmdName), _displayString(defaultDisplay), _defaultDisplay(defaultDisplay)
	{
	}
	CRSInput::CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription, const std::string& defaultDisplay)
		: CmdGuiElement(guiDesc, cmdName, cmdDescription), _displayString(defaultDisplay), _defaultDisplay(defaultDisplay)
	{
	}
	void CRSInput::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& CRSInput::printToIni(std::ostream& o) const
	{
		CoordRef crs;
		try {
			crs = CoordRef(_buffer.data());
		}
		catch (UnableToDeduceCRSException e) {}

		if (!crs.isEmpty()) {
			o << _cmdName << "=" << crs.getSingleLineWKT() << "\n";
		}

		return o;
	}
	bool CRSInput::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());
		bool changed = ImGui::InputTextMultiline(("##crsinput" + _cmdName).c_str(), _buffer.data(), _buffer.size(),
			ImVec2(350, 100), 0);
		if (changed) {
			_updateCrsAndDisplayString("Error interpretting user CRS");
		}
		ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
		if (ImGui::Button("Specify from file")) {
			NFD::OpenDialog(_nfdFile);
		}
		if (_nfdFile) {
			try {
				_cachedCrs = CoordRef(_nfdFile.get());
				_displayString = _cachedCrs.getShortName();
			}
			catch (UnableToDeduceCRSException e) {
				_cachedCrs = CoordRef();
				_displayString = "No CRS in File";
			}
			_nfdFile.reset();
			std::string fullWkt = _cachedCrs.isEmpty() ? "No CRS in File" : _cachedCrs.getPrettyWKT();
			strncpy_s(_buffer.data(), _buffer.size(), fullWkt.data(), _buffer.size());
			changed = true;
		}
		ImGui::SameLine();

		return changed;
	}
	bool CRSInput::importFromBoost()
	{
		bool changed = _boostString.size();
		if (changed) {
			strncpy_s(_buffer.data(), _buffer.size(), _boostString.c_str(), _buffer.size());
			_updateCrsAndDisplayString("Error reading CRS from ini file");
			_boostString.clear();
		}
		return changed;
	}
	const CoordRef& CRSInput::cachedCrs() const
	{
		return _cachedCrs;
	}
	void CRSInput::renderDisplayString() const
	{
		std::string label = "##" + _cmdName + "display";
		ImGui::BeginChild(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(_displayString.c_str());
		ImGui::EndChild();
	}
	void CRSInput::reset()
	{
		_buffer[0] = '\0';
		_displayString = _defaultDisplay;
		_cachedCrs = CoordRef();
	}
	void CRSInput::setCrs(const CoordRef& crs)
	{
		_cachedCrs = crs;
		_displayString = crs.getShortName();
		std::string wkt = crs.getPrettyWKT();
		strncpy_s(_buffer.data(), _buffer.size(), wkt.c_str(), _buffer.size());
	}
	void CRSInput::_updateCrsAndDisplayString(const std::string& displayIfEmpty)
	{
		try {
			_cachedCrs = CoordRef(_buffer.data());
			if (_cachedCrs.isEmpty()) {
				_displayString = displayIfEmpty;
			}
			else {
				_displayString = _cachedCrs.getShortName();
			}
		}
		catch (UnableToDeduceCRSException e) {
			_cachedCrs = CoordRef();
			_displayString = "Error reading CRS";
		}
	}
	void CRSInput::setZUnits(const Unit& zUnits)
	{
		_cachedCrs.setZUnits(zUnits);
	}

	ClassCheckBoxes::ClassCheckBoxes(const std::string& cmdName, const std::string& cmdDescription)
		: CmdGuiElement("", cmdName, cmdDescription)
	{
	}
	void ClassCheckBoxes::addToCmd(po::options_description& visible, po::options_description& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& ClassCheckBoxes::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=";
		if (_inverseDisplayString()) {
			o << "~";
		}
		o << _displayString << "\n";

		return o;
	}
	bool ClassCheckBoxes::renderGui()
	{
		bool changed = _window();

		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Change")) {
			_displayWindow = true;
		}

		ImGui::BeginChild(("##classlist" + _cmdName).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 50), true, ImGuiWindowFlags_HorizontalScrollbar);
		if (_inverseDisplayString()) {
			ImGui::Text("All but: ");
			ImGui::SameLine();
		}
		ImGui::Text(_displayString.c_str());
		ImGui::EndChild();

		return changed;
	}
	bool ClassCheckBoxes::importFromBoost()
	{
		if (!_boostString.size()) {
			return false;
		}
		std::regex whitelistregex{ "[0-9]+(,[0-9]+)*" };
		std::regex blacklistregex{ "~[0-9]+(,[0-9]+)*" };

		bool isWhiteList = true;

		if (std::regex_match(_boostString, whitelistregex)) {
			isWhiteList = true;
		}
		else if (std::regex_match(_boostString, blacklistregex)) {
			isWhiteList = false;
			_boostString = _boostString.substr(1, _boostString.size());
		}
		else {
			LapisLogger::getLogger().logMessage("Incorrect formatting for class string");
			_boostString.clear();
			return false;
		}

		std::stringstream tokenizer{ _boostString };
		std::string temp;
		std::unordered_set<int> set;
		while (std::getline(tokenizer, temp, ',')) {
			int cl = std::stoi(temp);
			if (cl > _checks.size() || cl < 0) {
				LapisLogger::getLogger().logMessage("Class values must be between 0 and 255");
			}
			else {
				set.insert(cl);
			}
		}
		for (size_t i = 0; i < _checks.size(); ++i) {
			bool inset = set.contains((int)i);
			if (inset) {
				_checks[i] = isWhiteList;
			}
			else {
				_checks[i] = !isWhiteList;
			}
		}
		_nChecked = 0;
		for (size_t i = 0; i < _checks.size(); ++i) {
			if (_checks[i]) {
				_nChecked++;
			}
		}

		_boostString.clear();
		_updateDisplayString();
		return true;
	}
	const std::array<bool, 256>& ClassCheckBoxes::allChecks() const
	{
		return _checks;
	}
	void ClassCheckBoxes::setState(size_t idx, bool b)
	{
		if (_checks[idx] && !b) {
			_nChecked--;
		}
		if (!_checks[idx] && b) {
			_nChecked++;
		}
		_checks[idx] = b;
		_updateDisplayString();
	}
	std::shared_ptr<LasFilter> ClassCheckBoxes::getFilter() const
	{
		std::unordered_set<uint8_t> classes;
		for (int i = 0; i < _checks.size(); ++i) {
			if (_inverseDisplayString() && !_checks[i]) {
				classes.insert(i);
			}
			if (!_inverseDisplayString() && _checks[i]) {
				classes.insert(i);
			}
		}
		if (_inverseDisplayString()) {
			return std::dynamic_pointer_cast<LasFilter>(std::make_shared<LasFilterClassBlacklist>(classes));
		}
		else {
			return std::dynamic_pointer_cast<LasFilter>(std::make_shared<LasFilterClassWhitelist>(classes));
		}
	}
	void ClassCheckBoxes::_updateDisplayString()
	{
		_displayString.clear();
		int nallowed = 0;
		int nblocked = 0;
		for (size_t i = 0; i < _checks.size(); ++i) {
			if (_checks[i]) {
				nallowed++;
			}
			else {
				nblocked++;
			}
		}
		bool comma = false;

		for (size_t i = 0; i < _checks.size(); ++i) {
			if (_checks[i] && (nblocked >= nallowed)) {
				if (comma) {
					_displayString += ",";
				}
				else {
					comma = true;
				}
				_displayString += std::to_string(i);
			}
			else if (!_checks[i] && (nblocked < nallowed)) {
				if (comma) {
					_displayString += ",";
				}
				else {
					comma = true;
				}
				_displayString += std::to_string(i);
			}
		}
	}
	bool ClassCheckBoxes::_inverseDisplayString() const
	{
		return _nChecked > (_checks.size() - _nChecked);
	}
	bool ClassCheckBoxes::_window()
	{
		if (!_displayWindow) {
			return false;
		}
		bool changed = false;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(300, 400));
		if (!ImGui::Begin(("##classwindow" + _cmdName).c_str(), &_displayWindow, flags)) {
			ImGui::End();
			return false;
		}
		else {
			if (ImGui::Button("Select All")) {
				for (size_t i = 0; i < _checks.size(); ++i) {
					_checks[i] = true;
				}
				_nChecked = _checks.size();
			}
			ImGui::SameLine();
			if (ImGui::Button("Deselect All")) {
				for (size_t i = 0; i < _checks.size(); ++i) {
					_checks[i] = false;
				}
				_nChecked = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("OK")) {
				_displayWindow = false;
				_updateDisplayString();
			}

			ImGui::BeginChild("##classcheckboxchild", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 60), true, 0);
			for (size_t i = 0; i < _checks.size(); ++i) {
				std::string label = std::to_string(i);
				if (i < _names.size()) {
					label += " " + _names[i];
				}

				if (ImGui::Checkbox(label.c_str(), &_checks[i])) {
					changed = true;
					if (_checks[i]) {
						_nChecked++;
					}
					else {
						_nChecked--;
					}
				}
			}
			ImGui::EndChild();

			ImGui::Text("Lapis does not classify points.\nThis filter only applies to\nclassification already performed\non the data.");
		}

		ImGui::End();
		return changed;
	}

	int UnitDecider::operator()(const std::string& s) const {
		{
			static const std::regex meterpattern{ ".*m.*",std::regex::icase };
			static const std::regex usfootpattern{ ".*us.*",std::regex::icase };
			static const std::regex intfootpattern{ ".*f.*",std::regex::icase };

			if (std::regex_match(s, meterpattern)) {
				return UnitRadio::METERS;
			}
			if (std::regex_match(s, usfootpattern)) {
				return UnitRadio::USFEET;
			}
			if (std::regex_match(s, intfootpattern)) {
				return UnitRadio::INTFEET;
			}
			return UnitRadio::UNKNOWN;
		}
	}
	std::string UnitDecider::operator()(int i) const
	{
		if (i == UnitRadio::METERS) {
			return "meters";
		}
		if (i == UnitRadio::INTFEET) {
			return "intfeet";
		}
		if (i == UnitRadio::USFEET) {
			return "usfeet";
		}
		return "unspecified";
	}
	int SmoothDecider::operator()(const std::string& s) const
	{
		int v = -1;
		try {
			v = std::stoi(s);
		}
		catch (std::invalid_argument e) {}
		return v;
	}
	std::string SmoothDecider::operator()(int i) const
	{
		return std::to_string(i);
	}
	int IdAlgoDecider::operator()(const std::string& s) const
	{
		const static std::regex highpointregex{ ".*high.*",std::regex::icase };
		if (std::regex_match(s, highpointregex)) {
			return IdAlgo::HIGHPOINT;
		}
		return IdAlgo::OTHER;
	}
	std::string IdAlgoDecider::operator()(int i) const
	{
		if (i == IdAlgo::HIGHPOINT) {
			return "HighPoint";
		}
		return "Other";
	}
	int SegAlgoDecider::operator()(const std::string& s) const
	{
		const static std::regex watershedregex{ ".*water.*",std::regex::icase };
		if (std::regex_match(s, watershedregex)) {
			return SegAlgo::WATERSHED;
		}
		return SegAlgo::OTHER;
	}
	std::string SegAlgoDecider::operator()(int i) const
	{
		if (i == SegAlgo::WATERSHED) {
			return "Watershed";
		}
		return "Other";
	}
}