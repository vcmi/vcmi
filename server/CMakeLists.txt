project(vcmiserver)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})

set(server_SRCS
		StdInc.cpp
		CGameHandler.cpp
		CVCMIServer.cpp
		CQuery.cpp
		NetPacksServer.cpp
)

add_executable(vcmiserver ${server_SRCS})

target_link_libraries(vcmiserver vcmi ${Boost_LIBRARIES} ${SYSTEM_LIBS})

if(WIN32)
	set_target_properties(vcmiserver PROPERTIES OUTPUT_NAME VCMI_server)
endif()

set_target_properties(vcmiserver PROPERTIES ${PCH_PROPERTIES})
set_target_properties(vcmiserver PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/")
cotire(vcmiserver)

if (NOT APPLE) # Already inside vcmiclient bundle
	install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
endif()

