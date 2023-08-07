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

		void logWarning(const std::string& s);
		void logError(const std::string& s);

		void reset();

		void beginVerboseBenchmarkTimer(const std::string& what);
		void pauseVerboseBenchmarkTimer(const std::string& what);
		void endVerboseBenchmarkTimer(const std::string& what);

		void turnOnVerboseBenchmarking();
		void turnOffVerboseBenchmarking();

		void setNThread(int nThread);

	private:
		LapisLogger();

		LapisLogger(const LapisLogger& other) = delete;
		LapisLogger& operator=(const LapisLogger& other) = delete;
		LapisLogger(LapisLogger&& other) = default;
		LapisLogger& operator=(LapisLogger&& other) = default;

		void _updateEllipsis();

		void _renderMessages();

		std::vector<std::string> _progressTracker;
		std::vector<std::string> _incrementStrings;

		int _totalForTask = 0;
		int _currentOutOfTotal = 0;

		int _frameCounter = 0;

		bool _needTimer = false;
		std::string _lastProgressNoEllipsis;

		mutable std::unique_ptr<std::mutex> _mut;

		void _updateIncStr();

		bool _displayVerboseBenchmark = false;

		int _nThread = 1;

		struct Message {

			enum class Type {
				Message, Warning, Error
			};

			std::string msg;
			Type type;

			Message(const std::string& msg, Type type) : msg(msg), type(type) {}
			void render();
		};
		std::list<Message> _messages;

		using Time = std::chrono::steady_clock::time_point;
		using Duration = std::chrono::duration<long long, std::nano>;
		struct BenchmarkTimer {
			Duration beforeLastPause;
			Time lastStarted;
			bool currentlyRunning;
			BenchmarkTimer() : beforeLastPause(0), lastStarted(std::chrono::high_resolution_clock::now()), currentlyRunning(true) {}
			BenchmarkTimer(Time startTime) : beforeLastPause(0), lastStarted(startTime), currentlyRunning(true) {}
		};
		class BenchmarkInfo {
		public:
			BenchmarkInfo();
			void stopTimer();
			Duration timeSinceStart();
			std::optional<Duration> meanDuration();

			void beginOnThisThread();
			void endOnThisThread();
			void pauseOnThisThread();

		private:
			std::vector<Duration> finishedTimers;
			std::unordered_map<std::thread::id, BenchmarkTimer> activeTimers;
			Time startTime;
			Time endTime;
			bool isRunning;
		};

		std::vector<std::optional<BenchmarkInfo>> _mainProgressTimers;
		std::unordered_map<std::string, BenchmarkInfo> _verboseTimers;

		void _renderVerboseBenchmarkWindow();
	};
}

#endif