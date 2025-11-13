if(ENABLE_INNOEXTRACT)
	# libiconv not required for our use case
	set(WITH_CONV "builtin" CACHE STRING "" FORCE)
	add_subdirectory("lib/innoextract")
endif()

set(launcher_SRCS
		StdInc.cpp
		aboutProject/aboutproject_moc.cpp
		modManager/cdownloadmanager_moc.cpp
		modManager/modstateitemmodel_moc.cpp
		modManager/cmodlistview_moc.cpp
		modManager/modstatecontroller.cpp
		modManager/modstatemodel.cpp
		modManager/modstate.cpp
		modManager/imageviewer_moc.cpp
		modManager/chroniclesextractor.cpp
		settingsView/csettingsview_moc.cpp
		settingsView/configeditordialog_moc.cpp
		startGame/StartGameTab.cpp
		firstLaunch/firstlaunch_moc.cpp
		main.cpp
		helper.cpp
		innoextract.cpp
		mainwindow_moc.cpp
		languages.cpp
		updatedialog_moc.cpp
		prepare.cpp
)
if(IOS)
	list(APPEND launcher_SRCS
		ios/launchGame.m
		ios/revealdirectoryinfiles.h
		ios/revealdirectoryinfiles.mm
		ios/selectdirectory.h
		ios/selectdirectory.mm
		prepare_ios.mm
	)
elseif(ANDROID)
	list(APPEND launcher_SRCS
		prepare_android.cpp
	)
endif()

set(launcher_HEADERS
		StdInc.h
		aboutProject/aboutproject_moc.h
		modManager/cdownloadmanager_moc.h
		modManager/modstateitemmodel_moc.h
		modManager/cmodlistview_moc.h
		modManager/modstatecontroller.h
		modManager/modstatemodel.h
		modManager/modstate.h
		modManager/imageviewer_moc.h
		modManager/chroniclesextractor.h
		settingsView/configeditordialog_moc.h
		settingsView/csettingsview_moc.h
		startGame/StartGameTab.h
		firstLaunch/firstlaunch_moc.h
		mainwindow_moc.h
		languages.h
		updatedialog_moc.h
		main.h
		helper.h
		innoextract.h
		prepare.h
		prepare_p.h
)

set(launcher_FORMS
		aboutProject/aboutproject_moc.ui
		modManager/cmodlistview_moc.ui
		modManager/imageviewer_moc.ui
		settingsView/configeditordialog_moc.ui
		settingsView/csettingsview_moc.ui
		firstLaunch/firstlaunch_moc.ui
		mainwindow_moc.ui
		startGame/StartGameTab.ui
		updatedialog_moc.ui
)

set(launcher_RESOURCES
	resources.qrc
)

set(translationsDir "translation")
set(launcher_TS
	"${translationsDir}/belarusian.ts"
	"${translationsDir}/bulgarian.ts"
	"${translationsDir}/chinese.ts"
	"${translationsDir}/czech.ts"
	"${translationsDir}/english.ts"
	"${translationsDir}/finnish.ts"
	"${translationsDir}/french.ts"
	"${translationsDir}/german.ts"
	"${translationsDir}/greek.ts"
	"${translationsDir}/hungarian.ts"
	"${translationsDir}/italian.ts"
	"${translationsDir}/japanese.ts"
	"${translationsDir}/korean.ts"
	"${translationsDir}/norwegian.ts"
	"${translationsDir}/polish.ts"
	"${translationsDir}/portuguese.ts"
	"${translationsDir}/romanian.ts"
	"${translationsDir}/russian.ts"
	"${translationsDir}/spanish.ts"
	"${translationsDir}/swedish.ts"
	"${translationsDir}/turkish.ts"
	"${translationsDir}/ukrainian.ts"
	"${translationsDir}/vietnamese.ts"
)
if(ENABLE_TRANSLATIONS)
	if(TARGET Qt5::Core)
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${translationsDir}")
		set_source_files_properties(${launcher_TS} PROPERTIES OUTPUT_LOCATION "${translationsDir}")
		qt5_add_translation(launcher_QM ${launcher_TS})

		set(translationsResource "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
		list(APPEND launcher_RESOURCES "${translationsResource}")

		set(rccQmFiles "")
		foreach(qmFile ${launcher_QM})
			string(APPEND rccQmFiles "<file>${qmFile}</file>\n")
		endforeach()
		file(WRITE "${translationsResource}"
"<!DOCTYPE RCC>
<RCC version=\"1.0\">
<qresource prefix=\"/\">
${rccQmFiles}
</qresource>
</RCC>"
		)
	endif()
endif()

if(WIN32)
	set(launcher_ICON VCMI_launcher.rc)
endif()

assign_source_group(${launcher_SRCS} ${launcher_HEADERS} ${launcher_RESOURCES} ${launcher_TS} ${launcher_ICON})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(POLICY CMP0071)
	cmake_policy(SET CMP0071 NEW)
endif()

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(ENABLE_SINGLE_APP_BUILD OR ANDROID)
	add_library(vcmilauncher OBJECT ${launcher_QM})
else()
	add_executable(vcmilauncher WIN32 ${launcher_QM} ${launcher_ICON})
	vcmi_create_exe_shim(vcmilauncher)
endif()

if(ENABLE_TRANSLATIONS)
	if(TARGET Qt6::Core)
		qt_add_translations(vcmilauncher
			TS_FILES ${launcher_TS}
			RESOURCE_PREFIX "/${translationsDir}"
			INCLUDE_DIRECTORIES
				${CMAKE_CURRENT_BINARY_DIR})
	endif()
endif()

if(ANDROID)
	get_target_property(rccPath Qt${QT_VERSION_MAJOR}::rcc IMPORTED_LOCATION)
	get_filename_component(qtDir "${rccPath}/../../" ABSOLUTE)
	set(qtDir "${qtDir}" PARENT_SCOPE)
 
	function(generate_binary_resource resourceName resourceDir)
		file(CREATE_LINK "${resourceDir}" "${CMAKE_CURRENT_BINARY_DIR}/${resourceName}"
			COPY_ON_ERROR
			SYMBOLIC
		)
		set(qrcFile "${CMAKE_CURRENT_BINARY_DIR}/${resourceName}.qrc")
		execute_process(COMMAND
			"${rccPath}" --project
			WORKING_DIRECTORY "${resourceDir}"
			OUTPUT_VARIABLE rccOutput
		)
		# add parent directory
		string(REPLACE "<file>." "<file>${resourceName}" rccOutput "${rccOutput}")
		file(WRITE "${qrcFile}" "${rccOutput}")
	endfunction()
 
	generate_binary_resource("config" "${CMAKE_SOURCE_DIR}/config")
	list(APPEND launcher_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/config.qrc")
	generate_binary_resource("Mods" "${CMAKE_SOURCE_DIR}/Mods")
	list(APPEND launcher_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/Mods.qrc")
endif()

target_sources(vcmilauncher PRIVATE
	${launcher_SRCS}
	${launcher_HEADERS}
	${launcher_RESOURCES}
	${launcher_FORMS}
)

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

if(ANDROID)
	target_link_libraries(vcmilauncher Qt${QT_VERSION_MAJOR}::AndroidExtras)
elseif(NOT IOS)
	target_link_libraries(vcmilauncher SDL2::SDL2)
endif()

if(ENABLE_STATIC_LIBS OR NOT (ENABLE_EDITOR AND ENABLE_LAUNCHER))
	target_compile_definitions(vcmilauncher PRIVATE VCMIQT_STATIC)
endif()

target_link_libraries(vcmilauncher vcmi vcmiqt Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(vcmilauncher
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
)
vcmi_set_output_dir(vcmilauncher "")
enable_pch(vcmilauncher)

if(ENABLE_INNOEXTRACT)
	target_compile_definitions(vcmilauncher PRIVATE ENABLE_INNOEXTRACT)
	target_link_libraries(vcmilauncher innoextract)
endif()

if(IOS)
	target_link_libraries(vcmilauncher
		iOS_utils
		"-framework UniformTypeIdentifiers"
	)

	# workaround https://github.com/conan-io/conan-center-index/issues/13332
	if(USING_CONAN)
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/QIOSIntegrationPlugin.h
			"#include <QtPlugin>\nQ_IMPORT_PLUGIN(QIOSIntegrationPlugin)"
		)
		target_link_libraries(vcmilauncher
			Qt${QT_VERSION_MAJOR}::QIOSIntegrationPlugin
			qt::QIOSIntegrationPlugin
		)
	endif()
elseif(ANDROID)
	set(androidSdkDir "$ENV{ANDROID_HOME}")
	configure_file(
		"${androidPackageSourceDir}/androiddeployqt.json.in"
		"${CMAKE_BINARY_DIR}/androiddeployqt.json"
		@ONLY
	)
else()
	install(TARGETS vcmilauncher DESTINATION ${BIN_DIR})

	# Install icons and desktop file on Linux
	if(NOT WIN32 AND NOT APPLE)
		install(FILES "vcmilauncher.desktop" DESTINATION share/applications)
		install(FILES "eu.vcmi.VCMI.metainfo.xml" DESTINATION share/metainfo)
	endif()
endif()
