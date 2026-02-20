# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix Mac bundle dependencies since binaries wouldn't be there when this code executed
if(MACOS)
	set(bundleDir "\${CMAKE_INSTALL_PREFIX}/${APP_BUNDLE_DIR}")
	set(bundleContentsDir "${bundleDir}/Contents")
	set(executablesDir "${bundleContentsDir}/MacOS")

	set(macdeployqtParams "\"${bundleDir}\"")
	if(ENABLE_EDITOR)
		string(APPEND macdeployqtParams " \"-executable=${executablesDir}/vcmieditor\"")
	endif()
	# note: cross-compiled Qt 5 builds macdeployqt for target platform instead of host
	vcmi_deploy_qt(macdeployqt "${macdeployqtParams}")

	# perform ad-hoc codesigning
	# Intel Macs don't need it
	if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
		set(NEEDS_ADHOC_SIGNING 1)
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

	set(codesignCommand "codesign --verbose=4 --force --options runtime --timestamp --strict --sign")
	install(CODE "
set(CODE_SIGN_IDENTITY \$ENV{MACOS_CODE_SIGN_IDENTITY})
if(NOT CODE_SIGN_IDENTITY AND \"${NEEDS_ADHOC_SIGNING}\")
	set(CODE_SIGN_IDENTITY \"-\")
	set(codesignParamsForExecutables --entitlements \"${CMAKE_CURRENT_SOURCE_DIR}/entitlements_adhoc.plist\")
	message(\"performing adhoc signing\")
endif()
if(CODE_SIGN_IDENTITY)
	execute_process(COMMAND
		bash -c \"
			for f in '${executablesDir}/vcmibuilder' \$(find '${bundleContentsDir}' -type f -iname '*.dylib'); do
				${codesignCommand} '\${CODE_SIGN_IDENTITY}' \\\"\$f\\\"
			done
		\"
	)
	foreach(executable ${executablesToSign})
		execute_process(COMMAND
			${codesignCommand} \${CODE_SIGN_IDENTITY} --identifier eu.vcmi.\${executable} \${codesignParamsForExecutables} \"${executablesDir}/\${executable}\"
		)
	endforeach()
	execute_process(COMMAND
		${codesignCommand} \${CODE_SIGN_IDENTITY} \"${bundleDir}\"
	)
endif()
	")
endif()
