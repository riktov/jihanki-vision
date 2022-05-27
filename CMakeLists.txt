cmake_minimum_required (VERSION 2.8.12)

add_definitions(-g -std=c++11 -Wall)

project (xxdummy)

include_directories(/usr/local/include/opencv4)
find_package(OpenCV REQUIRED)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(EXIV2 exiv2>0.1)
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

# add_library(lines STATIC lines.cpp)
# add_library(button_strip STATIC button_strip.cpp)
# add_library(run_length STATIC run_length.cpp)
# add_library(extract_drinks_draw STATIC extract_drinks_draw.cpp)
# add_library(extract_drinks_write STATIC extract_drinks_write.cpp)
# add_library(trim_rect STATIC trim_rect.cpp)
# add_library(threshold STATIC threshold.cpp)
# add_library(display STATIC display.cpp)
# add_library(histogram STATIC histogram.cpp)
# target_link_libraries (histogram ${OpenCV_LIBS})

# add_executable(fixperspective fixperspective.cpp)
# if(WITH_GUI)
#     target_link_libraries (fixperspective ${OpenCV_LIBS} exiv2 lines display)
# else()
#     target_link_libraries (fixperspective ${OpenCV_LIBS} exiv2 lines)
# endif()

#this needs to be placed after a project declaration
if(NOT MINGW)
    message("It's a UNIX system. I know this!")
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
endif()

# if(WITH_GUI)
#     target_link_libraries(display ${X11_LIBRARIES})
# endif()
# project (extract_drinks)
# add_executable(extract_drinks extract_drinks.cpp)

# if(WITH_GUI)
#     target_link_libraries (extract_drinks ${OpenCV_LIBS} button_strip run_length lines trim_rect threshold extract_drinks_write extract_drinks_draw display)
# else()
#     target_link_libraries (extract_drinks ${OpenCV_LIBS} button_strip run_length lines trim_rect threshold extract_drinks_write)
# endif()

# project (identify_drink)
# add_executable(identify_drink identify_drink.cpp)
# target_link_libraries (identify_drink ${OpenCV_LIBS} histogram)

# project (trim_drink)
# add_executable(trim_drink trim_drink.cpp)
# target_link_libraries (trim_drink ${OpenCV_LIBS} trim_rect)


if(NOT MINGW)
    #project(test)
    #add_executable(test test.cpp)
    #target_link_libraries (test ${GTEST_LIBS} ${GMOCK_LIBS})

#   project(catch_test)
#    add_executable(catch_test catch_test.cpp)
#    add_library(factorial STATIC factorial.cpp)
#    target_link_libraries(catch_test ${OpenCV_LIBS} factorial threshold)
endif()

# project(generate_histogram)
# add_executable(generate_histogram generate_histogram.cpp)
# target_link_libraries(generate_histogram ${OpenCV_LIBS} histogram)

#project(mytest)
#add_executable(mytest mytest.cpp)
#target_link_libraries(mytest ${OpenCV_LIBS} threshold)

#project (harris)
#add_executable(harris harris.cpp)
#target_link_libraries (harris ${OpenCV_LIBS} lines)

install(TARGETS fixperspective extract_drinks identify_drink trim_drink generate_histogram DESTINATION bin)
#install(TARGETS histogram DESTINATION lib)

