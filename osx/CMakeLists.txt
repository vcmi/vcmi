# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(APPLE_MACOS)
	set(bundleDir "\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}")
	set(bundleContentsDir "${bundleDir}/Contents")

	if(ENABLE_LAUNCHER OR ENABLE_EDITOR)
		# note: cross-compiled Qt 5 builds macdeployqt for target platform instead of host
		vcmi_deploy_qt(macdeployqt "\"${bundleDir}\"")
	endif()

	# perform ad-hoc codesigning
	# Intel Macs don't need it
	if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		return()
	endif()

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
