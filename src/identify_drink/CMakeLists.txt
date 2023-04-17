project(identify_drink)
add_executable(identify_drink identify_drink.cpp)
target_link_libraries (identify_drink ${OpenCV_LIBS} histogram)