project(vcmi_server)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS})

set(server_SRCS
        CGameHandler.cpp
        CVCMIServer.cpp
        NetPacksServer.cpp
)

IF(UNIX)
    add_executable(vcmi_server ${server_SRCS})
ELSEIF(WIN32)
    add_executable(vcmi_server WIN32 ${server_SRCS})
ENDIF()

target_link_libraries(vcmi_server vcmi_lib ${Boost_LIBRARIES})
