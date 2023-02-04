cmake_minimum_required(VERSION 3.24)

project(libfacade CXX)

option(LIBFACADE_TEST "Enable testing for libfacade." OFF)
option(LIBFACADE_BUILD_SHARED "Compile libfacade as a shared library." OFF)
option(LIBFACADE_USE_SYSTEM_ZLIB "Use the zlib on the system rather than the zlib in the repository." ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/facade/*.h ${PROJECT_SOURCE_DIR}/include/facade/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/facade.hpp)
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (LIBFACADE_BUILD_SHARED)
  add_compile_definitions(LIBFACADE_SHARED)
  add_compile_definitions(LIBFACADE_EXPORT)
  add_library(libfacade SHARED ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/facade.hpp ${SRC_FILES})
else()
  add_compile_definitions(LIBFACADE_STATIC)
  add_library(libfacade STATIC ${HEADER_FILES} ${PROJECT_SOURCE_DIR}/include/facade.hpp ${SRC_FILES})
  set(ZLIB_USE_STATIC_LIBS "ON")
endif()

if (UNIX)
  # this is so the library doesn't get named liblibfacade on Unix
  set_target_properties(libfacade PROPERTIES OUTPUT_NAME facade)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(ZLIB)
if (${ZLIB_FOUND} AND ${LIBFACADE_USE_SYSTEM_ZLIB})
  message(STATUS "Found zlib.")
else()
  message(STATUS "Local zlib not found or not wanted, using our copy instead.")
  add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib-1.2.13) # EXCLUDE_FROM_ALL
endif()

set_target_properties(libfacade PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(libfacade PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/lib/zlib-1.2.13"
  "${CMAKE_CURRENT_BINARY_DIR}/lib/zlib-1.2.13"
)

if (LIBFACADE_BUILD_SHARED)
  target_link_libraries(libfacade PUBLIC zlib)
else()
  if (ZLIB_FOUND)
    # we should already be compiling with the static option if we found the zlib package.
    if (UNIX)
      target_link_libraries(libfacade PUBLIC z)
    elseif("$<CONFIG>" STREQUAL "Debug")
      target_link_libraries(libfacade PUBLIC zlibd)
    else()
      target_link_libraries(libfacade PUBLIC zlib)
    endif()
  elseif(LIBFACADE_BUILD_SHARED)
    target_link_libraries(libfacade PUBLIC zlib)
  else()
    target_link_libraries(libfacade PUBLIC zlibstatic)
  endif()
endif()

if (MSVC)
  #message(STATUS "Running MSVC")
  if (LIBFACADE_BUILD_SHARED AND (NOT ZLIB_FOUND OR NOT LIBFACADE_USE_SYSTEM_ZLIB))
    #message(STATUS "Building shared and not using system lib: ${CMAKE_BUILD_TYPE}")
    # copy the zlib dll if we built it so we don't get an error when testing
    if ("$<CONFIG>" STREQUAL "Debug")
      add_custom_command(TARGET libfacade POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE_DIR:libfacade>/../lib/zlib-1.2.13/$<CONFIG>/zlibd.dll"
        "$<TARGET_FILE_DIR:libfacade>/")
    else()
      add_custom_command(TARGET libfacade POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE_DIR:libfacade>/../lib/zlib-1.2.13/$<CONFIG>/zlib.dll"
        "$<TARGET_FILE_DIR:libfacade>/")
    endif()
  endif()
endif()

if (UNIX)
  install(TARGETS libfacade DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
  install(FILES ${HEADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/include/facade")
  install(FILES ${PROJECT_SOURCE_DIR}/include/facade.hpp DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
endif()

if (LIBFACADE_TEST)
  enable_testing()
  add_executable(testfacade ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
  
  target_link_libraries(testfacade PUBLIC libfacade)
  target_include_directories(testfacade PUBLIC
    "${PROJECT_SOURCE_DIR}/test"
  )
  add_test(NAME testfacade COMMAND testfacade)
endif()
