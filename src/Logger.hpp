#pragma once
#ifndef lp_logger_h
#define lp_logger_h

#include"app_pch.hpp"

namespace lapis {
	
	class Logger {
	public:

		enum Level {
			nothing,
			errors,
			progress,
			warnings,
			diagnostic
		};

		static Logger& getLogger();

		void logWithLevel(const std::string& out, Level l) const;

		void logError(const std::string& out) const;

		void logProgress(const std::string& out) const;

		void logWarning(const std::string& out) const;

		void logDiagnostic(const std::string& out) const;

	private:
		std::ostream* output;
		Level level;
		mutable std::mutex mut;

		Logger();
	};
}

#endif