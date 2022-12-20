#pragma once
#ifndef LAPISPARAMETERS_H
#define LAPISPARAMETERS_H

#include"app_pch.hpp"
#include"LapisUtils.hpp"
#include"ParameterBase.hpp"
#include"CommonGuiElements.hpp"

namespace lapis {

	template<>
	class Parameter<ParamName::name> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const std::string& name();

	private:
		TextBox _name{ "Name of Run:","name",
		"The name of the run. If specified, will be included in the filenames and metadata." };

		std::string _nameString;
		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::las> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const Extent& getFullExtent();
		const std::vector<Extent>& sortedLasExtents();
		const std::set<std::string>& currentFileSpecifiers() const;
		const std::vector<std::string>& sortedLasFileNames();

	private:
		FileSpecifierSet _specifiers{ "Las","las",
		"Specify input point cloud (las/laz) files in one of three ways:\n"
			"\tAs a file name pointing to a point cloud file\n"
			"As a folder name, which will haves it and its subfolders searched for .las or .laz files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.laz\n"
			"This option can be specified multiple times",
			{"*.las","*.laz"},std::make_unique<nfdnfilteritem_t>(L"LAS Files", L"las,laz") };

		std::vector<std::string> _lasFileNames;
		std::vector<Extent> _lasExtents;
		Extent _fullExtent;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::dem> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const std::vector<Alignment>& demAligns();
		const std::vector<std::string>& demFileNames();
		const std::set<std::string>& currentFileSpecifiers() const;

	private:
		FileSpecifierSet _specifiers{ "Dem","dem",
		"Specify input raster elevation models in one of three ways:\n"
			"\tAs a file name pointing to a raster file\n"
			"As a folder name, which will haves it and its subfolders searched for raster files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.tif\n"
			"This option can be specified multiple times\n"
			"Most raster formats are supported, but arcGIS .gdb geodatabases are not" ,
			{"*.*"},
		std::unique_ptr<nfdnfilteritem_t>() };

		std::vector<Alignment> _demAligns;
		std::vector<std::string> _demFileNames;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::output> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const std::filesystem::path& path();
		bool isDebugNoOutput() const;

	private:
		FolderTextInput _output{ "Output Folder:","output",
			"The output folder to store results in" };
		CheckBox _debugNoOutput{ "Debug no Output","debug-no-output" };

		std::filesystem::path _outPath;
		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::lasOverride> : public ParamBase {
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const CoordRef& crs();
		const Unit& zUnits() const;

	private:
		RadioSelect<UnitDecider, Unit> _unit{ "Vertical units in laz files:","las-units" };
		CRSInput _crs{ "Laz CRS:","las-crs","Infer from files" };
		bool _displayWindow = false;
	};

	template<>
	class Parameter<ParamName::demOverride> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const CoordRef& crs();
		const Unit& zUnits() const;

	private:
		RadioSelect<UnitDecider, Unit> _unit{ "Vertical units in DEM files:","dem-units" };
		CRSInput _crs{ "DEM CRS:","dem-crs","Infer from files" };
		bool _displayWindow = false;
	};

	template<>
	class Parameter<ParamName::outUnits> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

		const std::string& unitPluralName() const;
		const std::string& unitSingularName() const;
		const Unit& unit() const;

	private:
		RadioSelect<UnitDecider, Unit> _unit{ "Output Units:","user-units","The units you want output to be in. All options will be interpretted as these units\n"
			"\tValues: m (for meters), f (for international feet), usft (for us survey feet)\n"
			"\tDefault is meters" };

		class UnitHasher {
		public:
			size_t operator()(const Unit& u) const {
				return std::hash<std::string>()(u.name + std::to_string(u.convFactor));
			}
		};
		const std::unordered_map<Unit, std::string, UnitHasher> _singularNames = {
			{LinearUnitDefs::meter,"Meter"},
			{LinearUnitDefs::foot,"Foot"},
			{LinearUnitDefs::surveyFoot,"Foot"},
			{LinearUnitDefs::unkLinear,"Unit"}
		};
		const std::unordered_map<Unit, std::string, UnitHasher> _pluralNames = {
			{LinearUnitDefs::meter,"Meters"},
			{LinearUnitDefs::foot,"Feet"},
			{LinearUnitDefs::surveyFoot,"Feet"},
			{LinearUnitDefs::unkLinear,"Units"}
		};
	};

	template<>
	class Parameter<ParamName::alignment> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		const CoordRef& getCurrentOutCrs() const;
		const Alignment& getFullAlignment();
		void prepareForRun() override;
		void cleanAfterRun() override;

		std::shared_ptr<Raster<int>> nLaz();
		std::shared_ptr<Alignment> metricAlign();

		bool isDebug() const;

	private:

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

		CheckBox _debugNoAlign{ "Debug No Alignment","debug-no-alignment" };

		std::string _alignCmd = "alignment";

		std::string _alignFileBoostString;

		NFD::UniquePathU8 _nfdAlignFile;

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

	template<>
	class Parameter<ParamName::csmOptions> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

		int smooth() const;
		std::shared_ptr<Alignment> csmAlign();
		coord_t footprintDiameter() const;
		bool doCsmMetrics() const;

	private:
		NumericTextBoxWithUnits _footprint{ "Diameter of Pulse Footprint:","footprint",0.4 };
		NumericTextBoxWithUnits _cellsize{ "Cellsize:","csm-cellsize",1,
		"The desired cellsize of the output canopy surface model\n"
			"Defaults to 1 meter" };
		InvertedCheckBox _doMetrics{ "Calculate CSM Metrics","skip-csm-metrics" };
		RadioSelect<SmoothDecider, int> _smooth{ "Smoothing:","smooth" };

		bool _showAdvanced = false;

		std::shared_ptr<Alignment> _csmAlign;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::pointMetricOptions> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

		bool doAllReturns() const;
		bool doFirstReturns() const;

		const std::vector<coord_t>& strata() const;
		const std::vector<std::string>& strataNames();
		coord_t canopyCutoff() const;
		bool doStratumMetrics() const;
		bool doAdvancedPointMetrics() const;

	private:
		NumericTextBoxWithUnits _canopyCutoff{ "Canopy Cutoff:","canopy",2,
		"The height threshold for a point to be considered canopy." };
		MultiNumericTextBoxWithUnits _strata{ "Stratum Breaks:","strata","0.5,1,2,4,8,16,32,48,64",
			"A comma-separated list of strata breaks on which to calculate strata metrics." };
		RadioBoolean _advMetrics{ "adv-point","All Metrics","Common Metrics",
		"Calculate a larger suite of point metrics." };
		InvertedCheckBox _doStrata{ "Calculate Strata Metrics","skip-strata" };
		
		static constexpr int FIRST_RETURNS = RadioDoubleBoolean::FIRST;
		static constexpr int ALL_RETURNS = RadioDoubleBoolean::SECOND;
		static constexpr bool DO_SKIP = true;
		static constexpr bool DONT_SKIP = false;
		RadioDoubleBoolean _whichReturns{ "skip-first-returns","skip-all-returns" };

		std::vector<std::string> _strataNames;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::filters> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		const std::vector<std::shared_ptr<LasFilter>>& filters();
		coord_t minht() const;
		coord_t maxht() const;

	private:

		NumericTextBoxWithUnits _minht{ "Minimum Height:","minht",-8,
		"The threshold for low outliers. Points with heights below this value will be excluded." };
		NumericTextBoxWithUnits _maxht{ "Maximum Height:","maxht",100,
		"The threshold for high outliers. Points with heights above this value will be excluded." };
		InvertedCheckBox _filterWithheld{ "Filter Withheld Points","use-withheld" };
		ClassCheckBoxes _class{ "class",
		"A comma-separated list of LAS point classifications to use for this run.\n"
				"Alternatively, preface the list with ~ to specify a blacklist." };

		std::vector<std::shared_ptr<LasFilter>> _filters;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::whichProducts> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

		bool doCsm() const;
		bool doPointMetrics() const;
		bool doTao() const;
		bool doTopo() const;
		bool doFineInt() const;

		bool isDebug() const;

	private:

		InvertedCheckBox _doCsm{ "Canopy Surface Model","skip-csm" };
		InvertedCheckBox _doPointMetrics{ "Point Metrics","skip-point-metrics" };
		InvertedCheckBox _doTao{ "Tree ID","skip-tao" };
		InvertedCheckBox _doTopo{ "Topographic Metrics","skip-topo" };
		CheckBox _doFineInt{ "Fine-Scale Intensity Image","fine-int",
		"Create a canopy mean intensity raster with the same resolution as the CSM" };
	};

	template<>
	class Parameter<ParamName::computerOptions> : public ParamBase {
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

		int nThread() const;

	private:
		IntegerTextBox _thread{ "Number of Threads:","thread", defaultNThread(),
		"The number of threads to run Lapis on. Defaults to the number of cores on the computer" };
		std::string _threadCmd = "thread";


		FloatBuffer _threadBuffer = FloatBuffer{ 0 };
	};

	template<>
	class Parameter<ParamName::taoOptions> : public ParamBase {
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		coord_t minTaoHt() const;
		coord_t minTaoDist() const;

		IdAlgo::IdAlgo taoIdAlgo() const;
		SegAlgo::SegAlgo taoSegAlgo() const;

	private:

		NumericTextBoxWithUnits _minht{ "","min-tao-ht",2 };
		NumericTextBoxWithUnits _mindist{ "Minimum Distance Between Trees:","min-tao-dist",0 };
		RadioSelect<IdAlgoDecider, IdAlgo::IdAlgo> _idAlgo{ "Tree ID Algorithm:","id-algo" };
		RadioSelect<SegAlgoDecider, SegAlgo::SegAlgo> _segAlgo{ "Canopy Segmentation Algorithm:","seg-algo" };
		RadioBoolean _sameMinHt{ "tao-same-min-ht","Same as Point Metric Canopy Cutoff","Other:" };
	};

	template<>
	class Parameter<ParamName::fineIntOptions> : public ParamBase {
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;

		std::shared_ptr<Alignment> fineIntAlign();
		coord_t fineIntCutoff() const;

	private:
		NumericTextBoxWithUnits _cellsize{ "","fine-int-cellsize",1 };
		InvertedCheckBox _useCutoff{ "Use Returns Above a Certain Height","fine-int-no-cutoff" };
		NumericTextBoxWithUnits _cutoff{ "","fine-int-cutoff",2 };
		RadioBoolean _sameCellsize{ "fine-int-same-cellsize","Same as CSM","Other:" };
		RadioBoolean _sameCutoff{ "fine-int-same-cutoff","Same as Point Metric Canopy Cutoff","Other:" };

		bool _initialSetup = true;

		std::shared_ptr<Alignment> _fineIntAlign;

		bool _runPrepared = false;
	};

	template<>
	class Parameter<ParamName::topoOptions> : public ParamBase {
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		void prepareForRun() override;
		void cleanAfterRun() override;
	};

	template<class T>
	using fileSpecOpen = void(const std::filesystem::path&, std::set<T>&,
		const CoordRef& crs, const Unit& u);
	template<class T>
	std::set<T> iterateOverFileSpecifiers(const std::set<std::string>& specifiers, fileSpecOpen<T> openFunc,
		const CoordRef& crs, const Unit& u);

	//attempts to open the given file as a las / laz file. If it succeeds, adds the given file to data
	//otherwise, reports the error to the logger and adds it to LapisData::reportFailedLas()
	void tryLasFile(const std::filesystem::path& file, std::set<LasFileExtent>& data,
		const CoordRef& crs, const Unit& u);

	//attempts to open the given file as a raster file. If it succeeds, adds the given file to data
	//otherwise, logs an error and does nothing
	void tryDtmFile(const std::filesystem::path& file, std::set<DemFileAlignment>& data,
		const CoordRef& crs, const Unit& u);
	
}

#endif