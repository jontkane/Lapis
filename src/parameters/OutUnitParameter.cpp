#include"param_pch.hpp"
#include"OutUnitParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t OutUnitParameter::registeredIndex = RunParameters::singleton().registerParameter(new OutUnitParameter());
	void OutUnitParameter::reset()
	{
		*this = OutUnitParameter();
	}

	OutUnitParameter::OutUnitParameter()
	{
		_unit.addOption("Meters", UnitRadio::METERS, LinearUnitDefs::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, LinearUnitDefs::foot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, LinearUnitDefs::surveyFoot);
	}
	void OutUnitParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_unit.addToCmd(visible, hidden);
	}
	std::ostream& OutUnitParameter::printToIni(std::ostream& o) {
		_unit.printToIni(o);
		return o;
	}
	ParamCategory OutUnitParameter::getCategory() const {
		return ParamCategory::process;
	}
	void OutUnitParameter::renderGui() {
		_unit.renderGui();
	}
	void OutUnitParameter::importFromBoost() {
		_unit.importFromBoost();
	}
	void OutUnitParameter::updateUnits() {}
	bool OutUnitParameter::prepareForRun() {
		return true;
	}
	void OutUnitParameter::cleanAfterRun() {
	}
	const std::string& OutUnitParameter::unitPluralName() const
	{
		return _pluralNames.at(unit());
	}
	const std::string& OutUnitParameter::unitSingularName() const
	{
		return _singularNames.at(unit());
	}
	const Unit& OutUnitParameter::unit() const
	{
		return _unit.currentSelection();
	}
}