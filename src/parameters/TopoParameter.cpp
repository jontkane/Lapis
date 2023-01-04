#include"param_pch.hpp"
#include"TopoParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t TopoParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new TopoParameter());
	void TopoParameter::reset()
	{
		*this = TopoParameter();
	}

	TopoParameter::TopoParameter() {}
	void TopoParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {}
	std::ostream& TopoParameter::printToIni(std::ostream& o) {
		return o;
	}
	ParamCategory TopoParameter::getCategory() const {
		return ParamCategory::process;
	}
	void TopoParameter::renderGui() {}
	void TopoParameter::importFromBoost() {}
	void TopoParameter::updateUnits() {}
	bool TopoParameter::prepareForRun() {
		return true;
	}
	void TopoParameter::cleanAfterRun() {}
}