#######################################
#          Output directories         #
#######################################
if(APPLE)
	set(MACOSX_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set(APP_BUNDLE_NAME "${CMAKE_PROJECT_NAME}.app")
	set(APP_BUNDLE_DIR "${CMAKE_BINARY_DIR}/build/${APP_BUNDLE_NAME}")
	set(APP_BUNDLE_CONTENTS_DIR "${APP_BUNDLE_DIR}/Contents")
	set(APP_BUNDLE_BINARY_DIR "${APP_BUNDLE_CONTENTS_DIR}/MacOS")
	set(APP_BUNDLE_RESOURCES_DIR "${APP_BUNDLE_CONTENTS_DIR}/Resources")
endif()

macro(vcmi_set_output_dir name dir)
	# Multi-config builds for Visual Studio, Xcode
	foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		 string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGUPPERCASE)
		 set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
		 set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
		 set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
	endforeach()

	# Generic no-config case for Makefiles, Ninja.
	# This is what Qt Creator is using
	if(APPLE)
		set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${APP_BUNDLE_BINARY_DIR}/${dir})
		set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${APP_BUNDLE_BINARY_DIR}/${dir})
		set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${APP_BUNDLE_BINARY_DIR}/${dir})
	else()
		set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
		set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
		set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
	endif()
endmacro()

#######################################
#      Better project generation      #
#######################################

# Let us have proper tree-like structure in IDEs such as Visual Studio
function(assign_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if(IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(assign_source_group)

# Options to enable folders in CMake generated projects for Visual Studio, Xcode, etc
# Very useful to put 3rd-party libraries such as Minizip, GoogleTest and FuzzyLite in their own folders
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
define_property(
	TARGET
	PROPERTY FOLDER
	INHERITED
	BRIEF_DOCS "Set the folder name."
	FULL_DOCS  "Use to organize targets in an IDE."
)

function(add_subdirectory_with_folder _folder_name _folder)
	add_subdirectory(${_folder} ${ARGN})
	set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()

# Macro for Xcode Projects generation
# Slightly outdated, but useful reference for all options available here:
# https://pewpewthespells.com/blog/buildsettings.html
# https://github.com/samdmarshall/Xcode-Build-Settings-Reference
if(${CMAKE_GENERATOR} MATCHES "Xcode")

	macro(set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
		set_property(TARGET ${TARGET} PROPERTY
			XCODE_ATTRIBUTE_${XCODE_PROPERTY} ${XCODE_VALUE})
	endmacro(set_xcode_property)

endif(${CMAKE_GENERATOR} MATCHES "Xcode")
