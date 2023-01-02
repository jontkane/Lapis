#pragma once
#ifndef LP_GUIFOLDERTEXTINPUT_H
#define LP_GUIFOLDERTEXTINPUT_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class FolderTextInput : public GuiCmdElement {
	public:
		FolderTextInput(const std::string& guiDesc, const std::string& cmdName,
			const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		std::filesystem::path path() const;

	private:
		std::string _boostString;
		std::vector<char> _buffer = std::vector<char>(256);
		NFD::UniquePathU8 _nfdFolder;
	};
}

#endif