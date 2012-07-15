project(stupidAI)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(stupidAI_SRCS
        StupidAI.cpp
        main.cpp
)

add_library(StupidAI SHARED ${stupidAI_SRCS})
target_link_libraries(StupidAI vcmi)
