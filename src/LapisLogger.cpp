#include"app_pch.hpp"
#include"LapisLogger.hpp"
#include"imgui/imgui.h"

namespace lapis {
	LapisLogger& LapisLogger::getLogger()
	{
		static LapisLogger log;
		return log;
	}
	void LapisLogger::renderGui()
	{
		_updateEllipsis();
		ImGui::BeginChild("logger main progress", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, ImGui::GetContentRegionAvail().y), true, 0);

		ImGui::BeginChild("progress main field", ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, ImGui::GetContentRegionAvail().y), false, 0);
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

		ImGui::SameLine();
		ImGui::BeginChild("logger messages", ImVec2(ImGui::GetContentRegionAvail().x - 2, ImGui::GetContentRegionAvail().y), true, 0);
		for (size_t i = 0; i < _messages.size(); ++i) {
			ImGui::Text(_messages[i].c_str());
		}
		ImGui::EndChild();
	}
	void LapisLogger::setProgress(RunProgress p, int total)
	{
		if (_progressTracker.size()) {
			size_t previdx = _progressTracker.size() - 1;
			_progressTracker[previdx] = taskMessage.at(_currentTask) + "...";
			_incrementStrings[previdx] = "Done!";
		}

		std::cout << taskMessage[p] << "\n";
		_totalForTask = total;
		_progressTracker.push_back(taskMessage.at(p));
		_currentOutOfTotal = 0;
		_currentTask = p;

		_incrementStrings.push_back("");
		if (total > 0) {
			_updateIncStr();
		}
	}
	void LapisLogger::incrementTask()
	{
		std::scoped_lock(_mut);
		_currentOutOfTotal++;
		if (incrementMessage.contains(_currentTask)) {
			std::cout << incrementMessage[_currentTask] << _currentOutOfTotal << "/" << _totalForTask;
		}
		_updateIncStr();
	}
	void LapisLogger::logMessage(const std::string& s)
	{
		std::scoped_lock(_mut);
		std::cout << s << "\n";
		_messages.push_back(s);
	}
	void LapisLogger::reset()
	{
		_progressTracker.clear();
		_messages.clear();
		_currentTask = RunProgress::notStarted;
	}
	LapisLogger::LapisLogger()
	{
	}
	void LapisLogger::_updateEllipsis()
	{
		_frameCounter = (_frameCounter + 1) % 60;
		if (_currentTask == RunProgress::notStarted) {
			return;
		}
		if (_currentTask == RunProgress::finished) {
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
		_progressTracker[idx] = taskMessage.at(_currentTask) + ellipsis;
	}
	void LapisLogger::_updateIncStr()
	{
		size_t idx = _incrementStrings.size() - 1;
		std::string str = std::to_string(_currentOutOfTotal) + "/" + std::to_string(_totalForTask);
		_incrementStrings[idx] = str;
	}
}