set(launcher_modmanager_SRCS
		modManager/cdownloadmanager_moc.cpp
		modManager/cmodlist.cpp
		modManager/cmodlistmodel_moc.cpp
		modManager/cmodlistview_moc.cpp
		modManager/cmodmanager.cpp
		modManager/imageviewer_moc.cpp
)

set(launcher_modmanager_HEADERS
		modManager/cdownloadmanager_moc.h
		modManager/cmodlist.h
		modManager/cmodlistmodel_moc.h
		modManager/cmodlistview_moc.h
		modManager/cmodmanager.h
		modManager/imageviewer_moc.h
)

set(launcher_settingsview_SRCS
		settingsView/csettingsview_moc.cpp
)

set(launcher_settingsview_HEADERS
		settingsView/csettingsview_moc.h
)

set(launcher_SRCS
		StdInc.cpp
		${launcher_modmanager_SRCS}
		${launcher_settingsview_SRCS}
		main.cpp
		mainwindow_moc.cpp
		launcherdirs.cpp
		jsonutils.cpp
		updatedialog_moc.cpp
)

set(launcher_HEADERS
		StdInc.h
		${launcher_modmanager_HEADERS}
		${launcher_settingsview_HEADERS}
		mainwindow_moc.h
		launcherdirs.h
		jsonutils.h
		updatedialog_moc.h
)

set(launcher_FORMS
		modManager/cmodlistview_moc.ui
		modManager/imageviewer_moc.ui
		settingsView/csettingsview_moc.ui
		mainwindow_moc.ui
		updatedialog_moc.ui
)

if(APPLE_IOS)
	set(launcher_SRCS ${launcher_SRCS} ios/mainwindow_moc.mm)
endif()

assign_source_group(${launcher_SRCS} ${launcher_HEADERS} VCMI_launcher.rc)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

if(POLICY CMP0071)
	cmake_policy(SET CMP0071 NEW)
endif()

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(TARGET Qt6::Core)
	qt_wrap_ui(launcher_UI_HEADERS ${launcher_FORMS})
else()
	qt5_wrap_ui(launcher_UI_HEADERS ${launcher_FORMS})
endif()

if(WIN32)
	set(launcher_ICON VCMI_launcher.rc)
endif()

add_executable(vcmilauncher WIN32 ${launcher_SRCS} ${launcher_HEADERS} ${launcher_UI_HEADERS} ${launcher_ICON})

if(WIN32)
	set_target_properties(vcmilauncher
		PROPERTIES
			OUTPUT_NAME "VCMI_launcher"
			PROJECT_LABEL "VCMI_launcher"
	)

	# FIXME: Can't to get CMP0020 working with Vcpkg and CMake 3.8.2
	# So far I tried:
	# - cmake_minimum_required set to 2.8.11 globally and in this file
	# - cmake_policy in all possible places
	# - used NO_POLICY_SCOPE to make sure no other parts reset policies
	# Still nothing worked, warning kept appearing and WinMain didn't link automatically
	target_link_libraries(vcmilauncher Qt${QT_VERSION_MAJOR}::WinMain)
endif()

if(APPLE)
	# This makes Xcode project prettier by moving vcmilauncher_autogen directory into vcmiclient subfolder
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER vcmilauncher)
endif()
if(APPLE_IOS)
	set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,-e,_qt_main_wrapper")

	set(ENTITLEMENTS_OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/VCMI.entitlements)
	configure_file(ios/Entitlements.in ${ENTITLEMENTS_OUT_PATH} @ONLY)

	set_target_properties(vcmilauncher PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
		XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "$(CODE_SIGNING_ALLOWED_FOR_APPS)"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
		XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${ENTITLEMENTS_OUT_PATH}
	)

	target_sources(vcmilauncher PRIVATE ${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	target_sources(vcmilauncher PRIVATE ios/Images.xcassets)
	set_source_files_properties(ios/Images.xcassets PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

target_link_libraries(vcmilauncher vcmi Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(vcmilauncher
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
)
vcmi_set_output_dir(vcmilauncher "")
enable_pch(vcmilauncher)

if(APPLE_IOS)
	install(DIRECTORY icons DESTINATION ${DATA_DIR})
	add_custom_command(TARGET vcmilauncher POST_BUILD
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --prefix "$<TARGET_BUNDLE_CONTENT_DIR:vcmilauncher>"
		COMMAND ${CMAKE_SOURCE_DIR}/apple_codesign.sh
	)
else()
	# Copy to build directory for easier debugging
	add_custom_command(TARGET vcmilauncher POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/launcher/icons
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/launcher/icons ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/launcher/icons
	)

	install(TARGETS vcmilauncher DESTINATION ${BIN_DIR})
	# copy whole directory
	install(DIRECTORY icons DESTINATION ${DATA_DIR}/launcher)
	# Install icons and desktop file on Linux
	if(NOT WIN32 AND NOT APPLE)
		install(FILES "vcmilauncher.desktop" DESTINATION share/applications)
		install(FILES "eu.vcmi.VCMI.metainfo.xml" DESTINATION share/metainfo)
	endif()
endif()
