#include"param_pch.hpp"
#include"FilterParameter.hpp"
#include"RunParameters.hpp"


namespace lapis {

	size_t FilterParameter::parameterRegisteredIndex = RunParameters::singleton().registerParameter(new FilterParameter());
	void FilterParameter::reset()
	{
		*this = FilterParameter();
	}

	FilterParameter::FilterParameter() {
		for (size_t i = 0; i < 256; ++i) {
			_class.setState(i, true);
		}
		_class.setState(7, false);
		_class.setState(9, false);
		_class.setState(18, false);

		_minht.addHelpText("Ocassionally, points in a laz file will be so far above or below the ground as to be obvious errors.\n\n"
			"You can specify those cutoffs here.");
		_class.addHelpText("Your laz files may have classification information indicating a best guess at what the lidar pulse struck.\n\n"
			"You can choose to exclude certain classifications from the analysis.");
		_filterWithheld.addHelpText("Some points in laz files are marked as withheld, indicating that they should not be used.\n\n"
			"For the vast majority of applications, they should be excluded, but you can choose to include them with this checkbox.");

		_scanAngle.addHelpText("In some acquisitions, points collected when the lidar instrument is firing off-nadir are lower quality than points collected at nadir.\n\n"
			"You can filter points with a large scan angle with this prompt. The value is in degrees.");
	}
	void FilterParameter::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden) {
		_minht.addToCmd(visible, hidden);
		_maxht.addToCmd(visible, hidden);
		_class.addToCmd(visible, hidden);
		_filterWithheld.addToCmd(visible, hidden);
		_scanAngle.addToCmd(visible, hidden);
	}
	std::ostream& FilterParameter::printToIni(std::ostream& o) {
		_minht.printToIni(o);
		_maxht.printToIni(o);
		_filterWithheld.printToIni(o);
		_class.printToIni(o);
		_scanAngle.printToIni(o);
		return o;
	}
	ParamCategory FilterParameter::getCategory() const {
		return ParamCategory::process;
	}
	void FilterParameter::renderGui() {
		_filterWithheld.renderGui();
		_minht.renderGui();
		_maxht.renderGui();

		_scanAngle.renderGui();

		ImGui::Text("Use Classes:");
		ImGui::SameLine();
		_class.renderGui();
	}
	void FilterParameter::updateUnits() {
		_minht.updateUnits();
		_maxht.updateUnits();
	}
	void FilterParameter::importFromBoost() {
		_minht.importFromBoost();
		_maxht.importFromBoost();
		_filterWithheld.importFromBoost();
		_class.importFromBoost();
		_scanAngle.importFromBoost();
	}
	bool FilterParameter::prepareForRun() {
		if (_runPrepared) {
			return true;
		}

		if (std::isnan(_minht.getValueLogErrors()) || std::isnan(_maxht.getValueLogErrors())) {
			return false;
		}

		if (_minht.getValueLogErrors() > _maxht.getValueLogErrors()) {
			LapisLogger& log = LapisLogger::getLogger();
			log.logMessage("Low outlier must be less than high outlier");
			return false;
		}
		if (_filterWithheld.currentState()) {
			_filters.emplace_back(new LasFilterWithheld());
		}
		_filters.push_back(_class.getFilter());

		double maxScanAngle = _scanAngle.getValueLogErrors();
		if (!(std::isnan(maxScanAngle) || maxScanAngle == 0 || maxScanAngle >= 90)) {
			_filters.emplace_back(new LasFilterMaxScanAngle(maxScanAngle));
		}

		_runPrepared = true;
		return true;
	}
	void FilterParameter::cleanAfterRun() {
		_filters.clear();
		_filters.shrink_to_fit();
		_runPrepared = false;
	}
	const std::vector<std::shared_ptr<LasFilter>>& FilterParameter::filters()
	{
		prepareForRun();
		return _filters;
	}
	coord_t FilterParameter::minht() const
	{
		return _minht.getValueLogErrors();
	}
	coord_t FilterParameter::maxht() const
	{
		return _maxht.getValueLogErrors();
	}
	void FilterParameter::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Point Filters");
		pdf.writeTextBlockWithWrap("It is very common for lidar data files to include a large number of points "
			"which are unsuitable for processing, either because they are errors or because you're doing a special run. "
			"This page describes the strategies used to exclude those points from the run.");

		_outlierPdf(pdf);
		_withheldPdf(pdf);
		_classPdf(pdf);
		_scanAnglePdf(pdf);
	}
	void FilterParameter::_outlierPdf(MetadataPdf& pdf)
	{
		RunParameters& rp = RunParameters::singleton();

		pdf.writeSubsectionTitle("Outlier Filter");
		std::stringstream outlier;
		outlier << "Points below " << pdf.numberWithUnits(minht(),rp.unitSingular(),rp.unitPlural()) << " ";
		outlier << "or above " << pdf.numberWithUnits(maxht(),rp.unitSingular(),rp.unitPlural());
		outlier << " were excluded.";
		pdf.writeTextBlockWithWrap(outlier.str());
	}
	void FilterParameter::_withheldPdf(MetadataPdf& pdf)
	{
		if (!_filterWithheld.currentState()) {
			return;
		}
		pdf.writeSubsectionTitle("Withheld Filter");
		pdf.writeTextBlockWithWrap("Points are sometimes marked 'withheld', indicating that a previous "
			"piece of software concluded they were unsuitable for use. Withheld points were excluded from "
			"this run.");
	}
	void FilterParameter::_classPdf(MetadataPdf& pdf)
	{
		bool anyFilter = false;
		bool anyPreDefinedFilter = false;
		bool anyUserDefinedFilter = false;
		for (size_t i = 0; i < _class.allChecks().size(); ++i) {
			if (!_class.allChecks()[i]) {
				anyFilter = true;
				if (i < _class.classNames().size()) {
					anyPreDefinedFilter = true;
				}
				else {
					anyUserDefinedFilter = true;
				}
			}
		}

		if (!anyFilter) {
			return;
		}

		pdf.writeSubsectionTitle("Class Filter");

		std::string intro = "Lapis does not perform point classification. However, it is common for input "
			"data to already be classified by some previous software. If this is the case for this data, then "
			"the following ";
		if (anyPreDefinedFilter) {
			intro += "standard classes were filtered: ";
		}
		else {
			intro += "classes with no standard meaning were filtered: ";
		}
		pdf.writeTextBlockWithWrap(intro);

		if (anyPreDefinedFilter) {
			for (size_t i = 0; i < _class.classNames().size(); ++i) {
				if (!_class.allChecks()[i]) {
					pdf.writeTextBlockWithWrap(_class.classNames()[i]);
				}
			}
		}
		if (anyPreDefinedFilter && anyUserDefinedFilter) {
			pdf.blankLine();
			pdf.writeTextBlockWithWrap("In addition, the following classes with no standard meaning were filtered:");
		}

		if (anyUserDefinedFilter) {
			bool needComma = false;
			std::stringstream display;
			for (size_t i = _class.classNames().size(); i < _class.allChecks().size(); ++i) {
				if (!_class.allChecks()[i]) {
					if (!needComma) {
						needComma = true;
					} else {
						display << ", ";
					}
					display << i;
				}
			}
			pdf.writeTextBlockWithWrap(display.str());
		}
	}
	void FilterParameter::_scanAnglePdf(MetadataPdf& pdf) {
		double maxScan = _scanAngle.getValueLogErrors();
		if (maxScan <= 0 || maxScan >= 90 || std::isnan(maxScan)) {
			return;
		}
		pdf.writeSubsectionTitle("Scan Angle Filter");
		std::stringstream ss;
		ss << "Aerial lidar sensors sweep back and forth as they emit pulses. "
			"The angle that a pulse was fired at is called the scan angle for that pulse. "
			"For some applications, points with a high scan angle are undesirable. In this run, "
			"points with a scan angle greater than "
			<< (int)std::round(maxScan) << " degrees were excluded.";
		pdf.writeTextBlockWithWrap(ss.str());
	}
}