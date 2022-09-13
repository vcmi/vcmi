# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(APPLE)
	set(bundleDir "\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}")
	set(bundleContentsDir "${bundleDir}/Contents")

	if(ENABLE_LAUNCHER)
		# cross-compiled Qt 5 builds macdeployqt for target platform instead of host
		if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)
			# deploy Qt dylibs with macdeployqt
			find_program(TOOL_MACDEPLOYQT NAMES macdeployqt PATHS ${qt_base_dir}/bin)
			if(NOT TOOL_MACDEPLOYQT)
				message(FATAL_ERROR "Could not find macdeployqt")
			endif()
			install(CODE "
				execute_process(COMMAND
					\"${TOOL_MACDEPLOYQT}\" \"${bundleDir}\" -verbose=2
				)
			")
		else()
			# simulate macdeployqt behavior, main Qt libs are copied by conan
			get_target_property(qmakePath Qt5::qmake IMPORTED_LOCATION)
			execute_process(COMMAND
				"${qmakePath}" -query QT_INSTALL_PLUGINS
				OUTPUT_VARIABLE qtPluginsDir
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
			install(DIRECTORY
				${qtPluginsDir}/
				DESTINATION ${APP_BUNDLE_DIR}/Contents/PlugIns
			)
			install(CODE "
				file(WRITE ${bundleContentsDir}/Resources/qt.conf
					\"[Paths]\nPlugins = PlugIns\"
				)
			")
		endif()
	endif()

	# deploy other dylibs with conan
	install(CODE "
		execute_process(COMMAND
			conan imports \"${CMAKE_SOURCE_DIR}\" --install-folder \"${CMAKE_SOURCE_DIR}/conan-generated\" --import-folder \"${bundleContentsDir}\"
		)
		file(REMOVE \"${bundleContentsDir}/conan_imports_manifest.txt\")
	")

	# perform ad-hoc codesigning
	set(codesignCommand "codesign --verbose=4 --force --options=runtime --timestamp=none --sign -")
	set(codesignCommandWithEntitlements "${codesignCommand} --entitlements \"${CMAKE_SOURCE_DIR}/osx/entitlements.plist\"")
	install(CODE "
		execute_process(COMMAND
			${codesignCommand} \"${bundleContentsDir}/MacOS/vcmibuilder\"
		)
		foreach(executable vcmiclient vcmiserver vcmilauncher)
			execute_process(COMMAND
				${codesignCommandWithEntitlements} \"${bundleContentsDir}/MacOS/\${executable}\"
			)
		endforeach()
	")
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
		install_vcpkg_imported_tgt(luajit::luajit)
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
