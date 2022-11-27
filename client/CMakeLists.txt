# Dependencies
find_package(yaml-cpp REQUIRED)
find_package(Threads)

include_directories(${YAML_CPP_INCLUDE_DIRS})

# Sources
set(SOURCES
    src/ClientApp.cpp
    src/Client.cpp
    src/prober/Prober.cpp
    src/zipf/ZipfGenerator.cpp
    src/transaction-generator/TransactionGenerator.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Target
add_executable(client ${SOURCES})
target_link_libraries(client
    PRIVATE
        proto
        $ENV{MY_INSTALL_DIR}/lib/libredis++.a
        $ENV{MY_INSTALL_DIR}/lib/libhiredis.a
        -pthread
        ${YAML_CPP_LIBRARIES}
)