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
#include "GlobalLobbyWindow.h"

#include "../CGameInfo.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/MetaString.h"

GlobalLobbyLoginWindow::GlobalLobbyLoginWindow()
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos.w = 200;
	pos.h = 200;

	background = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	labelTitle = std::make_shared<CLabel>( pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.lobby.login.title"));
	labelUsername = std::make_shared<CLabel>( 10, 45, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->translate("vcmi.lobby.login.username"));
	backgroundUsername = std::make_shared<TransparentFilledRectangle>(Rect(10, 70, 180, 20), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64));
	inputUsername = std::make_shared<CTextInput>(Rect(15, 73, 176, 16), FONT_SMALL, nullptr, ETextAlignment::TOPLEFT, true);
	buttonLogin = std::make_shared<CButton>(Point(10, 160), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ onLogin(); });
	buttonClose = std::make_shared<CButton>(Point(126, 160), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ onClose(); });
	labelStatus = std::make_shared<CTextBox>( "", Rect(15, 95, 175, 60), 1, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	background->playerColored(PlayerColor(1));
	inputUsername->setText(settings["lobby"]["displayName"].String());
	inputUsername->cb += [this](const std::string & text)
	{
		buttonLogin->block(text.empty());
	};

	center();
}

void GlobalLobbyLoginWindow::onClose()
{
	close();
	// TODO: abort ongoing connection attempt, if any
}

void GlobalLobbyLoginWindow::onLogin()
{
	Settings config = settings.write["lobby"]["displayName"];
	config->String() = inputUsername->getText();

	labelStatus->setText(CGI->generaltexth->translate("vcmi.lobby.login.connecting"));
	if(!CSH->getGlobalLobby().isConnected())
		CSH->getGlobalLobby().connect();
	buttonClose->block(true);
}

void GlobalLobbyLoginWindow::onConnectionSuccess()
{
	close();
	GH.windows().pushWindow(CSH->getGlobalLobby().createLobbyWindow());
}

void GlobalLobbyLoginWindow::onConnectionFailed(const std::string & reason)
{
	MetaString formatter;
	formatter.appendTextID("vcmi.lobby.login.error");
	formatter.replaceRawString(reason);

	labelStatus->setText(formatter.toString());
	buttonClose->block(false);
}
