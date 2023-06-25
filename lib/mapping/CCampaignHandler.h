/*
 * CCampaignHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;
class CGHeroInstance;
class CBinaryReader;
class CInputStream;
class CMap;
class CMapHeader;
class CMapInfo;
class JsonNode;

enum class CampaignVersion : uint8_t
{
	NONE = 0,
	RoE = 4,
	AB = 5,
	SoD = 6,
	WoG = 6,
	//		Chr = 7, // Heroes Chronicles, likely identical to SoD, untested

	VCMI = 1,
	VCMI_MIN = 1,
	VCMI_MAX = 1,
};

struct DLL_LINKAGE CampaignRegions
{
	std::string campPrefix;
	int colorSuffixLength;

	struct DLL_LINKAGE RegionDescription
	{
		std::string infix;
		int xpos, ypos;
		
		template <typename Handler> void serialize(Handler &h, const int formatVersion)
		{
			h & infix;
			h & xpos;
			h & ypos;
		}
		
		static CampaignRegions::RegionDescription fromJson(const JsonNode & node);
	};

	std::vector<RegionDescription> regions;
	
	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & campPrefix;
		h & colorSuffixLength;
		h & regions;
	}
	
	static CampaignRegions fromJson(const JsonNode & node);
	static CampaignRegions getLegacy(int campId);
};

class DLL_LINKAGE CampaignHeader
{
public:
	CampaignVersion version = CampaignVersion::NONE;
	CampaignRegions campaignRegions;
	int numberOfScenarios = 0;
	std::string name, description;
	bool difficultyChoosenByPlayer = false;
	bool valid = false;

	std::string filename;
	std::string modName;
	std::string encoding;
	
	void loadLegacyData(ui8 campId);

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & version;
		h & campaignRegions;
		h & numberOfScenarios;
		h & name;
		h & description;
		h & difficultyChoosenByPlayer;
		h & filename;
		h & modName;
		h & encoding;
		h & valid;
	}
};

enum class CampaignStartOptions: int8_t
{
	NONE = 0,
	START_BONUS,
	HERO_CROSSOVER,
	HERO_OPTIONS
};

enum class CampaignBonusType : int8_t
{
	NONE = -1,
	SPELL,
	MONSTER,
	BUILDING,
	ARTIFACT,
	SPELL_SCROLL,
	PRIMARY_SKILL,
	SECONDARY_SKILL,
	RESOURCE,
	HEROES_FROM_PREVIOUS_SCENARIO,
	HERO
};

enum class CampaignScenarioID : int8_t
{
	NONE = -1,
	// no members - fake enum to create integer type that is not implicitly convertible to int
};

struct DLL_LINKAGE CampaignBonus
{
	CampaignBonusType type = CampaignBonusType::NONE; //uses EBonusType

	//purpose depends on type
	int32_t info1 = 0;
	int32_t info2 = 0;
	int32_t info3 = 0;

	bool isBonusForHero() const;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & type;
		h & info1;
		h & info2;
		h & info3;
	}
};

class DLL_LINKAGE CampaignTravel
{
public:
	
	struct DLL_LINKAGE WhatHeroKeeps
	{
		bool experience = false;
		bool primarySkills = false;
		bool secondarySkills = false;
		bool spells = false;
		bool artifacts = false;
		
		template <typename Handler> void serialize(Handler &h, const int formatVersion)
		{
			h & experience;
			h & primarySkills;
			h & secondarySkills;
			h & spells;
			h & artifacts;
		}
	};
	
	std::set<CreatureID> monstersKeptByHero;
	std::set<ArtifactID> artifactsKeptByHero;
	std::vector<CampaignBonus> bonusesToChoose;

	WhatHeroKeeps whatHeroKeeps;
	CampaignStartOptions startOptions = CampaignStartOptions::NONE; //1 - start bonus, 2 - traveling hero, 3 - hero options
	PlayerColor playerColor = PlayerColor::NEUTRAL; //only for startOptions == 1

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & whatHeroKeeps;
		h & monstersKeptByHero;
		h & artifactsKeptByHero;
		h & startOptions;
		h & playerColor;
		h & bonusesToChoose;
	}
};

struct DLL_LINKAGE CampaignScenarioPrologEpilog
{
	bool hasPrologEpilog = false;
	std::string prologVideo; // from CmpMovie.txt
	std::string prologMusic; // from CmpMusic.txt
	std::string prologText;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & hasPrologEpilog;
		h & prologVideo;
		h & prologMusic;
		h & prologText;
	}
};

class DLL_LINKAGE CampaignScenario
{
public:
	std::string mapName; //*.h3m
	std::string scenarioName; //from header. human-readble
	std::set<CampaignScenarioID> preconditionRegions; //what we need to conquer to conquer this one (stored as bitfield in h3c)
	ui8 regionColor = 0;
	ui8 difficulty = 0;
	bool conquered = false;

	std::string regionText;
	CampaignScenarioPrologEpilog prolog;
	CampaignScenarioPrologEpilog epilog;

	CampaignTravel travelOptions;
	std::vector<HeroTypeID> keepHeroes; // contains list of heroes which should be kept for next scenario (doesn't matter if they lost)
	std::vector<JsonNode> crossoverHeroes; // contains all heroes with the same state when the campaign scenario was finished
	std::vector<JsonNode> placedCrossoverHeroes; // contains all placed crossover heroes defined by hero placeholders when the scenario was started

	void loadPreconditionRegions(ui32 regions);
	bool isNotVoid() const;
	// FIXME: due to usage of JsonNode I can't make these methods const
	const CGHeroInstance * strongestHero(const PlayerColor & owner);
	std::vector<CGHeroInstance *> getLostCrossoverHeroes(); /// returns a list of crossover heroes which started the scenario, but didn't complete it

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & mapName;
		h & scenarioName;
		h & preconditionRegions;
		h & regionColor;
		h & difficulty;
		h & conquered;
		h & regionText;
		h & prolog;
		h & epilog;
		h & travelOptions;
		h & crossoverHeroes;
		h & placedCrossoverHeroes;
		h & keepHeroes;
	}
};

class DLL_LINKAGE CampaignState
{
public:
	CampaignHeader header;
	std::map<CampaignScenarioID, CampaignScenario> scenarios;
	std::map<CampaignScenarioID, std::string > mapPieces; //binary h3ms, scenario number -> map data

	std::string fileEncoding;
	std::vector<CampaignScenarioID> mapsConquered;
	std::vector<CampaignScenarioID> mapsRemaining;
	std::optional<CampaignScenarioID> currentMap;

	std::map<CampaignScenarioID, ui8> chosenCampaignBonuses;

public:
	std::optional<CampaignBonus> getBonusForCurrentMap() const;
	const CampaignScenario & getCurrentScenario() const;
	ui8 currentBonusID() const;
	bool conquerable(CampaignScenarioID whichScenario) const;

	std::unique_ptr<CMap> getMap(CampaignScenarioID scenarioId) const;
	std::unique_ptr<CMapHeader> getHeader(CampaignScenarioID scenarioId) const;
	std::shared_ptr<CMapInfo> getMapInfo(CampaignScenarioID scenarioId) const;

	CampaignScenario & getCurrentScenario();
	void setCurrentMapAsConquered(const std::vector<CGHeroInstance*> & heroes);
	static JsonNode crossoverSerialize(CGHeroInstance * hero);
	static CGHeroInstance * crossoverDeserialize(JsonNode & node);

	CampaignState() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & header;
		h & scenarios;
		h & mapPieces;
		h & mapsRemaining;
		h & mapsConquered;
		h & currentMap;
		h & chosenCampaignBonuses;
	}
};

class DLL_LINKAGE CampaignHandler
{
	static std::string readLocalizedString(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier);
	
	//parsers for VCMI campaigns (*.vcmp)
	static CampaignHeader readHeaderFromJson(JsonNode & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromJson(JsonNode & reader);
	static CampaignTravel readScenarioTravelFromJson(JsonNode & reader);

	//parsers for original H3C campaigns
	static CampaignHeader readHeaderFromMemory(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromMemory(CBinaryReader & reader, const CampaignHeader & header);
	static CampaignTravel readScenarioTravelFromMemory(CBinaryReader & reader, CampaignVersion version);
	/// returns h3c split in parts. 0 = h3c header, 1-end - maps (binary h3m)
	/// headerOnly - only header will be decompressed, returned vector wont have any maps
	static std::vector<std::vector<ui8>> getFile(std::unique_ptr<CInputStream> file, bool headerOnly);

	static std::string prologVideoName(ui8 index);
	static std::string prologMusicName(ui8 index);
	static std::string prologVoiceName(ui8 index);

public:
	static CampaignHeader getHeader( const std::string & name); //name - name of appropriate file

	static std::shared_ptr<CampaignState> getCampaign(const std::string & name); //name - name of appropriate file
};

VCMI_LIB_NAMESPACE_END
