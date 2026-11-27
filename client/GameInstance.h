/*
 * GameInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameEngineUser.h"

#include "../lib/texts/CompositeTranslator.h"

class CServerHandler;
class GlobalLobbyClient;
class CPlayerInterface;
class CMapHandler;
class CMainMenu;
class CMainMenuConfig;
class ClientLibraries;
class ModManager;

class INetworkHandler;

class GameShutdownException final : public std::exception
{
public:
	const char* what() const noexcept final
	{
		return "Game shutdown has been requested";
	}
};

class GameInstance final : boost::noncopyable, public IGameEngineUser
{
	/// Only the overlay may compose the translator, so the concrete type stays out of reach
	friend class TranslatorOverlay;

	std::unique_ptr<ClientLibraries> librariesInstance;
	std::unique_ptr<CompositeTranslator> translatorInstance;
	std::unique_ptr<CServerHandler> serverInstance;
	std::unique_ptr<CMapHandler> mapInstance;
	std::shared_ptr<CMainMenu> mainMenuInstance;
	std::unique_ptr<CMainMenuConfig> mainMenuConfigInstance;
	CPlayerInterface * interfaceInstance;

	void pauseAutoSave();
	/// Reloads library of the active preset if current library has other mods, keeping the active tab of the main menu
	void returnToHomeLibrary();

public:
	explicit GameInstance(std::unique_ptr<ModManager> modManager);
	~GameInstance();

	ClientLibraries & libraries();
	CServerHandler & server();
	CMapHandler & map();
	ITranslator & translator();

	/// Creates client data of current LIBRARY: audio, graphics, render handler data, dialog borders, cursor
	void createLibraryClientState();
	/// Destroys windows, main menu and client data of current LIBRARY, so another library can become current. Requires no running game
	void releaseLibraryClientState();
	/// Shows mods of current LIBRARY that failed to load
	void checkForModLoadingFailure();

	std::shared_ptr<CMainMenu> mainmenu();
	const CMainMenuConfig & mainMenuConfig();
	CPlayerInterface * interface();

	void setServerInstance(std::unique_ptr<CServerHandler> ptr);
	void setMapInstance(std::unique_ptr<CMapHandler> ptr);

	/// installs a new map handler and hands the previous one back to the caller
	std::unique_ptr<CMapHandler> swapMapInstance(std::unique_ptr<CMapHandler> ptr);

	void setInterfaceInstance(CPlayerInterface * ptr);

	void onGlobalLobbyInterfaceActivated() final;
	void onUpdate() final;
	bool capturedAllEvents() final;
	bool wantsFrameRendered() final;
	void onShutdownRequested(bool askForConfirmation) final;
	void onAppPaused() final;
};

extern std::unique_ptr<GameInstance> GAME;
