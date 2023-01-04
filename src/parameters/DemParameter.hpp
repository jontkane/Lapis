#pragma once
#ifndef LP_DEMPARAMETER_H
#define LP_DEMPARAMETER_H

#include"Parameter.hpp"
#include"..\algorithms\DemAlgorithm.hpp"

namespace lapis {


	namespace DemAlgo {
		enum DemAlgo {
			OTHER,
			DONTNORMALIZE,
			VENDORRASTER
		};
	}

	class DemParameter : public Parameter {
	public:

		DemParameter();

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
		static size_t parameterRegisteredIndex;

		DemAlgorithm* demAlgorithm();

		const std::vector<Alignment>& demAligns();
		Raster<coord_t> getDem(size_t n);

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

		struct DemFileAlignment {
			std::filesystem::path file;
			Alignment align;
		};
		friend bool operator<(const DemFileAlignment& a, const DemFileAlignment& b);
		class DemOpener {
		public:

			DemOpener(const CoordRef& crsOverride, const Unit& unitOverride);

			DemFileAlignment operator()(const std::filesystem::path& f) const;

		private:
			const CoordRef& _crsOverride;
			const Unit& _unitOverride;
		};

		std::vector<Alignment> _demAligns;
		std::vector<std::string> _demFileNames;

		bool _runPrepared = false;

		RadioSelect<UnitDecider, Unit> _unit{ "Vertical units in DEM files:","dem-units" };
		CRSInput _crs{ "DEM CRS:","dem-crs","Infer from files" };
		bool _displayCrsWindow = false;
		bool _displayAdvanced = false;

		std::unique_ptr<DemAlgorithm> _algorithm;
		class DemAlgoDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<DemAlgoDecider, DemAlgo::DemAlgo> _demAlgo{ "Ground Model Method:","dem-algo" };

		void _renderAdvancedOptions();
	};
}

#endif