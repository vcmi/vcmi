set(client_SRCS
	StdInc.cpp
	../CCallback.cpp

	adventureMap/CAdvMapPanel.cpp
	adventureMap/CAdvMapInt.cpp
	adventureMap/CAdventureOptions.cpp
	adventureMap/CInGameConsole.cpp
	adventureMap/CInfoBar.cpp
	adventureMap/CList.cpp
	adventureMap/CMinimap.cpp
	adventureMap/CResDataBar.cpp
	adventureMap/CTerrainRect.cpp

	battle/BattleActionsController.cpp
	battle/BattleAnimationClasses.cpp
	battle/BattleEffectsController.cpp
	battle/BattleFieldController.cpp
	battle/BattleInterface.cpp
	battle/BattleInterfaceClasses.cpp
	battle/BattleObstacleController.cpp
	battle/BattleProjectileController.cpp
	battle/BattleRenderer.cpp
	battle/BattleSiegeController.cpp
	battle/BattleStacksController.cpp
	battle/BattleWindow.cpp
	battle/CreatureAnimation.cpp

	gui/CGuiHandler.cpp
	gui/CIntObject.cpp
	gui/CursorHandler.cpp
	gui/InterfaceObjectConfigurable.cpp
	gui/NotificationHandler.cpp

	lobby/CBonusSelection.cpp
	lobby/CCampaignInfoScreen.cpp
	lobby/CLobbyScreen.cpp
	lobby/CSavingScreen.cpp
	lobby/CScenarioInfoScreen.cpp
	lobby/CSelectionBase.cpp
	lobby/OptionsTab.cpp
	lobby/RandomMapTab.cpp
	lobby/SelectionTab.cpp

	mainmenu/CCampaignScreen.cpp
	mainmenu/CMainMenu.cpp
	mainmenu/CPrologEpilogVideo.cpp
	mainmenu/CreditsScreen.cpp

	mapRenderer/MapRenderer.cpp
	mapRenderer/MapView.cpp
	mapRenderer/mapHandler.cpp

	render/CAnimation.cpp
	render/CBitmapHandler.cpp
	render/CDefFile.cpp
	render/Canvas.cpp
	render/ColorFilter.cpp
	render/Colors.cpp
	render/Graphics.cpp
	render/IFont.cpp

	renderSDL/CBitmapFont.cpp
	renderSDL/CBitmapHanFont.cpp
	renderSDL/CTrueTypeFont.cpp
	renderSDL/CursorHardware.cpp
	renderSDL/CursorSoftware.cpp
	renderSDL/SDLImage.cpp
	renderSDL/SDLImageLoader.cpp
	renderSDL/SDLRWwrapper.cpp
	renderSDL/SDL_Extensions.cpp

	widgets/Buttons.cpp
	widgets/CArtifactHolder.cpp
	widgets/CComponent.cpp
	widgets/CGarrisonInt.cpp
	widgets/CreatureCostBox.cpp
	widgets/Images.cpp
	widgets/MiscWidgets.cpp
	widgets/ObjectLists.cpp
	widgets/TextControls.cpp

	windows/CCastleInterface.cpp
	windows/CCreatureWindow.cpp
	windows/CHeroWindow.cpp
	windows/CKingdomInterface.cpp
	windows/CMessage.cpp
	windows/CQuestLog.cpp
	windows/CSpellWindow.cpp
	windows/CTradeWindow.cpp
	windows/CWindowObject.cpp
	windows/CreaturePurchaseCard.cpp
	windows/GUIClasses.cpp
	windows/InfoWindows.cpp
	windows/QuickRecruitmentWindow.cpp
	windows/settings/GeneralOptionsTab.cpp
	windows/settings/OtherOptionsTab.cpp
	windows/settings/SettingsMainWindow.cpp
	windows/settings/BattleOptionsTab.cpp
	windows/settings/AdventureOptionsTab.cpp

	CGameInfo.cpp
	CMT.cpp
	CMusicHandler.cpp
	CPlayerInterface.cpp
	CServerHandler.cpp
	CVideoHandler.cpp
	Client.cpp
	ClientCommandManager.cpp
	NetPacksClient.cpp
	NetPacksLobbyClient.cpp
)

set(client_HEADERS
	StdInc.h

	adventureMap/CAdvMapPanel.h
	adventureMap/CAdvMapInt.h
	adventureMap/CAdventureOptions.h
	adventureMap/CInGameConsole.h
	adventureMap/CInfoBar.h
	adventureMap/CList.h
	adventureMap/CMinimap.h
	adventureMap/CResDataBar.h
	adventureMap/CTerrainRect.h

	battle/BattleActionsController.h
	battle/BattleAnimationClasses.h
	battle/BattleConstants.h
	battle/BattleEffectsController.h
	battle/BattleFieldController.h
	battle/BattleInterface.h
	battle/BattleInterfaceClasses.h
	battle/BattleObstacleController.h
	battle/BattleProjectileController.h
	battle/BattleRenderer.h
	battle/BattleSiegeController.h
	battle/BattleStacksController.h
	battle/BattleWindow.h
	battle/CreatureAnimation.h

	gui/CGuiHandler.h
	gui/CIntObject.h
	gui/CursorHandler.h
	gui/InterfaceObjectConfigurable.h
	gui/MouseButton.h
	gui/NotificationHandler.h
	gui/TextAlignment.h

	lobby/CBonusSelection.h
	lobby/CCampaignInfoScreen.h
	lobby/CLobbyScreen.h
	lobby/CSavingScreen.h
	lobby/CScenarioInfoScreen.h
	lobby/CSelectionBase.h
	lobby/OptionsTab.h
	lobby/RandomMapTab.h
	lobby/SelectionTab.h

	mainmenu/CCampaignScreen.h
	mainmenu/CMainMenu.h
	mainmenu/CPrologEpilogVideo.h
	mainmenu/CreditsScreen.h

	mapRenderer/MapRenderer.h
	mapRenderer/MapRendererContext.h
	mapRenderer/MapView.h
	mapRenderer/mapHandler.h

	render/CAnimation.h
	render/CBitmapHandler.h
	render/CDefFile.h
	render/Canvas.h
	render/ColorFilter.h
	render/Colors.h
	render/Graphics.h
	render/ICursor.h
	render/IFont.h
	render/IImage.h
	render/IImageLoader.h

	renderSDL/CBitmapFont.h
	renderSDL/CBitmapHanFont.h
	renderSDL/CTrueTypeFont.h
	renderSDL/CursorHardware.h
	renderSDL/CursorSoftware.h
	renderSDL/SDLImage.h
	renderSDL/SDLImageLoader.h
	renderSDL/SDLRWwrapper.h
	renderSDL/SDL_Extensions.h
	renderSDL/SDL_PixelAccess.h

	widgets/Buttons.h
	widgets/CArtifactHolder.h
	widgets/CComponent.h
	widgets/CGarrisonInt.h
	widgets/CreatureCostBox.h
	widgets/Images.h
	widgets/MiscWidgets.h
	widgets/ObjectLists.h
	widgets/TextControls.h

	windows/CCastleInterface.h
	windows/CCreatureWindow.h
	windows/CHeroWindow.h
	windows/CKingdomInterface.h
	windows/CMessage.h
	windows/CQuestLog.h
	windows/CSpellWindow.h
	windows/CTradeWindow.h
	windows/CWindowObject.h
	windows/CreaturePurchaseCard.h
	windows/GUIClasses.h
	windows/InfoWindows.h
	windows/QuickRecruitmentWindow.h
	windows/settings/GeneralOptionsTab.h
	windows/settings/OtherOptionsTab.h
	windows/settings/SettingsMainWindow.h
	windows/settings/BattleOptionsTab.h
	windows/settings/AdventureOptionsTab.h

	CGameInfo.h
	CMT.h
	CMusicHandler.h
	CPlayerInterface.h
	CServerHandler.h
	CVideoHandler.h
	Client.h
	ClientCommandManager.h
	ClientNetPackVisitors.h
	LobbyClientNetPackVisitors.h
	resource.h
)

if(APPLE_IOS)
	set(client_SRCS ${client_SRCS}
		CFocusableHelper.cpp
		ios/GameChatKeyboardHandler.m
		ios/main.m
		ios/startSDL.mm
		ios/utils.mm
	)
	set(client_HEADERS ${client_HEADERS}
		CFocusableHelper.h
		ios/GameChatKeyboardHandler.h
		ios/startSDL.h
		ios/utils.h
	)
endif()

assign_source_group(${client_SRCS} ${client_HEADERS} VCMI_client.rc)

if(ANDROID)
	add_library(vcmiclient SHARED ${client_SRCS} ${client_HEADERS})
else()
	add_executable(vcmiclient ${client_SRCS} ${client_HEADERS})
endif()

add_dependencies(vcmiclient vcmiserver)
if(NOT ENABLE_STATIC_AI_LIBS)
	add_dependencies(vcmiclient
		BattleAI
		EmptyAI
		StupidAI
		VCAI
	)
	if(ENABLE_NULLKILLER_AI)
		add_dependencies(vcmiclient Nullkiller)
	endif()
endif()
if(APPLE_IOS)
	if(ENABLE_ERM)
		add_dependencies(vcmiclient vcmiERM)
	endif()
	if(ENABLE_LUA)
		add_dependencies(vcmiclient vcmiLua)
	endif()
endif()

if(WIN32)
	target_sources(vcmiclient PRIVATE "VCMI_client.rc")
	set_target_properties(vcmiclient
		PROPERTIES
			OUTPUT_NAME "VCMI_client"
			PROJECT_LABEL "VCMI_client"
	)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT vcmiclient)
	if(NOT ENABLE_DEBUG_CONSOLE)
		set_target_properties(vcmiclient PROPERTIES WIN32_EXECUTABLE)
		target_link_libraries(vcmiclient SDL2::SDL2main)
	endif()
	target_compile_definitions(vcmiclient PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)

	# TODO: very hacky, find proper solution to copy AI dlls into bin dir
	if(MSVC)
		add_custom_command(TARGET vcmiclient POST_BUILD
			WORKING_DIRECTORY "$<TARGET_FILE_DIR:vcmiclient>"
			COMMAND ${CMAKE_COMMAND} -E copy AI/fuzzylite.dll fuzzylite.dll
			COMMAND ${CMAKE_COMMAND} -E copy AI/tbb12.dll tbb12.dll
		)
	endif()
elseif(APPLE_IOS)
	target_link_libraries(vcmiclient PRIVATE
		iOS_utils

		# FFmpeg
		bz2
		iconv
		z
		"-framework AudioToolbox"
		"-framework AVFoundation"
		"-framework CoreMedia"
		"-framework VideoToolbox"
	)

	set_target_properties(vcmiclient PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
		XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "$(CODE_SIGNING_ALLOWED_FOR_APPS)"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
	)

	foreach(XCODE_RESOURCE LaunchScreen.storyboard Images.xcassets Settings.bundle vcmi_logo.png)
		set(XCODE_RESOURCE_PATH ios/${XCODE_RESOURCE})
		target_sources(vcmiclient PRIVATE ${XCODE_RESOURCE_PATH})
		set_source_files_properties(${XCODE_RESOURCE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

		# workaround to prevent CMAKE_SKIP_PRECOMPILE_HEADERS being added as compile flag
		if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22.0" AND CMAKE_VERSION VERSION_LESS "3.25.0")
			set_source_files_properties(${XCODE_RESOURCE_PATH} PROPERTIES LANGUAGE CXX)
		endif()
	endforeach()

	set(CMAKE_EXE_LINKER_FLAGS "-Wl,-e,_client_main")
endif()

if(ENABLE_SINGLE_APP_BUILD)
	target_link_libraries(vcmiclient PRIVATE vcmiserver)
	if(ENABLE_LAUNCHER)
		target_link_libraries(vcmiclient PRIVATE vcmilauncher)
	endif()
endif()
target_link_libraries(vcmiclient PRIVATE
		${VCMI_LIB_TARGET} SDL2::SDL2 SDL2::Image SDL2::Mixer SDL2::TTF
)

if(ffmpeg_LIBRARIES)
	target_link_libraries(vcmiclient PRIVATE
		${ffmpeg_LIBRARIES}
	)
else()
	target_compile_definitions(vcmiclient PRIVATE DISABLE_VIDEO)
endif()

target_include_directories(vcmiclient PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (ffmpeg_INCLUDE_DIRS)
	target_include_directories(vcmiclient PRIVATE
		${ffmpeg_INCLUDE_DIRS}
	)
endif()

vcmi_set_output_dir(vcmiclient "")
enable_pch(vcmiclient)

if(APPLE_IOS)
	vcmi_install_conan_deps("\${CMAKE_INSTALL_PREFIX}")
	add_custom_command(TARGET vcmiclient POST_BUILD
		COMMAND ios/set_build_version.sh "$<TARGET_BUNDLE_CONTENT_DIR:vcmiclient>"
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --component "${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}" --config "$<CONFIG>" --prefix "$<TARGET_BUNDLE_CONTENT_DIR:vcmiclient>"
		COMMAND ios/rpath_remove_symlinks.sh
		COMMAND ios/codesign.sh
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	install(TARGETS vcmiclient DESTINATION Payload COMPONENT app) # for ipa generation with cpack
elseif(ANDROID)
	vcmi_install_conan_deps("\${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")
	add_custom_command(TARGET vcmiclient POST_BUILD
		COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --prefix "${CMAKE_SOURCE_DIR}/android/vcmi-app/src/main"
	)
	install(TARGETS vcmiclient DESTINATION ${LIB_DIR})
else()
	install(TARGETS vcmiclient DESTINATION ${BIN_DIR})
endif()

#install icons and desktop file on Linux
if(NOT WIN32 AND NOT APPLE AND NOT ANDROID)
	#FIXME: move to client makefile?
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.64x64.png"   DESTINATION share/icons/hicolor/64x64/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.48x48.png"   DESTINATION share/icons/hicolor/48x48/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.32x32.png"   DESTINATION share/icons/hicolor/32x32/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.256x256.png" DESTINATION share/icons/hicolor/256x256/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.desktop"     DESTINATION share/applications)
endif()
