/*
 * IGameServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/serializer/GameConnectionID.h"

VCMI_LIB_NAMESPACE_BEGIN
class PlayerColor;
struct CPackForClient;

VCMI_LIB_NAMESPACE_END

enum class EServerState : ui8
{
	LOBBY,
	GAMEPLAY,
	SHUTDOWN
};

/// Interface through which GameHandler can interact with server that controls it
class IGameServer
{
public:
	virtual ~IGameServer() = default;

	virtual void setState(EServerState value) = 0;
	virtual EServerState getState() const = 0;
	virtual bool isPlayerHost(const PlayerColor & color) const = 0;
	virtual bool hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const = 0;
	virtual bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const = 0;
	virtual void applyPack(CPackForClient & pack) = 0;
	virtual void sendPack(CPackForClient & pack, GameConnectionID connectionID) = 0;
};
