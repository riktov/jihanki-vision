project(generate_histogram)
add_executable(generate_histogram generate_histogram.cpp)
target_link_libraries(generate_histogram ${OpenCV_LIBS} histogram)

install(TARGETS generate_histogram DESTINATION bin)