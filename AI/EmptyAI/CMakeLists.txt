set(emptyAI_SRCS
		CEmptyAI.cpp
)

set(emptyAI_HEADERS
		StdInc.h

		CEmptyAI.h
)

assign_source_group(${emptyAI_SRCS} ${emptyAI_HEADERS})

add_library(EmptyAI OBJECT ${emptyAI_SRCS} ${emptyAI_HEADERS})
target_compile_definitions(EmptyAI PRIVATE VCMI_DLL=1)

target_include_directories(EmptyAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EmptyAI PRIVATE vcmiMain)

vcmi_set_output_dir(EmptyAI "AI")
enable_pch(EmptyAI)
