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

#define BOOST_ALL_DYN_LINK

//boost
#pragma warning(push)
#pragma warning(disable : 26495)
#include<boost/program_options.hpp>
#include<boost/optional.hpp>
#pragma warning(pop)

//imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

//glfw
#include <GLFW/glfw3.h>

//nfd
#include <nfd.hpp>

//lapis_gis
#include"gis/Unit.hpp"
#include"gis/LasReader.hpp"
#include"gis/RasterAlgos.hpp"

#endif