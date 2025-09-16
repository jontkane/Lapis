include_guard(GLOBAL)

message("Inside Lapis.cmake")

set(LAPIS_DIR ${CMAKE_CURRENT_LIST_DIR})

list(APPEND LAPIS_SCRIPTS
    ${CMAKE_CURRENT_LIST_FILE}
)

set(LAPISGISCMAKE_PATH "${LAPIS_DIR}/src/gis/LapisGis.cmake" CACHE PATH "Path to LapisGis.cmake")
include(${LAPISGISCMAKE_PATH})

file(GLOB LAPIS_PARAMETERS_SOURCES
	${LAPIS_DIR}/src/parameters/*.hpp
	${LAPIS_DIR}/src/parameters/*.cpp)

file(GLOB LAPIS_RUN_SOURCES
	${LAPIS_DIR}/src/run/*.hpp
	${LAPIS_DIR}/src/run/*.cpp)

file(GLOB LAPIS_UTILS_SOURCES
	${LAPIS_DIR}/src/utils/*.cpp
	${LAPIS_DIR}/src/utils/*.hpp)

file(GLOB LAPIS_ALGO_SOURCES
	${LAPIS_DIR}/src/algorithms/*.cpp
	${LAPIS_DIR}/src/algorithms/*.hpp)

file(GLOB LAPIS_IMGUI_SOURCES
	${LAPIS_DIR}/src/imgui/*.cpp
	${LAPIS_DIR}/src/imgui/*.h)

set(LAPIS_EXE_SOURCES
	${LAPIS_DIR}/src/Lapis.cpp
	${LAPIS_DIR}/src/Lapis.hpp
	${LAPIS_DIR}/src/LapisTypeDefs.hpp
	)

add_executable(Lapis WIN32 ${LAPIS_EXE_SOURCES})
copy_proj_db_after_build(Lapis)

add_library(Lapis_algorithms STATIC ${LAPIS_ALGO_SOURCES})
add_library(Lapis_params OBJECT ${LAPIS_PARAMETERS_SOURCES})
add_library(Lapis_run OBJECT ${LAPIS_RUN_SOURCES})
add_library(Lapis_utils STATIC ${LAPIS_UTILS_SOURCES})
add_library(Lapis_imgui STATIC ${LAPIS_IMGUI_SOURCES})

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(unofficial-libharu CONFIG REQUIRED)


add_subdirectory(${LAPIS_DIR}/src/nativefiledialog-extended nfd)

set(LAPIS_EXTERNAL_INCLUDES
	${LAPISGIS_INCLUDES}
	${Boost_INCLUDE_DIRS}
	${glfw_INCLUDE_DIRS}
	${OpenGL_INCLUDE_DIRS}
	${LIBHARU_INCLUDE_DIRS}
	${LAPIS_DIR}/src/nativefiledialog-extended/src/include
	)

set(LAPIS_EXTERNAL_LINKS
	${LAPISGIS_LINKS}
	${Boost_LIBRARIES}
	glfw
	OpenGL::GL
	unofficial::libharu::hpdf
	nfd
	)

set(LAPIS_INTERNAL_LINKS
	Lapis_algorithms
	Lapis_params
	Lapis_run
	Lapis_utils
	Lapis_imgui
	)
	
target_include_directories(Lapis_algorithms PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_params PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_run PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_utils PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_include_directories(Lapis_imgui PRIVATE ${LAPIS_EXTERNAL_INCLUDES})

target_include_directories(Lapis PRIVATE ${LAPIS_EXTERNAL_INCLUDES})
target_link_libraries(Lapis PRIVATE ${LAPIS_EXTERNAL_LINKS})
target_link_libraries(Lapis PRIVATE ${LAPIS_INTERNAL_LINKS})

target_precompile_headers(Lapis_params PRIVATE ${LAPIS_DIR}/src/parameters/param_pch.hpp)
target_precompile_headers(Lapis_run PRIVATE ${LAPIS_DIR}/src/run/run_pch.hpp)
target_precompile_headers(Lapis_algorithms PRIVATE ${LAPIS_DIR}/src/algorithms/algo_pch.hpp)

if (MSVC)
	target_compile_options(Lapis PRIVATE /W3 /WX)
	target_compile_options(Lapis_utils PRIVATE /W3 /WX)
	target_compile_options(Lapis_params PRIVATE /W3 /WX)
	target_compile_options(Lapis_algorithms PRIVATE /W3 /WX)
	target_compile_options(Lapis_run PRIVATE /W3 /WX)
	target_compile_options(nfd PRIVATE /W0)
	target_compile_options(Lapis_imgui PRIVATE /W0)
else()
	target_compile_options(Lapis PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_utils PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_params PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_algorithms PRIVATE -Wall -Wextra -Werror)
	target_compile_options(Lapis_run PRIVATE -Wall -Wextra -Werror)
endif()