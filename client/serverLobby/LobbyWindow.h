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

#include "../../lib/network/NetworkClient.h"

class LobbyWidget : public InterfaceObjectConfigurable
{
public:
	LobbyWidget();
};

class LobbyClient : public NetworkClient
{
	void onPacketReceived(const std::vector<uint8_t> & message) override;
public:
	LobbyClient() = default;
};

class LobbyWindow : public CWindowObject
{
	std::shared_ptr<LobbyWidget> widget;
	std::shared_ptr<LobbyClient> connection;

public:
	LobbyWindow();
};
