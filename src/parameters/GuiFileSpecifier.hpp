#pragma once
#ifndef LP_GUIFILESPECIFIER_H
#define LP_GUIFILESPECIFIER_H

#include"GuiCmdElement.hpp"

namespace lapis {

	//abstracted for dependency injection; mocked version in test code
	class FileSystemWrapper {
	public:
		virtual ~FileSystemWrapper() = default;
        virtual std::vector<std::filesystem::path> listDirectory(const std::filesystem::path& dirPath) const = 0;
        virtual bool isDirectory(const std::filesystem::path& path) const = 0;
		virtual bool isRegularFile(const std::filesystem::path& path) const = 0;
	};
	class RealFileSystem : public FileSystemWrapper {
	public:
		std::vector<std::filesystem::path> listDirectory(const std::filesystem::path& dirPath) const override;
		bool isDirectory(const std::filesystem::path& path) const override;
		bool isRegularFile(const std::filesystem::path& path) const override;
	};

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

		const std::unordered_set<std::string>& getSpecifiers() const;

		template<class OPENER, class RETURNTYPE>
		std::vector<RETURNTYPE> getFiles(const OPENER& opener, const FileSystemWrapper* fileSystem) const;

		//uses the real filesystem
		template<class OPENER, class RETURNTYPE>
		std::vector<RETURNTYPE> getFiles(const OPENER& opener) const;

	private:
		std::unordered_set<std::string> _fileSpecsSet;
		std::vector<std::string> _fileSpecsBoost;
		NFD::UniquePathU8 _nfdFolder;
		NFD::UniquePathSet _nfdFiles;
		bool _recursiveCheck = true;
		std::unique_ptr<nfdnfilteritem_t> _fileFilter;
		std::vector<std::string> _wildcards;
	};


	template<class OPENER, class RETURNTYPE>
	inline std::vector<RETURNTYPE> FileSpecifierSet::getFiles(const OPENER& opener, const FileSystemWrapper* fileSystem) const
	{
		namespace fs = std::filesystem;

		std::vector<RETURNTYPE> fileList;

		std::queue<std::string> toCheck;

		for (const std::string& spec : _fileSpecsSet) {
			toCheck.push(spec);
		}

		while (toCheck.size()) {
			fs::path specPath{ toCheck.front() };
			toCheck.pop();

			//specified directories get searched recursively
			if (fileSystem->isDirectory(specPath)) {
				for (const std::filesystem::path& subpath : fileSystem->listDirectory(specPath)) {
					toCheck.push(subpath.string());
				}
				//because of the dumbass ESRI grid format, I have to try to open folders as rasters as well
				try {
					fileList.push_back(opener(specPath));
				}
				catch (...) {}
			}

			if (fileSystem->isRegularFile(specPath)) {
				//if it's a file, try to add it to the map
				try {
					fileList.push_back(opener(specPath));
				}
				catch (...) {}
			}

			//wildcard specifiers (e.g. C:\data\*.laz) are basically a non-recursive directory check with an extension
			if (specPath.has_filename()) {
				if (fileSystem->isDirectory(specPath.parent_path())) {
					std::regex wildcard{ "^\\*\\..+" };
					std::string ext = "";
					if (std::regex_match(specPath.filename().string(), wildcard)) {
						ext = specPath.extension().string();
					}

					if (ext.size()) {
						for (auto& subpath : fileSystem->listDirectory(specPath.parent_path())) {
							if (subpath.has_extension() && subpath.extension() == ext || ext == ".*") {
								toCheck.push(subpath.string());
							}
						}
					}
				}
			}
		}
		return fileList;
	}
	template<class OPENER, class RETURNTYPE>
	inline std::vector<RETURNTYPE> FileSpecifierSet::getFiles(const OPENER& opener) const
	{
        RealFileSystem fileSystem = RealFileSystem{};
        return getFiles<OPENER, RETURNTYPE>(opener, &fileSystem);
	}
}

#endif