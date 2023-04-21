#include"LapisLogger.hpp"
#include"../imgui/imgui.h"
#include"LapisWindows.hpp"
#include"..\utils\LapisFonts.hpp"

namespace lapis {
	LapisLogger& LapisLogger::getLogger()
	{
		static LapisLogger log;
		return log;
	}
	void LapisLogger::renderGui()
	{
		_updateEllipsis();
		ImGui::BeginChild("logger main progress", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f - 2), true, 0);

		ImGui::BeginChild("progress main field", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y), false, 0);
		for (int i = (int)_progressTracker.size() - 1; i >= 0; --i) {
			ImGui::Text(_progressTracker.at(i).c_str());
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("progress increment", ImGui::GetContentRegionAvail(), false, 0);
		for (int i = (int)_incrementStrings.size() - 1; i >= 0; --i) {
			ImGui::Text(_incrementStrings.at(i).c_str());
		}
		ImGui::EndChild();

		ImGui::EndChild();

		bool needSep = false;

		auto centerAlignTitle = [](const std::string& s) {
			ImGui::PushFont(LapisFonts::getBoldFont());

			float size = ImGui::CalcTextSize(s.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float avail = ImGui::GetContentRegionAvail().x;
			float offset = (avail - size) * 0.5f;
			if (offset > 0.0f) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
			}
			ImGui::Text(s.c_str());

			ImGui::PopFont();
		};

		ImGui::BeginChild("logger messages title",
			ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y),
			true, ImGuiWindowFlags_HorizontalScrollbar);

		centerAlignTitle("Messages");

		ImGui::BeginChild("logger messages",
			ImGui::GetContentRegionAvail(),
			false,
			0);

		ImGui::PushTextWrapPos();
		for (auto it = _messages.rbegin(); it != _messages.rend(); ++it) {
			if (needSep) {
				ImGui::Separator();
			}
			ImGui::Text(it->c_str());
			needSep = true;
		}
		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		ImGui::EndChild();


		needSep = false;
		ImGui::SameLine();
		ImGui::BeginChild("logger errors title",
			ImGui::GetContentRegionAvail(),
				true,
				ImGuiWindowFlags_HorizontalScrollbar);

		centerAlignTitle("Warnings and Errors");
		ImGui::BeginChild("logger errors",
			ImGui::GetContentRegionAvail(),
			false, 0);

		ImGui::PushTextWrapPos();
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		for (auto it = _warningsAndErrors.rbegin(); it != _warningsAndErrors.rend(); ++it) {
			if (needSep) {
				ImGui::Separator();
			}
			ImGui::Text(it->c_str());
			needSep = true;
		}
		ImGui::PopStyleColor();
		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		ImGui::EndChild();
	}
	void LapisLogger::setProgress(const std::string& displayString, int total, bool needEllipsis)
	{
		if (_progressTracker.size()) {
			size_t previdx = _progressTracker.size() - 1;
			_progressTracker[previdx] = _lastProgressNoEllipsis + "...";
			_incrementStrings[previdx] = "Done!";
		}

		_needEllipsis = needEllipsis;
		_lastProgressNoEllipsis = displayString;

		lapisCout << displayString << "\n";
		_totalForTask = total;
		_progressTracker.push_back(displayString);
		_currentOutOfTotal = 0;

		_incrementStrings.push_back("");
		if (total > 0) {
			_updateIncStr();
		}
	}
	void LapisLogger::incrementTask(const std::string& message)
	{
		std::scoped_lock(_mut);
		_currentOutOfTotal++;
		if (message.size()) {
			lapisCout << message << " " << _currentOutOfTotal << "/" << _totalForTask << "\n";
		}
		if (_totalForTask > 0) {
			_updateIncStr();
		}
	}
	void LapisLogger::logMessage(const std::string& s)
	{
		std::scoped_lock(_mut);
		lapisCout << s << "\n";
		_messages.push_back(s);
		if (_messages.size() > 100000) {
			_messages.pop_front();
		}
	}
	void LapisLogger::logWarningOrError(const std::string& s) {
		std::scoped_lock(_mut);
		lapisCerr << s << "\n";
		_warningsAndErrors.push_back(s);
		if (_warningsAndErrors.size() > 100000) {
			_warningsAndErrors.pop_front();
		}
	}
	void LapisLogger::reset()
	{
		_progressTracker.clear();
		_incrementStrings.clear();
		_messages.clear();
	}
	void LapisLogger::displayBenchmarking()
	{
		_benchmark = true;
	}
	void LapisLogger::stopBenchmarking()
	{
		_benchmark = false;
	}
	void LapisLogger::beginBenchmarkTimer(const std::string& what)
	{
		if (!_benchmark) {
			return;
		}
		_benchmarkTimers[std::this_thread::get_id()][what] = std::chrono::high_resolution_clock::now();
	}
	void LapisLogger::endBenchmarkTimer(const std::string& what)
	{
		if (!_benchmark) {
			return;
		}
		auto id = std::this_thread::get_id();
		if (!_benchmarkTimers.contains(id)) {
			return;
		}
		if (!_benchmarkTimers[id].contains(what)) {
			return;
		}
		namespace chr = std::chrono;
		auto duration = chr::duration_cast<chr::milliseconds>(chr::high_resolution_clock::now() - _benchmarkTimers[id][what]);
		if (duration.count() <= 1) {
			return;
		}
		std::stringstream ss;
		ss << what << " " << duration.count();
		logMessage(ss.str());

		_benchmarkTimers[id].erase(what);
	}
	LapisLogger::LapisLogger()
	{
	}
	void LapisLogger::_updateEllipsis()
	{
		_frameCounter = (_frameCounter + 1) % 60;
		if (!_needEllipsis) {
			return;
		}
		size_t idx = _progressTracker.size() - 1;
		std::string ellipsis;
		if (_frameCounter < 20) {
			ellipsis = ".";
		}
		else  if (_frameCounter < 40) {
			ellipsis = "..";
		}
		else {
			ellipsis = "...";
		}
		_progressTracker[idx] = _lastProgressNoEllipsis + ellipsis;
	}
	void LapisLogger::_updateIncStr()
	{
		size_t idx = _incrementStrings.size() - 1;
		std::string str = std::to_string(_currentOutOfTotal) + "/" + std::to_string(_totalForTask);
		_incrementStrings[idx] = str;
	}
}