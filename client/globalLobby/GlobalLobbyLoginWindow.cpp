/*
 * GlobalLobbyLoginWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GlobalLobbyLoginWindow.h"

#include "GlobalLobbyClient.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/GameLibrary.h"

GlobalLobbyLoginWindow::GlobalLobbyLoginWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 284;
	pos.h = 220;

	MetaString loginAs;
	loginAs.appendTextID("vcmi.lobby.login.as");
	loginAs.replaceRawString(GAME->server().getGlobalLobby().getAccountDisplayName());

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>( pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.login.title"));
	labelUsernameTitle = std::make_shared<CLabel>( 10, 65, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.login.username"));
	labelUsername = std::make_shared<CLabel>( 10, 65, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, loginAs.toString(), 265);
	backgroundUsername = std::make_shared<TransparentFilledRectangle>(Rect(10, 90, 264, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputUsername = std::make_shared<CTextInput>(Rect(15, 93, 260, 16), FONT_SMALL, ETextAlignment::CENTERLEFT, true);
	buttonLogin = std::make_shared<CButton>(Point(10, 180), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onLogin(); }, EShortcut::GLOBAL_ACCEPT);
	buttonClose = std::make_shared<CButton>(Point(210, 180), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); }, EShortcut::GLOBAL_CANCEL);
	labelStatus = std::make_shared<CTextBox>( "", Rect(15, 115, 255, 60), 1, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	auto buttonRegister = std::make_shared<CToggleButton>(Point(10, 40),  AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	auto buttonLogin = std::make_shared<CToggleButton>(Point(146, 40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
	buttonRegister->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.create"), EFonts::FONT_SMALL, Colors::YELLOW);
	buttonLogin->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.login"), EFonts::FONT_SMALL, Colors::YELLOW);

	toggleMode = std::make_shared<CToggleGroup>(nullptr);
	toggleMode->addToggle(0, buttonRegister);
	toggleMode->addToggle(1, buttonLogin);
	toggleMode->setSelected(settings["lobby"]["roomType"].Integer());
	toggleMode->addCallback([this](int index){onLoginModeChanged(index);});

	if (GAME->server().getGlobalLobby().getAccountID().empty())
	{
		buttonLogin->block(true);
		toggleMode->setSelected(0);
		onLoginModeChanged(0); // call it manually to disable widgets - toggleMode will not emit this call if this is currently selected option
	}
	else
	{
		toggleMode->setSelected(1);
		onLoginModeChanged(1);
	}

	filledBackground->setPlayerColor(PlayerColor(1));
	inputUsername->setCallback([this](const std::string & text)
	{
		this->buttonLogin->block(text.empty());
	});

	center();
}

void GlobalLobbyLoginWindow::onLoginModeChanged(int value)
{
	if (value == 0)
	{
		inputUsername->enable();
		backgroundUsername->enable();
		labelUsernameTitle->enable();
		labelUsername->disable();
	}
	else
	{
		inputUsername->disable();
		backgroundUsername->disable();
		labelUsernameTitle->disable();
		labelUsername->enable();
	}
	redraw();
}

void GlobalLobbyLoginWindow::onClose()
{
	close();
	// TODO: abort ongoing connection attempt, if any
}

void GlobalLobbyLoginWindow::onLogin()
{
	labelStatus->setText(LIBRARY->generaltexth->translate("vcmi.lobby.login.connecting"));
	if(!GAME->server().getGlobalLobby().isConnected())
		GAME->server().getGlobalLobby().connect();
	else
		onConnectionSuccess();

	buttonClose->block(true);
	buttonLogin->block(true);
}

void GlobalLobbyLoginWindow::onConnectionSuccess()
{
	std::string accountID = GAME->server().getGlobalLobby().getAccountID();

	if(toggleMode->getSelected() == 0)
		GAME->server().getGlobalLobby().sendClientRegister(inputUsername->getText());
	else
		GAME->server().getGlobalLobby().sendClientLogin();
}

void GlobalLobbyLoginWindow::onLoginSuccess()
{
	close();
	GAME->server().getGlobalLobby().activateInterface();
}

void GlobalLobbyLoginWindow::onConnectionFailed(const std::string & reason)
{
	MetaString formatter;
	formatter.appendTextID("vcmi.lobby.login.error");
	formatter.replaceRawString(reason);

	labelStatus->setText(formatter.toString());
	buttonClose->block(false);
	buttonLogin->block(false);
}
