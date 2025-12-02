#pragma once
#ifndef LP_GUIFILESELECTBUTTON_H
#define LP_GUIFILESELECTBUTTON_H

#include"GuiCmdElement.hpp"

namespace lapis {
    class FileSelectButton : public GuiCmdElement {
    public:
        FileSelectButton(const std::string& guiDesc, const std::string& cmdName);
        FileSelectButton(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription);
        void addToCmd(BoostOptDesc& visible,
            BoostOptDesc& hidden) override;
        std::ostream& printToIni(std::ostream& o) const override;
        bool renderGui() override;
        bool importFromBoost() override;
        std::string currentPath() const;
        void setPath(const std::string& s);
    private:
        NFD::UniquePathU8 _nfdPath;
        std::string _boostString;
        std::string _currentPath;
    };
}

#endif