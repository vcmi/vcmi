/*
 * CampaignState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "CampaignConstants.h"
#include "CampaignScenarioPrologEpilog.h"

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;
class CGHeroInstance;
class CBinaryReader;
class CInputStream;
class CMap;
class CMapHeader;
class CMapInfo;
class JsonNode;

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
	int numberOfScenarios = 0;
	CampaignVersion version = CampaignVersion::NONE;
	CampaignRegions campaignRegions;
	std::string name;
	std::string description;
	bool difficultyChoosenByPlayer = false;

	void loadLegacyData(ui8 campId);

	std::string filename;
	std::string modName;
	std::string encoding;

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
	}
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
	std::unique_ptr<CMapHeader> getMapHeader(CampaignScenarioID scenarioId) const;
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

VCMI_LIB_NAMESPACE_END
