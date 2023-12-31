
#message(${CMAKE_ARGV0}) # cmake.exe
#message(${CMAKE_ARGV1}) # -P
#message(${CMAKE_ARGV2}) # thisfilename
#message(${CMAKE_ARGV3}) # existing
#message(${CMAKE_ARGV4}) # linkname
if (WIN32)
	file(TO_NATIVE_PATH ${CMAKE_ARGV3} existing_native)
	file(TO_NATIVE_PATH ${CMAKE_ARGV4} linkname_native)
	execute_process(COMMAND cmd.exe /c RD /Q "${linkname_native}")
	execute_process(COMMAND cmd.exe /c mklink /J "${linkname_native}" "${existing_native}")
else()
	execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_ARGV3} ${CMAKE_ARGV4})
endif()
