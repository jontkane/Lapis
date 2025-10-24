#include"param_pch.hpp"
#include"GuiFileSelectButton.hpp"

namespace lapis {
    FileSelectButton::FileSelectButton(const std::string& guiDesc, const std::string& cmdName)
        : GuiCmdElement(guiDesc, cmdName)
    {
    }
    FileSelectButton::FileSelectButton(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription)
        : GuiCmdElement(guiDesc, cmdName, cmdDescription)
    {
    }
    void FileSelectButton::addToCmd(BoostOptDesc& visible,
        BoostOptDesc& hidden)
    {
        addToCmdBase<std::string>(visible, hidden, &_boostString);
    }
    std::ostream& FileSelectButton::printToIni(std::ostream& o) const
    {
        o << _cmdName << "=" << _currentPath << "\n";
        return o;
    }
    bool FileSelectButton::renderGui()
    {
        std::string label = _buttonText;
        label += "##" + _cmdName;
        bool changed = false;
        if (ImGui::Button(label.c_str())) {
            NFD::OpenDialog(_nfdPath);
        }
        if (_nfdPath) {
            _currentPath = _nfdPath.get();
            changed = true;
            _nfdPath.reset();
        }
        displayHelp();
        return changed;
    }
    bool FileSelectButton::importFromBoost()
    {
        if (_boostString.empty()) {
            return false;
        }
        _currentPath = _boostString;
        return true;
    }
    std::string FileSelectButton::currentPath() const
    {
        return _currentPath;
    }
    void FileSelectButton::setPath(const std::string& s)
    {
        _currentPath = s;
    }
}