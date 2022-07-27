set(stupidAI_SRCS
		StdInc.cpp

		main.cpp
		StupidAI.cpp
)

set(stupidAI_HEADERS
		StdInc.h

		StupidAI.h
)

assign_source_group(${stupidAI_SRCS} ${stupidAI_HEADERS})

add_library(StupidAI SHARED ${stupidAI_SRCS} ${stupidAI_HEADERS})
target_link_libraries(StupidAI PRIVATE ${VCMI_LIB_TARGET})
target_include_directories(StupidAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(StupidAI "AI")
enable_pch(StupidAI)

install(TARGETS StupidAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR} COMPONENT AI)
