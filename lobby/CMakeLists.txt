set(lobby_SRCS
		StdInc.cpp

		EntryPoint.cpp
		LobbyDatabase.cpp
		LobbyServer.cpp
		SQLiteConnection.cpp
)

set(lobby_HEADERS
		StdInc.h

		LobbyDatabase.h
		LobbyDefines.h
		LobbyServer.h
		SQLiteConnection.h
)

assign_source_group(${lobby_SRCS} ${lobby_HEADERS})

add_executable(vcmilobby ${lobby_SRCS} ${lobby_HEADERS})
set(lobby_LIBS vcmi)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(lobby_LIBS execinfo ${lobby_LIBS})
endif()

target_link_libraries(vcmilobby PRIVATE ${lobby_LIBS} ${SQLite3_LIBRARIES})

target_include_directories(vcmilobby PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(vcmilobby PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	set_target_properties(vcmilobby
		PROPERTIES
			OUTPUT_NAME "VCMI_lobby"
			PROJECT_LABEL "VCMI_lobby"
	)
endif()

vcmi_set_output_dir(vcmilobby "")
enable_pch(vcmilobby)

install(TARGETS vcmilobby DESTINATION ${BIN_DIR})

