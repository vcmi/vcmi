if(FL_FOUND)
	include_directories(${FL_INCLUDE_DIRS})
else()
	include_directories(${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite/fuzzylite)
endif()
include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(VCAI_SRCS
		StdInc.cpp

		AIUtility.cpp
		Fuzzy.cpp
		Goals.cpp
		main.cpp
		VCAI.cpp
)

set(VCAI_HEADERS
		StdInc.h

		AIUtility.h
		Fuzzy.h
		Goals.h
		VCAI.h
)

assign_source_group(${VCAI_SRCS} ${VCAI_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(VCAI SHARED ${VCAI_SRCS} ${VCAI_HEADERS})
if(FL_FOUND)
	target_link_libraries(VCAI ${FL_LIBRARIES} vcmi)
else()
	target_link_libraries(VCAI fl-static vcmi)
endif()

vcmi_set_output_dir(VCAI "AI")

set_target_properties(VCAI PROPERTIES ${PCH_PROPERTIES})
cotire(VCAI)

install(TARGETS VCAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
