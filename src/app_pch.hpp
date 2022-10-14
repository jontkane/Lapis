#pragma once

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