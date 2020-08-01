cmake_minimum_required(VERSION 3.15)
project(remote_backup_client)

set(CMAKE_CXX_STANDARD 17)

add_executable(remote_backup_client main.cpp file_watcher.h)