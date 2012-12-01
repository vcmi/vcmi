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
        CSpellWindow.cpp
        CVideoHandler.cpp
        CQuestLog.cpp
        Graphics.cpp
        GUIClasses.cpp
        mapHandler.cpp
        NetPacksClient.cpp
)

if(WIN32)
    add_executable(vcmiclient WIN32 ${client_SRCS})
elseif(APPLE)
    # When building for OS X we need add SDLMain.m to source files
    add_executable(vcmiclient MACOSX_BUNDLE ${client_SRCS} SDLMain.m ${CMAKE_CURRENT_SOURCE_DIR}/vcmi.icns)

    # Because server and AI libs would be copies to bundle they need to be built before client
    add_dependencies(vcmiclient vcmiserver VCAI EmptyAI StupidAI BattleAI)

    # Copy icon file
    set(MACOSX_BUNDLE_ICON_FILE vcmi.icns)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/vcmi.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # Copy server executable, libs and game data to bundle
    set(BUNDLE_PATH ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/vcmiclient.app/Contents)
    set(MakeVCMIBundle
        # Copy all needed binaries
        mkdir -p ${BUNDLE_PATH}/MacOS/AI &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/vcmiserver ${BUNDLE_PATH}/MacOS/vcmiserver &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libvcmi.dylib ${BUNDLE_PATH}/MacOS/libvcmi.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libVCAI.dylib ${BUNDLE_PATH}/MacOS/AI/libVCAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libStupidAI.dylib ${BUNDLE_PATH}/MacOS/AI/libStupidAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libEmptyAI.dylib ${BUNDLE_PATH}/MacOS/AI/libEmptyAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libBattleAI.dylib ${BUNDLE_PATH}/MacOS/AI/libBattleAI.dylib &&

        # Copy frameworks
        cp -r ${CMAKE_HOME_DIRECTORY}/${CMAKE_FRAMEWORK_PATH} ${BUNDLE_PATH}/Frameworks &&

        # Copy vcmi data
        mkdir -p ${BUNDLE_PATH}/Data &&
        mkdir -p ${BUNDLE_PATH}/Data/Mods &&
        mkdir -p ${BUNDLE_PATH}/Data/Mods/WoG &&
        cp -r ${CMAKE_HOME_DIRECTORY}/config ${BUNDLE_PATH}/Data/config &&
        cp -r ${CMAKE_HOME_DIRECTORY}/Mods/vcmi ${BUNDLE_PATH}/Data/Mods/vcmi &&
        cp ${CMAKE_HOME_DIRECTORY}/Mods/WoG/filesystem.json ${BUNDLE_PATH}/Data/Mods/WoG/filesystem.json)
    add_custom_command(TARGET vcmiclient POST_BUILD COMMAND ${MakeVCMIBundle})
else()
    add_executable(vcmiclient ${client_SRCS})
endif()

target_link_libraries(vcmiclient vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLTTF_LIBRARY} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${RT_LIB} ${DL_LIB})

install(TARGETS vcmiclient DESTINATION ${BIN_DIR})

