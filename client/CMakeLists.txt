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
		gui/NotificationHandler.cpp

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
		windows/CreaturePurchaseCard.cpp
		windows/CHeroWindow.cpp
		windows/CKingdomInterface.cpp
		windows/CQuestLog.cpp
		windows/CSpellWindow.cpp
		windows/CTradeWindow.cpp
		windows/CWindowObject.cpp
		windows/GUIClasses.cpp
		windows/InfoWindows.cpp
		windows/QuickRecruitmentWindow.cpp

		mainmenu/CMainMenu.cpp
		mainmenu/CCampaignScreen.cpp
		mainmenu/CreditsScreen.cpp
		mainmenu/CPrologEpilogVideo.cpp

		lobby/CBonusSelection.cpp
		lobby/CSelectionBase.cpp
		lobby/CLobbyScreen.cpp
		lobby/CSavingScreen.cpp
		lobby/CScenarioInfoScreen.cpp
		lobby/CCampaignInfoScreen.cpp
		lobby/OptionsTab.cpp
		lobby/RandomMapTab.cpp
		lobby/SelectionTab.cpp

		CBitmapHandler.cpp
		CreatureCostBox.cpp
		CGameInfo.cpp
		Client.cpp
		CMessage.cpp
		CMT.cpp
		CMusicHandler.cpp
		CPlayerInterface.cpp
		CVideoHandler.cpp
		CServerHandler.cpp
		Graphics.cpp
		mapHandler.cpp
		NetPacksClient.cpp
		NetPacksLobbyClient.cpp
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
		gui/NotificationHandler.h

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
		windows/CreaturePurchaseCard.h
		windows/CHeroWindow.h
		windows/CKingdomInterface.h
		windows/CQuestLog.h
		windows/CSpellWindow.h
		windows/CTradeWindow.h
		windows/CWindowObject.h
		windows/GUIClasses.h
		windows/InfoWindows.h
		windows/QuickRecruitmentWindow.h

		mainmenu/CMainMenu.h
		mainmenu/CCampaignScreen.h
		mainmenu/CreditsScreen.h
		mainmenu/CPrologEpilogVideo.h

		lobby/CBonusSelection.h
		lobby/CSelectionBase.h
		lobby/CLobbyScreen.h
		lobby/CSavingScreen.h
		lobby/CScenarioInfoScreen.h
		lobby/CCampaignInfoScreen.h
		lobby/OptionsTab.h
		lobby/RandomMapTab.h
		lobby/SelectionTab.h

		CBitmapHandler.h
		CreatureCostBox.h
		CGameInfo.h
		Client.h
		CMessage.h
		CMT.h
		CMusicHandler.h
		CPlayerInterface.h
		CVideoHandler.h
		CServerHandler.h
		Graphics.h
		mapHandler.h
		resource.h
		SDLRWwrapper.h
)

if(APPLE_IOS)
	set(client_SRCS ${client_SRCS}
		CFocusableHelper.cpp
		ios/GameChatKeyboardHanlder.m
		ios/SDL_uikit_main.mm
	)
	set(client_HEADERS ${client_HEADERS}
		CFocusableHelper.h
		ios/GameChatKeyboardHanlder.h
	)
endif()

assign_source_group(${client_SRCS} ${client_HEADERS} VCMI_client.rc)

if(ANDROID) # android needs client/server to be libraries, not executables, so we can't reuse the build part of this script
	return()
endif()

if(WIN32)
	set(client_ICON "VCMI_client.rc")
endif()

if(ENABLE_DEBUG_CONSOLE)
	add_executable(vcmiclient ${client_SRCS} ${client_HEADERS} ${client_ICON})
else()
	add_executable(vcmiclient WIN32 ${client_SRCS} ${client_HEADERS} ${client_ICON})
endif(ENABLE_DEBUG_CONSOLE)
add_dependencies(vcmiclient vcmiserver BattleAI StupidAI VCAI Nullkiller)

if(WIN32)
	set_target_properties(vcmiclient
		PROPERTIES
			OUTPUT_NAME "VCMI_client"
			PROJECT_LABEL "VCMI_client"
	)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT vcmiclient)
	if(NOT ENABLE_DEBUG_CONSOLE)
		target_link_libraries(vcmiclient SDL2::SDL2main)
	endif()
	target_compile_definitions(vcmiclient PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
elseif(APPLE_IOS)
	target_link_libraries(vcmiclient PRIVATE
		"-framework Foundation -framework UIKit -framework QuartzCore -framework CoreGraphics -framework CoreServices -framework ImageIO -framework Metal -framework OpenGLES -framework AVFoundation -framework GameController -framework CoreMotion -weak_framework CoreHaptics"
	)

	set(ENTITLEMENTS_OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/VCMI.entitlements)
	configure_file(ios/Entitlements.in ${ENTITLEMENTS_OUT_PATH} @ONLY)

	set_target_properties(vcmiclient PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
		XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "$(CODE_SIGNING_ALLOWED_FOR_APPS)"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
		XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${ENTITLEMENTS_OUT_PATH}
	)

	target_sources(vcmiclient PRIVATE ${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/client/LaunchScreen.storyboard PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	target_sources(vcmiclient PRIVATE ios/Images.xcassets)
	set_source_files_properties(ios/Images.xcassets PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

target_link_libraries(vcmiclient PRIVATE
		vcmi SDL2::SDL2 SDL2::Image SDL2::Mixer SDL2::TTF
)

if(ffmpeg_LIBRARIES)
	target_link_libraries(vcmiclient PRIVATE
		ffmpeg::swscale ffmpeg::avutil ffmpeg::avcodec ffmpeg::avformat
	)
else()
	target_compile_definitions(vcmiclient PRIVATE DISABLE_VIDEO)
endif()

target_include_directories(vcmiclient
	PUBLIC	${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(vcmiclient "")
enable_pch(vcmiclient)

if(APPLE_IOS)
	add_custom_command(TARGET vcmiclient POST_BUILD
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --prefix "$<TARGET_BUNDLE_CONTENT_DIR:vcmiclient>"
		COMMAND ${CMAKE_SOURCE_DIR}/apple_codesign.sh
	)
else()
	install(TARGETS vcmiclient DESTINATION ${BIN_DIR})
endif()

#install icons and desktop file on Linux
if(NOT WIN32 AND NOT APPLE)
	#FIXME: move to client makefile?
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.64x64.png"   DESTINATION share/icons/hicolor/64x64/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.48x48.png"   DESTINATION share/icons/hicolor/48x48/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.32x32.png"   DESTINATION share/icons/hicolor/32x32/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.256x256.png" DESTINATION share/icons/hicolor/256x256/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.desktop"     DESTINATION share/applications)
endif()
