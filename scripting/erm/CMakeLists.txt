include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(lib_SRCS
		StdInc.cpp
		ERMParser.cpp
		ERMInterpreter.cpp
		ERMScriptModule.cpp
)

set(lib_HDRS
		StdInc.h
		ERMParser.h
		ERMInterpreter.h
		ERMScriptModule.h
)

add_library(vcmiERM SHARED ${lib_SRCS} ${lib_HDRS})
target_link_libraries(vcmiERM Boost::boost ${VCMI_LIB_TARGET})

vcmi_set_output_dir(vcmiERM "scripting")
enable_pch(vcmiERM)

if(APPLE_IOS)
	install(TARGETS vcmiERM LIBRARY DESTINATION ${SCRIPTING_LIB_DIR})
else()
	install(TARGETS vcmiERM DESTINATION ${SCRIPTING_LIB_DIR})
endif()
