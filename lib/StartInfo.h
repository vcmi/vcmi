/*
 * StartInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"
#include "TurnTimerInfo.h"
#include "campaign/CampaignConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapGenOptions;
class CampaignState;
class CMapInfo;
struct PlayerInfo;
class PlayerColor;

/// Struct which describes the name, the color, the starting bonus of a player
struct DLL_LINKAGE PlayerSettings
{
	enum { PLAYER_AI = 0 }; // for use in playerID

	enum Ebonus {
		NONE     = -2,
		RANDOM   = -1,
		ARTIFACT =  0,
		GOLD     =  1,
		RESOURCE =  2
	};

	Ebonus bonus;
	FactionID castle;
	HeroTypeID hero;
	HeroTypeID heroPortrait; //-1 if default, else ID

	std::string heroName;
	PlayerColor color; //from 0 -
	enum EHandicap {NO_HANDICAP, MILD, SEVERE};
	EHandicap handicap;//0-no, 1-mild, 2-severe

	std::string name;
	std::set<ui8> connectedPlayerIDs; //Empty - AI, or connectrd player ids
	bool compOnly; //true if this player is a computer only player; required for RMG
	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & castle;
		h & hero;
		h & heroPortrait;
		h & heroName;
		h & bonus;
		h & color;
		h & handicap;
		h & name;
		h & connectedPlayerIDs;
		h & compOnly;
	}

	PlayerSettings();
	bool isControlledByAI() const;
	bool isControlledByHuman() const;
};

/// Struct which describes the difficulty, the turn time,.. of a heroes match.
struct DLL_LINKAGE StartInfo
{
	enum EMode {NEW_GAME, LOAD_GAME, CAMPAIGN, INVALID = 255};

	EMode mode;
	ui8 difficulty; //0=easy; 4=impossible

	using TPlayerInfos = std::map<PlayerColor, PlayerSettings>;
	TPlayerInfos playerInfos; //color indexed

	ui32 seedToBeUsed; //0 if not sure (client requests server to decide, will be send in reply pack)
	ui32 seedPostInit; //so we know that game is correctly synced at the start; 0 if not known yet
	ui32 mapfileChecksum; //0 if not relevant
	std::string gameUuid;
	TurnTimerInfo turnTimerInfo;
	std::string mapname; // empty for random map, otherwise name of the map or savegame
	bool createRandomMap() const { return mapGenOptions != nullptr; }
	std::shared_ptr<CMapGenOptions> mapGenOptions;

	std::shared_ptr<CampaignState> campState;

	PlayerSettings & getIthPlayersSettings(const PlayerColor & no);
	const PlayerSettings & getIthPlayersSettings(const PlayerColor & no) const;
	PlayerSettings * getPlayersSettings(const ui8 connectedPlayerId);

	// TODO: Must be client-side
	std::string getCampaignName() const;

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & mode;
		h & difficulty;
		h & playerInfos;
		h & seedToBeUsed;
		h & seedPostInit;
		h & mapfileChecksum;
		h & gameUuid;
		h & turnTimerInfo;
		h & mapname;
		h & mapGenOptions;
		h & campState;
	}

	StartInfo() : mode(INVALID), difficulty(1), seedToBeUsed(0), seedPostInit(0),
		mapfileChecksum(0), uuid(boost::uuids::to_string(boost::uuids::random_generator()()))
	{

	}
};

struct ClientPlayer
{
	int connection;
	std::string name;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & connection;
		h & name;
	}
};

struct DLL_LINKAGE LobbyState
{
	std::shared_ptr<StartInfo> si;
	std::shared_ptr<CMapInfo> mi;
	std::map<ui8, ClientPlayer> playerNames; // id of player <-> player name; 0 is reserved as ID of AI "players"
	int hostClientId;
	// TODO: Campaign-only and we don't really need either of them.
	// Before start both go into CCampaignState that is part of StartInfo
	CampaignScenarioID campaignMap;
	int campaignBonus;

	LobbyState() : si(new StartInfo()), hostClientId(-1), campaignMap(CampaignScenarioID::NONE), campaignBonus(-1) {}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & si;
		h & mi;
		h & playerNames;
		h & hostClientId;
		h & campaignMap;
		h & campaignBonus;
	}
};

struct DLL_LINKAGE LobbyInfo : public LobbyState
{
	boost::mutex stateMutex;
	std::string uuid;

	LobbyInfo() {}

	void verifyStateBeforeStart(bool ignoreNoHuman = false) const;

	bool isClientHost(int clientId) const;
	std::set<PlayerColor> getAllClientPlayers(int clientId);
	std::vector<ui8> getConnectedPlayerIdsForClient(int clientId) const;

	// Helpers for lobby state access
	std::set<PlayerColor> clientHumanColors(int clientId);
	PlayerColor clientFirstColor(int clientId) const;
	bool isClientColor(int clientId, const PlayerColor & color) const;
	ui8 clientFirstId(int clientId) const; // Used by chat only!
	PlayerInfo & getPlayerInfo(int color);
	TeamID getPlayerTeamId(const PlayerColor & color);
};


VCMI_LIB_NAMESPACE_END
