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
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

#include "../../lib/MetaString.h"
#include "../../lib/CConfigHandler.h"

LobbyClient::LobbyClient(LobbyWindow * window)
	: window(window)
{

}

static std::string getCurrentTimeFormatted(int timeOffsetSeconds = 0)
{
	// FIXME: better/unified way to format date
	auto timeNowChrono = std::chrono::system_clock::now();
	timeNowChrono += std::chrono::seconds(timeOffsetSeconds);

	std::time_t timeNowC = std::chrono::system_clock::to_time_t(timeNowChrono);
	std::tm timeNowTm = *std::localtime(&timeNowC);

	MetaString timeFormatted;
	timeFormatted.appendRawString("%d:%d");
	timeFormatted.replaceNumber(timeNowTm.tm_hour);
	timeFormatted.replaceNumber(timeNowTm.tm_min);

	return timeFormatted.toString();
}

void LobbyClient::onPacketReceived(const std::vector<uint8_t> & message)
{
	// FIXME: find better approach
	const char * payloadBegin = reinterpret_cast<const char*>(message.data());
	JsonNode json(payloadBegin, message.size());

	if (json["type"].String() == "chatHistory")
	{
		for (auto const & entry : json["messages"].Vector())
		{
			std::string senderName = entry["senderName"].String();
			std::string messageText = entry["messageText"].String();
			int ageSeconds = entry["ageSeconds"].Integer();
			std::string timeFormatted = getCurrentTimeFormatted(-ageSeconds);
			window->onGameChatMessage(senderName, messageText, timeFormatted);
		}
	}

	if (json["type"].String() == "chatMessage")
	{
		std::string senderName = json["senderName"].String();
		std::string messageText = json["messageText"].String();
		std::string timeFormatted = getCurrentTimeFormatted();
		window->onGameChatMessage(senderName, messageText, timeFormatted);
	}
}

void LobbyClient::onConnectionEstablished()
{
	JsonNode toSend;
	toSend["type"].String() = "authentication";
	toSend["accountName"].String() = settings["general"]["playerName"].String();

	sendMessage(toSend);
}

void LobbyClient::onConnectionFailed(const std::string & errorMessage)
{
	GH.windows().popWindows(1);
	CInfoWindow::showInfoDialog("Failed to connect to game lobby!\n" + errorMessage, {});
}

void LobbyClient::onDisconnected()
{
	GH.windows().popWindows(1);
	CInfoWindow::showInfoDialog("Connection to game lobby was lost!", {});
}

void LobbyClient::sendMessage(const JsonNode & data)
{
	std::string payloadString = data.toJson(true);

	// FIXME: find better approach
	uint8_t * payloadBegin = reinterpret_cast<uint8_t*>(payloadString.data());
	uint8_t * payloadEnd = payloadBegin + payloadString.size();

	std::vector<uint8_t> payloadBuffer(payloadBegin, payloadEnd);

	sendPacket(payloadBuffer);
}

LobbyWidget::LobbyWidget(LobbyWindow * window)
	: window(window)
{
	addCallback("closeWindow", [](int) { GH.windows().popWindows(1); });
	addCallback("sendMessage", [this](int) { this->window->doSendChatMessage(); });

	const JsonNode config(JsonPath::builtin("config/widgets/lobbyWindow.json"));
	build(config);
}

std::shared_ptr<CLabel> LobbyWidget::getAccountNameLabel()
{
	return widget<CLabel>("accountNameLabel");
}

std::shared_ptr<CTextInput> LobbyWidget::getMessageInput()
{
	return widget<CTextInput>("messageInput");
}

std::shared_ptr<CTextBox> LobbyWidget::getGameChat()
{
	return widget<CTextBox>("gameChat");
}

LobbyWindow::LobbyWindow():
	CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget = std::make_shared<LobbyWidget>(this);
	pos = widget->pos;
	center();
	connection = std::make_shared<LobbyClient>(this);

	connection->start("127.0.0.1", 30303);
	widget->getAccountNameLabel()->setText(settings["general"]["playerName"].String());

	addUsedEvents(TIME);
}

void LobbyWindow::tick(uint32_t msPassed)
{
	connection->poll();
}

void LobbyWindow::doSendChatMessage()
{
	std::string messageText = widget->getMessageInput()->getText();

	JsonNode toSend;
	toSend["type"].String() = "sendChatMessage";
	toSend["messageText"].String() = messageText;

	connection->sendMessage(toSend);

	widget->getMessageInput()->setText("");
}

void LobbyWindow::onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when)
{
	MetaString chatMessageFormatted;
	chatMessageFormatted.appendRawString("[%s] {%s}: %s\n");
	chatMessageFormatted.replaceRawString(when);
	chatMessageFormatted.replaceRawString(sender);
	chatMessageFormatted.replaceRawString(message);

	chatHistory += chatMessageFormatted.toString();

	widget->getGameChat()->setText(chatHistory);
}
