#include"gis_pch.hpp"
#include"projwrappers.hpp"
#include"GDALWrappers.hpp"
#include"..\utils\LapisOSSpecific.hpp"

namespace lapis {
	PJ_CONTEXT* ProjContextByThread::get()
	{
		std::thread::id thisthread = std::this_thread::get_id();
		if (!_ctxs.count(thisthread)) {
			_ctxs[thisthread] = getNewPJContext();
			setProjDirectory(executableFilePath(), _ctxs[thisthread].get());
		}
		return _ctxs[thisthread].get();
	}
	SharedPJ makeSharedPJ(PJ* pj)
	{
		return SharedPJ(pj,
			[](PJ* pj) {
				if (pj) {
					proj_destroy(pj);
				}
			}
		);
	}
	SharedPJCtx getNewPJContext()
	{
		return SharedPJCtx(proj_context_create(),
			[](PJ_CONTEXT* pjc) {
				if (pjc) {
					proj_context_destroy(pjc);
				}
			}
		);
	}
	SharedPJ projCreateWrapper(const std::string& s)
	{
		return makeSharedPJ(proj_create(ProjContextByThread::get(), s.c_str()));
	}
	SharedPJ projCrsToCrsWrapper(SharedPJ from, SharedPJ to)
	{
		SharedPJ out = makeSharedPJ(proj_create_crs_to_crs_from_pj(
			ProjContextByThread::get(), from.get(), to.get(), nullptr, nullptr)
		);
		if (out) {
			out = makeSharedPJ(
				proj_normalize_for_visualization(ProjContextByThread::get(), out.get())
			);
		}
		return out;
	}
	SharedPJ getSubCrs(const SharedPJ base, int index)
	{
		return makeSharedPJ(
			proj_crs_get_sub_crs(ProjContextByThread::get(), base.get(), index)
		);
	}
	SharedPJ sharedPJFromOSR(const OGRSpatialReference& osr)
	{
		UniqueGdalString wgs = exportToWktWrapper(osr);
		return projCreateWrapper(wgs.get());
	}
	void setProjDirectory(const std::string& path, PJ_CONTEXT* context)
	{
		namespace fs = std::filesystem;
		std::string folder;
		if (!fs::is_directory(path)) {
			folder = fs::path(path).parent_path().string();
		}
		else {
			folder = path;
		}
		char* data = folder.data();
		proj_context_set_search_paths(context, 1, &data);
	}
	PJIdentifyWrapper::PJIdentifyWrapper(const SharedPJ& p, const std::string& auth) : _obj(nullptr), _confidence(nullptr)
	{
		_obj = proj_identify(ProjContextByThread::get(), p.get(), auth.c_str(), nullptr, &_confidence);
	}
	PJIdentifyWrapper::~PJIdentifyWrapper()
	{
		if (_confidence) {
			proj_int_list_destroy(_confidence);
		}
		if (_obj) {
			proj_list_destroy(_obj);
		}
	}
	const int* PJIdentifyWrapper::getConfidence() const
	{
		return _confidence;
	}
	const PJ_OBJ_LIST* PJIdentifyWrapper::getObjList() const
	{
		return _obj;
	}
}