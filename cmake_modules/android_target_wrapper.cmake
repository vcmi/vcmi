execute_process(
	COMMAND ${CMAKE_COMMAND} --build . --target ${QT_ANDROID_TARGET}
	RESULT_VARIABLE buildResult
)
# ignore CannotCopyApk error:
# internal call to androiddeployqt passes --apk param (see Qt5AndroidSupport.cmake),
# but VCMI outputs apk to a different path
if(buildResult EQUAL 20)
	message("${QT_ANDROID_TARGET} built fine, ignoring CannotCopyApk error")
elseif(NOT buildResult EQUAL 0)
	message(FATAL_ERROR "androiddeployqt failed with code ${buildResult}")
endif()
