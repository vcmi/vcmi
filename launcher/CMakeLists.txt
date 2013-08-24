project(vcmilauncher)
cmake_minimum_required(VERSION 2.8.7)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})

set(launcher_modmanager_SRCS
	modManager/cdownloadmanager_moc.cpp
	modManager/cmodlist.cpp
	modManager/cmodlistmodel_moc.cpp
	modManager/cmodlistview_moc.cpp
	modManager/cmodmanager.cpp
)

set(launcher_settingsview_SRCS
	settingsView/csettingsview_moc.cpp
)

set(launcher_SRCS
	${launcher_modmanager_SRCS}
	${launcher_settingsview_SRCS}
	main.cpp
	mainwindow_moc.cpp
	launcherdirs.cpp
)

set(launcher_FORMS
	modManager/cmodlistview_moc.ui
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

add_executable(vcmilauncher ${launcher_SRCS} ${launcher_UI_HEADERS})

# The Qt5Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries(vcmilauncher vcmi ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})

if (NOT APPLE) # Already inside bundle
    install(TARGETS vcmilauncher DESTINATION ${BIN_DIR})
endif()

