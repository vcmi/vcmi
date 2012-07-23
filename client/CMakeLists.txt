project(vcmiclient)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLTTF_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIR})

set(client_SRCS
        ../CCallback.cpp
        BattleInterface/CBattleAnimations.cpp
        BattleInterface/CBattleInterface.cpp
        BattleInterface/CBattleInterfaceClasses.cpp
        BattleInterface/CCreatureAnimation.cpp
        UIFramework/CGuiHandler.cpp
        UIFramework/CIntObject.cpp
        UIFramework/CIntObjectClasses.cpp
        UIFramework/Geometries.cpp
        UIFramework/CCursorHandler.cpp
        UIFramework/SDL_Extensions.cpp
        AdventureMapClasses.cpp
        CAdvmapInterface.cpp
        CAnimation.cpp
        CBitmapHandler.cpp
        CCastleInterface.cpp
        CConfigHandler.cpp
        CCreatureWindow.cpp
        CDefHandler.cpp
        CGameInfo.cpp
        CHeroWindow.cpp
        CKingdomInterface.cpp
        Client.cpp
        CMessage.cpp
        CMT.cpp
        CMusicHandler.cpp
        CPlayerInterface.cpp
        CPreGame.cpp
        CSndHandler.cpp
        CSpellWindow.cpp
        CVideoHandler.cpp
        CQuestLog.cpp
        Graphics.cpp
        GUIClasses.cpp
        mapHandler.cpp
        NetPacksClient.cpp
)

IF(UNIX)
    add_executable(vcmiclient ${client_SRCS})
ELSEIF(WIN32)
    add_executable(vcmiclient WIN32 ${client_SRCS})
ENDIF()

target_link_libraries(vcmiclient vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLTTF_LIBRARY} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${RT_LIB} ${DL_LIB})
