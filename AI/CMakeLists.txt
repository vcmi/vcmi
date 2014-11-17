project(AI)
cmake_minimum_required(VERSION 2.8)

find_package(Fuzzylite)

if(NOT MSVC)
    add_definitions(-DFL_CPP11)
    set(FL_CPP11 ON CACHE BOOL "")
endif()

if (NOT FL_FOUND)
    set(FL_BUILD_BINARY OFF CACHE BOOL "")
    set(FL_BUILD_SHARED OFF CACHE BOOL "")
    add_subdirectory(FuzzyLite/fuzzylite)
endif()
add_subdirectory(BattleAI)
add_subdirectory(StupidAI)
add_subdirectory(EmptyAI)
add_subdirectory(VCAI)