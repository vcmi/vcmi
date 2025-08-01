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

#include "vstd/DateUtils.h"

#include "GameConstants.h"
#include "TurnTimerInfo.h"
#include "ExtraOptionsInfo.h"
#include "campaign/CampaignConstants.h"
#include "serializer/GameConnectionID.h"
#include "serializer/Serializeable.h"
#include "serializer/PlayerConnectionID.h"
#include "ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapGenOptions;
class CampaignState;
class CMapInfo;
struct PlayerInfo;
class PlayerColor;

struct DLL_LINKAGE SimturnsInfo
{
	/// Minimal number of turns that must be played simultaneously even if contact has been detected
	int requiredTurns = 0;
	/// Maximum number of turns that might be played simultaneously unless contact is detected
	int optionalTurns = 0;
	/// If set to true, human and 1 AI can act at the same time
	bool allowHumanWithAI = false;
	/// If set to true, allied players can play simultaneously even after contacting each other
	bool ignoreAlliedContacts = true;

	bool operator == (const SimturnsInfo & other) const
	{
		return requiredTurns == other.requiredTurns &&
				optionalTurns == other.optionalTurns &&
				ignoreAlliedContacts == other.ignoreAlliedContacts &&
				allowHumanWithAI == other.allowHumanWithAI;
	}

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & requiredTurns;
		h & optionalTurns;
		h & allowHumanWithAI;
		h & ignoreAlliedContacts;
	}
};

enum class PlayerStartingBonus : int8_t
{
	RANDOM   = -1,
	ARTIFACT =  0,
	GOLD     =  1,
	RESOURCE =  2
};

struct DLL_LINKAGE Handicap {
	TResources startBonus = TResources();
	int percentIncome = 100;
	int percentGrowth = 100;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & startBonus;
		h & percentIncome;
		h & percentGrowth;
	}
};

/// Struct which describes the name, the color, the starting bonus of a player
struct DLL_LINKAGE PlayerSettings
{
	PlayerStartingBonus bonus;
	FactionID castle;
	HeroTypeID hero;
	HeroTypeID heroPortrait; //-1 if default, else ID

	std::string heroNameTextId;
	PlayerColor color; //from 0 -

	Handicap handicap;

	std::string name;
	std::set<PlayerConnectionID> connectedPlayerIDs; //Empty - AI, or connectrd player ids
	bool compOnly; //true if this player is a computer only player; required for RMG
	template <typename Handler>
	void serialize(Handler &h)
	{
		h & castle;
		h & hero;
		h & heroPortrait;
		h & heroNameTextId;
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

	FactionID getCastleValidated() const;
	HeroTypeID getHeroValidated() const;
};

enum class EStartMode : int32_t
{
	NEW_GAME,
	LOAD_GAME,
	CAMPAIGN,
	INVALID = 255
};

/// Struct which describes the difficulty, the turn time,.. of a heroes match.
struct DLL_LINKAGE StartInfo : public Serializeable
{
	EStartMode mode;
	ui8 difficulty; //0=easy; 4=impossible

	using TPlayerInfos = std::map<PlayerColor, PlayerSettings>;
	TPlayerInfos playerInfos; //color indexed

	time_t startTime;
	std::string fileURI;
	SimturnsInfo simturnsInfo;
	TurnTimerInfo turnTimerInfo;
	ExtraOptionsInfo extraOptionsInfo;
	std::string mapname; // empty for random map, otherwise name of the map or savegame
	bool createRandomMap() const { return mapGenOptions != nullptr; }
	std::shared_ptr<CMapGenOptions> mapGenOptions;

	std::shared_ptr<CampaignState> campState;

	PlayerSettings & getIthPlayersSettings(const PlayerColor & no);
	const PlayerSettings & getIthPlayersSettings(const PlayerColor & no) const;
	PlayerSettings * getPlayersSettings(PlayerConnectionID connectedPlayerId);

	// TODO: Must be client-side
	std::string getCampaignName() const;

	/// Controls check for handling of garrisons by AI in Restoration of Erathia campaigns to match H3 behavior
	bool restrictedGarrisonsForAI() const;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & mode;
		h & difficulty;
		h & playerInfos;
		h & startTime;
		h & fileURI;
		h & simturnsInfo;
		h & turnTimerInfo;
		h & extraOptionsInfo;
		h & mapname;
		h & mapGenOptions;
		h & campState;
	}

	StartInfo()
		: mode(EStartMode::INVALID)
		, difficulty(1)
		, startTime(std::time(nullptr))
	{

	}
};

struct ClientPlayer
{
	GameConnectionID connection;
	std::string name;

	template <typename Handler> void serialize(Handler &h)
	{
		h & connection;
		h & name;
	}
};

struct DLL_LINKAGE LobbyState
{
	std::shared_ptr<StartInfo> si;
	std::shared_ptr<CMapInfo> mi;
	std::map<PlayerConnectionID, ClientPlayer> playerNames; // id of player <-> player name;
	GameConnectionID hostClientId = GameConnectionID::INVALID;
	// TODO: Campaign-only and we don't really need either of them.
	// Before start both go into CCampaignState that is part of StartInfo
	CampaignScenarioID campaignMap;
	int campaignBonus;

	LobbyState() : si(new StartInfo()), campaignMap(CampaignScenarioID::NONE), campaignBonus(-1) {}

	template <typename Handler> void serialize(Handler &h)
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
	std::string uuid;

	LobbyInfo() {}

	void verifyStateBeforeStart(bool ignoreNoHuman = false) const;

	bool isClientHost(GameConnectionID clientId) const;
	bool isPlayerHost(const PlayerColor & color) const;
	std::set<PlayerColor> getAllClientPlayers(GameConnectionID clientId) const;
	std::vector<PlayerConnectionID> getConnectedPlayerIdsForClient(GameConnectionID clientId) const;

	// Helpers for lobby state access
	std::set<PlayerColor> clientHumanColors(GameConnectionID clientId);
	PlayerColor clientFirstColor(GameConnectionID clientId) const;
	bool isClientColor(GameConnectionID clientId, const PlayerColor & color) const;
	PlayerConnectionID clientFirstId(GameConnectionID clientId) const; // Used by chat only!
	PlayerInfo & getPlayerInfo(PlayerColor color);
	TeamID getPlayerTeamId(const PlayerColor & color);
};


VCMI_LIB_NAMESPACE_END
