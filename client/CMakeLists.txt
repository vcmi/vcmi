project(vcmiclient)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLTTF_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})

set(client_SRCS
        ../CCallback.cpp

		battle/CBattleInterface.cpp
        battle/CBattleAnimations.cpp
        battle/CBattleInterfaceClasses.cpp
        battle/CCreatureAnimation.cpp

        gui/CGuiHandler.cpp
        gui/CIntObject.cpp
        gui/CIntObjectClasses.cpp
        gui/Fonts.cpp
        gui/Geometries.cpp
        gui/CCursorHandler.cpp
        gui/SDL_Extensions.cpp

		CPreGame.cpp
		Client.cpp
		CPlayerInterface.cpp
		CMT.cpp
		GUIClasses.cpp
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
        CMessage.cpp
        CMusicHandler.cpp
        CSpellWindow.cpp
        CVideoHandler.cpp
        CQuestLog.cpp
        Graphics.cpp
        mapHandler.cpp
        NetPacksClient.cpp
)

set(client_HEADERS
		CSoundBase.h

		gui/SDL_Pixels.h
)

if(WIN32)
	add_executable(vcmiclient WIN32 ${client_SRCS} ${client_HEADERS})
elseif(APPLE)
    # OS X specific includes
    include_directories(${SPARKLE_INCLUDE_DIR})

    # OS X specific source files
    set(client_SRCS ${client_SRCS} SDLMain.m OSX.mm Info.plist vcmi.icns ../osx/vcmi_dsa_public.pem)
	add_executable(vcmiclient MACOSX_BUNDLE ${client_SRCS} ${client_HEADERS})

    # OS X specific libraries
    target_link_libraries(vcmiclient ${SPARKLE_FRAMEWORK})

    # Because server and AI libs would be copies to bundle they need to be built before client
    add_dependencies(vcmiclient vcmiserver VCAI EmptyAI StupidAI BattleAI minizip)

    # Custom Info.plist
    set_target_properties(vcmiclient PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

    # Copy icon file and public key for Sparkle
    set_source_files_properties(vcmi.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(../osx/vcmi_dsa_public.pem PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # Copy server executable, libs and game data to bundle
    set(BUNDLE_PATH ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/vcmiclient.app/Contents)
    set(MakeVCMIBundle
        # Copy all needed binaries
        mkdir -p ${BUNDLE_PATH}/MacOS/AI &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/vcmiserver ${BUNDLE_PATH}/MacOS/vcmiserver &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libvcmi.dylib ${BUNDLE_PATH}/MacOS/libvcmi.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libminizip.dylib ${BUNDLE_PATH}/MacOS/libminizip.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libVCAI.dylib ${BUNDLE_PATH}/MacOS/AI/libVCAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libStupidAI.dylib ${BUNDLE_PATH}/MacOS/AI/libStupidAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libEmptyAI.dylib ${BUNDLE_PATH}/MacOS/AI/libEmptyAI.dylib &&
        cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libBattleAI.dylib ${BUNDLE_PATH}/MacOS/AI/libBattleAI.dylib &&
        cp -r ${CMAKE_HOME_DIRECTORY}/osx/vcmibuilder.app ${BUNDLE_PATH}/MacOS/vcmibuilder.app &&

        # Copy frameworks
        cp -r ${CMAKE_HOME_DIRECTORY}/${CMAKE_FRAMEWORK_PATH} ${BUNDLE_PATH}/Frameworks/ &&

        # Copy vcmi data
        mkdir -p ${BUNDLE_PATH}/Data &&
        mkdir -p ${BUNDLE_PATH}/Data/Mods &&
        mkdir -p ${BUNDLE_PATH}/Data/launcher &&
        cp -r ${CMAKE_HOME_DIRECTORY}/config/ ${BUNDLE_PATH}/Data/config/ &&
        cp -r ${CMAKE_HOME_DIRECTORY}/Mods/vcmi/ ${BUNDLE_PATH}/Data/Mods/vcmi/ &&
        cp -r ${CMAKE_HOME_DIRECTORY}/Mods/WoG/ ${BUNDLE_PATH}/Data/Mods/WoG/ &&
        cp -r ${CMAKE_HOME_DIRECTORY}/launcher/icons/ ${BUNDLE_PATH}/Data/launcher/icons/)
        
    add_custom_command(TARGET vcmiclient POST_BUILD COMMAND ${MakeVCMIBundle})
else()
	add_executable(vcmiclient ${client_SRCS} ${client_HEADERS})
endif()

target_link_libraries(vcmiclient vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLTTF_LIBRARY} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${RT_LIB} ${DL_LIB})

install(TARGETS vcmiclient DESTINATION ${BIN_DIR})

