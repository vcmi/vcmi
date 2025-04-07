set(vcmiclientcommon_SRCS
	StdInc.cpp
	../CCallback.cpp

	adventureMap/AdventureMapInterface.cpp
	adventureMap/AdventureMapShortcuts.cpp
	adventureMap/AdventureMapWidget.cpp
	adventureMap/AdventureOptions.cpp
	adventureMap/CInGameConsole.cpp
	adventureMap/CInfoBar.cpp
	adventureMap/CList.cpp
	adventureMap/CMinimap.cpp
	adventureMap/CResDataBar.cpp
	adventureMap/MapAudioPlayer.cpp
	adventureMap/TurnTimerWidget.cpp

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
	battle/BattleOverlayLogVisualizer.cpp

	eventsSDL/NotificationHandler.cpp
	eventsSDL/InputHandler.cpp
	eventsSDL/InputSourceKeyboard.cpp
	eventsSDL/InputSourceMouse.cpp
	eventsSDL/InputSourceText.cpp
	eventsSDL/InputSourceTouch.cpp
	eventsSDL/InputSourceGameController.cpp

	gui/CIntObject.cpp
	gui/CursorHandler.cpp
	gui/EventDispatcher.cpp
	gui/EventsReceiver.cpp
	gui/InterfaceObjectConfigurable.cpp
	gui/FramerateManager.cpp
	gui/ShortcutHandler.cpp
	gui/WindowHandler.cpp

	lobby/CBonusSelection.cpp
	lobby/CCampaignInfoScreen.cpp
	lobby/CLobbyScreen.cpp
	lobby/CSavingScreen.cpp
	lobby/CScenarioInfoScreen.cpp
	lobby/CSelectionBase.cpp
	lobby/TurnOptionsTab.cpp
	lobby/ExtraOptionsTab.cpp
	lobby/OptionsTab.cpp
	lobby/OptionsTabBase.cpp
	lobby/RandomMapTab.cpp
	lobby/SelectionTab.cpp

	mainmenu/CCampaignScreen.cpp
	mainmenu/CMainMenu.cpp
	mainmenu/CPrologEpilogVideo.cpp
	mainmenu/CreditsScreen.cpp
	mainmenu/CHighScoreScreen.cpp
	mainmenu/CStatisticScreen.cpp

	mapView/MapRenderer.cpp
	mapView/MapRendererContext.cpp
	mapView/MapRendererContextState.cpp
	mapView/MapView.cpp
	mapView/MapViewActions.cpp
	mapView/MapViewCache.cpp
	mapView/MapViewController.cpp
	mapView/MapViewModel.cpp
	mapView/mapHandler.cpp
	mapView/MapOverlayLogVisualizer.cpp

	media/CAudioBase.cpp
	media/CMusicHandler.cpp
	media/CSoundHandler.cpp
	media/CVideoHandler.cpp

	render/AssetGenerator.cpp
	render/CAnimation.cpp
	render/CBitmapHandler.cpp
	render/CDefFile.cpp
	render/Canvas.cpp
	render/CanvasImage.cpp
	render/ColorFilter.cpp
	render/Colors.cpp
	render/Graphics.cpp
	render/IFont.cpp
	render/ImageLocator.cpp

	renderSDL/CBitmapFont.cpp
	renderSDL/CTrueTypeFont.cpp
	renderSDL/CursorHardware.cpp
	renderSDL/CursorSoftware.cpp
	renderSDL/FontChain.cpp
	renderSDL/ScalableImage.cpp
	renderSDL/RenderHandler.cpp
	renderSDL/SDLImage.cpp
	renderSDL/SDLImageLoader.cpp
	renderSDL/SDLImageScaler.cpp
	renderSDL/SDLRWwrapper.cpp
	renderSDL/ScreenHandler.cpp
	renderSDL/SDL_Extensions.cpp

	globalLobby/GlobalLobbyAddChannelWindow.cpp
	globalLobby/GlobalLobbyClient.cpp
	globalLobby/GlobalLobbyInviteWindow.cpp
	globalLobby/GlobalLobbyLoginWindow.cpp
	globalLobby/GlobalLobbyRoomWindow.cpp
	globalLobby/GlobalLobbyServerSetup.cpp
	globalLobby/GlobalLobbyWidget.cpp
	globalLobby/GlobalLobbyWindow.cpp

	widgets/Buttons.cpp
	widgets/CComponent.cpp
	widgets/CComponentHolder.cpp
	widgets/CExchangeController.cpp
	widgets/CGarrisonInt.cpp
	widgets/CreatureCostBox.cpp
	widgets/ComboBox.cpp
	widgets/CTextInput.cpp
	widgets/GraphicalPrimitiveCanvas.cpp
	widgets/Images.cpp
	widgets/MiscWidgets.cpp
	widgets/ObjectLists.cpp
	widgets/TextControls.cpp
	widgets/Scrollable.cpp
	widgets/Slider.cpp
	widgets/CArtifactsOfHeroBase.cpp
	widgets/CArtifactsOfHeroMain.cpp
	widgets/CArtifactsOfHeroKingdom.cpp
	widgets/CArtifactsOfHeroAltar.cpp
	widgets/CArtifactsOfHeroMarket.cpp
	widgets/CArtifactsOfHeroBackpack.cpp
	widgets/RadialMenu.cpp
	widgets/VideoWidget.cpp
	widgets/markets/CAltarArtifacts.cpp
	widgets/markets/CAltarCreatures.cpp
	widgets/markets/CArtifactsBuying.cpp
	widgets/markets/CArtifactsSelling.cpp
	widgets/markets/CFreelancerGuild.cpp
	widgets/markets/CMarketResources.cpp
	widgets/markets/CTransferResources.cpp
	widgets/markets/CMarketBase.cpp
	widgets/markets/TradePanels.cpp

	windows/CCastleInterface.cpp
	windows/CCreatureWindow.cpp
	windows/CExchangeWindow.cpp
	windows/CHeroOverview.cpp
	windows/CHeroWindow.cpp
	windows/CKingdomInterface.cpp
	windows/CMapOverview.cpp
	windows/CMarketWindow.cpp
	windows/CMessage.cpp
	windows/CPuzzleWindow.cpp
	windows/CQuestLog.cpp
	windows/CSpellWindow.cpp
	windows/CTutorialWindow.cpp
	windows/CWindowObject.cpp
	windows/CreaturePurchaseCard.cpp
	windows/GUIClasses.cpp
	windows/InfoWindows.cpp
	windows/QuickRecruitmentWindow.cpp
	windows/CHeroBackpackWindow.cpp
	windows/CWindowWithArtifacts.cpp
	windows/settings/GeneralOptionsTab.cpp
	windows/settings/OtherOptionsTab.cpp
	windows/settings/SettingsMainWindow.cpp
	windows/settings/BattleOptionsTab.cpp
	windows/settings/AdventureOptionsTab.cpp

	xBRZ/xbrz.cpp

	ArtifactsUIController.cpp
	GameEngine.cpp
	GameInstance.cpp
	CPlayerInterface.cpp
	PlayerLocalState.cpp
	CServerHandler.cpp
	Client.cpp
	ClientCommandManager.cpp
	GameChatHandler.cpp
	HeroMovementController.cpp
	NetPacksClient.cpp
	NetPacksLobbyClient.cpp
	ServerRunner.cpp
	UIHelper.cpp
)

set(vcmiclientcommon_HEADERS
	StdInc.h

	adventureMap/AdventureMapInterface.h
	adventureMap/AdventureMapShortcuts.h
	adventureMap/AdventureMapWidget.h
	adventureMap/AdventureState.h
	adventureMap/AdventureOptions.h
	adventureMap/CInGameConsole.h
	adventureMap/CInfoBar.h
	adventureMap/CList.h
	adventureMap/CMinimap.h
	adventureMap/CResDataBar.h
	adventureMap/MapAudioPlayer.h
	adventureMap/TurnTimerWidget.h

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
	battle/BattleOverlayLogVisualizer.h

	eventsSDL/NotificationHandler.h
	eventsSDL/InputHandler.h
	eventsSDL/InputSourceKeyboard.h
	eventsSDL/InputSourceMouse.h
	eventsSDL/InputSourceText.h
	eventsSDL/InputSourceTouch.h
	eventsSDL/InputSourceGameController.h

	gui/CIntObject.h
	gui/CursorHandler.h
	gui/EventDispatcher.h
	gui/EventsReceiver.h
	gui/InterfaceObjectConfigurable.h
	gui/FramerateManager.h
	gui/MouseButton.h
	gui/Shortcut.h
	gui/ShortcutHandler.h
	gui/TextAlignment.h
	gui/WindowHandler.h

	lobby/CBonusSelection.h
	lobby/CCampaignInfoScreen.h
	lobby/CLobbyScreen.h
	lobby/CSavingScreen.h
	lobby/CScenarioInfoScreen.h
	lobby/CSelectionBase.h
	lobby/TurnOptionsTab.h
	lobby/ExtraOptionsTab.h
	lobby/OptionsTab.h
	lobby/OptionsTabBase.h
	lobby/RandomMapTab.h
	lobby/SelectionTab.h

	mainmenu/CCampaignScreen.h
	mainmenu/CMainMenu.h
	mainmenu/CPrologEpilogVideo.h
	mainmenu/CreditsScreen.h
	mainmenu/CHighScoreScreen.h
	mainmenu/CStatisticScreen.h

	mapView/IMapRendererContext.h
	mapView/IMapRendererObserver.h
	mapView/MapRenderer.h
	mapView/MapRendererContext.h
	mapView/MapRendererContextState.h
	mapView/MapView.h
	mapView/MapViewActions.h
	mapView/MapViewCache.h
	mapView/MapViewController.h
	mapView/MapViewModel.h
	mapView/mapHandler.h
	mapView/MapOverlayLogVisualizer.h

	media/CAudioBase.h
	media/CEmptyVideoPlayer.h
	media/CMusicHandler.h
	media/CSoundHandler.h
	media/CVideoHandler.h
	media/IMusicPlayer.h
	media/ISoundPlayer.h
	media/IVideoPlayer.h

	render/AssetGenerator.h
	render/CAnimation.h
	render/CBitmapHandler.h
	render/CDefFile.h
	render/Canvas.h
	render/CanvasImage.h
	render/ColorFilter.h
	render/Colors.h
	render/EFont.h
	render/Graphics.h
	render/ICursor.h
	render/IFont.h
	render/IImage.h
	render/IImageLoader.h
	render/ImageLocator.h
	render/IRenderHandler.h
	render/IScreenHandler.h

	renderSDL/CBitmapFont.h
	renderSDL/CTrueTypeFont.h
	renderSDL/CursorHardware.h
	renderSDL/CursorSoftware.h
	renderSDL/FontChain.h
	renderSDL/ScalableImage.h
	renderSDL/RenderHandler.h
	renderSDL/SDLImage.h
	renderSDL/SDLImageLoader.h
	renderSDL/SDLImageScaler.h
	renderSDL/SDLRWwrapper.h
	renderSDL/ScreenHandler.h
	renderSDL/SDL_Extensions.h
	renderSDL/SDL_PixelAccess.h

	globalLobby/GlobalLobbyClient.h
	globalLobby/GlobalLobbyDefines.h
	globalLobby/GlobalLobbyAddChannelWindow.h
	globalLobby/GlobalLobbyInviteWindow.h
	globalLobby/GlobalLobbyLoginWindow.h
	globalLobby/GlobalLobbyRoomWindow.h
	globalLobby/GlobalLobbyServerSetup.h
	globalLobby/GlobalLobbyObserver.h
	globalLobby/GlobalLobbyWidget.h
	globalLobby/GlobalLobbyWindow.h

	widgets/Buttons.h
	widgets/CComponent.h
	widgets/CComponentHolder.h
	widgets/CExchangeController.h
	widgets/CGarrisonInt.h
	widgets/CreatureCostBox.h
	widgets/ComboBox.h
	widgets/CTextInput.h
	widgets/GraphicalPrimitiveCanvas.h
	widgets/Images.h
	widgets/MiscWidgets.h
	widgets/ObjectLists.h
	widgets/TextControls.h
	widgets/Scrollable.h
	widgets/Slider.h
	widgets/CArtifactsOfHeroBase.h
	widgets/CArtifactsOfHeroMain.h
	widgets/CArtifactsOfHeroKingdom.h
	widgets/CArtifactsOfHeroAltar.h
	widgets/CArtifactsOfHeroMarket.h
	widgets/CArtifactsOfHeroBackpack.h
	widgets/IVideoHolder.h
	widgets/RadialMenu.h
	widgets/VideoWidget.h
	widgets/markets/CAltarArtifacts.h
	widgets/markets/CAltarCreatures.h
	widgets/markets/CArtifactsBuying.h
	widgets/markets/CArtifactsSelling.h
	widgets/markets/CFreelancerGuild.h
	widgets/markets/CMarketResources.h
	widgets/markets/CTransferResources.h
	widgets/markets/CMarketBase.h
	widgets/markets/TradePanels.h

	windows/CCastleInterface.h
	windows/CCreatureWindow.h
	windows/CExchangeWindow.h
	windows/CHeroOverview.h
	windows/CHeroWindow.h
	windows/CKingdomInterface.h
	windows/CMessage.h
	windows/CMapOverview.h
	windows/CMarketWindow.h
	windows/CPuzzleWindow.h
	windows/CQuestLog.h
	windows/CSpellWindow.h
	windows/CTutorialWindow.h
	windows/CWindowObject.h
	windows/CreaturePurchaseCard.h
	windows/GUIClasses.h
	windows/InfoWindows.h
	windows/QuickRecruitmentWindow.h
	windows/CHeroBackpackWindow.h
	windows/CWindowWithArtifacts.h
	windows/settings/GeneralOptionsTab.h
	windows/settings/OtherOptionsTab.h
	windows/settings/SettingsMainWindow.h
	windows/settings/BattleOptionsTab.h
	windows/settings/AdventureOptionsTab.h

	xBRZ/xbrz.h
	xBRZ/xbrz_tools.h

	ArtifactsUIController.h
	CMT.h
	CPlayerInterface.h
	GameEngine.h
	GameEngineUser.h
	GameInstance.h
	PlayerLocalState.h
	CServerHandler.h
	Client.h
	ClientCommandManager.h
	ClientNetPackVisitors.h
	HeroMovementController.h
	GameChatHandler.h
	LobbyClientNetPackVisitors.h
	ServerRunner.h
	resource.h
	UIHelper.h
)

if(APPLE_IOS)
	set(vcmiclientcommon_SRCS ${vcmiclientcommon_SRCS}
		ios/utils.mm
	)
	set(vcmiclientcommon_HEADERS ${vcmiclientcommon_HEADERS}
		ios/utils.h
	)
endif()

assign_source_group(${vcmiclientcommon_SRCS} ${vcmiclientcommon_HEADERS})
add_library(vcmiclientcommon STATIC ${vcmiclientcommon_SRCS} ${vcmiclientcommon_HEADERS})

if(NOT ENABLE_STATIC_LIBS)
	add_dependencies(vcmiclientcommon
		BattleAI
		EmptyAI
		StupidAI
		VCAI
	)
	if(ENABLE_NULLKILLER_AI)
		add_dependencies(vcmiclientcommon Nullkiller)
	endif()
endif()
if(APPLE_IOS)
	if(ENABLE_ERM)
		add_dependencies(vcmiclientcommon vcmiERM)
	endif()
	if(ENABLE_LUA)
		add_dependencies(vcmiclientcommon vcmiLua)
	endif()
endif()

if(WIN32)
	set_target_properties(vcmiclientcommon
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmiclientcommon"
			PROJECT_LABEL "VCMI_vcmiclientcommon"
	)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT vcmiclientcommon)
	if(NOT ENABLE_DEBUG_CONSOLE)
		target_link_libraries(vcmiclientcommon SDL2::SDL2main)
	endif()
	target_compile_definitions(vcmiclientcommon PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
elseif(APPLE_IOS)
	target_link_libraries(vcmiclientcommon PRIVATE
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
endif()

target_link_libraries(vcmiclientcommon PRIVATE vcmiservercommon)

target_link_libraries(vcmiclientcommon PUBLIC
		vcmi SDL2::SDL2 SDL2::Image SDL2::Mixer SDL2::TTF
)

if(ENABLE_VIDEO)
	target_compile_definitions(vcmiclientcommon PRIVATE ENABLE_VIDEO)
	target_link_libraries(vcmiclientcommon PRIVATE
		${ffmpeg_LIBRARIES}
	)
endif()

target_include_directories(vcmiclientcommon PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (ffmpeg_INCLUDE_DIRS)
	target_include_directories(vcmiclientcommon PRIVATE
		${ffmpeg_INCLUDE_DIRS}
	)
endif()

if(VCMI_PORTMASTER)
	target_compile_definitions(vcmiclientcommon PRIVATE VCMI_PORTMASTER)
endif()

vcmi_set_output_dir(vcmiclientcommon "")
enable_pch(vcmiclientcommon)
