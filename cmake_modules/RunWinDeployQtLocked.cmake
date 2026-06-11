file(LOCK "${LOCK_FILE}" TIMEOUT 300)

execute_process(
    COMMAND "${WINDEPLOYQT}"
        --no-compiler-runtime
        --no-translations
        "${TARGET_FILE}"
    RESULT_VARIABLE windeployqt_result
)

if(NOT windeployqt_result EQUAL 0)
    message(FATAL_ERROR
        "windeployqt failed with exit code ${windeployqt_result}")
endif()

file(LOCK "${LOCK_FILE}" RELEASE)
