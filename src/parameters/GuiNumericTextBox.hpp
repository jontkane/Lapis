#pragma once
#ifndef LP_GUIINTEGERTEXTBOX_H
#define LP_GUIINTEGERTEXTBOX_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class NumericTextBox : public GuiCmdElement {
	public:
		NumericTextBox(const std::string& guiDesc, const std::string& cmdName, double defaultValue);
		NumericTextBox(const std::string& guiDesc, const std::string& cmdName, double defaultValue, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		double getValueLogErrors() const;

		const char* asText() const;

		void setValue(double i);

	private:
		double _boostValue;
		std::array<char, 15> _buffer;
	};
}

#endif