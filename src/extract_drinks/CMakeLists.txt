project(extract_drinks)

add_executable(extract_drinks extract_drinks.cpp)
add_library(extract_drinks_write STATIC extract_drinks_write.cpp)

add_library(button_strip STATIC button_strip.cpp)
add_library(run_length STATIC run_length.cpp)

target_link_libraries (extract_drinks ${OpenCV_LIBS} button_strip run_length lines trim_rect threshold extract_drinks_write)

if(WITH_GUI)
    add_library(extract_drinks_draw STATIC extract_drinks_draw.cpp)
    target_link_libraries (extract_drinks display extract_drinks_draw)
endif()

install(TARGETS extract_drinks DESTINATION bin)