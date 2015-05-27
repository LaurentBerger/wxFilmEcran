cmake_minimum_required(VERSION 2.8)

PROJECT (FilmEcran)
set (FilmEcran 1)
set (FilmEcran 0)

set(CMAKE_MODULE_PATH "${FilmEcran_SOURCE_DIR}/CMake")
find_package(wxWidgets COMPONENTS core base net REQUIRED)

set(wxWidgets_USE_STATIC=ON)
file(GLOB FilmEcran_SRCS
    "*.h"
    "*.cpp")
ADD_EXECUTABLE (FilmEcran ${FilmEcran_SRCS})



if (wxWidgets_FOUND)
	include_directories(${wxWidgets_INCLUDE_DIRS})
	target_link_libraries (FilmEcran ${wxWidgets_LIBRARIES} )
	set_target_properties(FilmEcran PROPERTIES LINK_FLAGS_DEBUG  "/SUBSYSTEM:WINDOWS")
	set_target_properties(FilmEcran PROPERTIES LINK_FLAGS_RELEASE  "/SUBSYSTEM:WINDOWS")
endif (wxWidgets_FOUND)

