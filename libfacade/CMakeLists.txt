cmake_minimum_required(VERSION 3.10)

project(libfacade CXX)

option(TEST_LIBFACADE "Enable testing for libfacade." OFF)
option(LIBFACADE_BUILD_SHARED "Compile libfacade as a shared library." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (LIBFACADE_BUILD_SHARED)
  add_compile_definitions(LIBFACADE_SHARED)
  add_library(libfacade SHARED ${HEADER_FILES} ${SRC_FILES})
else()
  add_compile_definitions(LIBFACADE_STATIC)
  add_library(libfacade STATIC ${HEADER_FILES} ${SRC_FILES})
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib-1.2.13)

set_target_properties(libfacade PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(libfacade PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/lib/zlib-1.2.13"
  "${CMAKE_CURRENT_BINARY_DIR}/lib/zlib-1.2.13"
)

if (LIBFACADE_BUILD_SHARED)
  target_link_libraries(libfacade PUBLIC zlib)
else()
  target_link_libraries(libfacade PUBLIC zlibstatic)
endif()

if (TEST_LIBFACADE)
  enable_testing()
  add_executable(testfacade ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
  
  target_link_libraries(testfacade PUBLIC libfacade)
  target_include_directories(testfacade PUBLIC
    "${PROJECT_SOURCE_DIR}/test"
  )
  add_test(NAME testfacade COMMAND testfacade)
endif()
