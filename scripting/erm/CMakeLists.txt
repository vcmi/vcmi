include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(lib_SRCS
		StdInc.cpp
		ERMParser.cpp
		ERMInterpreter.cpp
		ERMScriptModule.cpp
)

add_library(vcmiERM SHARED ${lib_SRCS})
target_link_libraries(vcmiERM ${Boost_LIBRARIES} vcmi)

vcmi_set_output_dir(vcmiERM "scripting")

set_target_properties(vcmiERM PROPERTIES ${PCH_PROPERTIES})
cotire(vcmiERM)

install(TARGETS vcmiERM DESTINATION ${SCRIPTING_LIB_DIR})
