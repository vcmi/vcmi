if(NOT CREATOR_BUILD_TYPE)
	message(FATAL_ERROR "CREATOR_BUILD_TYPE variable must be set")
endif()

set(apkRelativeDir "build/outputs/apk/${CREATOR_BUILD_TYPE}")
set(apkCreatorDir "${CMAKE_BINARY_DIR}/android-build/${apkRelativeDir}")
set(realScriptPath "${CMAKE_BINARY_DIR}/apk_for_creator_${CREATOR_BUILD_TYPE}.cmake")

file(MAKE_DIRECTORY ${apkCreatorDir})
file(CONFIGURE OUTPUT ${realScriptPath} CONTENT [=[
	set(apkVcmiDir "../../../../vcmi-app/@apkRelativeDir@")
	file(CREATE_LINK "${apkVcmiDir}/vcmi-@CREATOR_BUILD_TYPE@.apk" "android-build-@CREATOR_BUILD_TYPE@.apk"
		COPY_ON_ERROR SYMBOLIC
	)
	file(CREATE_LINK "${apkVcmiDir}/output-metadata.json" "output-metadata.json"
		COPY_ON_ERROR SYMBOLIC
	)
]=] @ONLY)
execute_process(
	COMMAND ${CMAKE_COMMAND} -P ${realScriptPath}
	WORKING_DIRECTORY ${apkCreatorDir}
)
