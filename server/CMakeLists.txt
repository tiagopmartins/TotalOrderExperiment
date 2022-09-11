# Dependencies
find_package(yaml-cpp REQUIRED)
find_package(Threads)

include_directories(${YAML_CPP_INCLUDE_DIRS})

# Sources
set(SOURCES
    src/ServerApp.cpp
    src/ServerStruct.cpp
    src/Prober.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Target
add_executable(server ${SOURCES})
target_link_libraries(server
    PRIVATE
        proto
        ${YAML_CPP_LIBRARIES}
)