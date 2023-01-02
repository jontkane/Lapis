#include"param_pch.hpp"
#include"GuiFileSpecifier.hpp"

namespace lapis {
	FileSpecifierSet::FileSpecifierSet(const std::string& guiDesc, const std::string& cmdName,
		const std::string& cmdDescription,
		const std::vector<std::string>& wildcards, std::unique_ptr<nfdnfilteritem_t>&& fileFilter)
		: GuiCmdElement(guiDesc, cmdName, cmdDescription), _fileFilter(std::move(fileFilter)), _wildcards(wildcards)
	{
	}
	void FileSpecifierSet::addToCmd(BoostOptDesc& visible,
		BoostOptDesc& hidden)
	{
		addToCmdBase(visible, hidden, &_fileSpecsBoost);
	}
	std::ostream& FileSpecifierSet::printToIni(std::ostream& o) const
	{
		for (const std::string& s : _fileSpecsSet) {
			o << _cmdName << "=" << s << "\n";
		}

		return o;
	}
	bool FileSpecifierSet::renderGui() {

		bool changed = false;

		std::string buttonText = "Add " + _guiDesc + " Files";
		auto filterptr = _fileFilter ? _fileFilter.get() : nullptr;
		int filtercnt = _fileFilter ? 1 : 0;
		if (ImGui::Button(buttonText.c_str())) {
			NFD::OpenDialogMultiple(_nfdFiles, filterptr, filtercnt, (const nfdnchar_t*)nullptr);
		}
		nfdpathsetsize_t selectedFileCount = 0;
		if (_nfdFiles) {
			NFD::PathSet::Count(_nfdFiles, selectedFileCount);
			for (nfdpathsetsize_t i = 0; i < selectedFileCount; ++i) {
				NFD::UniquePathSetPathU8 path;
				NFD::PathSet::GetPath(_nfdFiles, i, path);
				_fileSpecsSet.insert(path.get());
			}
			_nfdFiles.reset();
			changed = true;
		}

		ImGui::SameLine();
		buttonText = "Add " + _guiDesc + " Folder";
		if (ImGui::Button(buttonText.c_str())) {
			NFD::PickFolder(_nfdFolder);
		}
		if (_nfdFolder) {
			if (_recursiveCheck) {
				_fileSpecsSet.insert(_nfdFolder.get());
			}
			else {
				for (const std::string& s : _wildcards) {
					_fileSpecsSet.insert(_nfdFolder.get() + std::string("\\") + s);
				}
			}
			_nfdFolder.reset();
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove All")) {
			_fileSpecsSet.clear();
		}

		ImGui::Checkbox("Add subfolders", &_recursiveCheck);
		displayHelp();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		std::string childname = "##" + _cmdName;
		ImGui::BeginChild(childname.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, window_flags);
		for (const std::string& s : _fileSpecsSet) {
			ImGui::Text(s.c_str());
		}
		ImGui::EndChild();

		return changed;
	}
	bool FileSpecifierSet::importFromBoost()
	{
		bool changed = _fileSpecsBoost.size();
		for (const std::string& s : _fileSpecsBoost) {
			_fileSpecsSet.insert(s);
		}
		_fileSpecsBoost.clear();
		return changed;
	}
	const std::set<std::string>& FileSpecifierSet::getSpecifiers() const
	{
		return _fileSpecsSet;
	}
}