#pragma once
#ifndef LP_LASFILEPARAMETER_H
#define LP_LASFILEPARAMETER_H

#include"Parameter.hpp"

namespace lapis {

	class LasFileParameter : public Parameter {
		class LasOpenerAbstract;
		friend class MockLasOpener;
	public:

		LasFileParameter();
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

		const Extent& getFullExtent();
		const std::vector<Extent>& sortedLasExtents();
		
		LasReader getLas(size_t n);
        const std::string& getLasFileName(size_t n);

		std::optional<LinearUnit> lasZUnits();

		std::shared_ptr<VectorDataset<Polygon>> lasFileLayout();

		void setFileSystemWrapperForTests(std::unique_ptr<FileSystemWrapper>&& fsWrapper);
        void setLasOpenerForTests(std::unique_ptr<LasOpenerAbstract>&& lasOpener);

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
		std::shared_ptr<VectorDataset<Polygon>> _lasLayout;

		struct LasFileExtent {
			std::filesystem::path file;
			LasExtent ext;
		};
		class LasOpenerAbstract {
		public:
			virtual ~LasOpenerAbstract() = default;
            virtual LasFileExtent operator()(const std::filesystem::path& f) const = 0;
		};
		class LasOpener : public LasOpenerAbstract {
		public:

			LasOpener();

			LasFileExtent operator()(const std::filesystem::path& f) const override;
		};
		friend bool operator<(const LasFileParameter::LasFileExtent& a, const LasFileParameter::LasFileExtent& b);

        std::unique_ptr<LasOpenerAbstract> _mockedLasOpener = std::unique_ptr<LasOpenerAbstract>(new LasOpener{});
        std::unique_ptr<FileSystemWrapper> _fsWrapper = std::unique_ptr<FileSystemWrapper>(new RealFileSystem{});

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