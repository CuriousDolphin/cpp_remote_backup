cmake_minimum_required(VERSION 3.15)
project(remote_backup_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(remote_backup_server main.cpp ../test/main.cpp ../test/main.h)

find_package(Boost 1.67.0 COMPONENTS system filesystem thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(remote_backup_server ${Boost_LIBRARIES} ssl crypto)