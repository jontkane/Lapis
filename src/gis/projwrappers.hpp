#pragma once
#ifndef lapis_projwrappers_h
#define lapid_projwrappers_h

#include"gis_pch.hpp"
#include"..\LapisTypeDefs.hpp"


namespace lapis {

	using SharedPJ = std::shared_ptr<PJ>;
	SharedPJ makeSharedPJ(PJ* pj);

	using SharedPJCtx = std::shared_ptr<PJ_CONTEXT>;
	SharedPJCtx getNewPJContext();

	class ProjContextByThread {
	private:
		inline static std::unordered_map<std::thread::id, SharedPJCtx> _ctxs;
	public:
		static PJ_CONTEXT* get();
	};

	SharedPJ projCreateWrapper(const std::string& s);
	SharedPJ projCrsToCrsWrapper(SharedPJ from, SharedPJ to);
	SharedPJ getSubCrs(const SharedPJ base, int index);

	SharedPJ sharedPJFromOSR(const OGRSpatialReference& osr);

	class PJIdentifyWrapper {
	public:
		PJIdentifyWrapper(const SharedPJ& p, const std::string& auth = "EPSG");
		~PJIdentifyWrapper();

		const int* getConfidence() const;
		const PJ_OBJ_LIST* getObjList() const;
	private:
		int* _confidence;
		PJ_OBJ_LIST* _obj;
	};

	void setProjDirectory(const std::string& path, PJ_CONTEXT* context);
}

#endif