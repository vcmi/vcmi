/*
 * LobbyWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "LobbyWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"

void LobbyClient::onPacketReceived(const std::vector<uint8_t> & message)
{

}

LobbyWidget::LobbyWidget()
{
	addCallback("closeWindow", [](int) { GH.windows().popWindows(1); });

	const JsonNode config(JsonPath::builtin("config/widgets/lobbyWindow.json"));
	build(config);
}

LobbyWindow::LobbyWindow():
	CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget = std::make_shared<LobbyWidget>();
	pos = widget->pos;
	center();
	connection = std::make_shared<LobbyClient>();

	connection->start("127.0.0.1", 30303);
}
