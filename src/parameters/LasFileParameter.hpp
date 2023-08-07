#pragma once
#ifndef LP_LASFILEPARAMETER_H
#define LP_LASFILEPARAMETER_H

#include"Parameter.hpp"

namespace lapis {
	class LasFileParameter : public Parameter {
	public:

		LasFileParameter();

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

		const Extent& getFullExtent();
		const std::vector<Extent>& sortedLasExtents();
		
		LasReader getLas(size_t n);

		std::optional<LinearUnit> lasZUnits();

	private:
		FileSpecifierSet _specifiers{ "Las","las",
		"Specify input point cloud (las/laz) files in one of three ways:\n"
			"\tAs a file name pointing to a point cloud file\n"
			"As a folder name, which will haves it and its subfolders searched for .las or .laz files\n"
			"As a folder name with a wildcard specifier, e.g. C:\\data\\*.laz\n"
			"This option can be specified multiple times",
			{"*.las","*.laz"},std::make_unique<nfdnfilteritem_t>(L"LAS Files", L"las,laz") };

		Title _title{ "Las/Laz Files" };

		std::vector<std::string> _lasFileNames;
		std::vector<Extent> _lasExtents;
		Extent _fullExtent;

		struct LasFileExtent {
			std::filesystem::path file;
			LasExtent ext;
		};
		class LasOpener {
		public:

			LasOpener(const CoordRef& crsOverride, const LinearUnit& unitOverride);

			LasFileExtent operator()(const std::filesystem::path& f) const;

		private:
			const CoordRef& _crsOverride;
			const LinearUnit& _unitOverride;
		};
		friend bool operator<(const LasFileParameter::LasFileExtent& a, const LasFileParameter::LasFileExtent& b);

		bool _runPrepared = false;

		RadioSelect<UnitDecider, LinearUnit> _unit{ "Vertical units in laz files:","las-units" };
		CRSInput _crs{ "Laz CRS:","las-crs","Infer from files" };
		bool _displayCrsWindow = false;

		bool _displayAdvancedOptions = false;
		void _renderAdvancedOptions();

		bool _warnedAboutVersionMinor = false;
	};
}

#endif