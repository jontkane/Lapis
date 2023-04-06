#pragma once
#ifndef LP_DEMPARAMETER_H
#define LP_DEMPARAMETER_H

#include"Parameter.hpp"
#include"..\algorithms\DemAlgorithm.hpp"

namespace lapis {

	namespace DemAlgo {
		enum DemAlgo {
			ERROR,
			DONTNORMALIZE,
			VENDORRASTER
		};
	}

	class DemParameter : public Parameter {

	private:
		class DemContainerWrapper; //forward declare

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

		DemContainerWrapper demAligns();
		std::optional<Raster<coord_t>> getDem(size_t n, const Extent& e);

		//this function is used to expand an elevation raster calculated using whatever algorithm by background DEMs
		//The output is a raster which matched the alignment of the input raster, with an extent at least as large as desired
		//The values in the places where unbuffered has values will not be altered, but where unbuffered is NoData, the values will be filled in, where possible,
		//using a bilinear extraction from the rasters provided by the user
		Raster<coord_t> bufferElevation(const Raster<coord_t>& unbuffered, const Extent& desired);

	private:
		Title _title{ "Ground Model Method" };

		FileSpecifierSet _specifiers{ "Dem","dem",
		"Specify input raster elevation models in one of three ways:\n"
			"\tAs a file name pointing to a raster file\n"
			"As a folder name, which will haves it and its subfolders searched for raster files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.tif\n"
			"This option can be specified multiple times\n"
			"Most raster formats are supported, but ESRI .gdb geodatabases are not" ,
			{"*.*"},
		std::unique_ptr<nfdnfilteritem_t>() };

		struct DemFileAlignment {
			std::filesystem::path file;
			Alignment align;
		};
		friend bool operator<(const DemFileAlignment& a, const DemFileAlignment& b);
		class DemOpener {
		public:

			DemOpener(const CoordRef& crsOverride, const LinearUnit& unitOverride);

			DemFileAlignment operator()(const std::filesystem::path& f) const;

		private:
			const CoordRef& _crsOverride;
			const LinearUnit& _unitOverride;
		};

		std::vector<DemFileAlignment> _demFileAligns;

		bool _runPrepared = false;

		RadioSelect<UnitDecider, LinearUnit> _unit{ "Vertical units in DEM files:","dem-units" };
		CRSInput _crs{ "DEM CRS:","dem-crs","Infer from files" };
		bool _displayCrsWindow = false;
		bool _displayAdvanced = false;

		std::unique_ptr<DemAlgorithm> _algorithm;
		class DemAlgoDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<DemAlgoDecider, DemAlgo::DemAlgo> _demAlgo{ "","dem-algo" };

		void _renderAdvancedOptions();

		class DemIterator {
		public:
			DemIterator(std::vector<DemFileAlignment>::const_iterator it) : _it(it) {}
			DemIterator& operator++() {
				++_it;
				return *this;
			}
			const Alignment* operator->() {
				return &_it->align;
			}
			const Alignment& operator*() {
				return _it->align;
			}

			bool operator!=(const DemIterator& other) {
				return _it != other._it;
			}
		private:
			std::vector<DemFileAlignment>::const_iterator _it;
		};
		class DemContainerWrapper {
		public:
			DemContainerWrapper(std::vector<DemFileAlignment>& v) : _vec(v) {}
			DemIterator begin() {
				return DemIterator(_vec.begin());
			}
			DemIterator end() {
				return DemIterator(_vec.end());
			}

		private:
			const std::vector<DemFileAlignment>& _vec;
		};
	};
}

#endif