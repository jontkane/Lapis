#pragma once
#ifndef LP_LAPISLOGGER_H
#define LP_LAPISLOGGER_H

#include<string>
#include<unordered_map>
#include<vector>
#include<mutex>
#include<chrono>
#include<iostream>
#include<fstream>

namespace lapis {


	class LapisLogger {
	public:
		static LapisLogger& getLogger();

		void renderGui();

		void setProgress(const std::string& displayString, int total = 0, bool needEllipsis = true);
		void incrementTask(const std::string& message = "");

		void logMessage(const std::string& s);

		void logWarningOrError(const std::string& s);

		void reset();

		void displayBenchmarking();
		void stopBenchmarking();
		void beginBenchmarkTimer(const std::string& what);
		void endBenchmarkTimer(const std::string& what);

	private:
		LapisLogger();

		void _updateEllipsis();

		std::vector<std::string> _progressTracker;
		std::vector<std::string> _incrementStrings;
		std::list<std::string> _messages;
		std::list<std::string> _warningsAndErrors;

		int _totalForTask = 0;
		int _currentOutOfTotal = 0;

		char* _endOfNumber = nullptr;

		int _frameCounter = 0;

		bool _needEllipsis = false;
		std::string _lastProgressNoEllipsis;

		mutable std::mutex _mut;

		void _updateIncStr();

		bool _benchmark = false;

		using Time = decltype(std::chrono::high_resolution_clock::now());
		std::unordered_map<std::thread::id, std::unordered_map<std::string, Time>> _benchmarkTimers;
	};
}

#endif