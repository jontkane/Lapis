#pragma once
#ifndef LP_LAPISCONTROLLER_H
#define LP_LAPISCONTROLLER_H

#include"..\utils\lapislogger.hpp"
#include"ProductHandler.hpp"

namespace lapis {
	
	class LapisController {
	public:

		LapisController();

		bool processFullArea();

		bool isRunning() const;

		void sendAbortSignal();

	protected:
		mutable std::atomic_bool _isRunning = false;

		const void lasThread(size_t n);
		const void tileThread(cell_t tile);
		void cleanUp();

		void writeLayout() const;
		void writeParams() const;
		void writeMetadata() const;

	private:
		
		bool _needAbort = false;

		template<typename WORKERFUNC>
		void _distributeWork(uint64_t& sofar, uint64_t max, const WORKERFUNC& func, std::mutex& mut) {
			while (true) {
				cell_t thisidx;
				{
					std::lock_guard lock(mut);
					if (sofar >= max) {
						break;
					}
					thisidx = sofar;
					++sofar;
				}
#if LAPIS_HANDLE_ERRORS
				try {
#endif
					func(thisidx);
#if LAPIS_HANDLE_ERRORS
				}
				catch (std::exception e) {
					LapisLogger& log = LapisLogger::getLogger();
					log.logError("Fatal error: " + std::string(e.what()));
					log.logMessage("Please contact the developer at lapis-lidar@uw.edu for advice or to report this bug.");
					_needAbort = true;
					return;
				}
#endif
			}
		}
	};
}

#endif