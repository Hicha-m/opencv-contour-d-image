cmake_minimum_required(VERSION 3.16)

project(TPContours LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_executable(tp_contours main.cpp)
target_include_directories(tp_contours PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tp_contours PRIVATE ${OpenCV_LIBS})

if (MSVC)
    target_compile_options(tp_contours PRIVATE /W4)
else()
    target_compile_options(tp_contours PRIVATE -Wall -Wextra -Wpedantic)
endif()