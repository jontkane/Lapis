#pragma once
#ifndef LAPISPARAMETERS_H
#define LAPISPARAMETERS_H

#include<boost/program_options.hpp>
#include"imgui/imgui.h"
#include <nfd.hpp>
#include"gis/CoordRef.hpp"
#include"gis/Alignment.hpp"
#include<sstream>

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
		csmOptions,
		metricOptions,
		filterOptions,
		optionalMetrics,
		computerOptions,

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
		linearUnitDefs::surveyFoot};

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

	private:
		std::string _nameCmd;
		std::string _nameBoost;
		std::array<char, 30> _nameBuffer;
	};

	struct LasDemSpecifics {
		mutable std::vector<std::string> fileSpecsVector;
		NFD::UniquePathU8 nfdFolder;
		NFD::UniquePathSet nfdFiles;
		bool recursiveCheckbox;

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

	private:
		const std::string _cmdName = "las";
		LasDemSpecifics _spec;
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

	private:
		const std::string _cmdName = "dem";
		LasDemSpecifics _spec;
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

	private:
		std::string _cmdName = "output";
		std::string _string;
		std::array<char, 255> _buffer = std::array<char, 255>{0};
		NFD::UniquePathU8 _nfdFolder;
	};

	std::string getDisplayCrsString(const char* inStr, const char* defaultMessage);
	int unitRadioFromString(const std::string& s);

	struct LasDemOverrideSpecifics {
		CoordRef getAsCrs() {
			CoordRef crs;
			try {
				crs = CoordRef(crsBoostString);
			}
			catch (UnableToDeduceCRSException e) {}
			crs.setZUnits(ParamBase::unitRadioOrder[unitRadio]);
			return crs;
		}

		std::string crsDisplayString = "Infer From Files";
		std::vector<char> crsBuffer;
		int unitRadio = 0;
		std::string crsBoostString = "";
		std::string unitBoostString = "";

		std::string name;
		bool displayManualCrsWindow = false;
		bool textSiezeFocus = false;

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

	private:
		inline static int _radio = 1; //making it static so that the other template specializations can check what the units currently are
		inline static int _radioLastFrame = 1;

		std::string _boostString;
		std::string _cmdName;
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

	private:
		std::string _alignCmd = "alignment";
		std::string _cellsizeCmd = "cellsize";
		std::string _xresCmd = "xres";
		std::string _yresCmd = "yres";
		std::string _xoriginCmd = "xorigin";
		std::string _yoriginCmd = "yorigin";
		std::string _crsCmd = "out-crs";

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

	private:
		std::string _footprintDiamCmd = "footprint";
		std::string _smoothCmd = "smooth";
		std::string _csmCellsizeCmd = "csm-cellsize";

		coord_t _footprintDiamBoost;
		coord_t _csmCellsizeBoost;

		int _smooth;

		FloatBuffer _footprintDiamBuffer = FloatBuffer{ 0 };
		FloatBuffer _csmCellsizeBuffer = FloatBuffer{ 0 };

		bool _showAdvanced;
	};

	template<>
	class Parameter<ParamName::metricOptions> : public ParamBase {
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

	private:
		std::string _canopyCmd = "canopy";
		std::string _strataCmd = "strata";

		std::vector<FloatBuffer> _strataBuffers;
		FloatBuffer _canopyBuffer = FloatBuffer{ 0 };

		std::string _strataBoost;
		coord_t _canopyBoost;

		bool _displayStratumWindow;
	};

	template<>
	class Parameter<ParamName::filterOptions> : public ParamBase {
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

		bool _useWithheld;
		bool _filterWithheldCheck;

		bool _displayClassWindow;

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
	};

	template<>
	class Parameter<ParamName::optionalMetrics> : public ParamBase {
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

	private:
		std::string _fineIntCmd = "fine-int";
		std::string _advPointCmd = "adv-point";

		bool _fineIntCheck = false;
		bool _advPointCheck = false;
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

	private:
		std::string _threadCmd = "thread";
		std::string _perfCmd = "performance";
		std::string _displayGdalProjCmd = "gdalproj";

		int _threadBoost;

		bool _perfCheck;
		bool _gdalprojCheck;

		FloatBuffer _threadBuffer = FloatBuffer{ 0 };
	};

	const Unit& getCurrentUnits();
	const Unit& getUnitsLastFrame();
	const std::string& getUnitsPluralName();
}

#endif