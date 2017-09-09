# Detailed information about CMake compatibility available on Qt website
# https://doc.qt.io/qt-5/cmake-manual.html
include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ZLIB_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})

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
)

set(launcher_HEADERS
		StdInc.h
		${launcher_modmanager_HEADERS}
		${launcher_settingsview_HEADERS}
		mainwindow_moc.h
		launcherdirs.h
		jsonutils.h
)

set(launcher_FORMS
		modManager/cmodlistview_moc.ui
		modManager/imageviewer_moc.ui
		settingsView/csettingsview_moc.ui
		mainwindow_moc.ui
)

assign_source_group(${launcher_SRCS} ${launcher_HEADERS} VCMI_launcher.rc)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if("${CMAKE_VERSION}" VERSION_LESS 2.8.12)
	# Executables fail to build with Qt 5 in the default configuration
	# without -fPIE. We add that here.
	set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${CMAKE_CXX_FLAGS}")
endif()

qt5_wrap_ui(launcher_UI_HEADERS ${launcher_FORMS})

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
	target_link_libraries(vcmilauncher Qt5::WinMain)
endif()

if(APPLE)
	# This makes Xcode project prettier by moving vcmilauncher_autogen directory into vcmiclient subfolder
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER vcmilauncher)
endif()

target_link_libraries(vcmilauncher vcmi Qt5::Widgets Qt5::Network)

vcmi_set_output_dir(vcmilauncher "")

# temporary(?) disabled - generation of PCH takes too much time since cotire is trying to collect all Qt headers
#set_target_properties(vcmilauncher PROPERTIES ${PCH_PROPERTIES})
#cotire(vcmilauncher)

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
endif()
