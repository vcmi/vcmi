#######################################
#        FuzzyLite support            #
#######################################

if(NOT WIN32 AND NOT APPLE)
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" ON)
else()
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" OFF)
endif()

if(TBB_FOUND AND MSVC)
	   install_vcpkg_imported_tgt(TBB::tbb)
endif()

#FuzzyLite uses MSVC pragmas in headers, so, we need to disable -Wunknown-pragmas
if(MINGW)
    add_compile_options(-Wno-unknown-pragmas)
endif()

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
	if(ANDROID)
		set(FL_BACKTRACE OFF CACHE BOOL "" FORCE)
	endif()
	#It is for compiling FuzzyLite, it will not compile without it on GCC
	if("x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xGNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		add_compile_options(-Wno-error=deprecated-declarations)
	endif()
	add_subdirectory(FuzzyLite/fuzzylite EXCLUDE_FROM_ALL)
	add_library(fuzzylite::fuzzylite ALIAS fl-static)
	target_include_directories(fl-static PUBLIC ${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite/fuzzylite)
endif()

#######################################
#        Add subdirectories           #
#######################################

add_subdirectory(BattleAI)
add_subdirectory(VCAI)
if(NOT ANDROID)
	add_subdirectory(StupidAI)
	add_subdirectory(EmptyAI)
endif()
if(ENABLE_NULLKILLER_AI)
	add_subdirectory(Nullkiller)
endif()
