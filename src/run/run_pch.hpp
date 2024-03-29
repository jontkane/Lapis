#pragma once
#ifndef LP_APP_PCH
#define LP_APP_PCH

//std
#include<iostream>
#include<mutex>
#include<map>
#include<chrono>
#include<thread>
#include<functional>
#include<filesystem>
#include<unordered_set>
#include<queue>
#include<unordered_map>
#include<sstream>

//nfd
#include <nfd.hpp>

//lapis_gis
#include"../gis/Unit.hpp"
#include"../gis/LasReader.hpp"
#include"../gis/RasterAlgos.hpp"

//harupdf
#include<hpdf.h>

//lapis_utils
#include"../utils/LapisLogger.hpp"

#endif