#pragma once
#ifndef lp_pointmetriccontroller_h
#define lp_pointmetriccontroller_h


#include"LapisLogger.hpp"
#include"LapisData.hpp"
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

	protected:
		mutable std::atomic_bool _isRunning = false;

		using HandlerVector = std::vector<std::unique_ptr<ProductHandler>>;
		void lasThread(HandlerVector& handlers, size_t n) const;
		void tileThread(HandlerVector& handlers, cell_t tile) const;
		void cleanUp(HandlerVector& handlers) const;

		void writeLayout() const;
		void writeParams() const;
	};
}

#endif