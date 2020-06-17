cmake_minimum_required(VERSION 3.2)
project(collab-vm-server CXX)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS")
endif()

find_package(Threads REQUIRED)

# system implies we have header only Boost components
set(Boost_NO_BOOST_CMAKE OFF)
find_package(Boost REQUIRED COMPONENTS system program_options)

# setup Flatbuffers how we want it
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_STATIC_FLATC ON CACHE BOOL "" FORCE)
set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/collab-vm-common/vendor/flatbuffers)

set(COLLABVMSERVER_SOURCES
	 ${PROJECT_SOURCE_DIR}/src/Common.h
	 ${PROJECT_SOURCE_DIR}/src/Logger.h
	 ${PROJECT_SOURCE_DIR}/src/Logger.cpp
	 
	 ${PROJECT_SOURCE_DIR}/src/WebsocketServer.h
	 ${PROJECT_SOURCE_DIR}/src/WebsocketServer.cpp
	 
	 ${PROJECT_SOURCE_DIR}/src/User.h
	 ${PROJECT_SOURCE_DIR}/src/Server.h
	 ${PROJECT_SOURCE_DIR}/src/Server.cpp

	 ${PROJECT_SOURCE_DIR}/src/main.cpp
)

# Set up a target to build our flatbuffer schema(s)
build_flatbuffers("${PROJECT_SOURCE_DIR}/vendor/collab-vm-common/collabvm.fbs" "${PROJECT_SOURCE_DIR}/vendor/collab-vm-common/" "schema" "" "${PROJECT_BINARY_DIR}" "" "")

add_executable(collab-vm-server ${COLLABVMSERVER_SOURCES})
add_dependencies(collab-vm-server schema)

set_property(TARGET collab-vm-server PROPERTY CXX_STANDARD 17)
set_property(TARGET collab-vm-server PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(collab-vm-server ${CMAKE_THREAD_LIBS_INIT} flatbuffers Boost::system Boost::program_options)
target_include_directories(collab-vm-server PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(collab-vm-server PUBLIC ${PROJECT_SOURCE_DIR}/vendor/websocketpp/)