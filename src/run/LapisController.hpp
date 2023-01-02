#pragma once
#ifndef LP_LAPISCONTROLLER_H
#define LP_LAPISCONTROLLER_H


#include"..\logger\lapislogger.hpp"
#include"ProductHandler.hpp"

namespace std {
	namespace filesystem {
		class path;
	}
}

namespace lapis {

	namespace fs = std::filesystem;
	
	class LapisController {
	public:

		LapisController();

		void processFullArea();

		bool isRunning() const;

		void sendAbortSignal();

	protected:
		mutable std::atomic_bool _isRunning = false;

		using HandlerVector = std::vector<std::unique_ptr<ProductHandler>>;
		void lasThread(HandlerVector& handlers, size_t n) const;
		void tileThread(HandlerVector& handlers, cell_t tile) const;
		void cleanUp(HandlerVector& handlers) const;

		void writeLayout() const;
		void writeParams() const;

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
				func(thisidx);
			}
		}
	};
}

#endif