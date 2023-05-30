#include"param_pch.hpp"
#include"GuiClassCheckboxes.hpp"

namespace lapis {

	ClassCheckBoxes::ClassCheckBoxes(const std::string& cmdName, const std::string& cmdDescription)
		: GuiCmdElement("", cmdName, cmdDescription)
	{
	}
	void ClassCheckBoxes::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
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
		displayHelp();

		ImGui::BeginChild(("##classlist" + _cmdName).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 50), true, ImGuiWindowFlags_HorizontalScrollbar);

		if (_displayString.size()) {
			if (_inverseDisplayString()) {
				ImGui::Text("All but: ");
				ImGui::SameLine();
			}
			ImGui::Text(_displayString.c_str());
		}
		else {
			if (_inverseDisplayString()) {
				ImGui::Text("All");
			}
			else {
				ImGui::Text("None");
			}
		}
		ImGui::EndChild();

		return changed;
	}
	bool ClassCheckBoxes::importFromBoost()
	{
		if (!_boostString.size()) {
			return false;
		}
		std::regex whitelistregex{ "$|([0-9]+(,[0-9]+)*)" };
		std::regex blacklistregex{ "~($|([0-9]+(,[0-9]+)*))" };

		bool isWhiteList = true;

		if (std::regex_match(_boostString, whitelistregex)) {
			isWhiteList = true;
		}
		else if (std::regex_match(_boostString, blacklistregex)) {
			isWhiteList = false;
			_boostString = _boostString.substr(1, _boostString.size());
		}
		else {
			LapisLogger::getLogger().logError("Incorrect formatting for class string");
			_boostString.clear();
			return false;
		}

		std::stringstream tokenizer{ _boostString };
		std::string temp;
		std::unordered_set<int> set;
		while (std::getline(tokenizer, temp, ',')) {
			int cl = std::stoi(temp);
			if (cl > _checks.size() || cl < 0) {
				LapisLogger::getLogger().logError("Class values must be between 0 and 255");
			}
			else {
				set.insert(cl);
			}
		}
		for (size_t i = 0; i < _checks.size(); ++i) {
			if (set.contains((int)i)) {
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
	const std::vector<std::string>& ClassCheckBoxes::classNames() const
	{
		const static std::vector<std::string> names = { "Never Classified",
				"Unassigned",
				"Ground",
				"Low Vegetation",
				"Medium Vegetation",
				"High Vegetation",
				"Building",
				"Low Point",
				"Model Key (LAS 1.0-1.3)/Reserved (LAS 1.4)",
				"Water",
				"Rail",
				"Road Surface",
				"Overlap (LAS 1.0-1.3)/Reserved (LAS 1.4)",
				"Wire - Guard (Shield)",
				"Wire - Conductor (Phase)",
				"Transmission Tower",
				"Wire-Structure Connector (Insulator)",
				"Bridge Deck",
				"High Noise" };
		return names;
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
		ImGui::SetNextWindowSize(ImVec2(450, 600));
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
				if (i < classNames().size()) {
					label += " " + classNames()[i];
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

}