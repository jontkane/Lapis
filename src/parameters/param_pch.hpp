#pragma once
#ifndef LP_PARAM_PCH_H
#define LP_PARAM_PCH_H

#include<string>
#include<iostream>
#include<mutex>
#include<memory>
#include<vector>
#include<filesystem>
#include<queue>

#define BOOST_ALL_DYN_LINK

#pragma warning(push)
#pragma warning(disable : 26495)
#include<boost/program_options.hpp>
#pragma warning(pop)

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include"../LapisTypedefs.hpp"
#include"../gis/raster.hpp"
#include"../gis/Unit.hpp"
#include"../gis/LasReader.hpp"
#include"../gis/LasFilter.hpp"
#include"../utils/LapisLogger.hpp"

#include<nfd.hpp>

#include<hpdf.h>

using BoostOptDesc = boost::program_options::options_description;

#endif