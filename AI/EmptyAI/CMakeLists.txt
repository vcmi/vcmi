project(emptyAI)
cmake_minimum_required(VERSION 2.6)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(emptyAI_SRCS
        CEmptyAI.cpp
        exp_funcs.cpp
)

add_library(EmptyAI SHARED ${emptyAI_SRCS})
target_link_libraries(EmptyAI vcmi)

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS EmptyAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()
