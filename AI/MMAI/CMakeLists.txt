cmake_minimum_required(VERSION 3.24)

# Re-enable CMP0177 after it is reset by MMAI.
if(POLICY CMP0177)
	cmake_policy(SET CMP0177 NEW)
endif()

set(MMAI_FILES
  BAI/logger.h
  BAI/factory.h
  BAI/factory.cpp
  BAI/router.cpp
  BAI/router.h
  BAI/fallback/scripted_model.h
  BAI/fallback/scripted_model.cpp

  BAI/v13/BAI_v13.cpp
  BAI/v13/BAI_v13.h
  BAI/v13/nn_model_v13.h
  BAI/v13/nn_model_v13.cpp
  BAI/v13/action_v13.cpp
  BAI/v13/action_v13.h
  BAI/v13/attack_log_v13.h
  BAI/v13/battlefield_v13.cpp
  BAI/v13/battlefield_v13.h
  BAI/v13/encoder_v13.h
  BAI/v13/encoder_v13.cpp
  BAI/v13/global_stats_v13.cpp
  BAI/v13/global_stats_v13.h
  BAI/v13/hex_v13.cpp
  BAI/v13/hex_v13.h
  BAI/v13/hexaction_v13.h
  BAI/v13/hexactmask_v13.h
  BAI/v13/links_v13.h
  BAI/v13/player_stats_v13.cpp
  BAI/v13/player_stats_v13.h
  BAI/v13/render_v13.cpp
  BAI/v13/render_v13.h
  BAI/v13/stack_v13.cpp
  BAI/v13/stack_v13.h
  BAI/v13/state_v13.cpp
  BAI/v13/state_v13.h
  BAI/v13/supplementary_data_v13.cpp
  BAI/v13/supplementary_data_v13.h

  BAI/v15/BAI_v15.cpp
  BAI/v15/BAI_v15.h
  BAI/v15/nn_model_v15.h
  BAI/v15/nn_model_v15.cpp
  BAI/v15/attack_log_v15.h
  BAI/v15/encoder_v15.h
  BAI/v15/enum_flags_v15.h
  BAI/v15/fastbfs_v15.h
  BAI/v15/graph/edge_store_v15.h
  BAI/v15/graph/edges/action_ends_at_hex_v15.h
  BAI/v15/graph/edges/base_v15.h
  BAI/v15/graph/edges/generic_v15.h
  BAI/v15/graph/edges/hex_adjacent_hex_v15.h
  BAI/v15/graph/edges/hex_becomes_shoot_target_after_action_v15.h
  BAI/v15/graph/edges/hex_is_end_of_action_v15.h
  BAI/v15/graph/edges/unit_acts_before_unit_v15.h
  BAI/v15/graph/edges/unit_becomes_shoot_target_after_action_v15.h
  BAI/v15/graph/edges/unit_becomes_shoot_threat_after_action_v15.h
  BAI/v15/graph/edges/unit_is_meleed_by_action_v15.h
  BAI/v15/graph/edges/unit_is_shot_by_action_v15.h
  BAI/v15/graph/edges/unit_melee_dmg_unit_v15.cpp
  BAI/v15/graph/edges/unit_melee_dmg_unit_v15.h
  BAI/v15/graph/edges/unit_shoot_dmg_unit_v15.cpp
  BAI/v15/graph/edges/unit_shoot_dmg_unit_v15.h
  BAI/v15/graph/element_v15.h
  BAI/v15/graph/graph_v15.cpp
  BAI/v15/graph/graph_v15.h
  BAI/v15/graph/node_store_v15.h
  BAI/v15/graph/nodes/action_v15.h
  BAI/v15/graph/nodes/base_v15.h
  BAI/v15/graph/nodes/global_v15.cpp
  BAI/v15/graph/nodes/global_v15.h
  BAI/v15/graph/nodes/hex_v15.cpp
  BAI/v15/graph/nodes/hex_v15.h
  BAI/v15/graph/nodes/player_v15.cpp
  BAI/v15/graph/nodes/player_v15.h
  BAI/v15/graph/nodes/unit_v15.cpp
  BAI/v15/graph/nodes/unit_v15.h
  BAI/v15/graph/util_v15.h
  BAI/v15/render_v15.cpp
  BAI/v15/render_v15.h
  BAI/v15/state_v15.cpp
  BAI/v15/state_v15.h
  BAI/v15/supplementary_data_v15.cpp
  BAI/v15/supplementary_data_v15.h
  BAI/v15/verify_v15.cpp
  BAI/v15/verify_v15.h

  schema/base.h
  schema/schema.h
  schema/v13/constants.h
  schema/v13/schema.h
  schema/v13/types.h
  schema/v13/util.h
  schema/v15/constants.h
  schema/v15/encoding.h
  schema/v15/graph.h
  schema/v15/schema.h
  schema/v15/types.h
  schema/v15/util.h

  MMAI.h
  StdInc.h
  common.h
)

option(ENABLE_MMAI_TEST "Compile tests" OFF)

set(MMAI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DUSING_ONNX=1)
assign_source_group(${MMAI_FILES})

add_library(MMAI OBJECT ${MMAI_FILES})
target_compile_definitions(MMAI PRIVATE VCMI_DLL=1)

target_link_libraries(MMAI PRIVATE vcmiMain)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # First try CMake config (works on Fedora, Debian, Ubuntu, Arch)
  find_package(onnxruntime CONFIG QUIET)
  if(onnxruntime_FOUND)
    message(STATUS "Found onnxruntime via CMake config")
    target_link_libraries(MMAI PRIVATE onnxruntime::onnxruntime)
  else()
    # Fallback to manual search for custom installs
    message(STATUS "onnxruntime CMake config not found, trying system installation")
    set(ONNXRUNTIME_ROOT "/opt/onnxruntime" CACHE PATH "Path to onnxruntime containing lib/onnxruntime.so and include/onnxruntime_cxx_api.h")
    message(STATUS "Looking for onnxruntime via find_path/find_library (hint: ${ONNXRUNTIME_ROOT})")
    find_path(ONNXRUNTIME_INCLUDE_DIR
      NAMES onnxruntime_cxx_api.h
      HINTS "${ONNXRUNTIME_ROOT}/include"
      PATH_SUFFIXES
          onnxruntime
          include
          include/onnxruntime
    )

    find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime libonnxruntime
      HINTS "${ONNXRUNTIME_ROOT}/lib"
    )

    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
      message(STATUS "Using onnxruntime:")
      message(STATUS "  include: ${ONNXRUNTIME_INCLUDE_DIR}")
      message(STATUS "  library: ${ONNXRUNTIME_LIBRARY}")
      target_include_directories(MMAI PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
      target_link_libraries(MMAI PRIVATE "${ONNXRUNTIME_LIBRARY}")
    else()
      message(FATAL_ERROR "onnxruntime not found. Install onnxruntime-devel or set ONNXRUNTIME_ROOT to a custom installation path, or disable MMAI with -DENABLE_MMAI=OFF")
    endif()
  endif()
else()
  find_package(onnxruntime CONFIG REQUIRED)
  target_link_libraries(MMAI PRIVATE onnxruntime::onnxruntime)
endif()

# for fallback & spell casting
target_link_libraries(MMAI PRIVATE BattleAI StupidAI)

# Used in schema/base.h to determine if it is imported by MMAI or vcmi-gym
target_compile_definitions(MMAI PRIVATE MMAI_DLL=1)
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
