set(stupidAI_SRCS
		StupidAI.cpp
)

set(stupidAI_HEADERS
		StdInc.h

		StupidAI.h
)

if(NOT ENABLE_STATIC_AI_LIBS)
	list(APPEND stupidAI_SRCS main.cpp StdInc.cpp)
endif()
assign_source_group(${stupidAI_SRCS} ${stupidAI_HEADERS})

if(ENABLE_STATIC_AI_LIBS)
	add_library(StupidAI STATIC ${stupidAI_SRCS} ${stupidAI_HEADERS})
else()
	add_library(StupidAI SHARED ${stupidAI_SRCS} ${stupidAI_HEADERS})
	install(TARGETS StupidAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()

target_link_libraries(StupidAI PRIVATE ${VCMI_LIB_TARGET})
target_include_directories(StupidAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(StupidAI "AI")
enable_pch(StupidAI)
