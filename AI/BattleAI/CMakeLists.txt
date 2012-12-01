project(battleAI)
cmake_minimum_required(VERSION 2.6)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(battleAI_SRCS
        BattleAI.cpp
        main.cpp
)

add_library(BattleAI SHARED ${battleAI_SRCS})
target_link_libraries(BattleAI vcmi)

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS BattleAI DESTINATION ${AI_LIB_DIR})
endif()

