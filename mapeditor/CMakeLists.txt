set(editor_SRCS
		StdInc.cpp
		main.cpp
		launcherdirs.cpp
		jsonutils.cpp
		mainwindow.cpp
		BitmapHandler.cpp
		maphandler.cpp
		Animation.cpp
		graphics.cpp
		windownewmap.cpp
		generatorprogress.cpp
		mapview.cpp
		objectbrowser.cpp
		mapsettings.cpp
		playersettings.cpp
		playerparams.cpp
		scenelayer.cpp
		mapcontroller.cpp
		validator.cpp
		inspector/inspector.cpp
		inspector/townbulidingswidget.cpp
		inspector/armywidget.cpp
		inspector/messagewidget.cpp
		inspector/rewardswidget.cpp
		inspector/questwidget.cpp
)

set(editor_HEADERS
		StdInc.h
		launcherdirs.h
		jsonutils.h
		mainwindow.h
		BitmapHandler.h
		maphandler.h
		Animation.h
		graphics.h
		windownewmap.h
		generatorprogress.h
		mapview.h
		objectbrowser.h
		mapsettings.h
		playersettings.h
		playerparams.h
		scenelayer.h
		mapcontroller.h
		validator.h
		inspector/inspector.h
		inspector/townbulidingswidget.h
		inspector/armywidget.h
		inspector/messagewidget.h
		inspector/rewardswidget.h
		inspector/questwidget.h
)

set(editor_FORMS
		mainwindow.ui
		windownewmap.ui
		generatorprogress.ui
		mapsettings.ui
		playersettings.ui
		playerparams.ui
		validator.ui
		inspector/townbulidingswidget.ui
		inspector/armywidget.ui
		inspector/messagewidget.ui
		inspector/rewardswidget.ui
		inspector/questwidget.ui
)

assign_source_group(${editor_SRCS} ${editor_HEADERS} mapeditor.rc)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

if(POLICY CMP0071)
	cmake_policy(SET CMP0071 NEW)
endif()

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(TARGET Qt6::Core)
	qt_wrap_ui(editor_UI_HEADERS ${editor_FORMS})
else()
	qt5_wrap_ui(editor_UI_HEADERS ${editor_FORMS})
endif()

if(WIN32)
	set(editor_ICON mapeditor.rc)
endif()

add_executable(vcmieditor WIN32 ${editor_SRCS} ${editor_HEADERS} ${editor_UI_HEADERS} ${editor_ICON})

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
	# This makes Xcode project prettier by moving vcmilauncher_autogen directory into vcmiclient subfolder
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER vcmieditor)
endif()

target_link_libraries(vcmieditor vcmi Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(vcmieditor
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
)
vcmi_set_output_dir(vcmieditor "")
enable_pch(vcmieditor)

# Copy to build directory for easier debugging
add_custom_command(TARGET vcmieditor POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/mapeditor/icons
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/mapeditor/icons ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/mapeditor/icons
)

install(TARGETS vcmieditor DESTINATION ${BIN_DIR})
# copy whole directory
install(DIRECTORY icons DESTINATION ${DATA_DIR}/mapeditor)
# Install icons and desktop file on Linux
if(NOT WIN32 AND NOT APPLE)
	install(FILES "vcmilauncher.desktop" DESTINATION share/applications)
endif()
