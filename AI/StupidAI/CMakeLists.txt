project(stupidAI)
cmake_minimum_required(VERSION 2.6)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(stupidAI_SRCS
		StdInc.cpp
        StupidAI.cpp
        main.cpp
)

add_library(StupidAI SHARED ${stupidAI_SRCS})
target_link_libraries(StupidAI vcmi)

set_target_properties(StupidAI PROPERTIES ${PCH_PROPERTIES})
cotire(StupidAI)

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS StupidAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()

