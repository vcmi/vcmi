set(server_SRCS
		StdInc.cpp

		CGameHandler.cpp
		CQuery.cpp
		CVCMIServer.cpp
		NetPacksServer.cpp
		NetPacksLobbyServer.cpp
)

set(server_HEADERS
		StdInc.h

		CGameHandler.h
		CQuery.h
		CVCMIServer.h
)

if(APPLE_IOS)
	set(server_SRCS ${server_SRCS} ios/main.mm)
endif(APPLE_IOS)

assign_source_group(${server_SRCS} ${server_HEADERS})

if(ANDROID) # android needs client/server to be libraries, not executables, so we can't reuse the build part of this script
	return()
endif()

if(BUILD_SINGLE_APP)
	add_library(vcmiserver STATIC ${server_SRCS} ${server_HEADERS})
	set(server_LIBS vcmi_lib_server)
else()
	add_executable(vcmiserver ${server_SRCS} ${server_HEADERS})
	set(server_LIBS vcmi)
endif()

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
	set(server_LIBS execinfo ${server_LIBS})
elseif(APPLE_IOS)
	set(server_LIBS ${server_LIBS} "-framework UIKit")
endif()
target_link_libraries(vcmiserver PRIVATE ${server_LIBS} minizip::minizip)

target_include_directories(vcmiserver
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	set_target_properties(vcmiserver
		PROPERTIES
			OUTPUT_NAME "VCMI_server"
			PROJECT_LABEL "VCMI_server"
	)
elseif(APPLE_IOS)
	set(ENTITLEMENTS_OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/VCMI.entitlements)
	configure_file(ios/Entitlements.in ${ENTITLEMENTS_OUT_PATH} @ONLY)

	set_target_properties(vcmiserver PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
		XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "$(CODE_SIGNING_ALLOWED_FOR_APPS)"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
		XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${ENTITLEMENTS_OUT_PATH}
	)
	# TODO: move to a common dir / add macro?
	target_sources(vcmiserver PRIVATE ${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	target_sources(vcmiserver PRIVATE ios/Images.xcassets)
	set_source_files_properties(ios/Images.xcassets PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

vcmi_set_output_dir(vcmiserver "")
enable_pch(vcmiserver)

if(APPLE_IOS)
	# TODO: move to a common dir / add macro?
	add_custom_command(TARGET vcmiserver POST_BUILD
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --component core --prefix "$<TARGET_BUNDLE_CONTENT_DIR:vcmiserver>"
		COMMAND ${CMAKE_SOURCE_DIR}/apple_codesign.sh
	)
else()
	install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
endif()
