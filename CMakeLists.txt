cmake_minimum_required (VERSION 2.8.12)

add_definitions(-g -std=c++11 -Wall)

project (jihanki-vision)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(EXIV2 exiv2)
    pkg_check_modules(GTEST gtest>0.1)
    pkg_check_modules(GMOCK gmock>0.1)
endif()

option(WITH_GUI "Visual feedback with highgui" ON)

if(WITH_GUI)
    add_definitions(-DUSE_GUI)
endif()

if(EXIV2_FOUND)
    add_definitions(-DUSE_EXIV2)    
endif()

if(OpenCV_VERSION_MAJOR)
    add_definitions(-DCV_VERSION_MAJOR=${OpenCV_VERSION_MAJOR})    
endif()

add_subdirectory(src)

find_package(X11)
link_libraries(${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})



