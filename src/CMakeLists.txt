include_directories(${PROJECT_SOURCE_DIR}/src/ ${PROJECT_SOURCE_DIR})


if(WITH_GUI) #and not MINGW
    # project(display)
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    add_library(display STATIC display.cpp)
    target_link_libraries(display ${X11_LIBRARIES})
endif()

add_library(trim_rect STATIC trim_rect.cpp)
add_library(lines STATIC lines.cpp)
add_library(histogram STATIC histogram.cpp)

target_link_libraries (histogram ${OpenCV_LIBS})

add_subdirectory(fixperspective)
add_subdirectory(extract_drinks)
add_subdirectory(identify_drink)
add_subdirectory(generate_histogram)
add_subdirectory(trim_drink)