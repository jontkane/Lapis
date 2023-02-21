#include"param_pch.hpp"
#include"TopoParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t TopoParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new TopoParameter());
	void TopoParameter::reset()
	{
		*this = TopoParameter();
	}

	const std::vector<coord_t>& TopoParameter::topoWindows()
	{
		return _topoWindows.cachedValues();
	}

	const std::vector<std::string>& TopoParameter::topoWindowNames()
	{
		prepareForRun();
		return _topoWindowNames;
	}

	TopoParameter::TopoParameter() :
		_topoWindows("Radius:", "topo-scale", "500,1000,2000")
	{
		_topoWindows.addHelpText("Some topographic metrics, such as topographic position index (TPI), have a scale of calculations independent of the resolution"
			" of the output raster. You can choose the radius of those calculations here.");
	}
	void TopoParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_topoWindows.addToCmd(visible, hidden);
	}
	std::ostream& TopoParameter::printToIni(std::ostream& o) {
		_topoWindows.printToIni(o);
		return o;
	}
	ParamCategory TopoParameter::getCategory() const {
		return ParamCategory::process;
	}
	void TopoParameter::renderGui() {
		ImGui::Text("Window size for large-scale topo metrics");
		_topoWindows.renderGui();
	}
	void TopoParameter::importFromBoost() {
		_topoWindows.importFromBoost();
	}
	void TopoParameter::updateUnits() {
		_topoWindows.updateUnits();
	}
	bool TopoParameter::prepareForRun() {
		
		if (_runPrepared) {
			return true;
		}

		LapisLogger& log = LapisLogger::getLogger();
		RunParameters& rp = RunParameters::singleton();

		auto to_string_with_precision = [](coord_t v)->std::string {
			std::ostringstream out;
			out.precision(0);
			out << std::fixed;
			out << v;
			std::string s = out.str();
			return s;
		};

		for (coord_t v : _topoWindows.cachedValues()) {

			if (v <= 0) {
				log.logMessage("Topo windows must be positive");
				return false;
			}

			_topoWindowNames.push_back(to_string_with_precision(v) + rp.unitPlural());
		}

		_runPrepared = true;
		return true;
	}
	void TopoParameter::cleanAfterRun() {
		_runPrepared = false;
		_topoWindowNames.clear();
	}
}