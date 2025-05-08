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

#include "../GameConstants.h"
#include "../filesystem/ResourcePath.h"
#include "../serializer/Serializeable.h"
#include "../texts/TextLocalizationContainer.h"
#include "CampaignConstants.h"
#include "CampaignScenarioPrologEpilog.h"
#include "../gameState/HighScore.h"
#include "../Point.h"

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;
class CGHeroInstance;
class CBinaryReader;
class CInputStream;
class CMap;
class CMapHeader;
class CMapInfo;
class JsonNode;
class IGameCallback;

class DLL_LINKAGE CampaignRegions
{
	// Campaign editor
	friend class CampaignEditor;
	friend class CampaignProperties;
	friend class ScenarioProperties;

	std::string campPrefix;
	std::vector<std::string> campSuffix;
	std::string campBackground;
	int colorSuffixLength;

	struct DLL_LINKAGE RegionDescription
	{
		std::string infix;
		Point pos;
		std::optional<Point> labelPos;

		template <typename Handler> void serialize(Handler &h)
		{
			h & infix;
			h & pos;
			h & labelPos;
		}

		static CampaignRegions::RegionDescription fromJson(const JsonNode & node);
		static JsonNode toJson(CampaignRegions::RegionDescription & rd);
	};

	std::vector<RegionDescription> regions;

	ImagePath getNameFor(CampaignScenarioID which, int color, std::string type) const;

public:
	ImagePath getBackgroundName() const;
	Point getPosition(CampaignScenarioID which) const;
	std::optional<Point> getLabelPosition(CampaignScenarioID which) const;
	ImagePath getAvailableName(CampaignScenarioID which, int color) const;
	ImagePath getSelectedName(CampaignScenarioID which, int color) const;
	ImagePath getConqueredName(CampaignScenarioID which, int color) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & campPrefix;
		h & colorSuffixLength;
		h & regions;
		h & campSuffix;
		h & campBackground;
	}

	static CampaignRegions fromJson(const JsonNode & node);
	static JsonNode toJson(CampaignRegions cr);
	static CampaignRegions getLegacy(int campId);
};

class DLL_LINKAGE CampaignHeader : public boost::noncopyable
{
	friend class CampaignHandler;
	friend class Campaign;

	// Campaign editor
	friend class CampaignEditor;
	friend class CampaignProperties;
	friend class ScenarioProperties;

	CampaignVersion version = CampaignVersion::NONE;
	CampaignRegions campaignRegions;
	MetaString name;
	MetaString description;
	MetaString author;
	MetaString authorContact;
	MetaString campaignVersion;
	std::time_t creationDateTime;
	AudioPath music;
	std::string filename;
	std::string modName;
	std::string encoding;
	ImagePath loadingBackground;
	ImagePath videoRim;
	VideoPath introVideo;
	VideoPath outroVideo;

	int numberOfScenarios = 0;
	bool difficultyChosenByPlayer = false;

	void loadLegacyData(ui8 campId);
	void loadLegacyData(CampaignRegions regions, int numOfScenario);

	TextContainerRegistrable textContainer;

public:
	bool playerSelectedDifficulty() const;
	bool formatVCMI() const;

	std::string getDescriptionTranslated() const;
	std::string getNameTranslated() const;
	std::string getAuthor() const;
	std::string getAuthorContact() const;
	std::string getCampaignVersion() const;
	time_t getCreationDateTime() const;
	std::string getFilename() const;
	std::string getModName() const;
	std::string getEncoding() const;
	AudioPath getMusic() const;
	ImagePath getLoadingBackground() const;
	ImagePath getVideoRim() const;
	VideoPath getIntroVideo() const;
	VideoPath getOutroVideo() const;

	const CampaignRegions & getRegions() const;
	TextContainerRegistrable & getTexts();

	template <typename Handler> void serialize(Handler &h)
	{
		h & version;
		h & campaignRegions;
		h & numberOfScenarios;
		h & name;
		h & description;
		h & author;
		h & authorContact;
		h & campaignVersion;
		h & creationDateTime;
		h & difficultyChosenByPlayer;
		h & filename;
		h & modName;
		h & music;
		h & encoding;
		h & textContainer;
		h & loadingBackground;
		h & videoRim;
		h & introVideo;
		h & outroVideo;
	}
};

struct DLL_LINKAGE CampaignBonus
{
	CampaignBonusType type = CampaignBonusType::NONE;

	//purpose depends on type
	int32_t info1 = 0;
	int32_t info2 = 0;
	int32_t info3 = 0;

	bool isBonusForHero() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & type;
		h & info1;
		h & info2;
		h & info3;
	}
};

struct DLL_LINKAGE CampaignTravel
{
	struct DLL_LINKAGE WhatHeroKeeps
	{
		bool experience = false;
		bool primarySkills = false;
		bool secondarySkills = false;
		bool spells = false;
		bool artifacts = false;

		template <typename Handler> void serialize(Handler &h)
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

	template <typename Handler> void serialize(Handler &h)
	{
		h & whatHeroKeeps;
		h & monstersKeptByHero;
		h & artifactsKeptByHero;
		h & startOptions;
		h & playerColor;
		h & bonusesToChoose;
	}
};

struct DLL_LINKAGE CampaignScenario
{
	std::string mapName; //*.h3m
	MetaString scenarioName; //from header
	std::set<CampaignScenarioID> preconditionRegions; //what we need to conquer to conquer this one (stored as bitfield in h3c)
	ui8 regionColor = 0;
	ui8 difficulty = 0;

	MetaString regionText;
	CampaignScenarioPrologEpilog prolog;
	CampaignScenarioPrologEpilog epilog;

	CampaignTravel travelOptions;

	void loadPreconditionRegions(ui32 regions);
	bool isNotVoid() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & mapName;
		h & scenarioName;
		h & preconditionRegions;
		h & regionColor;
		h & difficulty;
		h & regionText;
		h & prolog;
		h & epilog;
		h & travelOptions;
	}
};

/// Class that represents loaded campaign information
class DLL_LINKAGE Campaign : public CampaignHeader, public Serializeable
{
	friend class CampaignHandler;

	// Campaign editor
	friend class CampaignEditor;
	friend class CampaignProperties;
	friend class ScenarioProperties;

	std::map<CampaignScenarioID, CampaignScenario> scenarios;

public:
	const CampaignScenario & scenario(CampaignScenarioID which) const;
	std::set<CampaignScenarioID> allScenarios() const;
	int scenariosCount() const;

	void overrideCampaign();
	void overrideCampaignScenarios();

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CampaignHeader&>(*this);
		h & scenarios;
	}
};

/// Class that represent campaign that is being played at
/// Contains campaign itself as well as current state of the campaign
class DLL_LINKAGE CampaignState : public Campaign
{
	friend class CampaignHandler;

	// Campaign editor
	friend class CampaignEditor;
	friend class CampaignProperties;
	friend class ScenarioProperties;

	using ScenarioPoolType = std::vector<JsonNode>;
	using CampaignPoolType = std::map<CampaignScenarioID, ScenarioPoolType>;
	using GlobalPoolType = std::map<HeroTypeID, JsonNode>;

	/// List of all maps completed by player, in order of their completion
	std::vector<CampaignScenarioID> mapsConquered;

	/// List of previously loaded campaign maps, to prevent translation of transferred hero names getting lost after their original map has been completed
	std::map<CampaignScenarioID, TextContainerRegistrable> mapTranslations;

	std::map<CampaignScenarioID, std::vector<uint8_t> > mapPieces; //binary h3ms, scenario number -> map data
	std::map<CampaignScenarioID, ui8> chosenCampaignBonuses;
	std::optional<CampaignScenarioID> currentMap;

	/// Heroes from specific scenario, ordered by descending strength
	CampaignPoolType scenarioHeroPool;

	/// Pool of heroes currently reserved for usage in campaign
	GlobalPoolType globalHeroPool;

public:
	CampaignState() = default;

	/// Returns last completed scenario, if any
	std::optional<CampaignScenarioID> lastScenario() const;

	std::optional<CampaignScenarioID> currentScenario() const;
	std::set<CampaignScenarioID> conqueredScenarios() const;

	/// Returns bonus selected for specific scenario
	std::optional<CampaignBonus> getBonus(CampaignScenarioID which) const;

	/// Returns index of selected bonus for specified scenario
	std::optional<ui8> getBonusID(CampaignScenarioID which) const;

	/// Returns true if selected scenario can be selected and started by player
	bool isAvailable(CampaignScenarioID whichScenario) const;

	/// Returns true if selected scenario has been already completed by player
	bool isConquered(CampaignScenarioID whichScenario) const;

	/// Returns true if all available scenarios have been completed and campaign is finished
	bool isCampaignFinished() const;

	std::unique_ptr<CMap> getMap(CampaignScenarioID scenarioId, IGameCallback * cb);
	std::unique_ptr<CMapHeader> getMapHeader(CampaignScenarioID scenarioId) const;
	std::shared_ptr<CMapInfo> getMapInfo(CampaignScenarioID scenarioId) const;

	void setCurrentMap(CampaignScenarioID which);
	void setCurrentMapBonus(ui8 which);
	void setCurrentMapAsConquered(std::vector<CGHeroInstance*> heroes);

	/// Returns list of heroes that must be reserved for campaign and can only be used for hero placeholders
	std::set<HeroTypeID> getReservedHeroes() const;

	/// Returns strongest hero from specified scenario, or null if none found
	std::shared_ptr<CGHeroInstance> strongestHero(CampaignScenarioID scenarioId, const PlayerColor & owner) const;

	/// Returns heroes that can be instantiated as hero placeholders by power
	const std::vector<JsonNode> & getHeroesByPower(CampaignScenarioID scenarioId) const;

	/// Returns hero for instantiation as placeholder by type
	/// May return empty JsonNode if such hero was not found
	const JsonNode & getHeroByType(HeroTypeID heroID) const;

	JsonNode crossoverSerialize(CGHeroInstance * hero) const;
	std::shared_ptr<CGHeroInstance> crossoverDeserialize(const JsonNode & node, CMap * map) const;

	std::string campaignSet;

	std::vector<HighScoreParameter> highscoreParameters;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<Campaign&>(*this);
		h & scenarioHeroPool;
		h & globalHeroPool;
		h & mapPieces;
		h & mapsConquered;
		h & currentMap;
		h & chosenCampaignBonuses;
		h & campaignSet;
		h & mapTranslations;
		h & highscoreParameters;
	}
};

VCMI_LIB_NAMESPACE_END
