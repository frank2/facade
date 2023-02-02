cmake_minimum_required(VERSION 3.10)

project(facade CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

add_subdirectory(${PROJECT_SOURCE_DIR}/libfacade)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/argparse)

add_executable(facade ${SRC_FILES})
set_target_properties(facade PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(facade PUBLIC libfacade argparse)
