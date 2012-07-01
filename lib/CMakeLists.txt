project(vcmi_lib)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIRECTORY})
include_directories(${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

set(lib_SRCS
        BattleAction.cpp
        BattleHex.cpp
        BattleState.cpp
        CArtHandler.cpp
        CBuildingHandler.cpp
        CCampaignHandler.cpp
        CConsoleHandler.cpp
        CCreatureHandler.cpp
        CCreatureSet.cpp
        CDefObjInfoHandler.cpp
        CFileUtility.cpp
        CGameInterface.cpp
        CGameState.cpp
        CGeneralTextHandler.cpp
        CHeroHandler.cpp
        CLodHandler.cpp
        CLogger.cpp
        CMapInfo.cpp
        CObjectHandler.cpp
        CObstacleInstance.cpp
        Connection.cpp
        CSpellHandler.cpp
        CThreadHelper.cpp
        CTownHandler.cpp
        HeroBonus.cpp
        IGameCallback.cpp
        JsonNode.cpp
        map.cpp
        NetPacksLib.cpp
        ResourceSet.cpp
        VCMI_Lib.cpp
)

add_library(vcmi_lib SHARED ${lib_SRCS})
target_link_libraries(vcmi_lib ${Boost_LIBRARIES} ${SDL_LIBRARY} ${ZLIB_LIBRARIES})
