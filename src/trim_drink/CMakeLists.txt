project(trim_drink)
add_executable(trim_drink trim_drink.cpp)
target_link_libraries (trim_drink ${OpenCV_LIBS} trim_rect)

install(TARGETS trim_drink DESTINATION bin)