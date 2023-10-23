#include"param_pch.hpp"
#include"AoIParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t AoIParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new AoIParameter());
	void AoIParameter::reset()
	{
		*this = AoIParameter();
	}
	bool AoIParameter::overlapsAoI(const Extent& e)
	{
		prepareForRun();
		if (_typeRadio.currentSelection() == AoIType::None) {
			return true;
		}
		if (_typeRadio.currentSelection() == AoIType::Extent) {
			if (!_extentFilter) {
				throw std::runtime_error("Extent AoI not properly initialized.");
			}
			Extent projE = QuadExtent(*_extentFilter, e.crs()).outerExtent();
			return projE.overlaps(e);
		}
		throw std::runtime_error("AoI type error.");
	}
	void AoIParameter::addFilter(LasReader& lr)
	{
		prepareForRun();
		if (_typeRadio.currentSelection() == AoIType::None) {
			return;
		}
		if (_typeRadio.currentSelection() == AoIType::Extent) {
			Extent projE = QuadExtent(*_extentFilter, lr.crs()).outerExtent();
			if (!projE.overlaps(lr)) {
				//no points pass. In theory, other code should ensure this branch is unreachable; this is a fallback.
				lr.addFilter(std::make_shared<LasFilterAlwaysFail>());
				return;
			}
			if (projE.xmin() <= lr.xmin() && projE.xmax() >= lr.xmax() && projE.ymin() <= lr.ymin() && projE.ymax() >= lr.ymax()) {
				//all points pass, no need to waste cycles on a filter
				return;
			}
			lr.addFilter(std::make_shared<LasFilterExtent>(projE));
		}
	}
	void AoIParameter::filterPointsInPlace(LidarPointVector& lpv, const Extent& e)
	{
		prepareForRun();
		if (_typeRadio.currentSelection() == AoIType::None) {
			return;
		}
		if (_typeRadio.currentSelection() == AoIType::Extent) {
			if (!_extentFilter) {
				throw std::runtime_error("Extent AoI not properly initialized.");
			}
			Extent projE = QuadExtent(*_extentFilter, e.crs()).outerExtent();
			if (!projE.overlaps(e)) {
				//if other code is written properly, this branch shouldn't be accessible, but there's no harm in checking
				lpv.resize(0);
				return;
			}
			if (projE.xmin() <= e.xmin() && projE.xmax() >= e.xmax() && projE.ymin() <= e.ymin() && projE.ymax() >= e.ymax()) {
				//if the contract of the function, that e contains all points in lpv, is met, then this is a quick check for the (common) case where all the points pass the AoI filter
				return;
			}

			size_t index = 0;
			size_t moveTo = 0;
			projE = QuadExtent(*_extentFilter, lpv.crs).outerExtent(); //e and lpv ought to have the same crs, but just in case
			while (index < lpv.size()) {
				LasPoint& p = lpv[index];
				if (projE.contains(p.x, p.y)) {
					if (index != moveTo) {
						lpv[moveTo] = p;
					}
					index++;
					moveTo++;
				} else {
					index++;
				}
			}
			lpv.resize(moveTo);
			return;
		}
	}
	void AoIParameter::_errorWindow()
	{
		static bool displayErrorWindow = false;
		if (_errorMessage.size()==0) {
			return;
		}
		displayErrorWindow = true;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("Alignment Error Window", &displayErrorWindow, flags)) {
			ImGui::End();
			return;
		}

		ImGui::Text(_errorMessage.c_str());
		if (ImGui::Button("OK")) {
			displayErrorWindow = false;
			_errorMessage = "";
		}

		ImGui::End();
	}

	AoIParameter::AoIParameter()
	{
		_typeRadio.addOption("Process Full Area", (int)AoIType::None, AoIType::None);
		_typeRadio.addOption("Process Area in Rectangular Extent", (int)AoIType::Extent, AoIType::Extent);
		//_typeRadio.addOption("Process Area in Polygon (slow)", (int)AoIType::Vector, AoIType::Vector);
		//_typeRadio.addOption("Process Area Covered by Raster (slow)", (int)AoIType::Raster, AoIType::Raster);

		_typeRadio.addHelpText("On this tab, you can specify a subset of the data to process in this run.\n\n"
			"Full Area: Process all data. Do not subset it.\n\n"
			"Rectangular Extent: Specify a rectangle to process. Data outside that rectangle will be skipped. Note that if your processing job involves reprojection, the area processed might be somewhat larger than the extent you specify.\n\n"
			"Polygon: Specify a vector file (.shp) containing polygon data. Lidar data that doesn't fall within this polygon will be skipped. The process for checking which points fall within the polygon slows down processing considerably. "
			"Recommended only for small jobs.\n\n"
			"Raster: Specify a raster file (many supported formats). Lidar data that falls within NoData cells of the raster will be skipped. The process for checking which points fall within the raster slows down the processing considerably. "
			"Recommended only for small jobs.");

		_specifyManually.displayFalseFirst();
	}
	void AoIParameter::addToCmd(BoostOptDesc& visible, BoostOptDesc& hidden)
	{
		_typeRadio.addToCmd(visible, hidden);
		_extentXMin.addToCmd(visible, hidden);
		_extentXMax.addToCmd(visible, hidden);
		_extentYMin.addToCmd(visible, hidden);
		_extentYMax.addToCmd(visible, hidden);
		_extentCRS.addToCmd(visible, hidden);
		hidden.add_options()
			("aoiextent", boost::program_options::value<std::string>(&_extentFile));
	}
	std::ostream& AoIParameter::printToIni(std::ostream& o)
	{
		_typeRadio.printToIni(o);

		if (_typeRadio.currentSelection() == AoIType::Extent) {
			_extentXMin.printToIni(o);
			_extentXMax.printToIni(o);
			_extentYMin.printToIni(o);
			_extentYMax.printToIni(o);
			_extentCRS.printToIni(o);
		}

		return o;
	}
	ParamCategory AoIParameter::getCategory() const
	{
		return ParamCategory::process;
	}
	void AoIParameter::renderGui()
	{
		_typeRadio.renderGui();
		if (_typeRadio.currentSelection() == AoIType::Extent) {
			ImGui::BeginChild("aoi extent child", ImGui::GetContentRegionAvail(), true);
			_specifyManually.renderGui();
			if (_specifyManually.currentState()) {
				_extentXMin.renderGui();
				ImGui::SameLine();
				_extentXMax.renderGui();
				_extentYMin.renderGui();
				ImGui::SameLine();
				_extentYMax.renderGui();
				_extentCRS.renderGui();
			}
			else {
				static NFD::UniquePathU8 extentFile;
				if (ImGui::Button("Select File##aoiextent")) {
					NFD::OpenDialog(extentFile);
				}
				if (extentFile) {
					_extentFile = extentFile.get();
					importFromBoost();
					extentFile.reset();
				}
				
				auto displayBound = [](const std::string& s, const NumericTextBox& b) {
					ImGui::Text((s + b.asText()).c_str());
				};
				displayBound("X Min: ", _extentXMin);
				ImGui::SameLine();
				displayBound("X Max: ", _extentXMax);

				displayBound("Y Min: ", _extentYMin);
				ImGui::SameLine();
				displayBound("Y Max: ", _extentYMax);

				_extentCRS.renderDisplayString();

			}
			ImGui::EndChild();
		}

		_errorWindow();
	}
	void AoIParameter::importFromBoost()
	{
		if (_extentFile.size()) {
			_typeRadio.setSelection((int)AoIType::Extent);
			try {
				Extent e{ _extentFile };
				_extentXMin.setValue(e.xmin());
				_extentXMax.setValue(e.xmax());
				_extentYMin.setValue(e.ymin());
				_extentYMax.setValue(e.ymax());
				_extentCRS.setCrs(e.crs());
				_specifyManually.setState(false);
			}
			catch (...) {
				_errorMessage = "Error reading " + std::string(_extentFile);
			}
			_extentFile = "";
		}
		bool extentspecified = false;
		extentspecified |= _extentXMin.importFromBoost();
		extentspecified |= _extentXMax.importFromBoost();
		extentspecified |= _extentYMin.importFromBoost();
		extentspecified |= _extentYMax.importFromBoost();
		extentspecified |= _extentCRS.importFromBoost();
		if (extentspecified) {
			_specifyManually.setState(true);
		}

		_typeRadio.importFromBoost();
	}
	void AoIParameter::updateUnits()
	{
	}
	bool AoIParameter::prepareForRun()
	{
		if (_runPrepared) {
			return true;
		}

		LapisLogger& log = LapisLogger::getLogger();
		switch (_typeRadio.currentSelection()) {
		case AoIType::None:
			break;
		case AoIType::Extent:
			try {
				coord_t xmin = _extentXMin.getValueLogErrors();
				coord_t xmax = _extentXMax.getValueLogErrors();
				coord_t ymin = _extentYMin.getValueLogErrors();
				coord_t ymax = _extentYMax.getValueLogErrors();
				CoordRef crs = _extentCRS.cachedCrs();
				if (std::isnan(xmin) || std::isnan(xmax) || std::isnan(ymin) || std::isnan(ymax)) {
					return false;
				}
				_extentFilter = std::make_unique<Extent>(xmin, xmax, ymin, ymax, crs);
			}
			catch (InvalidExtentException e) {
				log.logError("AoI Extent Invalid. Make sure the maxes are greater than the mins.");
				return false;
			}
			break;
		default:
			log.logError("Invalid Area of Interest Type");
			return false;
		}

		_runPrepared = true;
		return true;
	}
	void AoIParameter::cleanAfterRun()
	{
	}
	int AoIParameter::AoITypeDecider::operator()(const std::string& s) const
	{
		if (_stricmp(s.c_str(), "None") == 0) {
			return (int)AoIType::None;
		}
		if (_stricmp(s.c_str(), "Raster") == 0) {
			return (int)AoIType::Raster;
		}
		if (_stricmp(s.c_str(), "Vector") == 0) {
			return (int)AoIType::Vector;
		}
		if (_stricmp(s.c_str(), "Extent") == 0) {
			return (int)AoIType::Extent;
		}
		return (int)AoIType::Error;
	}
	std::string AoIParameter::AoITypeDecider::operator()(int i) const
	{
		if (i == (int)AoIType::None) {
			return "None";
		}
		if (i == (int)AoIType::Extent) {
			return "Extent";
		}
		if (i == (int)AoIType::Raster) {
			return "Raster";
		}
		if (i == (int)AoIType::Vector) {
			return "Vector";
		}
		return "Error";
	}
}