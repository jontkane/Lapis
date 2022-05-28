#include"lapis_pch.hpp"
#include"Logger.hpp"

namespace lapis {
	Logger::Logger() : output(&std::cout), level(diagnostic), mut() {}

	void Logger::logWithLevel(const std::string& out, Level l) const {
		std::lock_guard lock{mut};
		if (level >= l) {
			*output << out << "\n";
		}
	}

	void Logger::logError(const std::string& out) const {
		logWithLevel(out, errors);
	}

	void Logger::logProgress(const std::string& out) const {
		logWithLevel(out, progress);
	}

	void Logger::logWarning(const std::string& out) const {
		logWithLevel(out, warnings);
	}

	void Logger::logDiagnostic(const std::string& out) const {
		logWithLevel(out, diagnostic);
	}

}