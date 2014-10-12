project(vcmilauncher)
cmake_minimum_required(VERSION 2.8.7)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ZLIB_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})

set(launcher_modmanager_SRCS
	modManager/cdownloadmanager_moc.cpp
	modManager/cmodlist.cpp
	modManager/cmodlistmodel_moc.cpp
	modManager/cmodlistview_moc.cpp
	modManager/cmodmanager.cpp
	modManager/imageviewer_moc.cpp
)

set(launcher_settingsview_SRCS
	settingsView/csettingsview_moc.cpp
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

set(launcher_FORMS
	modManager/cmodlistview_moc.ui
	modManager/imageviewer_moc.ui
	settingsView/csettingsview_moc.ui
	mainwindow_moc.ui
)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
add_definitions(${Qt5Widgets_DEFINITIONS})
add_definitions(${Qt5Network_DEFINITIONS})

# Executables fail to build with Qt 5 in the default configuration
# without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${CMAKE_CXX_FLAGS}")

qt5_wrap_ui(launcher_UI_HEADERS ${launcher_FORMS})

if(WIN32)
	add_executable(vcmilauncher WIN32 ${launcher_SRCS} ${launcher_UI_HEADERS} VCMI_launcher.rc)
	set_target_properties(vcmilauncher PROPERTIES OUTPUT_NAME VCMI_launcher)
else()
	add_executable(vcmilauncher ${launcher_SRCS} ${launcher_UI_HEADERS})
endif()

if(MSVC)
	# Fix _WinMain@16 linking error
	target_link_libraries(vcmilauncher vcmi ${Qt5Core_QTMAIN_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
else()
	# The Qt5Widgets_LIBRARIES variable also includes QtGui and QtCore
	target_link_libraries(vcmilauncher vcmi ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
endif()

# temporary(?) disabled - generation of PCH takes too much time since cotire is trying to collect all Qt headers
#set_target_properties(vcmilauncher PROPERTIES ${PCH_PROPERTIES})
#cotire(vcmilauncher)

if (NOT APPLE) # Already inside bundle
	install(TARGETS vcmilauncher DESTINATION ${BIN_DIR})
	# copy whole directory but .svn control files
	install(DIRECTORY icons DESTINATION ${DATA_DIR}/launcher PATTERN ".svn" EXCLUDE)
endif()

