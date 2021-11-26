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
target_link_libraries(vcmiERM Boost::boost vcmi)

vcmi_set_output_dir(vcmiERM "scripting")
enable_pch(vcmiERM)

install(TARGETS vcmiERM LIBRARY DESTINATION ${SCRIPTING_LIB_DIR} OPTIONAL)
