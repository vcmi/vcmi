project(vcmiserver)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS})

set(server_SRCS
        CGameHandler.cpp
        CVCMIServer.cpp
        NetPacksServer.cpp
)

IF(UNIX)
    add_executable(vcmiserver ${server_SRCS})
ELSEIF(WIN32)
    add_executable(vcmiserver WIN32 ${server_SRCS})
ENDIF()

target_link_libraries(vcmiserver vcmi ${Boost_LIBRARIES} ${RT_LIB} ${DL_LIB})
