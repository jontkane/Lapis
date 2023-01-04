#pragma once
#ifndef LP_LAPISCONTROLLER_H
#define LP_LAPISCONTROLLER_H

#include"..\logger\lapislogger.hpp"
#include"ProductHandler.hpp"

namespace lapis {
	
	class LapisController {
	public:

		LapisController();

		void processFullArea();

		bool isRunning() const;

		void sendAbortSignal();

		static size_t registerHandler(ProductHandler* handler);
		template<class HANDLER>
		static void replaceHandlerWithMod(HANDLER* handler) {
			_handlers[HANDLER::handlerRegisteredIndex].reset(handler);
		}

	protected:
		mutable std::atomic_bool _isRunning = false;

		void lasThread(size_t n) const;
		void tileThread(cell_t tile) const;
		void cleanUp() const;

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

		static std::vector<std::unique_ptr<ProductHandler>> _handlers;
	};
}

#endif