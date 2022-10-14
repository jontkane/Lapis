#pragma once
#ifndef LP_LAPISLOGGER_H
#define LP_LAPISLOGGER_H

#include<unordered_map>
#include<mutex>

namespace lapis {

	enum class RunProgress {
		notStarted,
		findingLasFiles,
		findingDemFiles,
		settingUp,
		lasFiles,
		writeMetrics,
		csmTiles,
		writeCsmMetrics,
		cleanUp,
		finished
	};


	class LapisLogger {
	public:
		static LapisLogger& getLogger();

		void renderGui();

		void setProgress(RunProgress p, int total = 0);
		void incrementTask();

		void logMessage(const std::string& s);

		void reset();
	private:
		LapisLogger();

		void _updateEllipsis();

		inline static std::unordered_map<RunProgress, std::string> taskMessage = {
			{RunProgress::notStarted,""},
			{RunProgress::findingLasFiles,"Reading Las Files"},
			{RunProgress::findingDemFiles,"Reading Dem Files"},
			{RunProgress::settingUp,"Preparing Run"},
			{RunProgress::lasFiles,"Processing Las Files"},
			{RunProgress::writeMetrics,"Writing Point Metrics"},
			{RunProgress::csmTiles,"Processing CSM Tiles"},
			{RunProgress::writeCsmMetrics,"Writing CSM Metrics"},
			{RunProgress::cleanUp,"Final Cleanup"},
			{RunProgress::finished,"Done!"}
		};
		inline static std::unordered_map<RunProgress, std::string> incrementMessage = {
			{RunProgress::lasFiles,"Las File Finished: "},
			{RunProgress::csmTiles, "CSM Tile Processed: "},
		};

		constexpr static size_t messageLength = 30;
		std::vector<std::string> _progressTracker;
		constexpr static size_t incrementStrLength = 15;
		std::vector<std::string> _incrementStrings;
		std::vector<std::string> _messages;

		RunProgress _currentTask = RunProgress::notStarted;
		int _totalForTask = 0;
		int _currentOutOfTotal = 0;

		char* _endOfNumber = nullptr;

		int _frameCounter = 0;

		mutable std::mutex _mut;

		void _updateIncStr();
	};
}

#endif