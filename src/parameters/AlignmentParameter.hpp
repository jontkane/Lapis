#pragma once
#ifndef LP_ALIGNMENTPARAMETER_H
#define LP_ALIGNMENTPARAMETER_H

#include"Parameter.hpp"


namespace lapis {
	class MetadataPdf;

	class AlignmentParameter : public Parameter {
	public:

		AlignmentParameter();
		LAPIS_PARAMETER_REGISTER_DECLARE;

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		bool prepareForRun() override;
		void cleanAfterRun() override;

		void reset() override;

		const CoordRef& getCurrentOutCrs() const;
		const Alignment& getFullAlignment();
		std::shared_ptr<Alignment> metricAlign();

		void describeInPdf(MetadataPdf& pdf);

		void updateFromFile(const std::string& filename);

	private:

		Title _title{ "Output Alignment" };

		NumericTextBoxWithUnits _xorigin{ "X Origin:","xorigin",0 };
		NumericTextBoxWithUnits _yorigin{ "Y Origin:","yorigin",0 };
		NumericTextBoxWithUnits _origin{ "Origin:","origin",0 };
		NumericTextBoxWithUnits _xres{ "X Resolution:","xres",30 };
		NumericTextBoxWithUnits _yres{ "Y Resolution:","yres",30 };
		NumericTextBoxWithUnits _cellsize{ "Cellsize:","cellsize",30,
		"The desired cellsize of the output metric rasters\n"
				"Defaults to 30 meters" };

		CRSInput _crs{ "Output CRS:","out-crs",
		"The desired CRS for the output layers\n"
				"Can be a filename, or a manual specification (usually EPSG)","Same as Las Files" };

		std::string _alignCmd = "alignment";

		FileSelectButton _alignFile = FileSelectButton("Specify From File", "align",
			"A raster file you want the output metrics to align with\n"
			"Overwritten by --cellsize and --out-crs options");


		bool _displayManualWindow = false;
		bool _displayErrorWindow = false;
		bool _xyResDiffCheck = false;
		bool _xyOriginDiffCheck = false;
		bool _displayAdvanced = false;

		std::shared_ptr<Alignment> _align;

		void _manualWindow();
		void _errorWindow();

		bool _runPrepared = false;
	};
}

#endif