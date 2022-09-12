# Dependencies
find_package(yaml-cpp REQUIRED)
find_package(Threads)

include_directories(${YAML_CPP_INCLUDE_DIRS})

# Sources
set(SOURCES
    src/ClientApp.cpp
    src/Client.cpp
    src/Prober.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Target
add_executable(client ${SOURCES})
target_link_libraries(client
    PRIVATE
        proto
        $ENV{HOME}/.local/lib/libredis++.a
        $ENV{HOME}/.local/lib/libhiredis.a
        -pthread
        ${YAML_CPP_LIBRARIES}
)