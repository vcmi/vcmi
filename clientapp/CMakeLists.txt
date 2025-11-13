set(clientapp_SRCS
		StdInc.cpp
		EntryPoint.cpp
)

set(clientapp_HEADERS
		StdInc.h
)

if(IOS)
	set(clientapp_SRCS ${clientapp_SRCS}
		CFocusableHelper.cpp
		ios/GameChatKeyboardHandler.m
		ios/main.m
		ios/startSDL.mm
	)
	set(clientapp_HEADERS ${clientapp_HEADERS}
		CFocusableHelper.h
		ios/GameChatKeyboardHandler.h
		ios/startSDL.h
	)
endif()

assign_source_group(${clientapp_SRCS} ${clientapp_HEADERS})

if(ANDROID)
	add_library(vcmiclient SHARED ${clientapp_SRCS} ${clientapp_HEADERS})
	set_target_properties(vcmiclient PROPERTIES
		OUTPUT_NAME "vcmiclient_${ANDROID_ABI}" # required by Qt
	)
else()
	add_executable(vcmiclient ${clientapp_SRCS} ${clientapp_HEADERS})
	vcmi_create_exe_shim(vcmiclient)
endif()

target_link_libraries(vcmiclient PRIVATE vcmiclientcommon)

if(ENABLE_SINGLE_APP_BUILD AND ENABLE_LAUNCHER)
	target_link_libraries(vcmiclient PRIVATE vcmilauncher)
endif()

target_include_directories(vcmiclient
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	target_sources(vcmiclient PRIVATE "VCMI_client.rc")
	set_target_properties(vcmiclient
		PROPERTIES
			OUTPUT_NAME "VCMI_client"
			PROJECT_LABEL "VCMI_client"
	)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT vcmiclient)
	if(NOT ENABLE_DEBUG_CONSOLE)
		set_target_properties(vcmiclient PROPERTIES WIN32_EXECUTABLE)
		target_link_libraries(vcmiclient SDL2::SDL2main)
	endif()
	target_compile_definitions(vcmiclient PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
elseif(IOS)
	set_target_properties(vcmiclient PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
		XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "$(CODE_SIGNING_ALLOWED_FOR_APPS)"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
	)

	foreach(XCODE_RESOURCE LaunchScreen.storyboard Images.xcassets Settings.bundle vcmi_logo.png)
		set(XCODE_RESOURCE_PATH ios/${XCODE_RESOURCE})
		target_sources(vcmiclient PRIVATE ${XCODE_RESOURCE_PATH})
		set_source_files_properties(${XCODE_RESOURCE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

		# workaround to prevent CMAKE_SKIP_PRECOMPILE_HEADERS being added as compile flag
		if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22.0" AND CMAKE_VERSION VERSION_LESS "3.25.0")
			set_source_files_properties(${XCODE_RESOURCE_PATH} PROPERTIES LANGUAGE CXX)
		endif()
	endforeach()

	set(CMAKE_EXE_LINKER_FLAGS "-Wl,-e,_client_main")
endif()

vcmi_set_output_dir(vcmiclient "")
enable_pch(vcmiclient)

if(IOS)
	add_custom_command(TARGET vcmiclient POST_BUILD
		COMMAND ios/set_build_version.sh "$<TARGET_BUNDLE_CONTENT_DIR:vcmiclient>"
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --component "${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}" --config "$<CONFIG>" --prefix "$<TARGET_BUNDLE_CONTENT_DIR:vcmiclient>"
		COMMAND ios/rpath_remove_symlinks.sh
		COMMAND ios/codesign.sh
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	install(TARGETS vcmiclient DESTINATION Payload COMPONENT app) # for ipa generation with cpack
elseif(ANDROID)
	find_program(androidDeployQt androiddeployqt
		PATHS "${qtBinDir}"
	)
	add_custom_target(android_deploy ALL
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --prefix "${androidQtBuildDir}"
		COMMAND ${CMAKE_COMMAND} -E env "ORG_GRADLE_PROJECT_SDL_JAVA_SRC_DIR=${SDL_JAVA_SRC_DIR}" --
			"${androidDeployQt}" --input "${CMAKE_BINARY_DIR}/androiddeployqt.json" --output "${androidQtBuildDir}" --android-platform "android-${ANDROID_TARGET_SDK_VERSION}" --verbose $<$<NOT:$<CONFIG:Debug>>:--release> ${ANDROIDDEPLOYQT_OPTIONS}
		COMMAND_EXPAND_LISTS
		VERBATIM
		COMMENT "Create android package"
	)
	add_dependencies(android_deploy vcmiclient)
else()
	install(TARGETS vcmiclient DESTINATION ${BIN_DIR})
endif()

#install icons and desktop file on Linux
if(NOT WIN32 AND NOT APPLE AND NOT ANDROID)
	foreach(iconSize 16 22 32 48 64 128 256 512 1024 2048)
		install(FILES "icons/vcmiclient.${iconSize}x${iconSize}.png"
			DESTINATION "share/icons/hicolor/${iconSize}x${iconSize}/apps"
			RENAME vcmiclient.png
		)
	endforeach()

	install(FILES icons/vcmiclient.svg
		DESTINATION share/icons/hicolor/scalable/apps
		RENAME vcmiclient.svg
	)
	install(FILES icons/vcmiclient.desktop
		DESTINATION share/applications
	)
endif()
