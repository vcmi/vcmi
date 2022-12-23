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
target_include_directories(EmptyAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EmptyAI PRIVATE ${VCMI_LIB_TARGET})

vcmi_set_output_dir(EmptyAI "AI")
enable_pch(EmptyAI)

install(TARGETS EmptyAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR} OPTIONAL)
