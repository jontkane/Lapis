#pragma once
#ifndef LP_GUIFILESPECIFIER_H
#define LP_GUIFILESPECIFIER_H

#include"GuiCmdElement.hpp"

namespace lapis {
	class FileSpecifierSet : public GuiCmdElement {
	public:
		FileSpecifierSet(const std::string& guiDesc, const std::string& cmdName,
			const std::string& cmdDescription,
			const std::vector<std::string>& wildcards,
			std::unique_ptr<nfdnfilteritem_t>&& fileFilter);

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;
		std::ostream& printToIni(std::ostream& o) const override;

		bool renderGui() override;

		bool importFromBoost() override;

		const std::set<std::string>& getSpecifiers() const;

		template<class OPENER, class RETURNTYPE>
		std::set<RETURNTYPE> getFiles(const OPENER& opener) const;

	private:
		std::set<std::string> _fileSpecsSet;
		std::vector<std::string> _fileSpecsBoost;
		NFD::UniquePathU8 _nfdFolder;
		NFD::UniquePathSet _nfdFiles;
		bool _recursiveCheck = true;
		std::unique_ptr<nfdnfilteritem_t> _fileFilter;
		std::vector<std::string> _wildcards;
	};


	template<class OPENER, class RETURNTYPE>
	inline std::set<RETURNTYPE> FileSpecifierSet::getFiles(const OPENER& opener) const
	{
		namespace fs = std::filesystem;

		std::set<RETURNTYPE> fileList;

		std::queue<std::string> toCheck;

		for (const std::string& spec : _fileSpecsSet) {
			toCheck.push(spec);
		}

		while (toCheck.size()) {
			fs::path specPath{ toCheck.front() };
			toCheck.pop();

			//specified directories get searched recursively
			if (fs::is_directory(specPath)) {
				for (auto& subpath : fs::directory_iterator(specPath)) {
					toCheck.push(subpath.path().string());
				}
				//because of the dumbass ESRI grid format, I have to try to open folders as rasters as well
				try {
					fileList.insert(opener(specPath));
				}
				catch (...) {}
			}

			if (fs::is_regular_file(specPath)) {
				//if it's a file, try to add it to the map
				try {
					fileList.insert(opener(specPath));
				}
				catch (...) {}
			}

			//wildcard specifiers (e.g. C:\data\*.laz) are basically a non-recursive directory check with an extension
			if (specPath.has_filename()) {
				if (fs::is_directory(specPath.parent_path())) {
					std::regex wildcard{ "^\\*\\..+" };
					std::string ext = "";
					if (std::regex_match(specPath.filename().string(), wildcard)) {
						ext = specPath.extension().string();
					}

					if (ext.size()) {
						for (auto& subpath : fs::directory_iterator(specPath.parent_path())) {
							if (subpath.path().has_extension() && subpath.path().extension() == ext || ext == ".*") {
								toCheck.push(subpath.path().string());
							}
						}
					}
				}
			}
		}
		return fileList;
	}
}

#endif