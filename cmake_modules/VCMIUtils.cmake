# This file should contain custom functions and macro and them only.
# It's should not alter any configuration on inclusion

#######################################
#        Build output path            #
#######################################
macro(vcmi_set_output_dir name dir)
	# Multi-config builds for Visual Studio, Xcode
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		 string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGUPPERCASE)
		 set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/${dir})
		 set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/${dir})
		 set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/${dir})
	endforeach()

	# Generic no-config case for Makefiles, Ninja.
	# This is what Qt Creator is using
	set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${dir})
	set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${dir})
	set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${dir})
endmacro()

#######################################
#        Project generation           #
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

# Macro to add subdirectory and set appropriate FOLDER for generated projects files
function(add_subdirectory_with_folder _folder_name _folder)
	add_subdirectory(${_folder} ${ARGN})
	set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()

#######################################
#        CMake debugging              #
#######################################

# Can be called to see check cmake variables and environment variables
# For "install" debugging just copy it here. There no easy way to include modules from source.
function(vcmi_print_all_variables)

		message(STATUS "-- -- Start of all internal variables")
		get_cmake_property(_variableNames VARIABLES)
		foreach(_variableName ${_variableNames})
				message(STATUS "${_variableName}=${${_variableName}}")
		endforeach()
		message(STATUS "-- -- End of all internal variables")
		message(STATUS "--")
		message(STATUS "-- -- Start of all environment variables")
		execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "environment")
		message(STATUS "-- -- End of all environment variables")

endfunction()

# Print CMake variables most important for debugging
function(vcmi_print_important_variables)

	message(STATUS "-- -- Start of VCMI build debug information")

	message(STATUS "CMAKE_VERSION: " ${CMAKE_VERSION})
	message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
	message(STATUS "CMAKE_BINARY_DIR: " ${CMAKE_BINARY_DIR})
	message(STATUS "CMAKE_SOURCE_DIR: " ${CMAKE_SOURCE_DIR})
#	message(STATUS "PROJECT_BINARY_DIR: " ${PROJECT_BINARY_DIR})
#	message(STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
	message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
	message(STATUS "CMAKE_COMMAND: " ${CMAKE_COMMAND})
	message(STATUS "CMAKE_ROOT: " ${CMAKE_ROOT})
#	message(STATUS "CMAKE_CURRENT_LIST_FILE and LINE: ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}")
#	message(STATUS "CMAKE_INCLUDE_PATH: " ${CMAKE_INCLUDE_PATH})
#	message(STATUS "CMAKE_LIBRARY_PATH: " ${CMAKE_LIBRARY_PATH})

	message(STATUS "UNIX: ${UNIX} - WIN32: ${WIN32} - APPLE: ${APPLE}")
	message(STATUS "MINGW: ${MINGW} - CYGWIN: ${CYGWIN} - MSVC: ${MSVC}")
	message(STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID})
	message(STATUS "CMAKE_CXX_COMPILER_VERSION: " ${CMAKE_CXX_COMPILER_VERSION})
	message(STATUS "CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER})
	message(STATUS "CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER})
#	message(STATUS "CMAKE_COMPILER_IS_GNUCC: " ${CMAKE_COMPILER_IS_GNUCC})
#	message(STATUS "CMAKE_COMPILER_IS_GNUCXX: " ${CMAKE_COMPILER_IS_GNUCXX})
#	message(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
#	message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})

	message(STATUS "CMAKE_SYSTEM: " ${CMAKE_SYSTEM})
	message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
	message(STATUS "CMAKE_SYSTEM_VERSION: " ${CMAKE_SYSTEM_VERSION})
	message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})

	message(STATUS "-- -- End of VCMI build debug information")

endfunction()

# Print Git commit hash
function(vcmi_print_git_commit_hash)

	message(STATUS "-- -- Start of Git information")
	message(STATUS "GIT_SHA1: " ${GIT_SHA1})
	message(STATUS "-- -- End of Git information")

endfunction()

#install imported target on windows
function(install_vcpkg_imported_tgt tgt)
	get_target_property(TGT_LIB_LOCATION ${tgt} LOCATION)
	get_filename_component(TGT_LIB_FOLDER ${TGT_LIB_LOCATION} PATH)
	get_filename_component(tgt_name ${TGT_LIB_LOCATION} NAME_WE)
	get_filename_component(TGT_DLL ${TGT_LIB_FOLDER}/../bin/${tgt_name}.dll ABSOLUTE)
	message("${tgt_name}: ${TGT_DLL}")
	install(FILES ${TGT_DLL} DESTINATION ${BIN_DIR})
endfunction(install_vcpkg_imported_tgt)

# install dependencies from Conan, install_dir should contain \${CMAKE_INSTALL_PREFIX}
function(vcmi_install_conan_deps install_dir)
	if(NOT USING_CONAN)
		return()
	endif()
	install(CODE "
		execute_process(COMMAND
			conan imports \"${CMAKE_SOURCE_DIR}\" --install-folder \"${CONAN_INSTALL_FOLDER}\" --import-folder \"${install_dir}\"
		)
		file(REMOVE \"${install_dir}/conan_imports_manifest.txt\")
	")
endfunction()
