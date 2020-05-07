cmake_minimum_required (VERSION 3.1)

project (make_avi)

set (CMAKE_CXX_STANDARD 17)

file(GLOB HEADER_FILES *.h)
file(GLOB SOURCE_FILES *.cpp)

add_executable(make_avi ${SOURCE_FILES} ${HEADER_FILES})
