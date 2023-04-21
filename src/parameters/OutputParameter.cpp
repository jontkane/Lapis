#include"param_pch.hpp"
#include"OutputParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t OutputParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new OutputParameter());
	void OutputParameter::reset()
	{
		*this = OutputParameter();
	}

	OutputParameter::OutputParameter() {
		_output.addShortCmdAlias('O');
		_debugNoOutput.addHelpText("This checkbox should only be displayed in debug mode. If you see it in a public release, please contact the developer.");
	}
	void OutputParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_output.addToCmd(visible, hidden);
		_debugNoOutput.addToCmd(visible, hidden);
	}
	std::ostream& OutputParameter::printToIni(std::ostream& o) {
		_output.printToIni(o);
		return o;
	}
	ParamCategory OutputParameter::getCategory() const {
		return ParamCategory::data;
	}
	void OutputParameter::renderGui() {
		if (_debugNoOutput.currentState()) {
			ImGui::BeginDisabled();
		}
		_output.renderGui();
		if (_debugNoOutput.currentState()) {
			ImGui::EndDisabled();
		}
#ifndef NDEBUG
		ImGui::SameLine();
		_debugNoOutput.renderGui();
#endif
	}
	void OutputParameter::importFromBoost() {
		_output.importFromBoost();
		_debugNoOutput.importFromBoost();
	}
	void OutputParameter::updateUnits() {}
	bool OutputParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		_outPath = _output.path();

		RunParameters& rp = RunParameters::singleton();
		size_t maxFileLength = rp.maxLapisFileName + _outPath.string().size() + rp.name().size();
		LapisLogger& log = LapisLogger::getLogger();
		if (maxFileLength > rp.maxTotalFilePath) {
			log.logWarningOrError("Total file path is too long");
			return false;
		}
		namespace fs = std::filesystem;
		if (_debugNoOutput.currentState()) {
			_runPrepared = true;
			return true;
		}
		try {
			fs::create_directories(_outPath);
		}
		catch (fs::filesystem_error e) {
			log.logWarningOrError("Unable to create output directory");
			return false;
		}
		_runPrepared = true;
		return true;
	}
	void OutputParameter::cleanAfterRun() {
		_runPrepared = false;
	}
	const std::filesystem::path& OutputParameter::path()
	{
		prepareForRun();
		return _outPath;
	}
	bool OutputParameter::isDebugNoOutput() const
	{
		return _debugNoOutput.currentState();
	}
}