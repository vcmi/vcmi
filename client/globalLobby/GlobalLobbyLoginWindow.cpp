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
	pos.h = 260;

	const std::string authMethod = settings["lobby"]["authMethod"].String();

	MetaString loginAs;
	loginAs.appendTextID("vcmi.lobby.login.as");
	loginAs.replaceRawString(GAME->server().getGlobalLobby().getAccountDisplayName());

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>( pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.login.title"));
	labelUsernameTitle = std::make_shared<CLabel>( 10, 65, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.login.username"));
	labelUsername = std::make_shared<CLabel>( 10, 65, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, loginAs.toString(), 265);
	backgroundUsername = std::make_shared<TransparentFilledRectangle>(Rect(10, 90, 264, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputUsername = std::make_shared<CTextInput>(Rect(15, 93, 260, 16), FONT_SMALL, ETextAlignment::CENTERLEFT, true);

	labelPasswordTitle = std::make_shared<CLabel>( 10, 115, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.login.password"));
	backgroundPassword = std::make_shared<TransparentFilledRectangle>(Rect(10, 135, 264, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputPassword = std::make_shared<CTextInput>(Rect(15, 138, 260, 16), FONT_SMALL, ETextAlignment::CENTERLEFT, true);

	buttonLogin = std::make_shared<CButton>(Point(10, 218), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onLogin(); }, EShortcut::GLOBAL_ACCEPT);
	buttonClose = std::make_shared<CButton>(Point(210, 218), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); }, EShortcut::GLOBAL_CANCEL);
	labelStatus = std::make_shared<CTextBox>( "", Rect(15, 163, 255, 50), 1, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	if(authMethod == "forum")
	{
		labelUsername->disable(); // hide "logged in as X"

		const std::string savedName = GAME->server().getGlobalLobby().getAccountDisplayName();
		const bool hasCookie = !GAME->server().getGlobalLobby().getAccountCookie().empty();

		if(!savedName.empty())
			inputUsername->setText(savedName);

		// Hide password field when a cookie is available (no credentials needed initially)
		if(hasCookie)
		{
			labelPasswordTitle->disable();
			backgroundPassword->disable();
			inputPassword->disable();
		}

		auto updateLoginButton = [this]()
		{
			if(inputPassword->isDisabled())
			{
				// Cookie mode: cookie is present (field is only hidden when cookie exists)
				buttonLogin->block(false);
			}
			else
			{
				// Credentials mode: need both username and password
				buttonLogin->block(inputUsername->getText().empty() || inputPassword->getText().empty());
			}
		};

		inputUsername->setCallback([this, savedName, updateLoginButton](const std::string & text)
		{
			if(text != savedName && inputPassword->isDisabled())
			{
				// Username changed — must enter credentials manually
				labelPasswordTitle->enable();
				backgroundPassword->enable();
				inputPassword->enable();
				redraw();
			}
			updateLoginButton();
		});
		inputPassword->setCallback([updateLoginButton](const std::string &){ updateLoginButton(); });
		updateLoginButton();
	}
	else
	{
		// Classic mode: toggle between "create account" and "login with stored account"
		auto buttonRegister    = std::make_shared<CToggleButton>(Point(10,  40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		auto buttonLoginToggle = std::make_shared<CToggleButton>(Point(150, 40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		buttonRegister->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.create"), EFonts::FONT_SMALL, Colors::YELLOW);
		buttonLoginToggle->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.login"), EFonts::FONT_SMALL, Colors::YELLOW);

		toggleMode = std::make_shared<CToggleGroup>(nullptr);
		toggleMode->addToggle(0, buttonRegister);
		toggleMode->addToggle(1, buttonLoginToggle);
		toggleMode->addCallback([this](int index){ onLoginModeChanged(index); });

		if(GAME->server().getGlobalLobby().getAccountID().empty())
		{
			buttonLoginToggle->block(true);
			toggleMode->setSelected(0);
			onLoginModeChanged(0);
		}
		else
		{
			toggleMode->setSelected(1);
			onLoginModeChanged(1);
		}

		inputUsername->setCallback([this](const std::string & text)
		{
			buttonLogin->block(text.empty());
		});
	}

	filledBackground->setPlayerColor(PlayerColor(1));
	center();
}

void GlobalLobbyLoginWindow::onLoginModeChanged(int value)
{
	if (value == 0) // Create account
	{
		inputUsername->enable();
		backgroundUsername->enable();
		labelUsernameTitle->enable();
		labelUsername->disable();
		inputPassword->disable();
		backgroundPassword->disable();
		labelPasswordTitle->disable();
	}
	else // value == 1: Login with stored account
	{
		inputUsername->disable();
		backgroundUsername->disable();
		labelUsernameTitle->disable();
		labelUsername->enable();
		inputPassword->disable();
		backgroundPassword->disable();
		labelPasswordTitle->disable();
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
	const std::string authMethod = settings["lobby"]["authMethod"].String();
	if(authMethod == "forum")
	{
		if(inputPassword->getText().empty())
			GAME->server().getGlobalLobby().sendClientLogin();   // saved cookie
		else
			GAME->server().getGlobalLobby().sendForumLogin(inputUsername->getText(), inputPassword->getText());
		return;
	}

	// Classic mode
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

	const std::string authMethod = settings["lobby"]["authMethod"].String();
	if(authMethod == "forum")
	{
		// Show password field so user can enter credentials (cookie login failed)
		labelPasswordTitle->enable();
		backgroundPassword->enable();
		inputPassword->enable();

		const bool hasUsername = !inputUsername->getText().empty();
		const bool hasPassword = !inputPassword->getText().empty();
		buttonLogin->block(!hasUsername || !hasPassword);
		redraw();
	}
	else
	{
		buttonLogin->block(false);
	}
}
