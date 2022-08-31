cmake_minimum_required(VERSION 3.16)
project(wanto-opt LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(client)
add_subdirectory(client-frontend)
add_subdirectory(contract)
add_subdirectory(server)