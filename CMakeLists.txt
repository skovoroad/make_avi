cmake_minimum_required(VERSION 3.8)
project(make_avi)

set(FLAG_SUFF ${CMAKE_BUILD_TYPE})
string(TOUPPER ${FLAG_SUFF} FLAG_SUFF)
set(CMAKE_C_FLAGS_${FLAG_SUFF} "${CMAKE_C_FLAGS_${FLAG_SUFF}} /MD /MP")
set(CMAKE_CXX_FLAGS_${FLAG_SUFF} "${CMAKE_CXX_FLAGS_${FLAG_SUFF}} /MD /MP")
add_definitions(-DBOOST_ALL_NO_LIB)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_RELEASE_LIBS ON)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  add_subdirectory(example)
endif()

