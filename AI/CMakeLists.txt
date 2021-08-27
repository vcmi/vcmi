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

if(TBB_FOUND AND MSVC)
	get_target_property(TBB_LIB_LOCATION TBB::tbb LOCATION)
	get_filename_component(TBB_LIB_FOLDER ${TBB_LIB_LOCATION} PATH)
	get_filename_component(TBB_DLL ${TBB_LIB_FOLDER}/../bin/tbb.dll ABSOLUTE)
	message("${TBB_DLL}")
	install(FILES ${TBB_DLL} DESTINATION ${BIN_DIR})
endif()

if(FL_FOUND)
	if(MSVC)
		get_filename_component(FL_LIB_FOLDER ${FL_LIBRARIES} PATH)
		get_filename_component(FL_DLL ${FL_LIB_FOLDER}/../bin/fuzzylite.dll ABSOLUTE)
		message("${FL_DLL}")
		install(FILES ${FL_DLL} DESTINATION ${BIN_DIR})
	endif()
else()
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
add_subdirectory(Nullkiller)
