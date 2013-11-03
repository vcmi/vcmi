project(vcmiERM)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIRECTORY})

set(lib_SRCS
        ERMParser.cpp
        ERMInterpreter.cpp
        ERMScriptModule.cpp
)

add_library(vcmiERM SHARED ${lib_SRCS})
target_link_libraries(vcmiERM ${Boost_LIBRARIES})

install(TARGETS vcmiERM DESTINATION ${SCRIPTING_LIB_DIR})
