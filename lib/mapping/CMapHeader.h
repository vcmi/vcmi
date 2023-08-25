/*
 * CMapHeader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../modding/CModVersion.h"
#include "../LogicalExpression.h"
#include "../int3.h"
#include "../MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
enum class EMapFormat : uint8_t;
using ModCompatibilityInfo = std::map<std::string, CModVersion>;

/// The hero name struct consists of the hero id and the hero name.
struct DLL_LINKAGE SHeroName
{
	SHeroName();

	int heroId;
	std::string heroName;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & heroId;
		h & heroName;
	}
};

/// The player info constains data about which factions are allowed, AI tactical settings,
/// the main hero name, where to generate the hero, whether the faction should be selected randomly,...
struct DLL_LINKAGE PlayerInfo
{
	PlayerInfo();

	/// Gets the default faction id or -1 for a random faction.
	si8 defaultCastle() const;
	/// Gets the default hero id or -1 for a random hero.
	si8 defaultHero() const;
	bool canAnyonePlay() const;
	bool hasCustomMainHero() const;

	bool canHumanPlay;
	bool canComputerPlay;
	EAiTactic aiTactic; /// The default value is EAiTactic::RANDOM.

	std::set<FactionID> allowedFactions;
	bool isFactionRandom;

	///main hero instance (VCMI maps only)
	std::string mainHeroInstance;
	/// Player has a random main hero
	bool hasRandomHero;
	/// The default value is -1.
	si32 mainCustomHeroPortrait;
	std::string mainCustomHeroName;
	/// ID of custom hero (only if portrait and hero name are set, otherwise unpredicted value), -1 if none (not always -1)
	si32 mainCustomHeroId;

	std::vector<SHeroName> heroesNames; /// list of placed heroes on the map
	bool hasMainTown; /// The default value is false.
	bool generateHeroAtMainTown; /// The default value is false.
	int3 posOfMainTown;
	TeamID team; /// The default value NO_TEAM

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & hasRandomHero;
		h & mainCustomHeroId;
		h & canHumanPlay;
		h & canComputerPlay;
		h & aiTactic;
		h & allowedFactions;
		h & isFactionRandom;
		h & mainCustomHeroPortrait;
		h & mainCustomHeroName;
		h & heroesNames;
		h & hasMainTown;
		h & generateHeroAtMainTown;
		h & posOfMainTown;
		h & team;
		h & mainHeroInstance;
	}
};

/// The loss condition describes the condition to lose the game. (e.g. lose all own heroes/castles)
struct DLL_LINKAGE EventCondition
{
	enum EWinLoseType {
		//internal use, deprecated
		HAVE_ARTIFACT,     // type - required artifact
		HAVE_CREATURES,    // type - creatures to collect, value - amount to collect
		HAVE_RESOURCES,    // type - resource ID, value - amount to collect
		HAVE_BUILDING,     // position - town, optional, type - building to build
		CONTROL,           // position - position of object, optional, type - type of object
		DESTROY,           // position - position of object, optional, type - type of object
		TRANSPORT,         // position - where artifact should be transported, type - type of artifact

		//map format version pre 1.0
		DAYS_PASSED,       // value - number of days from start of the game
		IS_HUMAN,          // value - 0 = player is AI, 1 = player is human
		DAYS_WITHOUT_TOWN, // value - how long player can live without town, 0=instakill
		STANDARD_WIN,      // normal defeat all enemies condition
		CONST_VALUE,        // condition that always evaluates to "value" (0 = false, 1 = true)

		//map format version 1.0+
		HAVE_0,
		HAVE_BUILDING_0,
		DESTROY_0
	};

	EventCondition(EWinLoseType condition = STANDARD_WIN);
	EventCondition(EWinLoseType condition, si32 value, si32 objectType, const int3 & position = int3(-1, -1, -1));

	const CGObjectInstance * object; // object that was at specified position or with instance name on start
	EMetaclass metaType;
	si32 value;
	si32 objectType;
	si32 objectSubtype;
	std::string objectInstanceName;
	int3 position;
	EWinLoseType condition;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & object;
		h & value;
		h & objectType;
		h & position;
		h & condition;
		h & objectSubtype;
		h & objectInstanceName;
		h & metaType;
	}
};

using EventExpression = LogicalExpression<EventCondition>;

struct DLL_LINKAGE EventEffect
{
	enum EType
	{
		VICTORY,
		DEFEAT
	};

	/// effect type, using EType enum
	si8 type;

	/// message that will be sent to other players
	MetaString toOtherMessage;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & type;
		h & toOtherMessage;
	}
};

struct DLL_LINKAGE TriggeredEvent
{
	/// base condition that must be evaluated
	EventExpression trigger;

	/// string identifier read from config file (e.g. captureKreelah)
	std::string identifier;

	/// string-description, for use in UI (capture town to win)
	MetaString description;

	/// Message that will be displayed when this event is triggered (You captured town. You won!)
	MetaString onFulfill;

	/// Effect of this event. TODO: refactor into something more flexible
	EventEffect effect;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & identifier;
		h & trigger;
		h & description;
		h & onFulfill;
		h & effect;
	}
};

/// The map header holds information about loss/victory condition,map format, version, players, height, width,...
class DLL_LINKAGE CMapHeader
{
	void setupEvents();
public:

	static const int MAP_SIZE_SMALL = 36;
	static const int MAP_SIZE_MIDDLE = 72;
	static const int MAP_SIZE_LARGE = 108;
	static const int MAP_SIZE_XLARGE = 144;
	static const int MAP_SIZE_HUGE = 180;
	static const int MAP_SIZE_XHUGE = 216;
	static const int MAP_SIZE_GIANT = 252;

	CMapHeader();
	virtual ~CMapHeader() = default;

	ui8 levels() const;

	EMapFormat version; /// The default value is EMapFormat::SOD.
	ModCompatibilityInfo mods; /// set of mods required to play a map

	si32 height; /// The default value is 72.
	si32 width; /// The default value is 72.
	bool twoLevel; /// The default value is true.
	std::string name;
	std::string description;
	ui8 difficulty; /// The default value is 1 representing a normal map difficulty.
	/// Specifies the maximum level to reach for a hero. A value of 0 states that there is no
	///	maximum level for heroes. This is the default value.
	ui8 levelLimit;

	MetaString victoryMessage;
	MetaString defeatMessage;
	ui16 victoryIconIndex;
	ui16 defeatIconIndex;

	std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
	ui8 howManyTeams;
	std::vector<bool> allowedHeroes;
	std::vector<HeroTypeID> reservedCampaignHeroes; /// Heroes that have placeholders in this map and are reserverd for campaign

	bool areAnyPlayers; /// Unused. True if there are any playable players on the map.

	/// "main quests" of the map that describe victory and loss conditions
	std::vector<TriggeredEvent> triggeredEvents;

	template <typename Handler>
	void serialize(Handler & h, const int Version)
	{
		h & version;
		if(Version >= 821)
			h & mods;
		h & name;
		h & description;
		h & width;
		h & height;
		h & twoLevel;
		h & difficulty;
		h & levelLimit;
		h & areAnyPlayers;
		h & players;
		h & howManyTeams;
		h & allowedHeroes;
		h & reservedCampaignHeroes;
		//Do not serialize triggeredEvents in header as they can contain information about heroes and armies
		h & victoryMessage;
		h & victoryIconIndex;
		h & defeatMessage;
		h & defeatIconIndex;
	}
};

VCMI_LIB_NAMESPACE_END
