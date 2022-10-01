#pragma once
#ifndef lp_options_h
#define lp_options_h

#include"app_pch.hpp"
#include"Logger.hpp"
#include"gis/BaseDefs.hpp"
#include"gis/CoordRef.hpp"
#include"gis/Alignment.hpp"
#include"LapisUtils.hpp"

namespace lapis {

	struct LapisGuiObjects;

	namespace paramNames {
		const std::string lasFiles = "las";
		const std::string demFiles = "dem";
		const std::string output = "output";
		const std::string lasUnits = "las-units";
		const std::string demUnits = "dem-units";
		const std::string lasCrs = "las-crs";
		const std::string demCrs = "dem-crs";
		const std::string footprint = "footprint";
		const std::string smooth = "smooth";
		const std::string alignment = "alignment";
		const std::string cellsize = "cellsize";
		const std::string csmCellsize = "csm-cellsize";
		const std::string outCrs = "out-crs";
		const std::string userUnits = "user-units";
		const std::string canopy = "canopy";
		const std::string minht = "minht";
		const std::string maxht = "maxht";
		const std::string strata = "strata";
		const std::string fineint = "fineint";
		const std::string classFilter = "class";
		const std::string useWithheld = "use-withheld";
		const std::string maxScanAngle = "max-scan-angle";
		const std::string onlyReturns = "only";
		const std::string xres = "xres";
		const std::string yres = "yres";
		const std::string xorigin = "xorigin";
		const std::string yorigin = "yorigin";
		const std::string thread = "thread";
		const std::string performance = "performance";
		const std::string name = "name";
		const std::string gdalprojDisplay = "gdalproj";
		const std::string advancedPoint = "adv-metrics";
	}

	class Options {
	public:
		using RawParam = std::variant<std::string, std::vector<std::string>, bool>;
		enum class DataType {
			binary, single, multi
		};
		enum class ParamCategory {
			data, computer, processing
		};
		struct FullParam {
			RawParam value;
			ParamCategory cat;
			std::string cmdDoc;
			std::string cmdAlt;
			bool hidden;

			FullParam(DataType type, ParamCategory cat, const std::string& cmdDoc, const std::string& cmdAlt, bool hidden);
			FullParam(DataType type, ParamCategory cat);
		};
		struct AlignWithoutExtent {
			CoordRef crs;

			//The sign of these values indicates how they should be treated:
			//A positive value came from a raster file, and are in the correct units
			//A negative value came from the user, and should be converted before use
			//A value of zero means unspecified, and a default value should be used
			coord_t xres, yres, xorigin, yorigin;
		};
		struct ClassFilter {
			bool isWhiteList;
			std::unordered_set<uint8_t> list;
		};

		static Options& getOptionsObject();

		//For parameters of type single, this replaces the old value.
		//For parameters of type multi, it adds to the list.
		//For parameters of type binary, it sets the flag to true
		void updateValue(const std::string& key, const std::string& value);

		std::string getIniText(const std::string& key);

		const std::vector<std::string>& getLas() const;
		const std::vector<std::string>& getDem() const;
		const std::string& getOutput() const;
		Unit getLasUnits() const;
		Unit getDemUnits() const;
		CoordRef getLasCrs() const;
		CoordRef getDemCrs() const;
		coord_t getFootprintDiameter() const;
		int getSmoothWindow() const;
		const AlignWithoutExtent& getAlign() const;
		coord_t getCSMCellsize() const;
		Unit getUserUnits() const;
		coord_t getCanopyCutoff() const;
		coord_t getMinHt() const;
		coord_t getMaxHt() const;
		std::vector<coord_t> getStrataBreaks() const;
		bool getFineIntFlag() const;
		ClassFilter getClassFilter() const;
		bool getUseWithheldFlag() const;
		//value is negative if there isn't a scan angle filter
		int8_t getMaxScanAngle() const;
		bool getOnlyFlag() const;
		int getNThread() const;
		bool getPerformanceFlag() const;
		const std::string& getName() const;
		bool getGdalProjWarningFlag() const;
		bool getAdvancedPointFlag() const;

		void reset();

		std::map<std::string, FullParam>& getParamInfo();

private:
		Options();

		std::map<std::string, FullParam> params;
		mutable std::unique_ptr<AlignWithoutExtent> outAlign;

		const std::string& getAsString(const std::string& key) const;
		const std::vector<std::string>& getAsVector(const std::string& key) const;
		bool getAsBool(const std::string& key) const;
		double getAsDouble(const std::string& key, double defaultValue) const;
	};

	enum class ParseResults {
		invalidOpts, helpPrinted, validOpts, guiRequested
	};

	//takes command line input and parses it into the options
	ParseResults parseArgs(const std::vector<std::string>& args);

	ParseResults parseGui(const LapisGuiObjects& lgo);
	ParseResults parseIni(const std::string& path);

	void writeOptions(std::ostream& out, Options::ParamCategory cat);

}

#endif