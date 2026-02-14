set(lobby_SRCS
		StdInc.cpp

		EntryPoint.cpp
		HttpApiServer.cpp
		LobbyDatabase.cpp
		LobbyServer.cpp
		SQLiteConnection.cpp
)

set(lobby_HEADERS
		StdInc.h

		HttpApiServer.h
		LobbyDatabase.h
		LobbyDefines.h
		LobbyServer.h
		SQLiteConnection.h
)

# Generate header file from web assets
set(WEB_ASSETS
	"${CMAKE_CURRENT_SOURCE_DIR}/web/openapi.yaml"
	"${CMAKE_CURRENT_SOURCE_DIR}/web/swagger.html"
)
set(WEB_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedWebAssets.h")

# Convert list to semicolon-separated string for passing to script
string(REPLACE ";" "|" WEB_ASSETS_STRING "${WEB_ASSETS}")

add_custom_command(
	OUTPUT "${WEB_HEADER_FILE}"
	COMMAND ${CMAKE_COMMAND}
		-DINPUT_FILES=${WEB_ASSETS_STRING}
		-DOUTPUT_FILE=${WEB_HEADER_FILE}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/embed_file.cmake
	DEPENDS ${WEB_ASSETS}
	COMMENT "Embedding web assets into C++ header"
	VERBATIM
)
list(APPEND lobby_HEADERS "${WEB_HEADER_FILE}")

assign_source_group(${lobby_SRCS} ${lobby_HEADERS})

add_executable(vcmilobby ${lobby_SRCS} ${lobby_HEADERS})
set(lobby_LIBS vcmi)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(lobby_LIBS execinfo ${lobby_LIBS})
endif()

target_link_libraries(vcmilobby PRIVATE ${lobby_LIBS} ${SQLite3_LIBRARIES})

target_include_directories(vcmilobby PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(vcmilobby PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vcmilobby PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

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

