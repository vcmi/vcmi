#######################################
#        FuzzyLite support            #
#######################################

option(ENABLE_NULLKILLER_AI "Enable compilation of Nullkiller AI library" ON)
if(NOT WIN32 AND NOT APPLE)
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" ON)
else()
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" OFF)
endif()

if(ENABLE_NULLKILLER_AI)
	find_package(TBB REQUIRED)
	if(MSVC)
		install_vcpkg_imported_tgt(TBB::tbb)
	endif()
end()


if(NOT FORCE_BUNDLED_FL)
	find_package(fuzzylite)
else()
	set(fuzzylite_FOUND FALSE)
endif()

if(TARGET fuzzylite::fuzzylite AND MSVC)
	install_vcpkg_imported_tgt(fuzzylite::fuzzylite)
endif()

if(NOT fuzzylite_FOUND)
    set(FL_BUILD_BINARY OFF CACHE BOOL "")
    set(FL_BUILD_SHARED OFF CACHE BOOL "")
	set(FL_BUILD_TESTS OFF CACHE BOOL "")
	add_subdirectory(FuzzyLite/fuzzylite EXCLUDE_FROM_ALL)
	add_library(fuzzylite::fuzzylite ALIAS fl-static)
	target_include_directories(fl-static PUBLIC ${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite/fuzzylite)
endif()

#######################################
#        Add subdirectories           #
#######################################

add_subdirectory(BattleAI)
add_subdirectory(StupidAI)
add_subdirectory(EmptyAI)
add_subdirectory(VCAI)
if(ENABLE_NULLKILLER_AI)
	add_subdirectory(Nullkiller)
endif()
