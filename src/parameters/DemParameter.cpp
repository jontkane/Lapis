#include"param_pch.hpp"
#include"DemParameter.hpp"
#include"RunParameters.hpp"
#include"..\algorithms\AllDemAlgorithms.hpp"
#include"..\gis\CropView.hpp"

namespace lapis {

	size_t DemParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new DemParameter());
	void DemParameter::reset()
	{
		*this = DemParameter();
	}

	void DemParameter::_renderAdvancedOptions()
	{
		ImGui::Text("CRS of DEM Files:");
		_crs.renderDisplayString();
		if (ImGui::Button("Specify CRS")) {
			_displayCrsWindow = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}

		static HelpMarker help{ "Most of the time, Lapis can telln the projection and units of the dem raster files.\n\n"
			"However, sometimes the units are unexpected, or the projection isn't defined.\n\n"
			"You can specify the correct information here.\n\n"
			"Note that this will be applied to ALL DEM rasters. You cannot yet specify one projection for some and another for others.\n\n" };
		help.renderGui();

		_unit.renderGui();

		if (_displayCrsWindow) {
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
			ImGui::SetNextWindowSize(ImVec2(400, 250));
			if (!ImGui::Begin("DEM CRS window", &_displayCrsWindow, flags)) {
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

	bool operator<(const DemParameter::DemFileAlignment& a, const DemParameter::DemFileAlignment& b) {
		return a.file < b.file;
	}

	DemParameter::DemParameter() {
		_specifiers.addShortCmdAlias('D');
		_specifiers.addHelpText("Use these buttons to specify the vendor-provided DEM rasters you want to use for this run.\n\n"
			"If you select a folder, it will assume all rasters in that folder are DEMs.\n\n"
			"If the 'recursive' box is checked, it will also include any las/laz files in subfolders.\n\n"
			"Lapis cannot currently create its own ground model.\n\n"
			"If your rasters are in ESRI grid format, select the folder itself.");

		_unit.addOption("Same as Las Files", UnitRadio::UNKNOWN, linearUnitPresets::unknownLinear);
		_unit.addOption("Meters", UnitRadio::METERS, linearUnitPresets::meter);
		_unit.addOption("International Feet", UnitRadio::INTFEET, linearUnitPresets::internationalFoot);
		_unit.addOption("US Survey Feet", UnitRadio::USFEET, linearUnitPresets::usSurveyFoot);

		_demAlgo.addOption("Provide pre-made DEM rasters", DemAlgo::VENDORRASTER, DemAlgo::VENDORRASTER);
		_demAlgo.addOption("This data represents height above ground, not elevation above sea level (uncommon)",
			DemAlgo::DONTNORMALIZE, DemAlgo::DONTNORMALIZE);
		_title.addHelpText("Most lidar data is provided as elevation above sea level, not height above ground.\n"
			"Normalizing to height above ground is an important step in processing lidar data.\n"
			"Currently, the only normalization method supported is providing a DEM calculated in another program, usually done by the lidar vendor and provided to their customers.");
	}
	void DemParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_specifiers.addToCmd(visible, hidden);
		_unit.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);
		_demAlgo.addToCmd(visible, hidden);
	}
	std::ostream& DemParameter::printToIni(std::ostream& o) {
		_specifiers.printToIni(o);
		_unit.printToIni(o);
		_crs.printToIni(o);
		_demAlgo.printToIni(o);
		return o;
	}
	ParamCategory DemParameter::getCategory() const {
		return ParamCategory::data;
	}
	void DemParameter::renderGui() {
		_title.renderGui();
		_demAlgo.renderGui();

		switch (_demAlgo.currentSelection()) {
		case DemAlgo::DONTNORMALIZE:
			break;
		case DemAlgo::VENDORRASTER:
			ImGui::BeginChild("##lasspecifiers", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 2.f), false, 0);
			_specifiers.renderGui();
			ImGui::EndChild();

			//ImGui::Checkbox("Advanced Options", &_displayAdvanced);
			//if (_displayAdvanced) {
				_renderAdvancedOptions();
			//}
			break;
		}
	}
	void DemParameter::importFromBoost() {
	
		_unit.importFromBoost();
		_crs.importFromBoost();

		bool addedFiles = _specifiers.importFromBoost();

		if (!_demAlgo.importFromBoost() && addedFiles) {
			_demAlgo.setSelection(DemAlgo::VENDORRASTER);
		}

	}
	void DemParameter::updateUnits() {}
	bool DemParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}

		LapisLogger& log = LapisLogger::getLogger();

		std::set<DemFileAlignment> fileAligns;
		std::unordered_map<CoordRef, int, CoordRefHasher, CoordRefComparator> countByCRS;
		std::optional<LinearUnit> lasUnits;

		switch (_demAlgo.currentSelection()) {
		case DemAlgo::DONTNORMALIZE:
			_algorithm = std::make_unique<AlreadyNormalized>();
			break;

		case DemAlgo::VENDORRASTER:
			if (!_specifiers.getSpecifiers().size()) {
				log.logError("No DEM rasters specified");
				return false;
			}

			log.setProgress("Identifying DEM Files");

			_demUnitsCache = _unit.currentSelection();

			if (_demUnitsCache == linearUnitPresets::unknownLinear) {
				lasUnits = RunParameters::singleton().lasZUnits();
				if (!lasUnits.has_value()) {
					log.logError("Not all las files have the same units. \"Same as Las Files\" is an invalid option for dem units.");
					return false;
				}
				_demUnitsCache = lasUnits.value();
			}


			fileAligns = _specifiers.getFiles<DemOpener, DemFileAlignment>(DemOpener(_crs.cachedCrs(),_demUnitsCache));
			log.logMessage(std::to_string(fileAligns.size()) + " Dem Files Found");
			if (fileAligns.size() == 0) {
				return false;
			}

			for (const DemFileAlignment& dfa : fileAligns) {
				const CoordRef& crs = dfa.align.crs();
				countByCRS.try_emplace(crs, 0);
				countByCRS[crs]++;
			}
			for (auto& pair : countByCRS) {
				std::stringstream ss;
				ss << pair.second << " of the dem files had the following CRS: ";
				ss << pair.first.getShortName() << " and the following vertical units: ";
				ss << pair.first.getZUnits().name() << ". If this seems wrong, consider specifying the CRS and units manually.";
				log.logMessage(ss.str());
			}

			for (const DemFileAlignment& d : fileAligns) {
				_demFileAligns.push_back(d);
			}
			_algorithm = std::make_unique<VendorRaster<DemParameter>>(this);
			break;
		default:
			log.logError("Invalid DEM algorithm value");
			return false;
		}

		if (_demFileAligns.size()) {

			auto demSort = [](const DemFileAlignment& a, const DemFileAlignment& b) {
				if (a.align.crs().isConsistentHoriz(b.align.crs())) {
					return (a.align.xres() * a.align.yres()) < (b.align.xres() * b.align.yres());
				}
				else {
					Alignment tmp = b.align.transformAlignment(a.align.crs());
					return (a.align.xres() * a.align.yres()) < (tmp.xres() * tmp.yres());
				}
			};
			std::sort(_demFileAligns.begin(), _demFileAligns.end(), demSort);

		}

		_runPrepared = true;
		return true;
	}
	void DemParameter::cleanAfterRun() {
		_demFileAligns.clear();
		_algorithm.reset();
		_runPrepared = false;
	}
	DemAlgorithm* DemParameter::demAlgorithm()
	{
		return _algorithm.get();
	}
	DemParameter::DemContainerWrapper DemParameter::demAligns()
	{
		prepareForRun();
		return DemContainerWrapper{ _demFileAligns };
	}
	std::optional<Raster<coord_t>> DemParameter::getDem(size_t n, const Extent& e)
	{
		prepareForRun();

		if (n >= _demFileAligns.size()) {
			return std::optional<Raster<coord_t>>();
		}

		Alignment& thisAlign = _demFileAligns[n].align;

		Extent projE = QuadExtent(e, thisAlign.crs()).outerExtent();
		if (!projE.overlaps(thisAlign)) {
			return std::optional<Raster<coord_t>>();
		}

		projE.defineCRS(CoordRef("")); //if there's a crs override, then there may be a spurious CRS mismatch

		if (!std::filesystem::exists(_demFileAligns[n].file.string())) {
			std::stringstream ss;
			ss << "The following DEM file no longer exists: " << _demFileAligns[n].file.string();
			LapisLogger::getLogger().logWarning(ss.str());
			return std::optional<Raster<coord_t>>();
		}

		std::optional<Raster<coord_t>> outopt{ std::in_place, _demFileAligns[n].file.string(), projE, SnapType::out};
		Raster<coord_t>& out = outopt.value();
		const CoordRef& crsOverride = _crs.cachedCrs();
		if (!crsOverride.isEmpty()) {
			out.defineCRS(crsOverride);
		}
		out.setZUnits(_demUnitsCache);
		return outopt;
	}
	Raster<coord_t> DemParameter::bufferElevation(const Raster<coord_t>& unbuffered, const Extent& desired)
	{
		if (!desired.overlaps(unbuffered)) {
			throw OutsideExtentException("Outisde extent in bufferElevation");
		}

		Alignment a = unbuffered;
		a = extendAlignment(a, desired, SnapType::out);
		a = cropAlignment(a, desired, SnapType::out);

		Raster<coord_t> out{ a };
		
		CropView<coord_t> view{ &out, unbuffered, SnapType::near };
		for (cell_t cell = 0; cell < unbuffered.ncell(); ++cell) {
			if (unbuffered[cell].has_value()) {
				view[cell].has_value() = true;
				view[cell].value() = unbuffered[cell].value();
			}
		}

		if (!_demFileAligns.size()) {
			return out;
		}

		RunParameters& rp = RunParameters::singleton();
		Alignment layout = *rp.layout();

		layout = extendAlignment(layout, a, SnapType::out);


		for (size_t i = 0; i < _demFileAligns.size(); ++i) {

			std::optional<CoordTransform> tr;
				
			if (!_demFileAligns[i].align.crs().isConsistentHoriz(out.crs())) {
				tr = CoordTransform(out.crs(), _demFileAligns[i].align.crs());
			}

			Extent e = QuadExtent(_demFileAligns[i].align,layout.crs()).outerExtent();
			for (cell_t tile = 0; tile < layout.ncell(); ++tile) {
				if (!e.overlapsUnsafe(layout.extentFromCell(tile))) {
					continue;
				}
				std::optional<Raster<coord_t>> demopt;

				for (cell_t cell : CellIterator(out, e, SnapType::out)) {
					if (out[cell].has_value()) {
						continue;
					}

					if (!demopt.has_value()) {
						demopt = getDem(i, layout.extentFromCell(tile));
						if (!demopt.has_value()) {
							break;
						}
					}

					CoordXY xy{ out.xFromCellUnsafe(cell),out.yFromCellUnsafe(cell) };
					if (tr) {
						xy = tr.value().transformSingleXY(xy.x, xy.y);
					}

					auto v = demopt.value().extract(xy.x,xy.y, ExtractMethod::bilinear);
					if (v.has_value()) {
						out[cell].has_value() = true;
						out[cell].value() = v.value();
					}
				}
			}
		}
		return out;
	}
	DemParameter::DemOpener::DemOpener(const CoordRef& crsOverride, const LinearUnit& unitOverride)
		:_crsOverride(crsOverride), _unitOverride(unitOverride)
	{
	}
	DemParameter::DemFileAlignment DemParameter::DemOpener::operator()(const std::filesystem::path& f) const
	{
		if (f.extension() == ".aux" || f.extension() == ".ovr" || f.extension() == ".adf"
			|| f.extension() == ".xml") { //excluding commonly-found non-raster files to prevent slow calls to GDAL
			throw InvalidRasterFileException("");
		}
		Alignment a{ f.string() };

		if (!_crsOverride.isEmpty()) {
			a.defineCRS(_crsOverride);
		}
		a.setZUnits(_unitOverride);
		return { f,a };
	}
	int DemParameter::DemAlgoDecider::operator()(const std::string& s) const
	{
		static std::regex vendorRasterRegex{ "(.*raster.*)|(.*vendor.*)",std::regex::icase };
		if (std::regex_match(s, vendorRasterRegex)) {
			return DemAlgo::VENDORRASTER;
		}
		else {
			return DemAlgo::DONTNORMALIZE;
		}
	}
	std::string DemParameter::DemAlgoDecider::operator()(int i) const
	{
		if (i == DemAlgo::VENDORRASTER) {
			return "raster";
		}
		if (i == DemAlgo::DONTNORMALIZE) {
			return "none";
		}
		return "other";
	}
}