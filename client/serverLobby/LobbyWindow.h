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

class GlobalLobbyWindow;

class GlobalLobbyWidget : public InterfaceObjectConfigurable
{
	GlobalLobbyWindow * window;
public:
	GlobalLobbyWidget(GlobalLobbyWindow * window);

	std::shared_ptr<CLabel> getAccountNameLabel();
	std::shared_ptr<CTextInput> getMessageInput();
	std::shared_ptr<CTextBox> getGameChat();
};

class GlobalLobbyClient : public INetworkClientListener
{
	std::unique_ptr<NetworkClient> networkClient;
	GlobalLobbyWindow * window;

	void onPacketReceived(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<NetworkConnection> &) override;
	void onDisconnected(const std::shared_ptr<NetworkConnection> &) override;
	void onTimer() override;

public:
	explicit GlobalLobbyClient(GlobalLobbyWindow * window);

	void sendMessage(const JsonNode & data);
	void start(const std::string & host, uint16_t port);
	void run();
	void poll();

};

class GlobalLobbyWindow : public CWindowObject
{
	std::string chatHistory;

	std::shared_ptr<GlobalLobbyWidget> widget;
	std::shared_ptr<GlobalLobbyClient> connection;

	void tick(uint32_t msPassed);

public:
	GlobalLobbyWindow();

	void doSendChatMessage();

	void onGameChatMessage(const std::string & sender, const std::string & message, const std::string & when);
};
