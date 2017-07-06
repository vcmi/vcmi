project(vcmiclient)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${SDL_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLTTF_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})

set(client_SRCS
		StdInc.cpp
		../CCallback.cpp

		battle/CBattleAnimations.cpp
		battle/CBattleInterfaceClasses.cpp
		battle/CBattleInterface.cpp
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
		windows/CWindowObject.cpp
		windows/GUIClasses.cpp
		windows/InfoWindows.cpp

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
		SDLRWwrapper.cpp
)

set(client_HEADERS
		StdInc.h

		battle/CBattleAnimations.h
		battle/CBattleInterfaceClasses.h
		battle/CBattleInterface.h
		battle/CCreatureAnimation.h

		gui/CAnimation.h
		gui/CCursorHandler.h
		gui/CGuiHandler.h
		gui/CIntObject.h
		gui/Fonts.h
		gui/Geometries.h
		gui/SDL_Compat.h
		gui/SDL_Extensions.h
		gui/SDL_Pixels.h

		widgets/AdventureMapClasses.h
		widgets/Buttons.h
		widgets/CArtifactHolder.h
		widgets/CComponent.h
		widgets/CGarrisonInt.h
		widgets/Images.h
		widgets/MiscWidgets.h
		widgets/ObjectLists.h
		widgets/TextControls.h
		windows/CAdvmapInterface.h
		windows/CCastleInterface.h
		windows/CCreatureWindow.h
		windows/CHeroWindow.h
		windows/CKingdomInterface.h
		windows/CQuestLog.h
		windows/CSpellWindow.h
		windows/CTradeWindow.h
		windows/CWindowObject.h
		windows/GUIClasses.h
		windows/InfoWindows.h

		CBitmapHandler.h
		CDefHandler.h
		CGameInfo.h
		Client.h
		CMessage.h
		CMT.h
		CMusicHandler.h
		CPlayerInterface.h
		CPreGame.h
		CVideoHandler.h
		Graphics.h
		mapHandler.h
		resource.h
		SDLMain.h
		SDLRWwrapper.h
)

if(ANDROID) # android needs client/server to be libraries, not executables, so we can't reuse the build part of this script
	return()
endif()

if(MSVC)
	# workaround ffmpeg linking problems
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

if(APPLE)
	# OS X specific source files
	set(client_SRCS ${client_SRCS} SDLMain.m Autoupdate.mm Info.plist vcmi.icns ../osx/vcmi_dsa_public.pem)

  if(ENABLE_SPARKLE)
    # OS X specific includes
    include_directories(${SPARKLE_INCLUDE_DIR})

    # OS X specific libraries
    target_link_libraries(vcmiclient ${SPARKLE_FRAMEWORK})
  endif()

	add_executable(vcmiclient MACOSX_BUNDLE ${client_SRCS} ${client_HEADERS})

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

  set(CopyVendoredMinizip
		mkdir -p ${BUNDLE_PATH}/MacOS &&
    cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libminizip.dylib ${BUNDLE_PATH}/MacOS/libminizip.dylib)

	set(MakeVCMIBundle
		# Copy all needed binaries
		mkdir -p ${BUNDLE_PATH}/MacOS/AI &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/vcmiserver ${BUNDLE_PATH}/MacOS/vcmiserver &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libvcmi.dylib ${BUNDLE_PATH}/MacOS/libvcmi.dylib &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libVCAI.dylib ${BUNDLE_PATH}/MacOS/AI/libVCAI.dylib &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libStupidAI.dylib ${BUNDLE_PATH}/MacOS/AI/libStupidAI.dylib &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libEmptyAI.dylib ${BUNDLE_PATH}/MacOS/AI/libEmptyAI.dylib &&
		cp ${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)/libBattleAI.dylib ${BUNDLE_PATH}/MacOS/AI/libBattleAI.dylib &&
		cp -r ${CMAKE_HOME_DIRECTORY}/osx/vcmibuilder.app ${BUNDLE_PATH}/MacOS/vcmibuilder.app &&

		# Copy frameworks
		sh -c 'cp -r ${CMAKE_HOME_DIRECTORY}/${CMAKE_FRAMEWORK_PATH} ${BUNDLE_PATH}/Frameworks/ || true' &&

		# Copy vcmi data
		mkdir -p ${BUNDLE_PATH}/Data &&
		mkdir -p ${BUNDLE_PATH}/Data/Mods &&
		mkdir -p ${BUNDLE_PATH}/Data/launcher &&
		cp -r ${CMAKE_HOME_DIRECTORY}/config/ ${BUNDLE_PATH}/Data/config/ &&
		cp -r ${CMAKE_HOME_DIRECTORY}/Mods/vcmi/ ${BUNDLE_PATH}/Data/Mods/vcmi/ &&
		sh -c 'cp -r ${CMAKE_HOME_DIRECTORY}/Mods/WoG/ ${BUNDLE_PATH}/Data/Mods/WoG/ || echo "Download WoG mod from http://wiki.vcmi.eu/index.php?title=Mod_list" ' &&
		sh -c 'cp -r ${CMAKE_HOME_DIRECTORY}/Mods/hota/ ${BUNDLE_PATH}/Data/Mods/hota/ || echo "Download HOTA mod from http://wiki.vcmi.eu/index.php?title=Mod_list" ' &&
		cp -r ${CMAKE_HOME_DIRECTORY}/launcher/icons/ ${BUNDLE_PATH}/Data/launcher/icons/)

  if(NOT MINIZIP_FOUND)
    add_custom_command(TARGET vcmiclient POST_BUILD COMMAND ${CopyVendoredMinizip})
  endif()
	add_custom_command(TARGET vcmiclient POST_BUILD COMMAND ${MakeVCMIBundle})
elseif(WIN32)
	add_executable(vcmiclient ${client_SRCS} ${client_HEADERS} VCMI_client.rc)
else()
	add_executable(vcmiclient ${client_SRCS} ${client_HEADERS})
endif()

if(WIN32)
	set_target_properties(vcmiclient PROPERTIES OUTPUT_NAME VCMI_client)
endif()

target_link_libraries(vcmiclient vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLTTF_LIBRARY} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${SYSTEM_LIBS})

set_target_properties(vcmiclient PROPERTIES ${PCH_PROPERTIES})
cotire(vcmiclient)


install(TARGETS vcmiclient DESTINATION ${BIN_DIR})
