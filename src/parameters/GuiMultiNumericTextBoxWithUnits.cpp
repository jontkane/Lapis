#include"param_pch.hpp"
#include"GuiMultiNumericTextBoxWithUnits.hpp"
#include"GuiNumericTextBoxWithUnits.hpp"
#include"RunParameters.hpp"

namespace lapis {
	MultiNumericTextBoxWithUnits::MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultValue)
		: GuiCmdElement(guiDesc, cmdName), _boostString(defaultValue)
	{
		_cachedValues.reserve(LAPIS_MAX_STRATUM_COUNT);
		importFromBoost();
	}
	MultiNumericTextBoxWithUnits::MultiNumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName,
		const std::string& defaultValue, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _boostString(defaultValue)
	{
		importFromBoost();
	}
	void MultiNumericTextBoxWithUnits::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& MultiNumericTextBoxWithUnits::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=";
		for (auto& buffer : _buffers) {
			o << buffer.asText() << ",";
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
		displayHelp();
		for (auto& buffer : _buffers) {
			ImGui::Text(buffer.asText());
			ImGui::SameLine();
			ImGui::Text(RunParameters::singleton().unitPlural().c_str());
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
				LapisLogger::getLogger().logWarningOrError("Unable to interpret " + temp + " as a number");
				numericStrata.clear();
				break;
			}
		}
		if (numericStrata.size()) {
			std::sort(numericStrata.begin(), numericStrata.end());
			_buffers.clear();
			for (size_t i = 0; i < numericStrata.size(); ++i) {
				_addBuffer();
				_buffers.back().setValue(numericStrata[i]);
			}
		}
		_boostString.clear();
		_cache();
		return true;
	}
	void MultiNumericTextBoxWithUnits::updateUnits()
	{
		RunParameters& rp = RunParameters::singleton();
		if (!rp.prevUnits().isConsistent(rp.outUnits())) {
			for (auto& buffer : _buffers) {
				buffer.updateUnits();
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
		for (auto& buffer : _buffers) {
			coord_t v;
			try {
				v = std::stod(buffer.asText());
				uniqueValues.insert(v);
			}
			catch (std::invalid_argument e) {}
		}
		for (coord_t v : uniqueValues) {
			_cachedValues.push_back(v);
		}

		std::sort(_cachedValues.begin(), _cachedValues.end());
		_buffers.clear();
		for (size_t i = 0; i < _cachedValues.size(); ++i) {
			_addBuffer();
			_buffers.back().setValue(_cachedValues[i]);
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
		std::list<NumericTextBoxWithUnits>::iterator slatedForDeletion = _buffers.end();
		int idx = -1;
		for (auto it = _buffers.begin(); it != _buffers.end(); ++it) {
			idx++;
			changed = it->renderGui() || changed;
			ImGui::SameLine();
			std::string label = "Remove##" + std::to_string(idx);
			if (ImGui::Button(label.c_str())) {
				slatedForDeletion = it;
			}
		}

		if (slatedForDeletion != _buffers.end()) {
			_buffers.erase(slatedForDeletion);
		}

		ImGui::End();

		return changed;
	}
}