cmake_minimum_required(VERSION 3.15)
project(client)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ")

set(Boost_USE_STATIC_LIBS ON)

set(Boost_USE_STATIC_LIBS TRUE)

set(Boost_USE_MULTITHREADED ON)

set(OPENSSL_USE_STATIC_LIBS TRUE)

add_executable(client
        main.cpp
        file_watcher.h
        node.h
        client.h
        node.cpp
        file_watcher.cpp
        client.cpp)

find_package(OpenSSL REQUIRED)


find_package(Boost 1.71.0 COMPONENTS regex system filesystem)
IF (Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    message("BOOST   FOUND")
    include_directories(${Boost_INCLUDE_DIR})
    target_link_libraries(client ${Boost_LIBRARIES} OpenSSL::Crypto -DBOOST_NO_CXX11_SCOPED_ENUMS)
endif()



if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
