#include"app_pch.hpp"
#include"LapisUtils.hpp"

namespace fs = std::filesystem;

namespace lapis {


	bool operator<(const DemFileAlignment& a, const DemFileAlignment& b) {
		return a.filename < b.filename;
	}

	bool operator<(const LasFileExtent& a, const LasFileExtent& b) {
		if (a.filename == b.filename) { return false; }
		if (a.ext.ymax() > b.ext.ymax()) { return true; }
		if (a.ext.ymax() < b.ext.ymax()) { return false; }
		if (a.ext.ymin() > b.ext.ymin()) { return true; }
		if (a.ext.ymin() < b.ext.ymin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmin() < b.ext.xmin()) { return false; }
		if (a.ext.xmax() > b.ext.xmax()) { return true; }
		if (a.ext.xmax() < b.ext.xmax()) { return false; }
		return a.filename < b.filename;
	}

	bool extentSorter(const Extent& lhs, const Extent& rhs) {
		if (lhs.ymax() != rhs.ymax()) {
			return lhs.ymax() < rhs.ymax();
		}
		if (lhs.xmax() != rhs.xmax()) {
			return lhs.xmax() < rhs.xmax();
		}
		if (lhs.ymin() != rhs.ymin()) {
			return lhs.ymin() < rhs.ymin();
		}
		return lhs.xmin() < rhs.ymin();
	}

	//returns a useful default for the number of concurrent threads to run
	unsigned int defaultNThread() {
		unsigned int out = std::thread::hardware_concurrency();
		return out > 1 ? out - 1 : 1;
	}

	//copied from the ImGui demo
	void ImGuiHelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
}