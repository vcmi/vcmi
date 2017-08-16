include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(emptyAI_SRCS
		StdInc.cpp

		CEmptyAI.cpp
		exp_funcs.cpp
)

set(emptyAI_HEADERS
		StdInc.h

		CEmptyAI.h
)

assign_source_group(${emptyAI_SRCS} ${emptyAI_HEADERS})

add_library(EmptyAI SHARED ${emptyAI_SRCS} ${emptyAI_HEADERS})
target_link_libraries(EmptyAI vcmi)

vcmi_set_output_dir(EmptyAI "AI")

set_target_properties(EmptyAI PROPERTIES ${PCH_PROPERTIES})

install(TARGETS EmptyAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
