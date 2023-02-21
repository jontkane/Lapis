#include"LapisLogger.hpp"
#include"../imgui/imgui.h"
#include<iostream>

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

		ImGui::BeginChild("logger messages", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 2), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (auto it = _messages.rbegin(); it != _messages.rend(); ++it) {
			ImGui::Text(it->c_str());
		}
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

		std::cout << displayString << "\n";
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
			std::cout << message << " " << _currentOutOfTotal << "/" << _totalForTask << "\n";
		}
		if (_totalForTask > 0) {
			_updateIncStr();
		}
	}
	void LapisLogger::logMessage(const std::string& s)
	{
		std::scoped_lock(_mut);
		std::cout << s << "\n";
		_messages.push_back(s);
		if (_messages.size() > 100000) {
			_messages.pop_front();
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