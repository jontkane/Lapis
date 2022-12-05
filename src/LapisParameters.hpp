#pragma once
#ifndef LAPISPARAMETERS_H
#define LAPISPARAMETERS_H

#include"app_pch.hpp"
#include"LapisUtils.hpp"
#include"MetricTypeDefs.hpp"

namespace lapis {

	//Each of these should correspond to a cohesive unit of the parameters fed into Lapis--not necessarily a single parameter each
	//As a rule of thumb, each one should correspond to any number of parameters that parameterize a single data object in the final run
	enum class ParamName {
		name,
		las,
		dem,
		output,
		lasOverride,
		demOverride,
		outUnits,
		alignment,
		filters,
		computerOptions,
		whichProducts,
		csmOptions,
		pointMetricOptions,
		taoOptions,
		fineIntOptions,

		_Count_NotAParam //this element's value will be equal to the cardinality of the enum class
	};

	enum class ParamCategory {
		data, computer, process
	};

	namespace po = boost::program_options;
	class ParamBase {
	public:
		using FloatBuffer = std::array<char, 11>;

		const inline static std::vector<Unit> unitRadioOrder = {
		linearUnitDefs::unkLinear,
		linearUnitDefs::meter,
		linearUnitDefs::foot,
		linearUnitDefs::surveyFoot };

		const inline static std::vector<std::string> unitPluralNames = {
			"Units",
			"Meters",
			"Feet",
			"Feet"
		};

		ParamBase() {}

		virtual void addToCmd(po::options_description& visible,
			po::options_description& hidden) = 0;

		virtual std::ostream& printToIni(std::ostream& o) = 0;

		//this function should assume that it's already in the correct tab/child
		//it should feel limited by the value of ImGui::GetContentRegionAvail().x, but not by y
		//if making things look nice takes more vertical space than provided, adjustments should be made in the layout portion of the code
		virtual void renderGui() = 0;

		virtual ParamCategory getCategory() const = 0;

		virtual void importFromBoost() = 0;
		virtual void updateUnits() = 0;

		virtual void prepareForRun() = 0;
		virtual void cleanAfterRun() = 0;
	};

	template<ParamName N>
	class Parameter : public ParamBase {};

	//This is the skeleton of a new template specialization of Parameter
	/*
	template<>
	class Parameter<ParamName::newparam> : public ParamBase {
		friend class LapisData;
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
	*/

	template<>
	class Parameter<ParamName::name> : public ParamBase {
		friend class LapisData;
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

		const std::string& getName();

	private:
		std::string _nameCmd = "name";
		std::string _nameBoost;
		std::array<char, 30> _nameBuffer;

		std::string _runString;
	};

	struct LasDemSpecifics {
		mutable std::vector<std::string> fileSpecsVector;
		NFD::UniquePathU8 nfdFolder;
		NFD::UniquePathSet nfdFiles;
		bool recursiveCheckbox = true;

		std::unique_ptr<nfdnfilteritem_t> fileFilter;
		std::string name;
		std::vector<std::string> wildcards;

		std::set<std::string>& getFileSpecsSet();
		const std::set<std::string>& getFileSpecsSet() const;

		void importFromBoost();

	private:
		mutable std::set<std::string> fileSpecsSet;
	};

	void lasDemUnifiedGui(LasDemSpecifics& spec);

	template<>
	class Parameter<ParamName::las> : public ParamBase {
		friend class LapisData;
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

		Extent getFullExtent();
		const std::vector<LasFileExtent> getAllExtents();

	private:
		const std::string _cmdName = "las";
		LasDemSpecifics _spec;

		std::vector<LasFileExtent> _fileExtents;
	};

	template<>
	class Parameter<ParamName::dem> : public ParamBase {
		friend class LapisData;
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

	private:
		const std::string _cmdName = "dem";
		LasDemSpecifics _spec;
		std::set<DemFileAlignment> _fileAligns;
	};

	template<>
	class Parameter<ParamName::output> : public ParamBase {
		friend class LapisData;
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

	private:
		std::string _cmdName = "output";
		std::string _boostString;
		std::array<char, 256> _buffer = std::array<char, 256>{0};
		NFD::UniquePathU8 _nfdFolder;

		std::filesystem::path _path;
	};

	void updateCrsAndDisplayString(const char* inStr, const char* defaultMessage, std::string* display, CoordRef* crs);
	int unitRadioFromString(const std::string& s);

	struct LasDemOverrideSpecifics {
		std::string crsDisplayString = "Infer From Files";
		std::vector<char> crsBuffer;
		int unitRadio = 0;
		std::string crsBoostString = "";
		std::string unitBoostString = "";

		std::string name;
		bool displayManualCrsWindow = false;
		bool textSiezeFocus = false;

		CoordRef crs;

		NFD::UniquePathU8 nfdFile;

		void importFromBoost();
	};

	void lasDemOverrideUnifiedGui(LasDemOverrideSpecifics& spec);

	template<>
	class Parameter<ParamName::lasOverride> : public ParamBase {
		friend class LapisData;
	public:
		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		const CoordRef& getCurrentCrs();
		void prepareForRun() override;
		void cleanAfterRun() override;

	private:
		LasDemOverrideSpecifics _spec;

		const std::string _lasUnitCmd = "las-units";
		const std::string _lasCrsCmd = "las-crs";
	};

	template<>
	class Parameter<ParamName::demOverride> : public ParamBase {
		friend class LapisData;
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		const CoordRef& getCurrentCrs();
		void prepareForRun() override;
		void cleanAfterRun() override;

	private:
		LasDemOverrideSpecifics _spec;

		const std::string _demUnitCmd = "dem-units";
		const std::string _demCrsCmd = "dem-crs";
	};

	template<>
	class Parameter<ParamName::outUnits> : public ParamBase {
		friend class LapisData;
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		static const std::string& getUnitsPluralName();

		void importFromBoost() override;
		void updateUnits() override;
		void prepareForRun() override;
		void cleanAfterRun() override;

	private:
		int _radio = 1;

		std::string _boostString;
		std::string _cmdName = "user-units";
	};

	void updateUnitsBuffer(ParamBase::FloatBuffer& buff);

	void copyToBuffer(ParamBase::FloatBuffer& buff, coord_t x);

	template<>
	class Parameter<ParamName::alignment> : public ParamBase {
		friend class LapisData;
	public:

		Parameter();

		void addToCmd(po::options_description& visible,
			po::options_description& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		CoordRef getCurrentOutCrs() const;
		const Alignment& getFullAlignment();
		void prepareForRun() override;
		void cleanAfterRun() override;

		bool isDebug() const;

	private:
		std::string _alignCmd = "alignment";
		std::string _cellsizeCmd = "cellsize";
		std::string _xresCmd = "xres";
		std::string _yresCmd = "yres";
		std::string _xoriginCmd = "xorigin";
		std::string _yoriginCmd = "yorigin";
		std::string _crsCmd = "out-crs";
		std::string _debugCmd = "debug-no-alignment";

		std::string _alignFileBoostString;
		std::string _crsBoostString;
		coord_t _cellsizeBoost;
		coord_t _xresBoost;
		coord_t _yresBoost;
		coord_t _xoriginBoost;
		coord_t _yoriginBoost;

		FloatBuffer _xresBuffer = FloatBuffer{ 0 };
		FloatBuffer _yresBuffer = FloatBuffer{ 0 };
		FloatBuffer _xoriginBuffer = FloatBuffer{ 0 };
		FloatBuffer _yoriginBuffer = FloatBuffer{ 0 };
		FloatBuffer _cellsizeBuffer = FloatBuffer{ 0 };

		std::string _crsDisplayString = "Same as LAZ Files";
		std::vector<char> _crsBuffer;

		NFD::UniquePathU8 _nfdAlignFile;
		NFD::UniquePathU8 _nfdCrsFile;

		bool _displayManualWindow = false;
		bool _xyResDiffCheck = false;
		bool _xyOriginDiffCheck = false;

		std::shared_ptr<Alignment> _align;
		std::shared_ptr<Raster<int>> _nLaz;

		bool _debugBool;
	};

	template<>
	class Parameter<ParamName::csmOptions> : public ParamBase {
		friend class LapisData;
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

	private:
		std::string _footprintDiamCmd = "footprint";
		std::string _smoothCmd = "smooth";
		std::string _csmCellsizeCmd = "csm-cellsize";
		std::string _skipMetricsCmd = "skip-csm-metrics";

		coord_t _footprintDiamBoost;
		coord_t _csmCellsizeBoost;

		int _smooth;

		FloatBuffer _footprintDiamBuffer = FloatBuffer{ 0 };
		FloatBuffer _csmCellsizeBuffer = FloatBuffer{ 0 };

		bool _showAdvanced = false;

		coord_t _footprintDiamCache;
		std::shared_ptr<Alignment> _csmAlign;

		std::vector<CSMMetric> _csmMetrics;
		bool _doCsmMetrics = true;
		bool _skipCsmMetricsBoost = false;
	};

	template<>
	class Parameter<ParamName::pointMetricOptions> : public ParamBase {
		friend class LapisData;
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

	private:
		std::string _canopyCmd = "canopy";
		std::string _strataCmd = "strata";
		std::string _advPointCmd = "adv-point";
		std::string _skipAllReturnsCmd = "skip-all-returns";
		std::string _skipFirstReturnsCmd = "skip-first-returns";

		std::vector<FloatBuffer> _strataBuffers;
		FloatBuffer _canopyBuffer = FloatBuffer{ 0 };

		std::string _strataBoost;
		coord_t _canopyBoost;

		bool _displayStratumWindow = false;

		coord_t _canopyCache;
		std::vector<coord_t> _strataCache;

		struct WhichReturns {
			static constexpr int all = 1;
			static constexpr int first = 2;
			static constexpr int both = 3;
		};

		bool _skipAllReturnBoost = false;
		bool _skipFirstReturnBoost = false;
		int _whichReturnsRadio = WhichReturns::both;

		int _advPointRadio = 0;
		bool _advPointBoost = false;

		std::vector<PointMetricRaster> _allReturnPointMetrics;
		std::vector<PointMetricRaster> _firstReturnPointMetrics;
		std::vector<StratumMetricRasters> _allReturnStratumMetrics;
		std::vector<StratumMetricRasters> _firstReturnStratumMetrics;

		std::shared_ptr<Raster<PointMetricCalculator>> _allReturnCalculators;
		std::shared_ptr<Raster<PointMetricCalculator>> _firstReturnCalculators;
	};

	template<>
	class Parameter<ParamName::filters> : public ParamBase {
		friend class LapisData;
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

	private:
		void _updateClassListString();


		std::string _minhtCmd = "minht";
		std::string _maxhtCmd = "maxht";
		std::string _classCmd = "class";
		std::string _useWithheldCmd = "use-withheld";

		coord_t _minhtBoost;
		coord_t _maxhtBoost;
		std::string _classBoost;

		FloatBuffer _minhtBuffer = FloatBuffer{ 0 };
		FloatBuffer _maxhtBuffer = FloatBuffer{ 0 };
		std::array<bool, 255> _classChecks;
		std::string _classDisplayString;

		bool _useWithheld = false;
		bool _filterWithheldCheck = true;

		bool _displayClassWindow = false;

		const std::vector<std::string> _classNames = { "Never Classified",
		"Unassigned",
		"Ground",
		"Low Vegetation",
		"Medium Vegetation",
		"High Vegetation",
		"Building",
		"Low Point",
		"Model Key (LAS 1.0-1.3)/Reserved (LAS 1.4)",
		"Water",
		"Rail",
		"Road Surface",
		"Overlap (LAS 1.0-1.3)/Reserved (LAS 1.4)",
		"Wire - Guard (Shield)",
		"Wire - Conductor (Phase)",
		"Transmission Tower",
		"Wire-Structure Connector (Insulator)",
		"Bridge Deck",
		"High Noise" };

		std::vector<std::shared_ptr<LasFilter>> _filters;
		coord_t _minhtCache;
		coord_t _maxhtCache;
	};

	template<>
	class Parameter<ParamName::whichProducts> : public ParamBase {
		friend class LapisData;
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

		bool isDebug() const;

	private:
		std::string _skipCsmCmd = "skip-csm";
		std::string _skipPointCmd = "skip-point-metrics";
		std::string _skipTaoCmd = "skip-tao";
		std::string _skipTopoCmd = "skip-topo";
		std::string _fineIntCmd = "fine-int";
		std::string _debugNoAllocRaster = "debug-no-alloc-raster";

		bool _fineIntCheck = false;

		bool _doTaoCheck = true;
		bool _skipTaoBoost = false;

		bool _doCsmCheck = true;
		bool _skipCsmBoost = false;

		bool _doPointCheck = true;
		bool _skipPointBoost = false;

		bool _doTopoCheck = true;
		bool _skipTopoBoost = false;

		std::vector<TopoMetric> _topoMetrics;
		std::shared_ptr<Raster<coord_t>> _elevNumerator;
		std::shared_ptr<Raster<coord_t>> _elevDenominator;

		bool _debugBool;
	};

	template<>
	class Parameter<ParamName::computerOptions> : public ParamBase {
		friend class LapisData;
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

	private:
		std::string _threadCmd = "thread";
		std::string _perfCmd = "performance";

		int _threadBoost;

		bool _perfCheck = false;

		FloatBuffer _threadBuffer = FloatBuffer{ 0 };

		int _threadCache;
	};

	namespace IdAlgo {
		enum {
			highPoint,
		};
	}
	namespace SegAlgo {
		enum {
			watershed,
		};
	}

	template<>
	class Parameter<ParamName::taoOptions> : public ParamBase {
		friend class LapisData;
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

	private:

		std::string _idAlgoCmd = "id-algo";
		std::string _segAlgoCmd = "seg-algo";
		std::string _minHtCmd = "min-tao-ht";
		std::string _minDistCmd = "min-tao-dist";

		std::string _idAlgoBoost;
		std::string _segAlgoBoost;
		coord_t _minHtBoost;
		coord_t _minDistBoost;

		int _idAlgoRadio = IdAlgo::highPoint;
		int _segAlgoRadio = SegAlgo::watershed;

		static constexpr int SAME = 0;
		static constexpr int NOT_SAME = 1;
		int _sameMinHtRadio = SAME;
		FloatBuffer _minHtBuffer = FloatBuffer{ 0 };


		FloatBuffer _minDistBuffer = FloatBuffer{ 0 };

		coord_t _minHtCache;
		coord_t _minDistCache;

		//this is related to the behavior of minht; when importFromBoost is called as part of initial setup, that shouldn't change the _sameMinHt flag
		//but any future calls to importFromBoost should change that flag.
		bool _initialSetup = true;
	};

	template<>
	class Parameter<ParamName::fineIntOptions> : public ParamBase {
		friend class LapisData;
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

	private:
		std::string _cellsizeCmd = "fine-int-cellsize";
		std::string _noCutoffCmd = "fine-int-no-cutoff";
		std::string _cutoffCmd = "fine-int-cutoff";

		static constexpr int SAME = 0;
		static constexpr int NOT_SAME = 1;

		bool _initialSetup = true;

		int _cellsizeSameAsCsmRadio = SAME;
		coord_t _cellsizeBoost = 1;
		FloatBuffer _cellsizeBuffer = FloatBuffer{ 0 };

		bool _cutoffCheck = true;
		bool _noCutoffBoost = false;
		int _cutoffSameAsPointRadio = SAME;
		FloatBuffer _cutoffBuffer = FloatBuffer{ 0 };
		coord_t _cutoffBoost = 2;
		coord_t _cutoffCache = 2;

		std::shared_ptr<Alignment> _fineIntAlign;
	};

	const Unit& getCurrentUnits();
	const Unit& getUnitsLastFrame();
	const std::string& getUnitsPluralName();

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

	coord_t getValueWithError(const ParamBase::FloatBuffer& buff, const std::string& name);
}

#endif