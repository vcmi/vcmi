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
		if(ANDROID)
			set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
		else()
			set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/${dir})
		endif()
		set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/${dir})
	endforeach()

	# Generic no-config case for Makefiles, Ninja.
	# This is what Qt Creator is using
	set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${dir})
	if(NOT ANDROID)
		set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${dir})
	endif()
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
macro(add_subdirectory_with_folder _folder_name _folder)
	add_subdirectory(${_folder} ${ARGN})
	set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endmacro()

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

# install(FILES) of shared libs but using symlink instead of copying
function(vcmi_install_libs_symlink libs)
	install(CODE "
		foreach(lib ${libs})
			cmake_path(GET lib FILENAME filename)
			file(CREATE_LINK \${lib} \"\${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/\${filename}\"
				COPY_ON_ERROR SYMBOLIC
			)
		endforeach()
	")
endfunction()

# install dependencies from Conan, CONAN_RUNTIME_LIBS_FILE is set in conanfile.py
function(vcmi_install_conan_deps)
	if(NOT USING_CONAN)
		return()
	endif()

	file(STRINGS "${CONAN_RUNTIME_LIBS_FILE}" runtimeLibs)
	if(ANDROID)
		vcmi_install_libs_symlink("${runtimeLibs}")
	else()
		install(FILES ${runtimeLibs} DESTINATION ${LIB_DIR})
	endif()
endfunction()

function(vcmi_deploy_qt deployQtToolName deployQtOptions)
	# TODO: use qt_generate_deploy_app_script() with Qt 6
	find_program(TOOL_DEPLOYQT NAMES ${deployQtToolName} PATHS "${qtBinDir}")
	if(TOOL_DEPLOYQT)
		install(CODE "
			execute_process(
				COMMAND \"${TOOL_DEPLOYQT}\" ${deployQtOptions} -verbose=2
				COMMAND_ERROR_IS_FATAL ANY
			)
		")
	else()
		message(WARNING "${deployQtToolName} not found, running cpack would result in broken package")
	endif()
endfunction()

# generate .bat for .exe with proper PATH
function(vcmi_create_exe_shim tgt)
	if(NOT CONAN_RUNENV_SCRIPT)
		return()
	endif()

	set(exe "%~dp0$<TARGET_FILE_NAME:${tgt}>")
	if(EXISTS "${CONAN_RUNENV_SCRIPT}.bat")
		set(batContent "call \"${CONAN_RUNENV_SCRIPT}.bat\" & start \"\" \"${exe}\"")
	else()
		set(batContent "powershell -ExecutionPolicy Bypass -Command \"& '${CONAN_RUNENV_SCRIPT}.ps1' ; & '${exe}'\"")
	endif()
	file(GENERATE OUTPUT "$<TARGET_FILE_DIR:${tgt}>/$<TARGET_FILE_BASE_NAME:${tgt}>.bat" CONTENT "${batContent}")
endfunction()
