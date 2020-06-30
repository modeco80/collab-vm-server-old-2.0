# cairo-jpg CMake rules

find_package(JPEG REQUIRED)

add_library(cairo-jpg STATIC ${CMAKE_CURRENT_SOURCE_DIR}/cairo_jpg.c)

# imported by main CMakeLists
target_link_libraries(cairo-jpg Cairo::Cairo ${JPEG_LIBRARIES})

# API headers are in the current source directory
target_include_directories(cairo-jpg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${JPEG_INCLUDE_DIRS})