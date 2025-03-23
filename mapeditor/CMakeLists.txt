set(editor_SRCS
		StdInc.cpp
		main.cpp
		mainwindow.cpp
		BitmapHandler.cpp
		maphandler.cpp
		Animation.cpp
		graphics.cpp
		windownewmap.cpp
		generatorprogress.cpp
		mapview.cpp
		objectbrowser.cpp
		mapsettings/abstractsettings.cpp
		mapsettings/mapsettings.cpp
		mapsettings/generalsettings.cpp
		mapsettings/modsettings.cpp
		mapsettings/timedevent.cpp
		mapsettings/victoryconditions.cpp
		mapsettings/loseconditions.cpp
		mapsettings/eventsettings.cpp
		mapsettings/rumorsettings.cpp
		mapsettings/translations.cpp
		playersettings.cpp
		playerparams.cpp
		scenelayer.cpp
		mapcontroller.cpp
		validator.cpp
		inspector/inspector.cpp
		inspector/townbuildingswidget.cpp
		inspector/towneventdialog.cpp
		inspector/towneventswidget.cpp
		inspector/townspellswidget.cpp
		inspector/armywidget.cpp
		inspector/messagewidget.cpp
		inspector/rewardswidget.cpp
		inspector/questwidget.cpp
		inspector/heroartifactswidget.cpp
		inspector/artifactwidget.cpp
		inspector/heroskillswidget.cpp
		inspector/herospellwidget.cpp
		inspector/PickObjectDelegate.cpp
		inspector/portraitwidget.cpp
		resourceExtractor/ResourceConverter.cpp
		helper.cpp
		campaigneditor/campaigneditor.cpp
		campaigneditor/campaignproperties.cpp
		campaigneditor/scenarioproperties.cpp
		campaigneditor/startingbonus.cpp
		campaigneditor/campaignview.cpp
)

set(editor_HEADERS
		StdInc.h
		mainwindow.h
		BitmapHandler.h
		maphandler.h
		Animation.h
		graphics.h
		windownewmap.h
		generatorprogress.h
		mapview.h
		objectbrowser.h
		mapsettings/abstractsettings.h
		mapsettings/mapsettings.h
		mapsettings/generalsettings.h
		mapsettings/modsettings.h
		mapsettings/timedevent.h
		mapsettings/victoryconditions.h
		mapsettings/loseconditions.h
		mapsettings/eventsettings.h
		mapsettings/rumorsettings.h
		mapsettings/translations.h
		playersettings.h
		playerparams.h
		scenelayer.h
		mapcontroller.h
		validator.h
		inspector/inspector.h
		inspector/townbuildingswidget.h
		inspector/towneventdialog.h
		inspector/towneventswidget.h
		inspector/townspellswidget.h
		inspector/armywidget.h
		inspector/messagewidget.h
		inspector/rewardswidget.h
		inspector/questwidget.h
		inspector/heroartifactswidget.h
		inspector/artifactwidget.h
		inspector/heroskillswidget.h
		inspector/herospellwidget.h
		inspector/PickObjectDelegate.h
		inspector/portraitwidget.h
		inspector/baseinspectoritemdelegate.h
		resourceExtractor/ResourceConverter.h
		mapeditorroles.h
		helper.h
		campaigneditor/campaigneditor.h
		campaigneditor/campaignproperties.h
		campaigneditor/scenarioproperties.h
		campaigneditor/startingbonus.h
		campaigneditor/campaignview.h
)

set(editor_FORMS
		mainwindow.ui
		windownewmap.ui
		generatorprogress.ui
		mapsettings/mapsettings.ui
		mapsettings/generalsettings.ui
		mapsettings/modsettings.ui
		mapsettings/timedevent.ui
		mapsettings/victoryconditions.ui
		mapsettings/loseconditions.ui
		mapsettings/eventsettings.ui
		mapsettings/rumorsettings.ui
		mapsettings/translations.ui
		playersettings.ui
		playerparams.ui
		validator.ui
		inspector/townbuildingswidget.ui
		inspector/towneventdialog.ui
		inspector/towneventswidget.ui
		inspector/townspellswidget.ui
		inspector/armywidget.ui
		inspector/messagewidget.ui
		inspector/rewardswidget.ui
		inspector/questwidget.ui
		inspector/heroartifactswidget.ui
		inspector/artifactwidget.ui
		inspector/heroskillswidget.ui
		inspector/herospellwidget.ui
		inspector/portraitwidget.ui
		campaigneditor/campaigneditor.ui
		campaigneditor/campaignproperties.ui
		campaigneditor/scenarioproperties.ui
		campaigneditor/startingbonus.ui
)

set(editor_RESOURCES
	resources.qrc
)

set(translationsDir "translation")
set(editor_TS
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
		set_source_files_properties(${editor_TS} PROPERTIES OUTPUT_LOCATION "${translationsDir}")
		qt5_add_translation(editor_QM ${editor_TS})

		set(translationsResource "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
		list(APPEND editor_RESOURCES "${translationsResource}")

		set(rccQmFiles "")
		foreach(qmFile ${editor_QM})
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
	set(editor_ICON mapeditor.rc)
endif()

assign_source_group(${editor_SRCS} ${editor_HEADERS} ${editor_RESOURCES} ${editor_TS} ${editor_ICON})

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
	add_library(vcmieditor OBJECT ${editor_QM})
else()
	add_executable(vcmieditor WIN32 ${editor_QM} ${editor_SRCS} ${editor_HEADERS} ${editor_UI_HEADERS} ${editor_ICON})
endif()

if(ENABLE_TRANSLATIONS)
	if(TARGET Qt6::Core)
		qt_add_translations(vcmieditor
			TS_FILES ${editor_TS}
			RESOURCE_PREFIX "/${translationsDir}"
			INCLUDE_DIRECTORIES
				${CMAKE_CURRENT_BINARY_DIR})
	endif()
endif()

target_sources(vcmieditor PRIVATE
	${editor_SRCS}
	${editor_HEADERS}
	${editor_FORMS}
	${editor_RESOURCES}
)

if(WIN32)
	set_target_properties(vcmieditor
		PROPERTIES
			OUTPUT_NAME "VCMI_mapeditor"
			PROJECT_LABEL "VCMI_mapeditor"
	)

	# FIXME: Can't to get CMP0020 working with Vcpkg and CMake 3.8.2
	# So far I tried:
	# - cmake_minimum_required set to 2.8.11 globally and in this file
	# - cmake_policy in all possible places
	# - used NO_POLICY_SCOPE to make sure no other parts reset policies
	# Still nothing worked, warning kept appearing and WinMain didn't link automatically
	target_link_libraries(vcmieditor Qt${QT_VERSION_MAJOR}::WinMain)
endif()

if(APPLE)
	# This makes Xcode project prettier by moving mapeditor_autogen directory into vcmiclient subfolder
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER vcmieditor)
endif()

# Qt defines 'emit' as macros, which conflicts with TBB definition of method with same name
target_compile_definitions(vcmieditor PRIVATE QT_NO_EMIT)

if(ENABLE_STATIC_LIBS OR NOT (ENABLE_EDITOR AND ENABLE_LAUNCHER))
	target_compile_definitions(vcmieditor PRIVATE VCMIQT_STATIC)
endif()

target_link_libraries(vcmieditor vcmi vcmiqt Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(vcmieditor
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
)
vcmi_set_output_dir(vcmieditor "")
enable_pch(vcmieditor)

install(TARGETS vcmieditor DESTINATION ${BIN_DIR})

# Install icons and desktop file on Linux
if(NOT WIN32 AND NOT APPLE)
	foreach(iconSize 32 48 64 128 256)
		install(FILES "icons/mapeditor.${iconSize}x${iconSize}.png"
			DESTINATION "share/icons/hicolor/${iconSize}x${iconSize}/apps"
			RENAME vcmieditor.png
		)
	endforeach()
	install(FILES "vcmieditor.desktop" DESTINATION share/applications)
endif()
