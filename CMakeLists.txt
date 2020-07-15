cmake_minimum_required(VERSION 3.2)
project(collab-vm-server LANGUAGES C CXX)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS")
endif()

find_package(Threads REQUIRED)

if(NOT DEFINED _VCPKG_INSTALLED_DIR)
	message(STATUS "Building without vcpkg, preferring find module")
	set(HAS_VCPKG FALSE)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
	find_package(Cairo REQUIRED)
else()
	message(STATUS "Building with vcpkg, preferring vcpkg package")
	set(HAS_VCPKG TRUE)
	find_package(unofficial-cairo REQUIRED)
endif()

# system implies we have header only Boost components
set(Boost_NO_BOOST_CMAKE OFF)
find_package(Boost REQUIRED COMPONENTS system program_options)

# setup Flatbuffers how we want it
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_STATIC_FLATC ON CACHE BOOL "" FORCE)
set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)

# Custom flatbuffers flatc options
set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS --gen-object-api --gen-name-strings --scoped-enums)
set(FLATC_ARGS "${FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS}")
add_subdirectory(vendor/collab-vm-common/vendor/flatbuffers)

# configure libvncserver
# we do not want ANY of these toggled (they hinder build/configure performance, and we don't use them), so we force them off.
set(LIBVNCSERVER_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# this doesn't turn LZO off, however,
# it forces libvncserver to use its own integrated built-in implementation
# should not bother compliation performance much
set(WITH_LZO OFF CACHE BOOL "" FORCE)
# OpenSSL is built with boost with vcpkg, 
# so assume that instead of gnutls
set(WITH_GNUTLS OFF CACHE BOOL "" FORCE)
set(WITH_SYSTEMD OFF CACHE BOOL "" FORCE)
set(WITH_FFMPEG OFF CACHE BOOL "" FORCE)
set(WITH_TIGHTVNC_FILETRANSFER OFF CACHE BOOL "" FORCE)
set(WITH_WEBSOCKETS OFF CACHE BOOL "" FORCE)
set(WITH_SASL OFF CACHE BOOL "" FORCE)
set(WITH_SDL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/libvncserver)

add_subdirectory(vendor/cairo-jpg)

# All of the source code.
# Chuck in headers in here so CMake generates a proper dependency graph
set(COLLABVMSERVER_SOURCES
	# platform-independent stuff and common headers
	${PROJECT_SOURCE_DIR}/src/Common.h
	${PROJECT_SOURCE_DIR}/src/Logger.h
	${PROJECT_SOURCE_DIR}/src/Logger.cpp
	
	# Websocket server code
	${PROJECT_SOURCE_DIR}/src/WebsocketServer.h
	${PROJECT_SOURCE_DIR}/src/WebsocketServer.cpp
	 
	# CollabVM server code, split up
	${PROJECT_SOURCE_DIR}/src/User.h
	${PROJECT_SOURCE_DIR}/src/UserList.h

	# Server code
	${PROJECT_SOURCE_DIR}/src/Server.h
	${PROJECT_SOURCE_DIR}/src/Server.cpp
	
	# protocol handling code
	${PROJECT_SOURCE_DIR}/src/Protocol.h
	${PROJECT_SOURCE_DIR}/src/Protocol.cpp
	
	# VMCommon
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/Surface.h
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/Surface.cpp
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/VNCClient.h
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/VNCClient.cpp
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/QEMUAudio.h
	${PROJECT_SOURCE_DIR}/src/VMControllers/Common/QEMUAudio.cpp

	${PROJECT_SOURCE_DIR}/src/main.cpp
)

# We generate PCHs of the following include files to speed up compile performance
set(COLLABVMSERVER_PCH
	${PROJECT_SOURCE_DIR}/src/Common.h
	${PROJECT_SOURCE_DIR}/src/Logger.h
)

# Set up a target to build our flatbuffer schema(s)
# TODO for Agent: include its schema here ${PROJECT_SOURCE_DIR}/vendor/collab-vm-common/agent.fbs
build_flatbuffers("${PROJECT_SOURCE_DIR}/vendor/collab-vm-common/collabvm.fbs" "${PROJECT_SOURCE_DIR}/vendor/collab-vm-common/" "schema" "" "${PROJECT_BINARY_DIR}" "" "")

add_executable(collab-vm-server ${COLLABVMSERVER_SOURCES})
add_dependencies(collab-vm-server schema)

set_property(TARGET collab-vm-server PROPERTY CXX_STANDARD 17)
set_property(TARGET collab-vm-server PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(collab-vm-server ${CMAKE_THREAD_LIBS_INIT} flatbuffers Boost::system Boost::program_options vncclient)

# boost.Stacktrace (well, and we're gonna need DL anyways)
if(NOT WIN32)
	target_link_libraries(collab-vm-server dl backtrace)
endif()

if(NOT HAS_VCPKG)
	target_link_libraries(collab-vm-server Cairo::Cairo)
else()
	target_link_libraries(collab-vm-server unofficial::cairo::cairo)
endif()

# Argh, libvncclient CMakeLists doesn't make any usage of target_include_directories
# so I have to do it myself.
target_include_directories(collab-vm-server PUBLIC ${PROJECT_SOURCE_DIR}/vendor/libvncserver ${PROJECT_BINARY_DIR}/vendor/libvncserver)

target_include_directories(collab-vm-server PUBLIC ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR})

target_precompile_headers(collab-vm-server PRIVATE ${COLLABVMSERVER_PCH})