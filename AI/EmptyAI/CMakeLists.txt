set(emptyAI_SRCS
		CEmptyAI.cpp
)

set(emptyAI_HEADERS
		StdInc.h

		CEmptyAI.h
)

if(NOT ENABLE_STATIC_AI_LIBS)
	list(APPEND emptyAI_SRCS main.cpp StdInc.cpp)
endif()
assign_source_group(${emptyAI_SRCS} ${emptyAI_HEADERS})

if(ENABLE_STATIC_AI_LIBS)
	add_library(EmptyAI STATIC ${emptyAI_SRCS} ${emptyAI_HEADERS})
else()
	add_library(EmptyAI SHARED ${emptyAI_SRCS} ${emptyAI_HEADERS})
	install(TARGETS EmptyAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()

target_include_directories(EmptyAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EmptyAI PRIVATE ${VCMI_LIB_TARGET})

vcmi_set_output_dir(EmptyAI "AI")
enable_pch(EmptyAI)
