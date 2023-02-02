#include"param_pch.hpp"
#include"GuiNumericTextBoxWithUnits.hpp"
#include"RunParameters.hpp"

namespace lapis {
	NumericTextBoxWithUnits::NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue)
		: GuiCmdElement(guiDesc, cmdName), _boostValue(defaultValue), _buffer()
	{
		importFromBoost();
	}
	NumericTextBoxWithUnits::NumericTextBoxWithUnits(const std::string& guiDesc, const std::string& cmdName, coord_t defaultValue, const std::string& cmdDescription)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _boostValue(defaultValue)
	{
		importFromBoost();
	}
	void NumericTextBoxWithUnits::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase<coord_t>(visible, hidden, &_boostValue);
	}
	std::ostream& NumericTextBoxWithUnits::printToIni(std::ostream& o) const
	{
		o << _cmdName << "=" << _buffer.data() << "\n";

		return o;
	}
	bool NumericTextBoxWithUnits::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());

		ImGui::SameLine();
		static constexpr int pixelWidth = 100;
		ImGui::PushItemWidth(pixelWidth);
		const std::string name = "##" + _cmdName;
		bool changed = ImGui::InputText(name.c_str(), _buffer.data(), _buffer.size(), ImGuiInputTextFlags_CharsDecimal);
		ImGui::PopItemWidth();

		ImGui::SameLine();
		RunParameters& rp = RunParameters::singleton();
		if (std::atof(_buffer.data()) == 1.f) {
			ImGui::Text(rp.unitSingular().c_str());
		}
		else {
			ImGui::Text(rp.unitPlural().c_str());
		}
		displayHelp();

		return changed;
	}
	bool NumericTextBoxWithUnits::importFromBoost()
	{
		if (!std::isnan(_boostValue)) {
			std::string s = truncDtoS(_boostValue);
			strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), s.size());
			_boostValue = std::nan("");
			return true;
		}
		return false;
	}
	void NumericTextBoxWithUnits::updateUnits()
	{
		RunParameters& rp = RunParameters::singleton();
		const Unit& src = rp.prevUnits();
		const Unit& dst = rp.outUnits();
		try {
			double v = std::stod(_buffer.data());
			v = convertUnits(v, src, dst);
			std::string s = truncDtoS(v);
			strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
		}
		catch (...) {
			if (strlen(_buffer.data())) {
				const char* msg = "Error";
				strncpy_s(_buffer.data(), _buffer.size(), msg, _buffer.size());
			}
		}
	}
	const char* NumericTextBoxWithUnits::asText() const
	{
		return _buffer.data();
	}
	coord_t NumericTextBoxWithUnits::getValueLogErrors() const
	{
		
		try {
			return std::stod(_buffer.data());
		}
		catch (std::invalid_argument e) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Error reading value of " + _guiDesc);
			return std::nan("");
		}
	}
	void NumericTextBoxWithUnits::copyFrom(const NumericTextBoxWithUnits& other)
	{
		strncpy_s(_buffer.data(), _buffer.size(), other._buffer.data(), other._buffer.size());
	}
	void NumericTextBoxWithUnits::setValue(coord_t v)
	{
		std::string s = truncDtoS(v);
		strncpy_s(_buffer.data(), _buffer.size(), s.c_str(), _buffer.size());
	}
	void NumericTextBoxWithUnits::setError()
	{
		strncpy_s(_buffer.data(), _buffer.size(), "Error", _buffer.size());
	}
	std::string NumericTextBoxWithUnits::truncDtoS(coord_t v) const
	{
		std::string s = std::to_string(v);
		s.erase(s.find_last_not_of('0') + 1, std::string::npos);
		s.erase(s.find_last_not_of('.') + 1, std::string::npos);
		if (s.size() > _buffer.size() - 1) {
			s.erase(_buffer.size() - 1, std::string::npos);
		}
		return s;
	}
}