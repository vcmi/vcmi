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
#include "serializer/Serializeable.h"
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

		static_assert(Handler::Version::RELEASE_143 < Handler::Version::CURRENT, "Please add ignoreAlliedContacts to serialization for 1.6");
		// disabled to allow multiplayer compatibility between 1.5.2 and 1.5.1
		// h & ignoreAlliedContacts
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
	enum { PLAYER_AI = 0 }; // for use in playerID

	PlayerStartingBonus bonus;
	FactionID castle;
	HeroTypeID hero;
	HeroTypeID heroPortrait; //-1 if default, else ID

	std::string heroNameTextId;
	PlayerColor color; //from 0 -

	Handicap handicap;

	std::string name;
	std::set<ui8> connectedPlayerIDs; //Empty - AI, or connectrd player ids
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
		if (h.version >= Handler::Version::PLAYER_HANDICAP)
			h & handicap;
		else
		{
			enum EHandicap {NO_HANDICAP, MILD, SEVERE};
			EHandicap handicapLegacy = NO_HANDICAP;
			h & handicapLegacy;
		}
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

	std::string startTimeIso8601;
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
	PlayerSettings * getPlayersSettings(const ui8 connectedPlayerId);

	// TODO: Must be client-side
	std::string getCampaignName() const;

	/// Controls hardcoded check for "Steadwick's Fall" scenario from "Dungeon and Devils" campaign
	bool isSteadwickFallCampaignMission() const;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & mode;
		h & difficulty;
		h & playerInfos;
		if (h.version < Handler::Version::REMOVE_LIB_RNG)
		{
			uint32_t oldSeeds = 0;
			h & oldSeeds;
			h & oldSeeds;
			h & oldSeeds;
		}
		h & startTimeIso8601;
		h & fileURI;
		h & simturnsInfo;
		h & turnTimerInfo;
		if(h.version >= Handler::Version::HAS_EXTRA_OPTIONS)
			h & extraOptionsInfo;
		else
			extraOptionsInfo = ExtraOptionsInfo();
		h & mapname;
		h & mapGenOptions;
		h & campState;
	}

	StartInfo()
		: mode(EStartMode::INVALID)
		, difficulty(1)
		, startTimeIso8601(vstd::getDateTimeISO8601Basic(std::time(nullptr)))
	{

	}
};

struct ClientPlayer
{
	int connection;
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
	std::map<ui8, ClientPlayer> playerNames; // id of player <-> player name; 0 is reserved as ID of AI "players"
	int hostClientId;
	// TODO: Campaign-only and we don't really need either of them.
	// Before start both go into CCampaignState that is part of StartInfo
	CampaignScenarioID campaignMap;
	int campaignBonus;

	LobbyState() : si(new StartInfo()), hostClientId(-1), campaignMap(CampaignScenarioID::NONE), campaignBonus(-1) {}

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

	bool isClientHost(int clientId) const;
	bool isPlayerHost(const PlayerColor & color) const;
	std::set<PlayerColor> getAllClientPlayers(int clientId) const;
	std::vector<ui8> getConnectedPlayerIdsForClient(int clientId) const;

	// Helpers for lobby state access
	std::set<PlayerColor> clientHumanColors(int clientId);
	PlayerColor clientFirstColor(int clientId) const;
	bool isClientColor(int clientId, const PlayerColor & color) const;
	ui8 clientFirstId(int clientId) const; // Used by chat only!
	PlayerInfo & getPlayerInfo(PlayerColor color);
	TeamID getPlayerTeamId(const PlayerColor & color);
};


VCMI_LIB_NAMESPACE_END
