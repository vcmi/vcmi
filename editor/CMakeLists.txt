project(vcmieditor)
cmake_minimum_required(VERSION 2.8.7)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Qt5Widgets_INCLUDE_DIRS})


set(maped_SRCS
	StdInc.cpp
	Editor.cpp
	Main.cpp
)

set(maped_MOC_HEADERS
	Editor.h
)

set(maped_FORMS
	editor.ui
)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
add_definitions(${Qt5Widgets_DEFINITIONS})

# Executables fail to build with Qt 5 in the default configuration
# without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${CMAKE_CXX_FLAGS}")

qt5_wrap_ui(maped_FORMS_OUT ${maped_FORMS})

add_executable(vcmieditor ${maped_SRCS} ${maped_FORMS_OUT})

# The Qt5Widgets_LIBRARIES variable also includes QtGui and QtCore
target_link_libraries(vcmieditor vcmi ${Qt5Widgets_LIBRARIES})

set_target_properties(vcmieditor PROPERTIES ${PCH_PROPERTIES})
cotire(vcmieditor)

if (NOT APPLE) # Already inside bundle
    install(TARGETS vcmieditor DESTINATION ${BIN_DIR})
endif()

