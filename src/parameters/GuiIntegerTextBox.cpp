#include"param_pch.hpp"
#include"GuiIntegerTextBox.hpp"

namespace lapis {
	IntegerTextBox::IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue)
		: GuiCmdElement(guiDesc, cmdName), _boostValue(defaultValue)
	{
		_buffer[0] = '\0';
		importFromBoost();
	}
	IntegerTextBox::IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _boostValue(defaultValue)
	{
		_buffer[0] = '\0';
		importFromBoost();
	}
	void IntegerTextBox::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
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
		ImGui::PushItemWidth(100);
		bool changed = ImGui::InputText(label.c_str(), _buffer.data(), _buffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();
		displayHelp();
		return changed;
	}
	bool IntegerTextBox::importFromBoost()
	{
		if (_boostValue <= 0) {
			return false;
		}
		std::string s = std::to_string(_boostValue);
		if (s.size() > _buffer.size() - 1) {
			s = "999";
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
			return 1;
		}
	}

	const char* IntegerTextBox::asText() const {
		return _buffer.data();
	}

	void IntegerTextBox::setValue(int i) {
		std::string s = std::to_string(i);
		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
	}
}