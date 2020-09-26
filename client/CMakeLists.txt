cmake_minimum_required(VERSION 3.15)
project(client)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ")

add_executable(client main.cpp file_watcher.h node.h ../shared/hasher.h ../shared/to_time_t.h client.h)
find_package(Boost)
IF (Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    message("BOOST   FOUND")
    include_directories(${Boost_INCLUDE_DIR})
    target_link_libraries(client ${Boost_LIBRARIES} ssl crypto )
endif()



if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
