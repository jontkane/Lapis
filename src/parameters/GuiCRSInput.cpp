#include"param_pch.hpp"
#include"GuiCRSInput.hpp"

namespace lapis {
	CRSInput::CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& defaultDisplay)
		: GuiCmdElement(guiDesc, cmdName), _displayString(defaultDisplay), _defaultDisplay(defaultDisplay)
	{
	}
	CRSInput::CRSInput(const std::string& guiDesc, const std::string& cmdName, const std::string& cmdDescription, const std::string& defaultDisplay)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _displayString(defaultDisplay), _defaultDisplay(defaultDisplay)
	{
	}
	void CRSInput::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_boostString);
	}
	std::ostream& CRSInput::printToIni(std::ostream& o) const
	{
		CoordRef crs;
		try {
			crs = CoordRef(_buffer.data());
		}
		catch (UnableToDeduceCRSException e) {}

		if (!crs.isEmpty()) {
			o << _cmdName << "=" << crs.getSingleLineWKT() << "\n";
		}

		return o;
	}
	bool CRSInput::renderGui()
	{
		ImGui::Text(_guiDesc.c_str());

		displayHelp();

		bool changed = ImGui::InputTextMultiline(("##crsinput" + _cmdName).c_str(), _buffer.data(), _buffer.size(),
			ImVec2(350, 100), 0);
		if (changed) {
			_updateCrsAndDisplayString("Error interpretting user CRS");
		}
		ImGui::Text("The CRS deduction is flexible but not perfect.\nFor best results when manually specifying,\nspecify an EPSG code or a WKT string");
		if (ImGui::Button("Specify from file")) {
			NFD::OpenDialog(_nfdFile);
		}
		if (_nfdFile) {
			try {
				_cachedCrs = CoordRef(_nfdFile.get());
				_displayString = _cachedCrs.getShortName();
			}
			catch (UnableToDeduceCRSException e) {
				_cachedCrs = CoordRef();
				_displayString = "No CRS in File";
			}
			_nfdFile.reset();
			std::string fullWkt = _cachedCrs.isEmpty() ? "No CRS in File" : _cachedCrs.getPrettyWKT();
			strncpy_s(_buffer.data(), _buffer.size(), fullWkt.data(), _buffer.size());
			changed = true;
		}
		ImGui::SameLine();

		return changed;
	}
	bool CRSInput::importFromBoost()
	{
		bool changed = _boostString.size();
		if (changed) {
			strncpy_s(_buffer.data(), _buffer.size(), _boostString.c_str(), _buffer.size());
			_updateCrsAndDisplayString("Error reading CRS from ini file");
			_boostString.clear();
		}
		return changed;
	}
	const CoordRef& CRSInput::cachedCrs() const
	{
		return _cachedCrs;
	}
	void CRSInput::renderDisplayString() const
	{
		std::string label = "##" + _cmdName + "display";
		ImGui::BeginChild(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 40), true, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::Text(_displayString.c_str());
		ImGui::EndChild();
	}
	void CRSInput::reset()
	{
		_buffer[0] = '\0';
		_displayString = _defaultDisplay;
		_cachedCrs = CoordRef();
	}
	void CRSInput::setCrs(const CoordRef& crs)
	{
		_cachedCrs = crs;
		_displayString = crs.getShortName();
		std::string wkt = crs.getPrettyWKT();
		strncpy_s(_buffer.data(), _buffer.size(), wkt.c_str(), _buffer.size());
	}
	void CRSInput::_updateCrsAndDisplayString(const std::string& displayIfEmpty)
	{
		try {
			_cachedCrs = CoordRef(_buffer.data());
			if (_cachedCrs.isEmpty()) {
				_displayString = displayIfEmpty;
			}
			else {
				_displayString = _cachedCrs.getShortName();
			}
		}
		catch (UnableToDeduceCRSException e) {
			_cachedCrs = CoordRef();
			_displayString = "Error reading CRS";
		}
	}
	void CRSInput::setZUnits(const Unit& zUnits)
	{
		_cachedCrs.setZUnits(zUnits);
	}
}