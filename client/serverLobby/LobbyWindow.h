/*
 * LobbyWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/InterfaceObjectConfigurable.h"
#include "../windows/CWindowObject.h"

#include "../../lib/network/NetworkListener.h"

class LobbyWindow;

class LobbyWidget : public InterfaceObjectConfigurable
{
	LobbyWindow * window;
public:
	LobbyWidget(LobbyWindow * window);

	std::shared_ptr<CLabel> getAccountNameLabel();
	std::shared_ptr<CTextInput> getMessageInput();
	std::shared_ptr<CTextBox> getGameChat();
};

class LobbyClient : public INetworkClientListener
{
	std::unique_ptr<NetworkClient> networkClient;
	LobbyWindow * window;

	void onPacketReceived(const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished() override;
	void onDisconnected() override;

public:
	explicit LobbyClient(LobbyWindow * window);

	void sendMessage(const JsonNode & data);
	void start(const std::string & host, uint16_t port);
	void run();
	void poll();

};

class LobbyWindow : public CWindowObject
{
	std::string chatHistory;

	std::shared_ptr<LobbyWidget> widget;
	std::shared_ptr<LobbyClient> connection;

	void tick(uint32_t msPassed);

public:
	LobbyWindow();

	void doSendChatMessage();

	void onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when);
};
