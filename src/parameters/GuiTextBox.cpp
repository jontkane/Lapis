#include"param_pch.hpp"
#include"GuiTextBox.hpp"

namespace lapis {
	TextBox::TextBox(const std::string& guiDesc, const std::string& cmdName)
		: GuiCmdElement(guiDesc, cmdName)
	{
		_buffer[0] = '\0';
	}
	TextBox::TextBox(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription)
	{
		_buffer[0] = '\0';
	}
	void TextBox::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
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
		displayHelp();
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
}