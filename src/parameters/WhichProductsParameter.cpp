#include"param_pch.hpp"
#include"WhichProductsParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t WhichProductsParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new WhichProductsParameter());
	void WhichProductsParameter::reset()
	{
		*this = WhichProductsParameter();
	}

	WhichProductsParameter::WhichProductsParameter() {
		_doCsm.addHelpText("Calculate a canopy surface model.\n\n"
			"A canopy surface model is a rasterized product usually produced at a fine (2m or smaller) resolution.\n\n"
			"It indicates a best guess as to the height of the canopy in each pixel.");
		_doPointMetrics.addHelpText("Calculate point metrics.\n\n"
			"Point metrics are a rasterized product usually produced at a coarse (10m or large) resolution.\n\n"
			"They represent summaries (such as the mean, standard deviation, etc) of the heights of the returns in each pixel.\n\n"
			"They can give a good idea of the structure of the forest within the pixel.");
		_doTao.addHelpText("Identify Tree-Approximate Objects (TAOs)\n\n"
			"A TAO represents something that the algorithm believes to be an overstory tree.\n\n"
			"No attempt is made to identify understory trees, and there is considerable inaccuracy even in the overstory.");
		_doTopo.addHelpText("Calculate topographic metrics such as slope and aspect off of the DEM.");
		_doFineInt.addHelpText("Calculate a fine-scale mean intensity image\n\n"
			"Intensity is a measure of how bright a lidar pulse is. Chlorophyll is very bright in near infrared, so high intensity can indicate the presence of live vegetation.");
	}
	void WhichProductsParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_doFineInt.addToCmd(visible, hidden);
		_doCsm.addToCmd(visible, hidden);
		_doPointMetrics.addToCmd(visible, hidden);
		_doTao.addToCmd(visible, hidden);
		_doTopo.addToCmd(visible, hidden);
	}
	std::ostream& WhichProductsParameter::printToIni(std::ostream& o) {
		_doFineInt.printToIni(o);
		_doCsm.printToIni(o);
		_doPointMetrics.printToIni(o);
		_doTao.printToIni(o);
		_doTopo.printToIni(o);
		return o;
	}
	ParamCategory WhichProductsParameter::getCategory() const {
		return ParamCategory::process;
	}
	void WhichProductsParameter::renderGui() {
		_title.renderGui();
		_doPointMetrics.renderGui();
		_doCsm.renderGui();
		if (!_doCsm.currentState()) {
			ImGui::BeginDisabled();
			_doTao.setState(false);
		}
		_doTao.renderGui();
		if (!_doCsm.currentState()) {
			ImGui::EndDisabled();
		}
		_doTopo.renderGui();
		_doFineInt.renderGui();
	}
	void WhichProductsParameter::importFromBoost() {
		_doFineInt.importFromBoost();
		_doCsm.importFromBoost();
		_doPointMetrics.importFromBoost();
		_doTao.importFromBoost();
		_doTopo.importFromBoost();
	}
	void WhichProductsParameter::updateUnits() {}
	bool WhichProductsParameter::prepareForRun() {
		return true;
	}
	void WhichProductsParameter::cleanAfterRun() {}
	bool WhichProductsParameter::doCsm() const
	{
		return _doCsm.currentState();
	}
	bool WhichProductsParameter::doPointMetrics() const
	{
		return _doPointMetrics.currentState();
	}
	bool WhichProductsParameter::doTao() const
	{
		return _doTao.currentState();
	}
	bool WhichProductsParameter::doTopo() const
	{
		return _doTopo.currentState();
	}
	bool WhichProductsParameter::doFineInt() const
	{
		return _doFineInt.currentState();
	}
}