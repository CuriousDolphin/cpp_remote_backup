cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(server main.cpp server.h session.h db.h ../shared/const.h)

find_package(Boost )
IF (Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    message("BOOST   FOUND")
    include_directories(${Boost_INCLUDE_DIR})
    target_link_libraries(server ${Boost_LIBRARIES} ssl crypto cpp_redis tacopie -lboost_regex )
endif()