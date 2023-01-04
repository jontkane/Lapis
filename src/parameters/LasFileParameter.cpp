#include"param_pch.hpp"
#include"LasFileParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t LasFileParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new LasFileParameter());
	void LasFileParameter::reset()
	{
		*this = LasFileParameter();
	}

	bool operator<(const LasFileParameter::LasFileExtent& a, const LasFileParameter::LasFileExtent& b) {
		if (a.file == b.file) { return false; }
		if (a.ext.ymax() > b.ext.ymax()) { return true; }
		if (a.ext.ymax() < b.ext.ymax()) { return false; }
		if (a.ext.ymin() > b.ext.ymin()) { return true; }
		if (a.ext.ymin() < b.ext.ymin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmin() < b.ext.xmin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmax() < b.ext.xmax()) { return false; }
		return a.file < b.file;
	}

	LasFileParameter::LasFileParameter() {
		_specifiers.addShortCmdAlias('L');
		_specifiers.addHelpText("Use these buttons to specify the las/laz format lidar point clouds you want to process.\n\n"
			"If you select a folder, it will include all las/laz files in that folder.\n\n"
			"If the 'recursive' box is checked, it will also include any las/laz files in subfolders.");

		_unit.addOption("Infer from files",
			UnitRadio::UNKNOWN, LinearUnitDefs::unkLinear);
		_unit.addOption("Meters", UnitRadio::METERS, LinearUnitDefs::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, LinearUnitDefs::foot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, LinearUnitDefs::surveyFoot);
	}
	void LasFileParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_specifiers.addToCmd(visible, hidden);
		_unit.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);
	}
	std::ostream& LasFileParameter::printToIni(std::ostream& o) {
		_specifiers.printToIni(o);
		_unit.printToIni(o);
		_crs.printToIni(o);
		return o;
	}
	ParamCategory LasFileParameter::getCategory() const {
		return ParamCategory::data;
	}
	void LasFileParameter::renderGui() {
		ImGui::BeginChild("##lasspecifiers", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 2.f), false, 0);
		_specifiers.renderGui();
		ImGui::EndChild();
		ImGui::Checkbox("Show advanced options", &_displayAdvancedOptions);
		if (_displayAdvancedOptions) {
			_renderAdvancedOptions();
		}
	}
	void LasFileParameter::importFromBoost() {
		_specifiers.importFromBoost();
		_unit.importFromBoost();
		_crs.importFromBoost();
	}
	void LasFileParameter::updateUnits() {}
	bool LasFileParameter::prepareForRun() {

		if (_runPrepared) {
			return true;
		}

		RunParameters& rp = RunParameters::singleton();

		if (rp.isDebugNoAlign()) {
			_runPrepared = true;
			return true;
		}
		LapisLogger& log = LapisLogger::getLogger();
		log.setProgress(RunProgress::findingLasFiles);

		std::set<LasFileExtent> s = _specifiers.getFiles<LasOpener,LasFileExtent>(LasOpener(_crs.cachedCrs(),_unit.currentSelection()));


		CoordRef outCrs = rp.userCrs();
		if (outCrs.isEmpty()) {
			for (const LasFileExtent& e : s) {
				outCrs = e.ext.crs();
				if (!outCrs.isEmpty()) {
					break;
				}
			}
		}

		std::vector<LasFileExtent> fileExtentVector;

		for (const LasFileExtent& l : s) {
			LasExtent e = { QuadExtent(l.ext, outCrs).outerExtent(), l.ext.nPoints() };
			fileExtentVector.emplace_back(l.file, e);
		}
		std::sort(fileExtentVector.begin(), fileExtentVector.end());
		log.logMessage(std::to_string(fileExtentVector.size()) + " Las Files Found");
		if (fileExtentVector.size() == 0) {
			return false;
		}

		bool init = false;

		for (const LasFileExtent& l : fileExtentVector) {
			_lasFileNames.push_back(l.file.string());
			_lasExtents.push_back(l.ext);
			if (!init) {
				_fullExtent = l.ext;
				init = true;
			}
			else {
				_fullExtent = extendExtent(_fullExtent, l.ext);
			}
		}

		_runPrepared = true;
		return true;
	}
	void LasFileParameter::cleanAfterRun() {
		_lasExtents.clear();
		_lasFileNames.clear();

		_fullExtent = Extent();

		_runPrepared = false;
	}
	const Extent& LasFileParameter::getFullExtent() {
		prepareForRun();
		return _fullExtent;
	}
	const std::vector<Extent>& LasFileParameter::sortedLasExtents()
	{
		prepareForRun();
		return _lasExtents;
	}
	LasReader LasFileParameter::getLas(size_t n)
	{
		prepareForRun();
		LasReader out{ _lasFileNames[n] };
		
		const CoordRef& crsOverride = _crs.cachedCrs();
		const Unit& unitOverride = _unit.currentSelection();
		if (!crsOverride.isEmpty()) {
			out.defineCRS(crsOverride);
		}
		if (!unitOverride.isUnknown()) {
			out.setZUnits(unitOverride);
		}
		return out;
	}

	void LasFileParameter::_renderAdvancedOptions()
	{
		ImGui::Text("CRS of Laz Files:");
		_crs.renderDisplayString();
		if (ImGui::Button("Specify CRS")) {
			_displayCrsWindow = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}

		static HelpMarker help{ "Most of the time, Lapis can tell the projection and units of the laz files.\n\n"
			"However, especially with very old (pre-2013) laz files, it can sometimes be wrong.\n\n"
			"You can specify the correct information here.\n\n"
			"Note that this will be applied to ALL las/laz files. You cannot specify one projection for some and another for others.\n\n" };
		help.renderGui();

		_unit.renderGui();

		if (_displayCrsWindow) {
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
			ImGui::SetNextWindowSize(ImVec2(400, 220));
			if (!ImGui::Begin("Laz CRS window", &_displayCrsWindow, flags)) {
				ImGui::End();
			}
			else {
				_crs.renderGui();
				ImGui::SameLine();
				if (ImGui::Button("OK")) {
					_displayCrsWindow = false;
				}
				ImGui::End();
			}
		}
	}

	LasFileParameter::LasOpener::LasOpener(const CoordRef& crsOverride, const Unit& unitOverride)
		: _crsOverride(crsOverride), _unitOverride(unitOverride)
	{
	}

	LasFileParameter::LasFileExtent LasFileParameter::LasOpener::operator()(const std::filesystem::path& f) const
	{
		LapisLogger& log = LapisLogger::getLogger();
		if (!f.has_extension()) {
			throw InvalidLasFileException("");
		}
		if (f.extension() != ".laz" && f.extension() != ".las") {
			throw InvalidLasFileException("");
		}
		try {
			LasExtent e{ f.string() };

			if (!_crsOverride.isEmpty()) {
				e.defineCRS(_crsOverride);
			}

			if (!_unitOverride.isUnknown()) {
				e.setZUnits(_unitOverride);
			}
			return { f,e };
		}
		catch (...) {
			log.logMessage("Error reading " + f.string());
			throw InvalidLasFileException("");
		}
	}
}