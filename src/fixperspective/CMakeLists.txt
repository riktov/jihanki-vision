project(fixperspective)

add_executable(fixperspective fixperspective.cpp)

target_link_libraries (fixperspective ${OpenCV_LIBS} lines perspective_lines)

if(WITH_GUI)
    add_library(fixperspective_draw STATIC fixperspective_draw.cpp)
    target_link_libraries (fixperspective display fixperspective_draw)
endif()

if(EXIV2_FOUND)
    target_link_libraries (fixperspective exiv2)
endif()
