#pragma once
#ifndef LP_GUIHELPMARKER_H
#define LP_GUIHELPMARKER_H

#include"GuiCmdElement.hpp"

namespace lapis {

	class HelpMarker : public GuiCmdElement {
	public:
		HelpMarker(const std::string& helpText);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;
	};
}

#endif