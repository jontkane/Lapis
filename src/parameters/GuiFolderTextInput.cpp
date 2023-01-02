#include"param_pch.hpp"
#include"GuiFolderTextInput.hpp"

namespace lapis {
	FolderTextInput::FolderTextInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription)
	{
		_buffer[0] = '\0';
	}
	void FolderTextInput::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
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

		displayHelp();

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
		_boostString.clear();
		return true;
	}
	std::filesystem::path FolderTextInput::path() const
	{
		return _buffer.data();
	}
}