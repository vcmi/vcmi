/*
 * CGameHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CGTownInstance;
class CConnection;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

class PlayerMessageProcessor
{
	std::set<PlayerColor> cheaters;

	void executeCheatCode(const std::string & cheatName, PlayerColor player, ObjectInstanceID currObj, const std::vector<std::string> & arguments );
	bool handleCheatCode(const std::string & cheatFullCommand, PlayerColor player, ObjectInstanceID currObj);
	bool handleHostCommand(PlayerColor player, const std::string & message);

	void cheatGiveSpells(PlayerColor player, const CGHeroInstance * hero);
	void cheatBuildTown(PlayerColor player, const CGTownInstance * town);
	void cheatGiveArmy(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatGiveMachines(PlayerColor player, const CGHeroInstance * hero);
	void cheatGiveArtifacts(PlayerColor player, const CGHeroInstance * hero);
	void cheatLevelup(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatExperience(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatMovement(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatResources(PlayerColor player, std::vector<std::string> words);
	void cheatVictory(PlayerColor player);
	void cheatDefeat(PlayerColor player);
	void cheatMapReveal(PlayerColor player, bool reveal);

public:
	CGameHandler * gameHandler;

	PlayerMessageProcessor();
	PlayerMessageProcessor(CGameHandler * gameHandler);

	/// incoming NetPack handling
	void playerMessage(PlayerColor player, const std::string & message, ObjectInstanceID currObj);

	/// Send message to specific client with "System" as sender
	void sendSystemMessage(std::shared_ptr<CConnection> connection, const std::string & message);

	/// Send message to all players with "System" as sender
	void broadcastSystemMessage(const std::string & message);

	/// Send message from specific player to all other players
	void broadcastMessage(PlayerColor playerSender, const std::string & message);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & cheaters;
	}
};
