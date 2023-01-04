cmake_minimum_required(VERSION 3.24)
project(Lapis VERSION 0.1)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(VCPKG_TARGET_TRIPLET x64-windows)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

file(GLOB LAPIS_GIS_SOURCES
	src/gis/*.hpp
	src/gis/*.cpp)

file(GLOB_RECURSE LAPIS_PARAMETERS_SOURCES
	src/parameters/*.hpp
	src/parameters/*.cpp)

file(GLOB LAPIS_RUN_SOURCES
	src/run/*.hpp
	src/run/*.cpp)

file(GLOB LAPIS_LOGGER_SOURCES
	src/logger/*.cpp
	src/logger/*.hpp)

file(GLOB LAPIS_ALGO_SOURCES
	src/algorithms/*.cpp
	src/algorithms/*.hpp)

file(GLOB LAPIS_TEST_SOURCES
	src/test/*.cpp
	src/test/*.hpp)

file(GLOB LAPIS_IMGUI_SOURCES
	src/imgui/*.cpp
	src/imgui/*.h)

file(GLOB LAPIS_EXE_SOURCES
	src/*.cpp
	src/*.hpp)

add_library(Lapis_gis STATIC ${LAPIS_GIS_SOURCES})
add_library(Lapis_algorithms STATIC ${LAPIS_ALGO_SOURCES})
add_library(Lapis_params STATIC ${LAPIS_PARAMETERS_SOURCES})
add_library(Lapis_run STATIC ${LAPIS_RUN_SOURCES})
add_library(Lapis_logger STATIC ${LAPIS_LOGGER_SOURCES})
add_library(Lapis_imgui STATIC ${LAPIS_IMGUI_SOURCES})

add_executable(Lapis WIN32 src/lapis.cpp)
target_link_libraries(Lapis PUBLIC Lapis_gis)
target_link_libraries(Lapis PUBLIC Lapis_logger)
target_link_libraries(Lapis PUBLIC Lapis_algorithms)
target_link_libraries(Lapis PUBLIC Lapis_params)
target_link_libraries(Lapis PUBLIC Lapis_run)
target_link_libraries(Lapis PUBLIC Lapis_imgui)

add_executable(Lapis_test ${LAPIS_TEST_SOURCES})
target_link_libraries(Lapis_test PUBLIC Lapis_gis)
target_link_libraries(Lapis_test PUBLIC Lapis_logger)
target_link_libraries(Lapis_test PUBLIC Lapis_algorithms)
target_link_libraries(Lapis_test PUBLIC Lapis_params)
target_link_libraries(Lapis_test PUBLIC Lapis_run)
target_link_libraries(Lapis_test PUBLIC Lapis_imgui)

find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GDAL_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GDAL_LIBRARIES})

find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${Boost_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${Boost_LIBRARIES})

find_package(GeoTIFF REQUIRED)
include_directories(${GeoTIFF_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE ${GeoTIFF_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${GeoTIFF_LIBRARIES})

find_package(PROJ REQUIRED)
include_directories(${PROJ_INCLUDE_DIR})
target_link_libraries(Lapis PRIVATE ${PROJ_LIBRARIES})
target_link_libraries(Lapis_test PRIVATE ${PROJ_LIBRARIES})

find_package(xtl REQUIRED)
include_directories(${xtl_INCLUDE_DIR})

find_package(glfw3 REQUIRED)
include_directories(${glfw_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE glfw)
target_link_libraries(Lapis_test PRIVATE glfw)

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
target_link_libraries(Lapis PRIVATE OpenGL::GL)
target_link_libraries(Lapis_test PRIVATE OpenGL::GL)

add_subdirectory(src/nativefiledialog-extended)
include_directories(src/nativefiledialog-extended/src/include)
target_link_libraries(Lapis PRIVATE nfd)
target_link_libraries(Lapis_test PRIVATE nfd)

find_package(GTest REQUIRED)
target_include_directories(Lapis_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(Lapis_test PRIVATE ${GTEST_BOTH_LIBRARIES})

#not using add_subdirectory here because lazperf generates a very annoying number of targets
include_directories(src/lazperf/cpp)
file(GLOB_RECURSE LAZPERF_FILES 
	src/lazperf/cpp/lazperf/*.cpp
	src/lazperf/cpp/lazperf/*.hpp
)
add_library(lazperf STATIC ${LAZPERF_FILES})
target_link_libraries(Lapis PRIVATE lazperf)
target_link_libraries(Lapis_test PRIVATE lazperf)

target_precompile_headers(Lapis_params PRIVATE src/parameters/param_pch.hpp)
target_precompile_headers(Lapis_run PRIVATE src/run/run_pch.hpp)
target_precompile_headers(Lapis_gis PRIVATE src/gis/gis_pch.hpp)
target_precompile_headers(Lapis_algorithms PRIVATE src/algorithms/algo_pch.hpp)
target_precompile_headers(Lapis_test PRIVATE src/test/test_pch.hpp)

if(MSVC)
  target_compile_options(Lapis PRIVATE /W3 /WX)
  target_compile_options(Lapis_logger PRIVATE /W3 /WX)
  target_compile_options(Lapis_params PRIVATE /W3 /WX)
  target_compile_options(Lapis_algorithms PRIVATE /W3 /WX)
  target_compile_options(Lapis_run PRIVATE /W3 /WX)
  target_compile_options(Lapis_gis PRIVATE /W3 /WX)
  target_compile_options(Lapis_test PRIVATE /W3 /WX)
  target_compile_options(lazperf PRIVATE /W0)
  target_compile_options(nfd PRIVATE /W0)
  target_compile_options(Lapis_imgui PRIVATE /W0)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Lapis)
else()
  target_compile_options(Lapis PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_logger PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_params PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_algorithms PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_run PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_gis PRIVATE -Wall -Wextra -Werror)
  target_compile_options(Lapis_test PRIVATE -Wall -WExtra -Werror)
endif()

add_compile_definitions(LAPISTESTFILES="${CMAKE_SOURCE_DIR}/src/test/testfiles/")