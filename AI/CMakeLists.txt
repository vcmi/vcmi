#######################################
#        FuzzyLite support            #
#######################################

if(NOT WIN32 AND NOT APPLE)
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" ON)
else()
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" OFF)
endif()

if(NOT FORCE_BUNDLED_FL)
	find_package(FuzzyLite)
else()
	set(FL_FOUND FALSE)
endif()

if(NOT FL_FOUND)
    set(FL_BUILD_BINARY OFF CACHE BOOL "")
    set(FL_BUILD_SHARED OFF CACHE BOOL "")
	set(FL_BUILD_TESTS OFF CACHE BOOL "")
	add_subdirectory(FuzzyLite/fuzzylite EXCLUDE_FROM_ALL)
endif()

#######################################
#        Add subdirectories           #
#######################################

add_subdirectory(BattleAI)
add_subdirectory(StupidAI)
add_subdirectory(EmptyAI)
add_subdirectory(VCAI)
