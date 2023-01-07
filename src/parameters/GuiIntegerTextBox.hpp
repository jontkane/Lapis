#pragma once
#ifndef LP_GUIINTEGERTEXTBOX_H
#define LP_GUIINTEGERTEXTBOX_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class IntegerTextBox : public GuiCmdElement {
	public:
		IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue);
		IntegerTextBox(const std::string& guiDesc, const std::string& cmdName, int defaultValue, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		int getValueLogErrors() const;

		const char* asText() const;

		void setValue(int i);

	private:
		int _boostValue;
		std::array<char, 4> _buffer;
	};
}

#endif