#pragma once
#ifndef LP_GIS_PCH
#define LP_GIS_PCH

//std
#include<unordered_set>
#include<vector>
#include<fstream>
#include<array>
#include<variant>
#include<memory>
#include<stdexcept>
#include<regex>
#include<iostream>
#include<unordered_map>
#include<thread>
#include<mutex>
#include<cassert>
#include<filesystem>

//lazperf
#pragma warning (push)
#pragma warning (disable : 26495)
#pragma warning (disable : 4251)
#pragma warning (disable : 4267)
#pragma warning (disable : 26451)
#pragma warning (disable : 4244)
#pragma warning (disable : 26439)
#include<lazperf/readers.hpp>
#include<lazperf/vlr.hpp>
#include<lazperf/filestream.hpp>
#pragma warning (pop)

//lazperf must include windows.h somewhere, clean up the more annoying macros
#undef near
#undef far
#undef min
#undef max

//gdal
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 26812)
#pragma warning(disable : 4101)
#include<gdal_priv.h>
#include<ogrsf_frmts.h>
#pragma warning(pop)

//proj
#include<proj.h>

//this prevents geotiff from loading a file which already has its symbols defined in GDAL's lib file
#define CPL_SERV_H_INCLUDED

//geotiff
#include<geo_simpletags.h>
#include<geo_normalize.h>
#include<geo_tiffp.h>

//xtl
#include<xtl/xoptional_sequence.hpp>

#endif