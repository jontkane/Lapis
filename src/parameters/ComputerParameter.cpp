#include"param_pch.hpp"
#include"ComputerParameter.hpp"

namespace lapis {

	LAPIS_PARAMETER_REGISTER_DEFINE(ComputerParameter);
	void ComputerParameter::reset()
	{
		*this = ComputerParameter();
	}

	ComputerParameter::ComputerParameter() {
		_thread.addHelpText("This controls how many independent threads to run the Lapis process on.\n\n"
			"On most computers, this should be set to 2 or 3 below the number of logical cores on the machine.\n\n"
			"If Lapis is causing your computer to slow down, considering lowering this.");
        _concurrentIO.addHelpText("This controls how many concurrent read/write operations to perform per drive.\n\n"
            "This can be higher on SSDs, or lowered to 1-2 on spinning drives.");
	}
	void ComputerParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_thread.addToCmd(visible, hidden);
		_concurrentIO.addToCmd(visible, hidden);
	}
	std::ostream& ComputerParameter::printToIni(std::ostream& o) {
		_thread.printToIni(o);
        _concurrentIO.printToIni(o);
		return o;
	}
	ParamCategory ComputerParameter::getCategory() const {
		return ParamCategory::computer;
	}
	void ComputerParameter::renderGui() {
		_title.renderGui();
		_thread.renderGui();
		_concurrentIO.renderGui();
	}
	void ComputerParameter::importFromBoost() {
		_thread.importFromBoost();
        _concurrentIO.importFromBoost();
	}
	void ComputerParameter::updateUnits() {}
	bool ComputerParameter::prepareForRun() {
		if ((int)_thread.getValueLogErrors() <= 0) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logError("Number of threads must be positive");
			return false;
		}
		if ((int)_concurrentIO.getValueLogErrors() <= 0) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logError("Number of concurrent read/write operations must be positive");
			return false;
		}
        if ((int)_concurrentIO.getValueLogErrors() > MAX_CONCURRENT_IO) {
            LapisLogger& log = LapisLogger::getLogger();
            log.logError("Number of concurrent read/write operations must be less than or equal to " + std::to_string(MAX_CONCURRENT_IO));
            return false;
        }
		return true;
	}
	void ComputerParameter::cleanAfterRun() {}
	int ComputerParameter::nThread() const
	{
		return (int)_thread.getValueLogErrors();
	}

	int ComputerParameter::concurrentIO() const
	{
		return (int)_concurrentIO.getValueLogErrors();
	}

	int ComputerParameter::_defaultNThread() {
		int out = std::thread::hardware_concurrency();
		return out > 2 ? out - 2 : 1;
	}
}