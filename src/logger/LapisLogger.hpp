#pragma once
#ifndef LP_LAPISLOGGER_H
#define LP_LAPISLOGGER_H

#include<string>
#include<unordered_map>
#include<vector>
#include<mutex>

namespace lapis {


	class LapisLogger {
	public:
		static LapisLogger& getLogger();

		void renderGui();

		void setProgress(const std::string& displayString, int total = 0, bool needEllipsis = true);
		void incrementTask(const std::string& message = "");

		void logMessage(const std::string& s);

		void reset();
	private:
		LapisLogger();

		void _updateEllipsis();

		constexpr static size_t messageLength = 30;
		std::vector<std::string> _progressTracker;
		constexpr static size_t incrementStrLength = 15;
		std::vector<std::string> _incrementStrings;
		std::vector<std::string> _messages;

		int _totalForTask = 0;
		int _currentOutOfTotal = 0;

		char* _endOfNumber = nullptr;

		int _frameCounter = 0;

		bool _needEllipsis = false;
		std::string _lastProgressNoEllipsis;

		mutable std::mutex _mut;

		void _updateIncStr();
	};
}

#endif