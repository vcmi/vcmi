project(VCAI)
cmake_minimum_required(VERSION 2.6)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib ${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite)

set(VCAI_SRCS
        main.cpp
        VCAI.cpp
        Fuzzy.cpp
)

add_library(VCAI SHARED ${VCAI_SRCS})
target_link_libraries(VCAI FuzzyLite_lib vcmi)

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS VCAI DESTINATION ${AI_LIB_DIR})
endif()
