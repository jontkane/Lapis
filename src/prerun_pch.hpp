#include<vector>
#include<unordered_map>
#include<iostream>
#include<mutex>

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