/*
 * GlobalLobbyServerSetup.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyServerSetup.h"

#include "GlobalLobbyClient.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../mainmenu/CMainMenu.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/GameLibrary.h"

GlobalLobbyServerSetup::GlobalLobbyServerSetup()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 284;
	pos.h = 340;

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>( pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.room.create.hover"));
	labelPlayerLimit = std::make_shared<CLabel>( pos.w / 2, 48, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.room.players.limit"));
	labelRoomType = std::make_shared<CLabel>( pos.w / 2, 108, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.room.type"));
	labelGameMode = std::make_shared<CLabel>( pos.w / 2, 158, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.room.mode"));

	togglePlayerLimit = std::make_shared<CToggleGroup>(nullptr);
	togglePlayerLimit->addToggle(2, std::make_shared<CToggleButton>(Point(10 + 39*0, 60), AnimationPath::builtin("RanNum2"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(3, std::make_shared<CToggleButton>(Point(10 + 39*1, 60), AnimationPath::builtin("RanNum3"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(4, std::make_shared<CToggleButton>(Point(10 + 39*2, 60), AnimationPath::builtin("RanNum4"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(5, std::make_shared<CToggleButton>(Point(10 + 39*3, 60), AnimationPath::builtin("RanNum5"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(6, std::make_shared<CToggleButton>(Point(10 + 39*4, 60), AnimationPath::builtin("RanNum6"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(7, std::make_shared<CToggleButton>(Point(10 + 39*5, 60), AnimationPath::builtin("RanNum7"), CButton::tooltip(), 0));
	togglePlayerLimit->addToggle(8, std::make_shared<CToggleButton>(Point(10 + 39*6, 60), AnimationPath::builtin("RanNum8"), CButton::tooltip(), 0));
	togglePlayerLimit->setSelected(settings["lobby"]["roomPlayerLimit"].Integer());
	togglePlayerLimit->addCallback([this](int index){onPlayerLimitChanged(index);});

	auto buttonPublic  = std::make_shared<CToggleButton>(Point(10, 120),  AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	auto buttonPrivate = std::make_shared<CToggleButton>(Point(146, 120), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	buttonPublic->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.room.state.public"), EFonts::FONT_SMALL, Colors::YELLOW);
	buttonPrivate->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.room.state.private"), EFonts::FONT_SMALL, Colors::YELLOW);

	toggleRoomType = std::make_shared<CToggleGroup>(nullptr);
	toggleRoomType->addToggle(0, buttonPublic);
	toggleRoomType->addToggle(1, buttonPrivate);
	toggleRoomType->setSelected(settings["lobby"]["roomType"].Integer());
	toggleRoomType->addCallback([this](int index){onRoomTypeChanged(index);});

	auto buttonNewGame = std::make_shared<CToggleButton>(Point(10, 170),  AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	auto buttonLoadGame = std::make_shared<CToggleButton>(Point(146, 170), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	buttonNewGame->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.room.new"), EFonts::FONT_SMALL, Colors::YELLOW);
	buttonLoadGame->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.room.load"), EFonts::FONT_SMALL, Colors::YELLOW);

	toggleGameMode = std::make_shared<CToggleGroup>(nullptr);
	toggleGameMode->addToggle(0, buttonNewGame);
	toggleGameMode->addToggle(1, buttonLoadGame);
	toggleGameMode->setSelected(settings["lobby"]["roomMode"].Integer());
	toggleGameMode->addCallback([this](int index){onGameModeChanged(index);});

	labelDescription = std::make_shared<CTextBox>("", Rect(10, 195, pos.w - 20, 80), 1, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	buttonCreate = std::make_shared<CButton>(Point(10, 300), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onCreate(); }, EShortcut::GLOBAL_ACCEPT);
	buttonClose = std::make_shared<CButton>(Point(210, 300), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); }, EShortcut::GLOBAL_CANCEL);

	filledBackground->setPlayerColor(PlayerColor(1));

	updateDescription();
	center();
}

void GlobalLobbyServerSetup::updateDescription()
{
	MetaString description;
	description.appendRawString("%s %s %s");
	if(toggleRoomType->getSelected() == 0)
		description.replaceTextID("vcmi.lobby.room.description.public");
	else
		description.replaceTextID("vcmi.lobby.room.description.private");

	if(toggleGameMode->getSelected() == 0)
		description.replaceTextID("vcmi.lobby.room.description.new");
	else
		description.replaceTextID("vcmi.lobby.room.description.load");

	description.replaceTextID("vcmi.lobby.room.description.limit");
	description.replaceNumber(togglePlayerLimit->getSelected());

	labelDescription->setText(description.toString());
}

void GlobalLobbyServerSetup::onPlayerLimitChanged(int value)
{
	Settings config = settings.write["lobby"]["roomPlayerLimit"];
	config->Integer() = value;
	updateDescription();
}

void GlobalLobbyServerSetup::onRoomTypeChanged(int value)
{
	Settings config = settings.write["lobby"]["roomType"];
	config->Integer() = value;
	updateDescription();
}

void GlobalLobbyServerSetup::onGameModeChanged(int value)
{
	Settings config = settings.write["lobby"]["roomMode"];
	config->Integer() = value;
	updateDescription();
}

void GlobalLobbyServerSetup::onCreate()
{
	if(toggleGameMode->getSelected() == 0)
		GAME->server().resetStateForLobby(EStartMode::NEW_GAME, ESelectionScreen::newGame, EServerMode::LOBBY_HOST, { GAME->server().getGlobalLobby().getAccountDisplayName() });
	else
		GAME->server().resetStateForLobby(EStartMode::LOAD_GAME, ESelectionScreen::loadGame, EServerMode::LOBBY_HOST, { GAME->server().getGlobalLobby().getAccountDisplayName() });

	GAME->server().loadMode = ELoadMode::MULTI;
	GAME->server().startLocalServerAndConnect(true);

	buttonCreate->block(true);
	buttonClose->block(true);
}

void GlobalLobbyServerSetup::onClose()
{
	close();
}
