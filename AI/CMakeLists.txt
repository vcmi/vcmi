option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" OFF)

if(NOT FORCE_BUNDLED_FL)
	find_package(FuzzyLite)
else()
	set(FL_FOUND FALSE)
endif()

if(NOT FL_FOUND)
    set(FL_BUILD_BINARY OFF CACHE BOOL "")
    set(FL_BUILD_SHARED OFF CACHE BOOL "")
	set(FL_BUILD_TESTS OFF CACHE BOOL "")
	add_subdirectory_with_folder("AI" FuzzyLite/fuzzylite EXCLUDE_FROM_ALL)
endif()

add_subdirectory_with_folder("AI" BattleAI)
add_subdirectory_with_folder("AI" StupidAI)
add_subdirectory_with_folder("AI" EmptyAI)
add_subdirectory_with_folder("AI" VCAI)
