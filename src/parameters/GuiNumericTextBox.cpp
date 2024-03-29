#include"param_pch.hpp"
#include"GuiNumericTextBox.hpp"

namespace lapis {
	NumericTextBox::NumericTextBox(const std::string& guiDesc, const std::string& cmdName, double defaultValue)
		: GuiCmdElement(guiDesc, cmdName), _boostValue(defaultValue)
	{
		_buffer[0] = '\0';
		importFromBoost();
	}
	NumericTextBox::NumericTextBox(const std::string& guiDesc, const std::string& cmdName, double defaultValue, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _boostValue(defaultValue)
	{
		_buffer[0] = '\0';
		importFromBoost();
	}
	void NumericTextBox::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_boostValue);
	}
	std::ostream& NumericTextBox::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=" << _buffer.data() << "\n";
		return o;
	}
	bool NumericTextBox::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());
		ImGui::SameLine();
		std::string label = "##" + _cmdName;
		ImGui::PushItemWidth(100);
		bool changed = ImGui::InputText(label.c_str(), _buffer.data(), _buffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();
		displayHelp();
		return changed;
	}
	bool NumericTextBox::importFromBoost()
	{
		if (_boostValue <= 0) {
			return false;
		}
		std::string s = std::to_string(_boostValue);

		std::regex hasdecimalpoint{ ".*\\..*" };
		if (std::regex_match(s, hasdecimalpoint)) {
			s.erase(s.find_last_not_of('0') + 1, std::string::npos);
			s.erase(s.find_last_not_of('.') + 1, std::string::npos);
		}
		if (s.size() > _buffer.size() - 1) {
			s.erase(_buffer.size() - 1, std::string::npos);
		}

		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
		_boostValue = -1;
		return true;
	}
	double NumericTextBox::getValueLogErrors() const
	{
		LapisLogger& log = LapisLogger::getLogger();
		try {
			return std::stod(_buffer.data());
		}
		catch (std::invalid_argument e) {
			log.logError("Error reading value of " + _guiDesc);
			return 1;
		}
	}

	const char* NumericTextBox::asText() const {
		return _buffer.data();
	}

	void NumericTextBox::setValue(double i) {
		std::string s = std::to_string(i);
		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
	}
}