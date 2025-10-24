#include"param_pch.hpp"
#include"OutputParameter.hpp"
#include"ParameterGetter.hpp"

namespace lapis {

	LAPIS_PARAMETER_REGISTER_DEFINE(OutputParameter);
	void OutputParameter::reset()
	{
		*this = OutputParameter();
	}

	OutputParameter::OutputParameter() {
		_output.addShortCmdAlias('O');
	}
	void OutputParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_output.addToCmd(visible, hidden);
	}
	std::ostream& OutputParameter::printToIni(std::ostream& o) {
		_output.printToIni(o);
		return o;
	}
	ParamCategory OutputParameter::getCategory() const {
		return ParamCategory::data;
	}
	void OutputParameter::renderGui() {
		_output.renderGui();
	}
	void OutputParameter::importFromBoost() {
		_output.importFromBoost();
	}
	void OutputParameter::updateUnits() {}
	bool OutputParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		_outPath = _output.path();

		ParameterManager& pm = parameterManager();
		size_t maxFileLength = pm.maxLapisFileName + _outPath.string().size() + pm.name().size();
		LapisLogger& log = LapisLogger::getLogger();
		if (maxFileLength > pm.maxTotalFilePath) {
			log.logError("Total file path is too long");
			return false;
		}
		namespace fs = std::filesystem;
		try {
			fs::create_directories(_outPath);
		}
		catch (fs::filesystem_error e) {
			log.logError("Unable to create output directory");
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
}