#pragma once
#ifndef LAPISDATA_H
#define LAPISDATA_H

#include"LapisParameters.hpp"

namespace lapis {


	class LapisData {

	public:
		template<ParamName N>
		using data = Parameter<N>::data_type;


		static LapisData& getDataObject();

		struct AlignWithoutExtent {
			CoordRef crs;
			coord_t xres, yres, xorigin, yorigin;
		};
		struct ClassFilter {
			bool isWhiteList;
			std::unordered_set<uint8_t> list;
		};

		void renderGui(ParamName pn);
		void setPrevUnits(const Unit& u);
		const Unit& getPrevUnits() const;
		void importBoostAndUpdateUnits();

		const std::set<std::string>& getLas() const;
		const std::set<std::string>& getDem() const;
		const std::string& getOutput() const;
		const Unit& getLasUnits() const;
		const Unit& getDemUnits() const;
		CoordRef getLasCrs() const;
		CoordRef getDemCrs() const;
		coord_t getFootprintDiameter() const;
		int getSmoothWindow() const;
		AlignWithoutExtent getAlign() const;
		coord_t getCSMCellsize() const;
		const Unit& getUserUnits() const;
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
		std::string getName() const;
		bool getGdalProjWarningFlag() const;
		bool getAdvancedPointFlag() const;

		enum class ParseResults {
			invalidOpts, helpPrinted, validOpts, guiRequested
		};

		ParseResults parseArgs(const std::vector<std::string>& args);
		ParseResults parseIni(const std::string& path);

		std::ostream& writeOptions(std::ostream& out, ParamCategory cat);


	private:
		LapisData();
		std::vector<std::unique_ptr<ParamBase>> _params;

		//template trickery to get a loop over ParamName values recursively
		template<ParamName N>
		void _addParam();

		template<ParamName N>
		Parameter<N>& _getRawParam();

		template<ParamName N>
		const Parameter<N>& _getRawParam() const;

		Unit _prevUnits;
	};

	template<ParamName N>
	inline void LapisData::_addParam()
	{
		if constexpr(static_cast<int>(N) >= static_cast<int>(ParamName::_Count_NotAParam)) {
			return;
		}
		else {
			constexpr ParamName NEXT = (ParamName)((int)N + 1);
			_params.emplace_back(std::make_unique<Parameter<N>>());
			_addParam<NEXT>();
		}
	}

	template<ParamName N>
	inline Parameter<N>& LapisData::_getRawParam()
	{
		return *dynamic_cast<Parameter<N>*>(_params[(size_t)N].get());
	}
	template<ParamName N>
	inline const Parameter<N>& LapisData::_getRawParam() const
	{
		return *dynamic_cast<Parameter<N>*>(_params[(size_t)N].get());
	}
}

#endif