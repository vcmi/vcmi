set(stupidAI_SRCS
		StupidAI.cpp
)

set(stupidAI_HEADERS
		StdInc.h

		StupidAI.h
)

assign_source_group(${stupidAI_SRCS} ${stupidAI_HEADERS})

add_library(StupidAI OBJECT ${stupidAI_SRCS} ${stupidAI_HEADERS})
target_compile_definitions(StupidAI PRIVATE VCMI_DLL=1)

target_link_libraries(StupidAI PRIVATE vcmiMain)
target_include_directories(StupidAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(StupidAI "AI")
enable_pch(StupidAI)
