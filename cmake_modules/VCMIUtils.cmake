#######################################
#       Output directories            #
#######################################

macro(vcmi_set_output_dir name dir)
	# multi-config builds (e.g. msvc)
	foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGUPPERCASE)
		set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
		set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
		set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/${dir})
	endforeach()

	# generic no-config case (e.g. with mingw or MacOS)
	set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
	set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
	set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${dir})
endmacro()

#######################################
#    Better Visual Studio solution    #
#######################################

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

#######################################
#    Setting folders for 3rd-party    #
#######################################

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
