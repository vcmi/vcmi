project(vcmiclient)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLTTF_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})

set(client_SRCS
		StdInc.cpp
		../CCallback.cpp

		battle/CBattleInterface.cpp
		battle/CBattleAnimations.cpp
		battle/CBattleInterfaceClasses.cpp
		battle/CCreatureAnimation.cpp

		gui/CAnimation.cpp
		gui/CCursorHandler.cpp
		gui/CGuiHandler.cpp
		gui/CIntObject.cpp
		gui/Fonts.cpp
		gui/Geometries.cpp
		gui/SDL_Extensions.cpp

		widgets/AdventureMapClasses.cpp
		widgets/Buttons.cpp
		widgets/CArtifactHolder.cpp
		widgets/CComponent.cpp
		widgets/CGarrisonInt.cpp
		widgets/Images.cpp
		widgets/MiscWidgets.cpp
		widgets/ObjectLists.cpp
		widgets/TextControls.cpp

		windows/CAdvmapInterface.cpp
		windows/CCastleInterface.cpp
		windows/CCreatureWindow.cpp
		windows/CHeroWindow.cpp
		windows/CKingdomInterface.cpp
		windows/CQuestLog.cpp
		windows/CSpellWindow.cpp
		windows/CTradeWindow.cpp
		windows/CWindowObject
		windows/InfoWindows.cpp
		windows/GUIClasses.cpp

		CBitmapHandler.cpp
		CDefHandler.cpp
		CGameInfo.cpp
		Client.cpp
		CMessage.cpp
		CMT.cpp
		CMusicHandler.cpp
		CPlayerInterface.cpp
		CPreGame.cpp
		CVideoHandler.cpp
		Graphics.cpp
		mapHandler.cpp
		NetPacksClient.cpp
)

set(client_HEADERS
		gui/SDL_Pixels.h
		gui/SDL_Compat.h
)

if(APPLE)
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

	set_target_properties(vcmiclient PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/")

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
elseif(WIN32)
	add_executable(vcmiclient ${client_SRCS} VCMI_client.rc)
else()
	add_executable(vcmiclient ${client_SRCS})
endif()

if(WIN32)
	set_target_properties(vcmiclient PROPERTIES OUTPUT_NAME VCMI_client)
endif()

target_link_libraries(vcmiclient vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLTTF_LIBRARY} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${SYSTEM_LIBS})

set_target_properties(vcmiclient PROPERTIES ${PCH_PROPERTIES})
cotire(vcmiclient)


install(TARGETS vcmiclient DESTINATION ${BIN_DIR})

