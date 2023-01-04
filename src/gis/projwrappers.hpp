#pragma once
#ifndef lapis_projwrappers_h
#define lapid_projwrappers_h

#include"gis_pch.hpp"
#include"..\LapisTypeDefs.hpp"


namespace lapis {

	class ProjCtxWrapper {
	public:

		PJ_CONTEXT* ptr;

		ProjCtxWrapper(nullptr_t) : ptr(nullptr) {}
		ProjCtxWrapper() : ptr(proj_context_create()) {}
		~ProjCtxWrapper() {
			if (ptr != nullptr)
				proj_context_destroy(ptr);
		}
		ProjCtxWrapper(const ProjCtxWrapper& other) = delete;
	};

	class ProjContextByThread {
	public:
		static PJ_CONTEXT* get() {
			std::thread::id thisthread = std::this_thread::get_id();
			_ctxs.try_emplace(thisthread);
			return _ctxs[thisthread].ptr;
		}
	private:
		inline static std::unordered_map<std::thread::id, ProjCtxWrapper> _ctxs;
	};

	class ProjPJWrapper {
	public:
		
		ProjPJWrapper() : _ptr(nullptr) {}

		//constructs the PJ using proj_create
		ProjPJWrapper(const std::string& s) {
			PJ* p = proj_create(ProjContextByThread::get(), s.c_str());
			_ptr = std::shared_ptr<PJ>(p, [](auto x) {
				proj_destroy(x); });
		}

		//if you create a PJ using some other function, you can pass the pointer here and get the wrapper you need
		ProjPJWrapper(PJ* p) : _ptr(p, [](auto x) {proj_destroy(x); }) {}

		PJ* ptr() {
			return _ptr.get();
		}
		const PJ* ptr() const {
			return _ptr.get();
		}

	private:
		std::shared_ptr<PJ> _ptr;
	};

	class wrapperPJIntList {
	public:
		int* ptr;
		wrapperPJIntList() : ptr(nullptr) {}
		~wrapperPJIntList() {
			proj_int_list_destroy(ptr);
		}

		wrapperPJIntList(const wrapperPJIntList& other) = delete;
		wrapperPJIntList& operator=(const wrapperPJIntList& other) = delete;
	};

	class ProjPJObjListWrapper {
	public:
		PJ_OBJ_LIST* ptr;

		ProjPJObjListWrapper() : ptr(nullptr) {}
		~ProjPJObjListWrapper() {
			proj_list_destroy(ptr);
		}

		ProjPJObjListWrapper(const ProjPJWrapper& p, wrapperPJIntList& conf, const std::string& auth = "EPSG") { //runs proj_identify
			ptr = proj_identify(ProjContextByThread::get(), p.ptr(), auth.c_str(), nullptr, &(conf.ptr));
		}

		ProjPJObjListWrapper(const ProjPJObjListWrapper& other) = delete;
		ProjPJObjListWrapper& operator=(const ProjPJObjListWrapper& other) = delete;
	};
}

#endif