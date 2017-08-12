# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(APPLE)
	if(ENABLE_LAUNCHER)
		find_program(TOOL_MACDEPLOYQT NAMES macdeployqt PATHS ${qt_base_dir}/bin)
		if(NOT TOOL_MACDEPLOYQT)
			message(FATAL_ERROR "Could not find macdeployqt")
		endif()
		install(CODE "
			execute_process(COMMAND ${TOOL_MACDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}\" -verbose=2)")
	endif()

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}\" \"\" \"\")
	" COMPONENT Runtime)
endif(APPLE)
