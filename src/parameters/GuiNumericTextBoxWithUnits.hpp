#pragma once
#ifndef LP_GUINUMERICTEXTBOXWITHUNITS_H
#define LP_GUINUMERICTEXTBOXWITHUNITS_H

#include"GuiCmdElement.hpp"

namespace lapis {

	//a parameter which is a number, which can be specified on in a text input in the gui, or on the command line
	class NumericTextBoxWithUnits : public GuiCmdElement {
	public:
		using FloatBuffer = std::array<char, LAPIS_NUMERIC_BUFFER_SIZE>;

		NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue);
		NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue, const std::string& cmdDescription);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;
		bool importFromBoost() override;

		void updateUnits();
		const char* asText() const;
		coord_t getValueLogErrors() const;

		void copyFrom(const NumericTextBoxWithUnits& other);
		void setValue(coord_t v);
		void setError();

	private:
		coord_t _boostValue;
		FloatBuffer _buffer;

		std::string truncDtoS(coord_t v) const;
	};
}

#endif