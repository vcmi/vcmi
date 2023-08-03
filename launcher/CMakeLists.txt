set(launcher_SRCS
		StdInc.cpp
		aboutProject/aboutproject_moc.cpp
		modManager/cdownloadmanager_moc.cpp
		modManager/cmodlist.cpp
		modManager/cmodlistmodel_moc.cpp
		modManager/cmodlistview_moc.cpp
		modManager/cmodmanager.cpp
		modManager/imageviewer_moc.cpp
		settingsView/csettingsview_moc.cpp
		firstLaunch/firstlaunch_moc.cpp
		main.cpp
		mainwindow_moc.cpp
		languages.cpp
		launcherdirs.cpp
		jsonutils.cpp
		updatedialog_moc.cpp
		lobby/lobby.cpp
		lobby/lobby_moc.cpp
		lobby/lobbyroomrequest_moc.cpp
)

set(launcher_HEADERS
		StdInc.h
		aboutProject/aboutproject_moc.h
		modManager/cdownloadmanager_moc.h
		modManager/cmodlist.h
		modManager/cmodlistmodel_moc.h
		modManager/cmodlistview_moc.h
		modManager/cmodmanager.h
		modManager/imageviewer_moc.h
		settingsView/csettingsview_moc.h
		firstLaunch/firstlaunch_moc.h
		mainwindow_moc.h
		languages.h
		launcherdirs.h
		jsonutils.h
		updatedialog_moc.h
		lobby/lobby.h
		lobby/lobby_moc.h
		lobby/lobbyroomrequest_moc.h
		main.h
)

set(launcher_FORMS
		aboutProject/aboutproject_moc.ui
		modManager/cmodlistview_moc.ui
		modManager/imageviewer_moc.ui
		settingsView/csettingsview_moc.ui
		firstLaunch/firstlaunch_moc.ui
		mainwindow_moc.ui
		updatedialog_moc.ui
		lobby/lobby_moc.ui
		lobby/lobbyroomrequest_moc.ui
)

set(launcher_TS
	translation/chinese.ts
	translation/english.ts
	translation/french.ts
	translation/german.ts
	translation/polish.ts
	translation/russian.ts
	translation/spanish.ts
	translation/ukrainian.ts
)

if(APPLE_IOS)
	list(APPEND launcher_SRCS
		ios/main.m
	)
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
	if(ENABLE_TRANSLATIONS)
		set_source_files_properties(${launcher_TS} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translation)
		qt5_add_translation( launcher_QM ${launcher_TS} )
	endif()
endif()


if(WIN32)
	set(launcher_ICON VCMI_launcher.rc)
endif()

if(ENABLE_SINGLE_APP_BUILD)
	add_library(vcmilauncher STATIC ${launcher_QM} ${launcher_SRCS} ${launcher_HEADERS} ${launcher_UI_HEADERS})
else()
	add_executable(vcmilauncher WIN32 ${launcher_QM} ${launcher_SRCS} ${launcher_HEADERS} ${launcher_UI_HEADERS} ${launcher_ICON})
endif()

if(TARGET Qt6::Core)
	if(ENABLE_TRANSLATIONS)
		set_source_files_properties(${launcher_TS} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translation)
		qt_add_translations(vcmilauncher
			TS_FILES ${launcher_TS}
			QM_FILES_OUTPUT_VARIABLE launcher_QM
			INCLUDE_DIRECTORIES
				${CMAKE_CURRENT_BINARY_DIR})
	endif()
endif()

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

if (NOT APPLE_IOS AND NOT ANDROID)
	target_link_libraries(vcmilauncher SDL2::SDL2)
endif()

target_link_libraries(vcmilauncher ${VCMI_LIB_TARGET} Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(vcmilauncher
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
)
vcmi_set_output_dir(vcmilauncher "")
enable_pch(vcmilauncher)

if(APPLE_IOS)
	set(RESOURCES_DESTINATION ${DATA_DIR})

	# TODO: remove after fixing Conan's Qt recipe
	if(XCODE_VERSION VERSION_GREATER_EQUAL 14.0)
		target_link_libraries(vcmilauncher "-framework IOKit")
	endif()

	# workaround https://github.com/conan-io/conan-center-index/issues/13332
	if(USING_CONAN)
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/QIOSIntegrationPlugin.h
			"#include <QtPlugin>\nQ_IMPORT_PLUGIN(QIOSIntegrationPlugin)"
		)
		# target_include_directories(vcmilauncher PRIVATE ${CMAKE_BINARY_DIR})
		target_link_libraries(vcmilauncher
			Qt${QT_VERSION_MAJOR}::QIOSIntegrationPlugin
			qt::QIOSIntegrationPlugin
		)
	endif()
else()
	set(RESOURCES_DESTINATION ${DATA_DIR}/launcher)

	# Copy to build directory for easier debugging
	add_custom_command(TARGET vcmilauncher POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/launcher
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/launcher/icons ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/launcher/icons
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/translation ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/launcher/translation

	)

	install(TARGETS vcmilauncher DESTINATION ${BIN_DIR})

	# Install icons and desktop file on Linux
	if(NOT WIN32 AND NOT APPLE)
		install(FILES "vcmilauncher.desktop" DESTINATION share/applications)
		install(FILES "eu.vcmi.VCMI.metainfo.xml" DESTINATION share/metainfo)
	endif()
endif()

install(DIRECTORY icons DESTINATION ${RESOURCES_DESTINATION})
if(ENABLE_TRANSLATIONS)
	install(FILES ${launcher_QM} DESTINATION ${RESOURCES_DESTINATION}/translation)
endif()
