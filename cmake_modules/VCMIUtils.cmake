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
