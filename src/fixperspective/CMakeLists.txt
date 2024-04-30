project(fixperspective)

add_executable(fixperspective fixperspective.cpp)
add_library(perspective_lines STATIC perspective_lines.cpp)
add_library(detect STATIC detect.cpp)
add_library(cabinet STATIC cabinet.cpp)

target_link_libraries (fixperspective ${OpenCV_LIBS} lines perspective_lines detect cabinet)

if(WITH_GUI)
    add_library(fixperspective_draw STATIC fixperspective_draw.cpp perspective_lines)
    
    target_link_libraries (fixperspective display fixperspective_draw)
    
    add_executable(calibrator calibrator.cpp)
    target_link_libraries (calibrator ${OpenCV_LIBS} perspective_lines detect lines display fixperspective_draw)
endif()

if(EXIV2_FOUND)
    target_link_libraries (fixperspective exiv2)
endif()

install(TARGETS fixperspective DESTINATION bin)