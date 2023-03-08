#include"param_pch.hpp"
#include"AlignmentParameter.hpp"
#include"RunParameters.hpp"

namespace lapis {

	size_t AlignmentParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new AlignmentParameter());
	void AlignmentParameter::reset()
	{
		*this = AlignmentParameter();
	}

	AlignmentParameter::AlignmentParameter()
	{
		_xorigin.addHelpText("The origin is what sometimes causes rasters with the same cellsize and CRS to not line up.\n\n"
			"Most of the time, when you specify this, it's to ensure a match with some other specific raster.\n\n"
			"You can specify the coordinates of the lower-left corner of any cell in the desired output grid.\n\n");
		_yorigin.addHelpText("");
		_origin.addHelpText("The origin is what sometimes causes rasters with the same cellsize and CRS to not line up.\n\n"
			"Most of the time, when you specify this, it's to ensure a match with some other specific raster.\n\n"
			"You can specify the coordinates of the lower-left corner of any cell in the desired output grid.\n\n");
		_xres.addHelpText("The cellsize is the size of each pixel on a side.\n\n"
			"This cellsize will be used for coarse output rasters, like point metrics, but not for fine output rasters, like the canopy surface model.\n\n");
		_yres.addHelpText("");
		_cellsize.addHelpText("The cellsize is the size of each pixel on a side.\n\n"
			"This cellsize will be used for coarse output rasters, like point metrics, but not for fine output rasters, like the canopy surface model.\n\n");

		_debugNoAlign.addHelpText("This checkbox should only be displayed in debug mode. If you see it in a public release, please contact the developer.");
	}
	void AlignmentParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		visible.add_options()
			((_alignCmd + ",A").c_str(), boost::program_options::value<std::string>(&_alignFileBoostString),
				"A raster file you want the output metrics to align with\n"
				"Overwritten by --cellsize and --out-crs options");

		_cellsize.addToCmd(visible, hidden);
		_xres.addToCmd(visible, hidden);
		_yres.addToCmd(visible, hidden);
		_origin.addToCmd(visible, hidden);
		_xorigin.addToCmd(visible, hidden);
		_yorigin.addToCmd(visible, hidden);
		_crs.addToCmd(visible, hidden);

		_debugNoAlign.addToCmd(visible, hidden);
	}
	std::ostream& AlignmentParameter::printToIni(std::ostream& o) {

		_xres.printToIni(o);
		_yres.printToIni(o);
		_xorigin.printToIni(o);
		_yorigin.printToIni(o);
		_crs.printToIni(o);

		return o;
	}
	ParamCategory AlignmentParameter::getCategory() const {
		return ParamCategory::process;
	}
	void AlignmentParameter::renderGui() {

		_manualWindow();
		_errorWindow();
		ImGui::Text("Output Alignment");
		if (ImGui::Button("Specify From File")) {
			NFD::OpenDialog(_nfdAlignFile);
		}

		if (_nfdAlignFile) {
			_alignFileBoostString = _nfdAlignFile.get();
			importFromBoost();
			_nfdAlignFile.reset();
		}

		ImGui::SameLine();
		if (ImGui::Button("Specify Manually")) {
			_displayManualWindow = true;
		}

		static HelpMarker help{ "This is how you specify a snap raster for the output of the run.\n\n"
			"If you specify a raster file, the output will have the same cellsize, projection, and alignment as that file.\n\n"
			"If you don't have a file with the alignment you want, you can also specify it manually." };

		help.renderGui();

		ImGui::Text("Output CRS:");
		_crs.renderDisplayString();
		ImGui::Text("Cellsize: ");
		ImGui::SameLine();
		if (_xyResDiffCheck) {
			ImGui::Text("Diff X/Y");
		}
		else {
			ImGui::Text(_cellsize.asText());
			ImGui::SameLine();
			ImGui::Text(RunParameters::singleton().unitPlural().c_str());
		}

#ifndef NDEBUG
		_debugNoAlign.renderGui();
#endif
	}
	void AlignmentParameter::updateUnits() {
		_cellsize.updateUnits();
		_xres.updateUnits();
		_yres.updateUnits();
		_origin.updateUnits();
		_xorigin.updateUnits();
		_yorigin.updateUnits();
	}
	void AlignmentParameter::importFromBoost() {
		if (_alignFileBoostString.size()) {
			try {
				Alignment a{ _alignFileBoostString };

				const Unit& src = a.crs().getXYUnits();
				const Unit& dst = RunParameters::singleton().outUnits();

				_xres.setValue(convertUnits(a.xres(), src, dst));
				_yres.setValue(convertUnits(a.yres(), src, dst));
				if (a.xres() == a.yres()) {
					_xyResDiffCheck = false;
					_cellsize.setValue(convertUnits(a.xres(), src, dst));
				}
				else {
					_xyResDiffCheck = true;
				}

				_xorigin.setValue(convertUnits(a.xOrigin(), src, dst));
				_yorigin.setValue(convertUnits(a.yOrigin(), src, dst));
				if (a.xOrigin() == a.yOrigin()) {
					_xyOriginDiffCheck = true;
					_origin.setValue(convertUnits(a.xOrigin(), src, dst));
				}
				else {
					_xyOriginDiffCheck = false;
				}

				if (!a.crs().isEmpty()) {
					_crs.setCrs(a.crs());
				}
				else {
					_crs.reset();
				}
			}
			catch (InvalidRasterFileException e) {
				_displayErrorWindow = true;
			}

		}
		_alignFileBoostString.clear();
		if (_cellsize.importFromBoost()) {
			_xres.copyFrom(_cellsize);
			_yres.copyFrom(_cellsize);
			_xyResDiffCheck = false;
		}
		if (_origin.importFromBoost()) {
			_xorigin.copyFrom(_origin);
			_yorigin.copyFrom(_origin);
			_xyOriginDiffCheck = false;
		}

		bool ximport = _xres.importFromBoost();
		bool yimport = _yres.importFromBoost();
		if (ximport || yimport) {
			if (strcmp(_xres.asText(), _yres.asText()) == 0) {
				_cellsize.copyFrom(_xres);
				_xyResDiffCheck = false;
			}
			else {
				_xyResDiffCheck = true;
			}
		}

		ximport = _xorigin.importFromBoost();
		yimport = _yorigin.importFromBoost();
		if (ximport || yimport) {
			if (strcmp(_xorigin.asText(), _yres.asText()) == 0) {
				_origin.copyFrom(_xorigin);
				_xyOriginDiffCheck = false;
			}
			else {
				_xyOriginDiffCheck = true;
			}
		}

		_crs.importFromBoost();
		_debugNoAlign.importFromBoost();
	}
	const CoordRef& AlignmentParameter::getCurrentOutCrs() const {
		return _crs.cachedCrs();
	}
	const Alignment& AlignmentParameter::getFullAlignment()
	{
		prepareForRun();
		return *_align;
	}
	bool AlignmentParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}
		if (_debugNoAlign.currentState()) {
			//something intentionally weird so nothing will match it unless it copied from it
			_align = std::make_shared<Alignment>(Extent(0, 50, 10, 60), 8, 6, 13, 19);
			_runPrepared = true;
			return true;
		}
		RunParameters& rp = RunParameters::singleton();
		LapisLogger& log = LapisLogger::getLogger();

		Extent e = rp.fullExtent();

		coord_t xres, yres, xorigin, yorigin;

		if (_xyResDiffCheck) {
			xres = _xres.getValueLogErrors();
			yres = _yres.getValueLogErrors();
		}
		else {
			xres = _cellsize.getValueLogErrors();
			yres = _cellsize.getValueLogErrors();
		}

		if (_xyOriginDiffCheck) {
			xorigin = _xorigin.getValueLogErrors();
			yorigin = _yorigin.getValueLogErrors();
		}
		else {
			xorigin = _origin.getValueLogErrors();
			yorigin = _origin.getValueLogErrors();
		}
		if (std::isnan(xres) || std::isnan(yres) || std::isnan(xorigin) || std::isnan(yorigin)) {
			return false;
		}

		const Unit& src = rp.outUnits();
		Unit dst = e.crs().getXYUnits();
		xres = convertUnits(xres, src, dst);
		yres = convertUnits(yres, src, dst);
		xorigin = convertUnits(xorigin, src, dst);
		yorigin = convertUnits(yorigin, src, dst);

		if (xres <= 0 || yres <= 0) {
			log.logMessage("Cellsize must be positive");
			return false;
		}

		_align.reset();
		_align = std::make_shared<Alignment>(e, xorigin, yorigin, xres, yres);

#ifndef NDEBUG
		log.logMessage("Out CRS: " + _align->crs().getShortName());
#endif
		_runPrepared = true;
		return true;
	}
	void AlignmentParameter::cleanAfterRun() {
		_align.reset();
		_runPrepared = false;
	}
	std::shared_ptr<Alignment> AlignmentParameter::metricAlign()
	{
		prepareForRun();
		return _align;
	}
	bool AlignmentParameter::isDebug() const
	{
		return _debugNoAlign.currentState();
	}
	void AlignmentParameter::describeInPdf(MetadataPdf& pdf)
	{
		RunParameters& rp = RunParameters::singleton();

		prepareForRun();
		pdf.newPage();
		pdf.writePageTitle("Output Data Characteristics");
	
		std::string xresDisplay = pdf.numberWithUnits(
			convertUnits(_align->xres(), _align->crs().getXYUnits(), rp.outUnits()),
			rp.unitSingular(),rp.unitPlural());
		std::string yresDisplay = pdf.numberWithUnits(
			convertUnits(_align->yres(), _align->crs().getXYUnits(), rp.outUnits()),
			rp.unitSingular(),rp.unitPlural());
		std::string cellsizeDesc;
		if (_align->xres() == _align->yres()) {
			cellsizeDesc = "All metrics were processed at a cellsize of " +
				xresDisplay + ".";
		}
		else {
			cellsizeDesc = "Metrics were processed with cells of unequal x and y size. The x resolution is " +
				xresDisplay + " and the y resolution is " +
				yresDisplay + ".";
		}
		pdf.writeTextBlockWithWrap(cellsizeDesc);
		std::string unitDisplay = rp.unitPlural();
		std::transform(unitDisplay.begin(), unitDisplay.end(), unitDisplay.begin(),
			[](unsigned char c) { return std::tolower(c); });
		pdf.writeTextBlockWithWrap("Where appropriate, the units of all output data are " + unitDisplay + ".");


		std::string crs = "All output data of this run is in the following coordinate reference system: " +
			_align->crs().getShortName() + ". The full WKT string is provided below for reference:";
		pdf.writeTextBlockWithWrap(crs);
		pdf.blankLine();

		std::istringstream wkt{ _align->crs().getPrettyWKT() };
		std::string line;
		while (std::getline(wkt, line)) {
			pdf.writeLeftAlignedTextLine(line, pdf.normalFont(), 12.f);
		}
	}
	void AlignmentParameter::_manualWindow()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!_displayManualWindow) {
			return;
		}
		if (!ImGui::Begin("Manual Alignment Window", &_displayManualWindow, flags)) {
			ImGui::End();
			return;
		}

		ImGui::Checkbox("Show Advanced Options", &_displayAdvanced);

		if (_displayAdvanced) {
			ImGui::Checkbox("Different X/Y Resolution", &_xyResDiffCheck);
			ImGui::Checkbox("Different X/Y Origin", &_xyOriginDiffCheck);
		}

		if (_displayAdvanced && _xyResDiffCheck) {
			_xres.renderGui();
			_yres.renderGui();
		}
		else {
			if (_cellsize.renderGui()) {
				_xres.copyFrom(_cellsize);
				_yres.copyFrom(_cellsize);
			}
		}

		if (_displayAdvanced && _xyOriginDiffCheck) {
			_xorigin.renderGui();
			_yorigin.renderGui();
		}
		else if (_displayAdvanced) {
			if (_origin.renderGui()) {
				_xorigin.copyFrom(_origin);
				_yorigin.copyFrom(_origin);
			}
		}

		_crs.renderGui();
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			_crs.reset();
		}
		if (ImGui::Button("OK")) {
			_displayManualWindow = false;
		}

		ImGui::End();
	}
	void AlignmentParameter::_errorWindow()
	{
		if (!_displayErrorWindow) {
			return;
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!ImGui::Begin("Alignment Error Window", &_displayManualWindow, flags)) {
			ImGui::End();
			return;
		}

		ImGui::Text("Error Reading that Raster File");
		if (ImGui::Button("OK")) {
			_displayErrorWindow = false;
		}

		ImGui::End();
	}
}