cmake_minimum_required(VERSION 3.15)
project(remote_backup_client)

set(CMAKE_CXX_STANDARD 20)

add_executable(remote_backup_client main.cpp file_watcher.h node.h hasher.h client.h)

find_package(Boost 1.67.0 COMPONENTS system filesystem thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(remote_backup_client ${Boost_LIBRARIES} ssl crypto )