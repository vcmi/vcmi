# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(APPLE_MACOS)
	set(bundleDir "\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}")
	set(bundleContentsDir "${bundleDir}/Contents")

	if(ENABLE_LAUNCHER)
		# cross-compiled Qt 5 builds macdeployqt for target platform instead of host
		if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)
			# deploy Qt dylibs with macdeployqt
			find_program(TOOL_MACDEPLOYQT NAMES macdeployqt PATHS ${qt_base_dir}/bin)
		endif()
		if(TOOL_MACDEPLOYQT)
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
	vcmi_install_conan_deps("${bundleContentsDir}")

	# perform ad-hoc codesigning
	set(executablesToSign vcmiserver)
	if(ENABLE_EDITOR)
		list(APPEND executablesToSign vcmieditor)
	endif()
	# main executable must be last
	list(APPEND executablesToSign vcmiclient)
	if(ENABLE_LAUNCHER)
		list(APPEND executablesToSign vcmilauncher)
	endif()

	set(codesignCommand "codesign --verbose=4 --force --options=runtime --timestamp=none --sign -")
	set(codesignCommandWithEntitlements "${codesignCommand} --entitlements \"${CMAKE_SOURCE_DIR}/osx/entitlements.plist\"")
	install(CODE "
		execute_process(COMMAND
			${codesignCommand} \"${bundleContentsDir}/MacOS/vcmibuilder\"
		)
		foreach(executable ${executablesToSign})
			execute_process(COMMAND
				${codesignCommandWithEntitlements} --identifier eu.vcmi.\${executable} \"${bundleContentsDir}/MacOS/\${executable}\"
			)
		endforeach()
	")
endif()
