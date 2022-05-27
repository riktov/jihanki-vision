include_directories(${PROJECT_SOURCE_DIR}/src/ ${PROJECT_SOURCE_DIR})

add_library(trim_rect STATIC trim_rect.cpp)
add_library(threshold STATIC threshold.cpp)

if(WITH_GUI) #and not MINGW
    # project(display)
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    add_library(display STATIC display.cpp)
    target_link_libraries(display ${X11_LIBRARIES})
endif()

project (fixperspective)

add_executable(fixperspective fixperspective.cpp)
add_library(lines STATIC lines.cpp)
target_link_libraries (fixperspective ${OpenCV_LIBS} lines)

if(WITH_GUI)
    add_library(fixperspective_display STATIC fixperspective_display.cpp)
    target_link_libraries (fixperspective display fixperspective_display)
endif()

if(EXIV2_FOUND)
    target_link_libraries (fixperspective exiv2)
endif()


project (extract_drinks)
add_executable(extract_drinks extract_drinks.cpp)
add_library(extract_drinks_write STATIC extract_drinks_write.cpp)
add_library(button_strip STATIC button_strip.cpp)
add_library(run_length STATIC run_length.cpp)

target_link_libraries (extract_drinks ${OpenCV_LIBS} button_strip run_length lines trim_rect threshold extract_drinks_write)

if(WITH_GUI)
    add_library(extract_drinks_draw STATIC extract_drinks_draw.cpp)
    target_link_libraries (extract_drinks display extract_drinks_draw)
endif()

project (identify_drink)
add_executable(identify_drink identify_drink.cpp)
add_library(histogram STATIC histogram.cpp)
target_link_libraries (histogram ${OpenCV_LIBS})
target_link_libraries (identify_drink ${OpenCV_LIBS} histogram)

project (trim_drink)
add_executable(trim_drink trim_drink.cpp)
target_link_libraries (trim_drink ${OpenCV_LIBS} trim_rect)

project(generate_histogram)
add_executable(generate_histogram generate_histogram.cpp)
target_link_libraries(generate_histogram ${OpenCV_LIBS} histogram)
