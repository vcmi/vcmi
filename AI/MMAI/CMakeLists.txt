cmake_minimum_required(VERSION 3.24)

set(MMAI_FILES
  BAI/base.cpp
  BAI/base.h
  BAI/router.cpp
  BAI/router.h
  BAI/model/ScriptedModel.h
  BAI/model/ScriptedModel.cpp
  BAI/model/NNModel.h
  BAI/model/NNModel.cpp
  BAI/model/util/bucketing.cpp
  BAI/model/util/bucketing.h
  BAI/model/util/common.h
  BAI/model/util/sampling.cpp
  BAI/model/util/sampling.h

  BAI/v13/BAI.cpp
  BAI/v13/BAI.h
  BAI/v13/action.cpp
  BAI/v13/action.h
  BAI/v13/attack_log.h
  BAI/v13/battlefield.cpp
  BAI/v13/battlefield.h
  BAI/v13/encoder.cpp
  BAI/v13/encoder.h
  BAI/v13/global_stats.cpp
  BAI/v13/global_stats.h
  BAI/v13/hex.cpp
  BAI/v13/hex.h
  BAI/v13/hexaction.h
  BAI/v13/hexactmask.h
  BAI/v13/links.h
  BAI/v13/player_stats.cpp
  BAI/v13/player_stats.h
  BAI/v13/render.cpp
  BAI/v13/render.h
  BAI/v13/stack.cpp
  BAI/v13/stack.h
  BAI/v13/state.cpp
  BAI/v13/state.h
  BAI/v13/supplementary_data.cpp
  BAI/v13/supplementary_data.h

  schema/schema.h
  schema/v13/constants.h
  schema/v13/schema.h
  schema/v13/types.h

  MMAI.h
  StdInc.h
  common.h
)

option(ENABLE_MMAI_TEST "Compile tests" OFF)

set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DUSING_ONNX=1)
assign_source_group(${MMAI_FILES})

if(ENABLE_STATIC_LIBS)
  add_library(MMAI STATIC ${MMAI_FILES})
else()
  add_library(MMAI SHARED ${MMAI_FILES} main.cpp StdInc.cpp)
  install(TARGETS MMAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})

  # This is needed to allow MMAI to link against BattleAI
  # For windows, this is handled at runtime instead via LoadLibraryExW(..., LOAD_WITH_ALTERED_SEARCH_PATH)
  if(APPLE)
    set_target_properties(MMAI PROPERTIES INSTALL_RPATH "@loader_path")
  elseif(UNIX)
    set_target_properties(MMAI PROPERTIES INSTALL_RPATH "$ORIGIN")
  endif()
endif()

target_link_libraries(MMAI PRIVATE vcmi)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(ONNXRUNTIME_ROOT "/opt/onnxruntime" CACHE PATH "Path to onnxruntime containing lib/onnxruntime.so and include/onnxruntime_cxx_api.h")

  message(STATUS "Looking for a system onnxruntime")
  find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATH_SUFFIXES onnxruntime include
  )

  find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime libonnxruntime)

  if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
    message(STATUS "Using system onnxruntime:")
    message(STATUS "  include: ${ONNXRUNTIME_INCLUDE_DIR}")
    message(STATUS "  library: ${ONNXRUNTIME_LIBRARY}")

    target_include_directories(MMAI PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(MMAI PRIVATE "${ONNXRUNTIME_LIBRARY}")
  else()
    message(STATUS "System onnxruntime not found, falling back to ${ONNXRUNTIME_ROOT}")
    target_include_directories(MMAI PRIVATE "${ONNXRUNTIME_ROOT}/include")
    target_link_libraries(MMAI PRIVATE "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
  endif()
else()
  find_package(onnxruntime CONFIG REQUIRED)
  target_link_libraries(MMAI PRIVATE onnxruntime::onnxruntime)
endif()

# for fallback & spell casting
add_dependencies(MMAI BattleAI)
target_link_libraries(MMAI PRIVATE BattleAI)

# Used in schema/base.h to determine if it is imported by MMAI or vcmi-gym
set_target_properties(MMAI PROPERTIES COMPILE_DEFINITIONS "MMAI_DLL=1")
target_include_directories(MMAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_MMAI_TEST)
  include(GoogleTest)
  include(CheckCXXCompilerFlag)
  enable_testing()

  target_link_libraries(MMAI PUBLIC gtest gtest_main)

  target_include_directories(MMAI PRIVATE "${CMAKE_SOURCE_DIR}/test/googletest/googletest/include")
  add_subdirectory(${CMAKE_SOURCE_DIR}/test/googletest ${CMAKE_SOURCE_DIR}/test/googletest/build EXCLUDE_FROM_ALL)
  add_executable(MMAI_test test/encoder_test.cpp)
  target_link_libraries(MMAI_test PRIVATE MMAI)
  gtest_discover_tests(MMAI_test)

  # default visibility is needed for testing
  set_target_properties(MMAI PROPERTIES CXX_VISIBILITY_PRESET "default")
  set_target_properties(MMAI_test PROPERTIES CXX_VISIBILITY_PRESET "default")

  # Run tests with:
  # ctest --test-dir build/AI/MMAI/
endif()

vcmi_set_output_dir(MMAI "AI")
enable_pch(MMAI)
