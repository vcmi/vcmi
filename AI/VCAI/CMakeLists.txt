project(VCAI)
cmake_minimum_required(VERSION 2.6)

if (FL_FOUND)
    include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib ${FL_INCLUDE_DIRS})
else()
    include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib ${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite/fuzzylite)
endif()

set(VCAI_SRCS
        StdInc.cpp
        VCAI.cpp
        Goals.cpp
        AIUtility.cpp
        main.cpp
        Fuzzy.cpp
)

add_library(VCAI SHARED ${VCAI_SRCS})
if (FL_FOUND)
    target_link_libraries(VCAI ${FL_LIBRARIES} vcmi)
else()
    target_link_libraries(VCAI fl-static vcmi)
endif()

set_target_properties(VCAI PROPERTIES ${PCH_PROPERTIES})
cotire(VCAI)

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS VCAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()
