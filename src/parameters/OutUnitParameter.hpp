#pragma once
#ifndef LP_OUTUNITPARAMETER_H
#define LP_OUTUNITPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class OutUnitParameter : public Parameter {
	public:

		OutUnitParameter();

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		bool prepareForRun() override;
		void cleanAfterRun() override;

		void reset() override;
		static size_t parameterRegisteredIndex;

		const std::string& unitPluralName() const;
		const std::string& unitSingularName() const;
		const LinearUnit& unit() const;

	private:
		Title _title{ "Output Units" };
		RadioSelect<UnitDecider, LinearUnit> _unit{ "","user-units","The units you want output to be in. All options will be interpretted as these units\n"
			"\tValues: m (for meters), f (for international feet), usft (for us survey feet)\n"
			"\tDefault is meters" };

		class UnitHasher {
		public:
			size_t operator()(const LinearUnit& u) const {
				return std::hash<std::string>()(u.name());
			}
		};
		std::unordered_map<LinearUnit, std::string, UnitHasher> _singularNames = {
			{linearUnitPresets::meter,"Meter"},
			{linearUnitPresets::internationalFoot,"Foot"},
			{linearUnitPresets::usSurveyFoot,"Foot"},
			{linearUnitPresets::unknownLinear,"Unit"}
		};
		std::unordered_map<LinearUnit, std::string, UnitHasher> _pluralNames = {
			{linearUnitPresets::meter,"Meters"},
			{linearUnitPresets::internationalFoot,"Feet"},
			{linearUnitPresets::usSurveyFoot,"Feet"},
			{linearUnitPresets::unknownLinear,"Units"}
		};
	};
}

#endif