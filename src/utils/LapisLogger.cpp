#include"LapisLogger.hpp"
#include"../imgui/imgui.h"
#include"LapisWindows.hpp"
#include"..\utils\LapisFonts.hpp"

namespace lapis {

	void centerAlignTitle(const std::string& s) {
		ImGui::PushFont(LapisFonts::getBoldFont());

		float size = ImGui::CalcTextSize(s.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float avail = ImGui::GetContentRegionAvail().x;
		float offset = (avail - size) * 0.5f;
		if (offset > 0.0f) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
		}
		ImGui::Text(s.c_str());

		ImGui::PopFont();
	}
	void renderDuration(std::chrono::duration<long long, std::nano> duration) {
		namespace chr = std::chrono;
		std::stringstream ss;
		ss << chr::duration_cast<chr::hours>(duration).count();
		ss << ":";

		auto minutes = chr::duration_cast<chr::minutes>(duration).count() % 60;
		auto seconds = chr::duration_cast<chr::seconds>(duration).count() % 60;

		auto insertZeroes = [](auto x) {
			if (x < 10) {
				return "0" + std::to_string(x);
			}
			return std::to_string(x);
		};

		ss << insertZeroes(minutes);
		ss << ":";
		ss << insertZeroes(seconds);

		ImGui::Text(ss.str().c_str());
	}

	LapisLogger& LapisLogger::getLogger()
	{
		static LapisLogger log;
		return log;
	}
	void LapisLogger::renderGui()
	{
		_updateEllipsis();
		ImGui::BeginChild("logger main progress", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f), true, 0);
		centerAlignTitle("Progress");

		/*
		if (_verboseTimers.size()) {
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 150);
			if (ImGui::Button("Detailed Runtime")) {
				_displayVerboseBenchmark = true;
			}
		}
		*/

		for (int i = (int)_progressTracker.size() - 1; i >= 0; --i) {

			const std::string& name = _progressTracker.at(i);

			ImGui::Text(name.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.3f);
			ImGui::Text(_incrementStrings.at(i).c_str());

			if (!_mainProgressTimers[i].has_value()) {
				continue;
			}

			BenchmarkInfo& bench = _mainProgressTimers[i].value();

			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f);
			renderDuration(bench.timeSinceStart());

			if (i == _progressTracker.size() - 1 && _totalForTask) {
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.7f);
				ImGui::Text("Est. Time Left (This Step): ");
				ImGui::SameLine();
				auto meanTime = bench.meanDuration();
				if (!meanTime.has_value()) {
					ImGui::Text("???");
				}
				else {
					Duration estimate = (meanTime.value() * _totalForTask) / _nThread - bench.timeSinceStart();
					if (estimate.count() < 0) {
						if (_totalForTask - _currentOutOfTotal < _nThread) {
							ImGui::Text("Soon");
						}
						else {
							ImGui::Text("???");
						}
					}
					else {
						renderDuration(estimate);
					}
				}
			}
		}
		ImGui::EndChild();

		_renderMessages();

		if (_displayVerboseBenchmark) {
			_renderVerboseBenchmarkWindow();
		}
	}
	void LapisLogger::setProgress(const std::string& displayString, int total, bool needTimer)
	{
		if (_progressTracker.size()) {
			size_t previdx = _progressTracker.size() - 1;
			_progressTracker[previdx] = _lastProgressNoEllipsis + "...";
			_incrementStrings[previdx] = "Done!";
		}

		_needTimer = needTimer;
		_lastProgressNoEllipsis = displayString;

		lapisCout << displayString << "\n";
		_totalForTask = total;
		_progressTracker.push_back(displayString);
		_currentOutOfTotal = 0;

		_incrementStrings.push_back("");
		if (total > 0) {
			_updateIncStr();
		}

		if (_mainProgressTimers.size()) {
			_mainProgressTimers.back().value().stopTimer();
		}
		if (needTimer) {
			_mainProgressTimers.emplace_back(std::in_place);
		}
		else {
			_mainProgressTimers.emplace_back();
		}
	}
	void LapisLogger::incrementTask(const std::string& message)
	{
		std::scoped_lock lock{ *_mut };
		_currentOutOfTotal++;
		if (message.size()) {
			lapisCout << message << " " << _currentOutOfTotal << "/" << _totalForTask << "\n";
		}
		if (_totalForTask > 0) {
			_updateIncStr();
		}
		if (!_mainProgressTimers.back().has_value()) {
			return;
		}
		_mainProgressTimers.back().value().endOnThisThread();
		_mainProgressTimers.back().value().beginOnThisThread();
	}
	void LapisLogger::logMessage(const std::string& s)
	{
		std::scoped_lock lock{ *_mut };
		lapisCout << s << "\n";
		_messages.emplace_back(s, Message::Type::Message);
		if (_messages.size() > 100000) {
			_messages.pop_front();
		}
	}
	void LapisLogger::logWarning(const std::string& s) {
		std::scoped_lock lock{ *_mut };
		lapisCerr << s << "\n";
		_messages.emplace_back(s, Message::Type::Warning);

	}
	void LapisLogger::logError(const std::string& s) {
		std::scoped_lock lock{ *_mut };
		lapisCerr << s << "\n";
		_messages.emplace_back(s, Message::Type::Error);
	}
	void LapisLogger::reset()
	{
		*this = LapisLogger();
	}
	void LapisLogger::beginVerboseBenchmarkTimer(const std::string& what)
	{
		_verboseTimers[what].beginOnThisThread();
	}
	void LapisLogger::pauseVerboseBenchmarkTimer(const std::string& what)
	{
		_verboseTimers[what].pauseOnThisThread();
	}
	void LapisLogger::endVerboseBenchmarkTimer(const std::string& what)
	{
		_verboseTimers[what].endOnThisThread();
	}
	void LapisLogger::turnOnVerboseBenchmarking()
	{
		_displayVerboseBenchmark = true;
	}
	void LapisLogger::turnOffVerboseBenchmarking()
	{
		_displayVerboseBenchmark = false;
	}

	void LapisLogger::setNThread(int nThread)
	{
		_nThread = nThread;
	}

	LapisLogger::LapisLogger() : _mut(std::make_unique<std::mutex>())
	{
	}
	void LapisLogger::_updateEllipsis()
	{
		_frameCounter = (_frameCounter + 1) % 60;
		if (!_needTimer) {
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
	void LapisLogger::_renderMessages()
	{

		bool needSep = false;

		ImGui::BeginChild("logger messages",
			ImGui::GetContentRegionAvail(),
			true, ImGuiWindowFlags_HorizontalScrollbar);

		centerAlignTitle("Messages, Warnings, and Errors");

		ImGui::PushTextWrapPos();
		for (auto it = _messages.rbegin(); it != _messages.rend(); ++it) {
			if (needSep) {
				ImGui::Separator();
			}
			it->render();
			needSep = true;
		}
		ImGui::PopTextWrapPos();
		ImGui::EndChild();
	}
	void LapisLogger::_updateIncStr()
	{
		size_t idx = _incrementStrings.size() - 1;
		std::string str = std::to_string(_currentOutOfTotal) + "/" + std::to_string(_totalForTask);
		_incrementStrings[idx] = str;
	}
	void LapisLogger::_renderVerboseBenchmarkWindow()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
		if (!_displayVerboseBenchmark) {
			return;
		}

		if (!ImGui::Begin("Verbose Benchmarking", &_displayVerboseBenchmark, flags)) {
			ImGui::End();
			return;
		}

		for (auto& b : _verboseTimers) {
			ImGui::Text(b.first.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(300);
			renderDuration(b.second.meanDuration().value_or(Duration{ 0 }));
		}

		if (ImGui::Button("OK")) {
			_displayVerboseBenchmark = false;
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(300);
		ImGui::Text("Average");

		ImGui::End();
	}
	inline LapisLogger::BenchmarkInfo::BenchmarkInfo() : startTime(std::chrono::high_resolution_clock::now()), endTime(), isRunning(true) {}
	void LapisLogger::BenchmarkInfo::stopTimer()
	{
		isRunning = false;
		endTime = std::chrono::high_resolution_clock::now();
	}
	LapisLogger::Duration LapisLogger::BenchmarkInfo::timeSinceStart()
	{
		if (isRunning) {
			endTime = std::chrono::high_resolution_clock::now();
		}
		return endTime - startTime;
	}
	std::optional<LapisLogger::Duration> LapisLogger::BenchmarkInfo::meanDuration()
	{
		if (!finishedTimers.size()) {
			return std::optional<Duration>();
		}
		Duration sum{ 0 };
		for (Duration d : finishedTimers) {
			sum += d;
		}
		return sum / finishedTimers.size();
	}
	void LapisLogger::BenchmarkInfo::beginOnThisThread()
	{
		auto thread = std::this_thread::get_id();
		if (activeTimers.contains(thread)) {
			//paused
			BenchmarkTimer& bench = activeTimers[thread];
			bench.lastStarted = std::chrono::high_resolution_clock::now();
			bench.currentlyRunning = true;
		}
		else {
			//not yet started
			activeTimers.try_emplace(thread);
		}
	}
	void LapisLogger::BenchmarkInfo::endOnThisThread()
	{
		auto thread = std::this_thread::get_id();
		
		if (!activeTimers.contains(thread)) {
			//this branch happens for the initial batch of increments, which will all have a start time the same as the global start
			activeTimers[thread] = BenchmarkTimer(startTime);
		}

		BenchmarkTimer& bench = activeTimers[thread];
		auto totalDuration = bench.beforeLastPause;
		if (bench.currentlyRunning) {
			totalDuration += std::chrono::high_resolution_clock::now() - bench.lastStarted;
		}
		finishedTimers.push_back(totalDuration);

		activeTimers.erase(thread);
	}
	void LapisLogger::BenchmarkInfo::pauseOnThisThread()
	{
		auto thread = std::this_thread::get_id();
		if (!activeTimers.contains(thread)) {
			//this branch happens for the initial batch of increments, which will all have a start time the same as the global start
			activeTimers[thread] = BenchmarkTimer(startTime);
		}
		BenchmarkTimer& bench = activeTimers[thread];
		auto now = std::chrono::high_resolution_clock::now();
		bench.beforeLastPause += now - bench.lastStarted;
		bench.currentlyRunning = false;
	}
	void LapisLogger::Message::render()
	{
		switch (type) {
		case Type::Message:
			ImGui::Text(msg.c_str());
			break;
		case Type::Warning:
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::Text("Warning! ");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text(msg.c_str());
			break;
		case Type::Error:
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::Text("Error! ");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text(msg.c_str());
			break;
		}
	}
}