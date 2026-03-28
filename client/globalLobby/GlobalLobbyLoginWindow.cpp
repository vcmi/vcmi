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

	const std::string savedName = GAME->server().getGlobalLobby().getAccountDisplayName();

	MetaString loginAs;
	loginAs.appendTextID("vcmi.lobby.login.as");
	loginAs.replaceRawString(savedName);

	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>(pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.login.title"));

	// Auth method toggle [Forum Login] [Classic Login] – always visible
	{
		auto buttonForum   = std::make_shared<CToggleButton>(Point(10,  40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		auto buttonClassic = std::make_shared<CToggleButton>(Point(150, 40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		buttonForum->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.forum"),   EFonts::FONT_SMALL, Colors::YELLOW);
		buttonClassic->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.classic"), EFonts::FONT_SMALL, Colors::YELLOW);
		toggleAuthMethod = std::make_shared<CToggleGroup>(nullptr);
		toggleAuthMethod->addToggle(0, buttonForum);
		toggleAuthMethod->addToggle(1, buttonClassic);
		toggleAuthMethod->addCallback([this](int index){ onAuthMethodChanged(index); });
	}

	// Username label (forum mode) / "Login as X" label (classic-saved mode)
	// labelUsernameTitle is at y=70 (same as sub-toggle, mutually exclusive).
	// labelUsername is at y=96, below the classic sub-toggle buttons.
	labelUsernameTitle = std::make_shared<CLabel>(10, 70, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.login.username"));
	labelUsername      = std::make_shared<CLabel>(10, 96, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, loginAs.toString(), 265);

	// Username input (forum mode + classic-create mode)
	backgroundUsername = std::make_shared<TransparentFilledRectangle>(Rect(10, 93, 264, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputUsername      = std::make_shared<CTextInput>(Rect(15, 96, 260, 16), FONT_SMALL, ETextAlignment::CENTERLEFT, true);

	// Password (forum mode only – progressive reveal)
	labelPasswordTitle = std::make_shared<CLabel>(10, 121, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.login.password"));
	backgroundPassword = std::make_shared<TransparentFilledRectangle>(Rect(10, 139, 264, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputPassword      = std::make_shared<CTextInput>(Rect(15, 142, 260, 16), FONT_SMALL, ETextAlignment::CENTERLEFT, true);
	inputPassword->setPasswordMode(true);

	// Classic sub-toggle [New Account] [Login]
	// Buttons sit at y=70, same row as labelUsernameTitle – only one is visible at a time.
	{
		auto buttonRegister    = std::make_shared<CToggleButton>(Point(10,  70), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		auto buttonLoginToggle = std::make_shared<CToggleButton>(Point(150, 70), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), 0);
		buttonRegister->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.create"), EFonts::FONT_SMALL, Colors::YELLOW);
		buttonLoginToggle->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.login.login"), EFonts::FONT_SMALL, Colors::YELLOW);
		if(GAME->server().getGlobalLobby().getAccountID().empty())
			buttonLoginToggle->block(true);
		toggleMode = std::make_shared<CToggleGroup>(nullptr);
		toggleMode->addToggle(0, buttonRegister);
		toggleMode->addToggle(1, buttonLoginToggle);
		toggleMode->addCallback([this](int index){ onLoginModeChanged(index); });
	}

	buttonLogin = std::make_shared<CButton>(Point(10, 218), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onLogin(); }, EShortcut::GLOBAL_ACCEPT);
	buttonClose = std::make_shared<CButton>(Point(210, 218), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); }, EShortcut::GLOBAL_CANCEL);
	labelStatus = std::make_shared<CTextBox>("", Rect(15, 167, 255, 45), 1, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	// Username callback: handles both forum and classic-create mode
	inputUsername->setCallback([this, savedName](const std::string & text)
	{
		if(toggleAuthMethod->getSelected() == 0) // Forum
		{
			if(text != savedName && inputPassword->isDisabled())
			{
				labelPasswordTitle->enable();
				backgroundPassword->enable();
				inputPassword->enable();
				redraw();
			}
			updateLoginButton();
		}
		else // Classic-create
		{
			buttonLogin->block(text.empty());
		}
	});
	inputPassword->setCallback([this](const std::string &){ updateLoginButton(); });

	// Pre-fill username for forum mode
	if(!savedName.empty())
		inputUsername->setText(savedName);

	// Select forum tab by default (calls onAuthMethodChanged(0))
	toggleAuthMethod->setSelected(0);

	filledBackground->setPlayerColor(PlayerColor(1));
	center();
}

void GlobalLobbyLoginWindow::updateLoginButton()
{
	if(inputPassword->isDisabled())
		buttonLogin->block(false); // cookie mode – cookie is present
	else
		buttonLogin->block(inputUsername->getText().empty() || inputPassword->getText().empty());
}

void GlobalLobbyLoginWindow::onAuthMethodChanged(int mode)
{
	// CToggleGroup stores CToggleBase; need CButton interface for enable/disable/block
	auto classicBtn = [&](int idx) -> CButton *
	{
		return dynamic_cast<CButton *>(toggleMode->buttons[idx].get());
	};

	if(mode == 0) // Forum
	{
		// Hide classic sub-toggle buttons
		if(auto * b = classicBtn(0)) b->disable();
		if(auto * b = classicBtn(1)) b->disable();
		labelUsername->disable();

		// Show forum username area
		labelUsernameTitle->enable();
		backgroundUsername->enable();
		inputUsername->enable();

		// Show password conditionally (hide when saved cookie matches current username)
		const std::string savedName = GAME->server().getGlobalLobby().getAccountDisplayName();
		const bool hasCookie = !GAME->server().getGlobalLobby().getAccountCookie().empty();
		if(hasCookie && inputUsername->getText() == savedName)
		{
			labelPasswordTitle->disable();
			backgroundPassword->disable();
			inputPassword->disable();
		}
		else
		{
			labelPasswordTitle->enable();
			backgroundPassword->enable();
			inputPassword->enable();
		}
		updateLoginButton();
	}
	else // Classic
	{
		// Hide forum password area
		labelPasswordTitle->disable();
		backgroundPassword->disable();
		inputPassword->disable();
		// Hide forum username label (sub-toggle buttons go at same y-position)
		labelUsernameTitle->disable();

		// Show classic sub-toggle buttons
		if(auto * b = classicBtn(0)) b->enable();
		if(auto * b = classicBtn(1)) b->enable();

		// Select appropriate classic sub-mode and apply
		if(GAME->server().getGlobalLobby().getAccountID().empty())
		{
			if(auto * b = classicBtn(1)) b->block(true);
			toggleMode->setSelected(0);
			onLoginModeChanged(0);
		}
		else
		{
			toggleMode->setSelected(1);
			onLoginModeChanged(1);
		}
	}
	redraw();
}

void GlobalLobbyLoginWindow::onLoginModeChanged(int value)
{
	if(value == 0) // Create account
	{
		inputUsername->enable();
		backgroundUsername->enable();
		labelUsername->disable();
		// No password in classic mode
		labelPasswordTitle->disable();
		backgroundPassword->disable();
		inputPassword->disable();
		buttonLogin->block(inputUsername->getText().empty());
	}
	else // value == 1: Login with stored account
	{
		inputUsername->disable();
		backgroundUsername->disable();
		labelUsername->enable();
		labelPasswordTitle->disable();
		backgroundPassword->disable();
		inputPassword->disable();
		buttonLogin->block(false);
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
	if(toggleAuthMethod->getSelected() == 0) // Forum
	{
		if(inputPassword->isDisabled())
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

	if(toggleAuthMethod->getSelected() == 0) // Forum
	{
		// Reveal password field so the user can enter credentials
		labelPasswordTitle->enable();
		backgroundPassword->enable();
		inputPassword->enable();
		buttonLogin->block(inputUsername->getText().empty() || inputPassword->getText().empty());
		redraw();
	}
	else // Classic
	{
		buttonLogin->block(false);
	}
}
