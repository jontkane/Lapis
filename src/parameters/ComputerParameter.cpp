#include"param_pch.hpp"
#include"ComputerParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t ComputerParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new ComputerParameter());
	void ComputerParameter::reset()
	{
		*this = ComputerParameter();
	}

	ComputerParameter::ComputerParameter() {
		_thread.addHelpText("This controls how many independent threads to run the Lapis process on.\n\n"
			"On most computers, this should be set to 2 or 3 below the number of logical cores on the machine.\n\n"
			"If Lapis is causing your computer to slow down, considering lowering this.");
		_benchmark.addHelpText("Display output on how long individual steps take. Intended as a development feature, and will be changed to be more user-friendly in future releases.");
	}
	void ComputerParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_thread.addToCmd(visible, hidden);
		_benchmark.addToCmd(visible, hidden);
	}
	std::ostream& ComputerParameter::printToIni(std::ostream& o) {
		_thread.printToIni(o);
		_benchmark.printToIni(o);
		return o;
	}
	ParamCategory ComputerParameter::getCategory() const {
		return ParamCategory::computer;
	}
	void ComputerParameter::renderGui() {
		_title.renderGui();
		_thread.renderGui();
		_benchmark.renderGui();
	}
	void ComputerParameter::importFromBoost() {
		_thread.importFromBoost();
		_benchmark.importFromBoost();
	}
	void ComputerParameter::updateUnits() {}
	bool ComputerParameter::prepareForRun() {
		if (_benchmark.currentState()) {
			LapisLogger::getLogger().turnOnVerboseBenchmarking();
		}
		else {
			LapisLogger::getLogger().turnOffVerboseBenchmarking();
		}

		if ((int)_thread.getValueLogErrors() <= 0) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logError("Number of threads must be positive");
			return false;
		}
		return true;
	}
	void ComputerParameter::cleanAfterRun() {}
	int ComputerParameter::nThread() const
	{
		return (int)_thread.getValueLogErrors();
	}

	int ComputerParameter::_defaultNThread() {
		int out = std::thread::hardware_concurrency();
		return out > 2 ? out - 2 : 1;
	}
}