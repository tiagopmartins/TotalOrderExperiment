# Dependencies
find_package(Threads)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Sources
set(SOURCES
    src/ClientFrontendApp.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Target
add_executable(client-frontend ${SOURCES})
target_link_libraries(client-frontend
    PRIVATE
        proto
		libredis++.a
		/usr/local/lib/libhiredis.a
		nlohmann_json::nlohmann_json
	    -pthread
)