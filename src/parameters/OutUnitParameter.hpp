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
		const Unit& unit() const;

	private:
		RadioSelect<UnitDecider, Unit> _unit{ "Output Units:","user-units","The units you want output to be in. All options will be interpretted as these units\n"
			"\tValues: m (for meters), f (for international feet), usft (for us survey feet)\n"
			"\tDefault is meters" };

		class UnitHasher {
		public:
			size_t operator()(const Unit& u) const {
				return std::hash<std::string>()(u.name + std::to_string(u.convFactor));
			}
		};
		std::unordered_map<Unit, std::string, UnitHasher> _singularNames = {
			{LinearUnitDefs::meter,"Meter"},
			{LinearUnitDefs::foot,"Foot"},
			{LinearUnitDefs::surveyFoot,"Foot"},
			{LinearUnitDefs::unkLinear,"Unit"}
		};
		std::unordered_map<Unit, std::string, UnitHasher> _pluralNames = {
			{LinearUnitDefs::meter,"Meters"},
			{LinearUnitDefs::foot,"Feet"},
			{LinearUnitDefs::surveyFoot,"Feet"},
			{LinearUnitDefs::unkLinear,"Units"}
		};
	};
}

#endif