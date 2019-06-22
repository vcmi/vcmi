# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(APPLE)
	if(ENABLE_LAUNCHER)
		find_program(TOOL_MACDEPLOYQT NAMES macdeployqt PATHS ${qt_base_dir}/bin)
		if(NOT TOOL_MACDEPLOYQT)
			message(FATAL_ERROR "Could not find macdeployqt")
		endif()
		install(CODE "
			execute_process(COMMAND ${TOOL_MACDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}\" -verbose=2)
		")
	endif()

	install(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}\" \"\" \"\")
	" COMPONENT Runtime)
endif(APPLE)

# This will likely only work for Vcpkg
if(WIN32)
	if(ENABLE_LAUNCHER)
		# Temporary ugly fix for Qt deployment since windeployqt broken in Vcpkg
		
		#there are some weird issues with variables used in path not evaluated properly when trying to remove code duplication from below lines
		if(EXISTS ${CMAKE_BINARY_DIR}/../../vcpkg) #current path to vcpkg main folder on appveyor CI
			if(CMAKE_SIZEOF_VOID_P EQUAL 8) #64 bit build
				install(CODE "
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x64-windows/plugins/bearer DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x64-windows/plugins/iconengines DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x64-windows/plugins/imageformats DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x64-windows/plugins/platforminputcontexts DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x64-windows/plugins/platforms DESTINATION \${CMAKE_INSTALL_PREFIX})
				")
			else()
				install(CODE "
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x86-windows/plugins/bearer DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x86-windows/plugins/iconengines DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x86-windows/plugins/imageformats DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x86-windows/plugins/platforminputcontexts DESTINATION \${CMAKE_INSTALL_PREFIX})
					file(COPY ${CMAKE_BINARY_DIR}/../../vcpkg/installed/x86-windows/plugins/platforms DESTINATION \${CMAKE_INSTALL_PREFIX})
				")
			endif()
		else() #not appveyor build - lines below do not work properly
			install(CODE "
			execute_process(
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/\${BUILD_TYPE}/bearer \${CMAKE_INSTALL_PREFIX}/bearer
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/\${BUILD_TYPE}/iconengines \${CMAKE_INSTALL_PREFIX}/iconengines
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/\${BUILD_TYPE}/imageformats \${CMAKE_INSTALL_PREFIX}/imageformats
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/\${BUILD_TYPE}/platforminputcontexts \${CMAKE_INSTALL_PREFIX}/platforminputcontexts
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/\${BUILD_TYPE}/platforms \${CMAKE_INSTALL_PREFIX}/platforms)
			")
		endif()
	endif()

	
	#TODO: check if some equivalent of block below can be used for above block (easy qt dependencies copy)
	if(ENABLE_LUA)
		if(EXISTS ${LUA_INCLUDE_DIR}/../../bin/lua51.dll)
			install(CODE "
				file(COPY ${LUA_INCLUDE_DIR}/../../bin/lua51.dll DESTINATION \${CMAKE_INSTALL_PREFIX})
			")
		endif()
	endif()

	#LuaJIT will not be copied automatically by not meeting criteria for this block of code
	install(CODE "
		if(\"\${BUILD_TYPE}\" STREQUAL \"Debug\")
			set(dirs \"${CMAKE_PREFIX_PATH}/debug/bin/\")
		else()
			set(dirs \"${CMAKE_PREFIX_PATH}/bin/\")
		endif()
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/VCMI_Client.exe\" \"\" \"\${dirs}\")

	" COMPONENT Runtime)
endif(WIN32)
