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

#include "../../lib/GameConstants.h"
#include "../../lib/serializer/GameConnectionID.h"
#include "../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CGTownInstance;
class MetaString;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

enum class ECurrentChatVote : int8_t
{
	NONE = -1,
	SIMTURNS_ALLOW,
	SIMTURNS_FORCE,
	SIMTURNS_ABORT,
	TIMER_PROLONG,
};

class PlayerMessageProcessor
{
	CGameHandler * gameHandler;

	ECurrentChatVote currentVote = ECurrentChatVote::NONE;
	int currentVoteParameter = 0;
	std::set<PlayerColor> awaitingPlayers;
	JsonNode cheatConfig;

	void executeCheatCode(const std::string & cheatName, PlayerColor player, ObjectInstanceID currObj, const std::vector<std::string> & arguments );
	bool handleCheatCode(const std::string & cheatFullCommand, PlayerColor player, ObjectInstanceID currObj);
	void handleCommand(PlayerColor player, const std::string & message);

	void cheatGiveSpells(PlayerColor player, const CGHeroInstance * hero);
	void cheatBuildTown(PlayerColor player, const CGTownInstance * town);
	void cheatGiveArmy(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatGiveMachines(PlayerColor player, const CGHeroInstance * hero);
	void cheatGiveArtifacts(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatGiveScrolls(PlayerColor player, const CGHeroInstance * hero);
	void cheatColorSchemeChange(PlayerColor player, ColorScheme scheme);
	void cheatLevelup(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatExperience(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatMovement(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatResources(PlayerColor player, std::vector<std::string> words);
	void cheatVictory(PlayerColor player);
	void cheatDefeat(PlayerColor player);
	void cheatMapReveal(PlayerColor player, bool reveal);
	void cheatPuzzleReveal(PlayerColor player);
	void cheatMaxLuck(PlayerColor player, const CGHeroInstance * hero);
	void cheatMaxMorale(PlayerColor player, const CGHeroInstance * hero);
	void cheatFly(PlayerColor player, const CGHeroInstance * hero);
	void cheatSkill(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);
	void cheatTeleport(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words);

	void commandExit(PlayerColor player, const std::vector<std::string> & words);
	void commandKick(PlayerColor player, const std::vector<std::string> & words);
	void commandSave(PlayerColor player, const std::vector<std::string> & words);
	void commandCheaters(PlayerColor player, const std::vector<std::string> & words);
	void commandStatistic(PlayerColor player, const std::vector<std::string> & words);
	void commandHelp(PlayerColor player, const std::vector<std::string> & words);
	void commandVote(PlayerColor player, const std::vector<std::string> & words);

	void finishVoting();
	void abortVoting();
	void startVoting(PlayerColor initiator, ECurrentChatVote what, int parameter);

public:
	PlayerMessageProcessor(CGameHandler * gameHandler);

	/// incoming NetPack handling
	void playerMessage(PlayerColor player, const std::string & message, ObjectInstanceID currObj);

	/// Send message to specific client with "System" as sender
	void sendSystemMessage(GameConnectionID connectionID, const MetaString & message);
	void sendSystemMessage(GameConnectionID connectionID, const std::string & message);

	/// Send message to all players with "System" as sender
	void broadcastSystemMessage(MetaString message);
	void broadcastSystemMessage(const std::string & message);

	/// Send message from specific player to all other players
	void broadcastMessage(PlayerColor playerSender, const std::string & message);

	template <typename Handler> void serialize(Handler &h)
	{
	}
};
