#include"param_pch.hpp"
#include"NameParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t NameParameter::registeredIndex = RunParameters::singleton().registerParameter(new NameParameter());
	void NameParameter::reset()
	{
		*this = NameParameter();
	}

	NameParameter::NameParameter() {
		_name.addShortCmdAlias('N');
		_name.addHelpText("If you supply a name for the run, it will be included in the filenames of the output.");
	}
	void NameParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {

		_name.addToCmd(visible, hidden);
	}
	std::ostream& NameParameter::printToIni(std::ostream& o) {
		_name.printToIni(o);
		return o;
	}
	ParamCategory NameParameter::getCategory() const {
		return ParamCategory::data;
	}
	void NameParameter::renderGui() {
		_name.renderGui();
	}
	void NameParameter::importFromBoost() {
		_name.importFromBoost();
	}
	void NameParameter::updateUnits() {}
	bool NameParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}
		_nameString = _name.getValue();
		_runPrepared = true;
		return true;
	}
	void NameParameter::cleanAfterRun() {
		_nameString.clear();
		_runPrepared = false;
	}
	const std::string& NameParameter::name()
	{
		prepareForRun();
		return _nameString;
	}
}