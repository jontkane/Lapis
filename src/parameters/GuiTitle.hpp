#pragma once
#ifndef LP_GUITITLE_H
#define LP_GUITITLE_H

#include"GuiCmdElement.hpp"

namespace lapis {
	//a parameter which is a simple bool, which defaults to false. The presence of a command line argument switches it to true.
	class Title : public GuiCmdElement {
	public:
		Title(const std::string& guiDesc);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;
	};
}

#endif