file(LOCK "${LOCK_FILE}" TIMEOUT 300)

execute_process(
	COMMAND "${WINDEPLOYQT}"
		--no-compiler-runtime
		--no-translations
		"${TARGET_FILE}"
	COMMAND_ERROR_IS_FATAL ANY
)

file(LOCK "${LOCK_FILE}" RELEASE)
