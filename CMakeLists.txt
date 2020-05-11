cmake_minimum_required (VERSION 3.1)

project (make_avi)

set (CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS program_options)

file(GLOB HEADER_FILES *.h)
file(GLOB SOURCE_FILES *.cpp)

add_executable(make_avi ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME}
  ${Boost_LIBRARIES} 
  Boost::program_options
)

