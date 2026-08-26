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

#include "CampaignBonus.h"
#include "CampaignRegions.h"
#include "CampaignScenarioPrologEpilog.h"

#include "../filesystem/ResourcePath.h"
#include "../gameState/HighScore.h"
#include "../scripting/ScriptVariablesStorage.h"
#include "../serializer/Serializeable.h"
#include "../texts/TextLocalizationContainer.h"

#ifdef ENABLE_EDITOR
class CampaignEditor;
class CampaignProperties;
class ScenarioProperties;
#endif

struct StartInfo;
class CGHeroInstance;
class CBinaryReader;
class CInputStream;
class CMap;
class ITranslator;
class CMapHeader;
class CMapInfo;
class JsonNode;
class IGameInfoCallback;

class DLL_LINKAGE CampaignHeader : public boost::noncopyable
{
	friend class CampaignHandler;
	friend class Campaign;

#ifdef ENABLE_EDITOR
	friend class ::CampaignEditor;
	friend class ::CampaignProperties;
	friend class ::ScenarioProperties;
#endif

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

	HeroTypeID yogWizardID;
	HeroTypeID gemSorceressID;
	HeroTypeID mutareDrakeID;

	int hotaVersion = 0; // not serialized - loading only
	int numberOfScenarios = 0;
	bool difficultyChosenByPlayer = false;
	bool restrictGarrisonsAI = false;

	/// Shared with the rendering side, which installs it as an overlay - see CMapHeader::texts
	std::shared_ptr<TextLocalizationContainer> textContainer = std::make_shared<TextLocalizationContainer>();
public:
	bool playerSelectedDifficulty() const;
	CampaignVersion getFormat() const;

	std::string getDescriptionTranslated(const ITranslator * translator) const;
	std::string getNameTranslated(const ITranslator * translator) const;
	std::string getAuthor(const ITranslator * translator) const;
	std::string getAuthorContact(const ITranslator * translator) const;
	std::string getCampaignVersion(const ITranslator * translator) const;
	time_t getCreationDateTime() const;
	std::string getFilename() const;
	std::string getModName() const;
	std::string getEncoding() const;
	AudioPath getMusic() const;
	ImagePath getLoadingBackground() const;
	ImagePath getVideoRim() const;
	VideoPath getIntroVideo() const;
	VideoPath getOutroVideo() const;

	HeroTypeID getYogWizardID() const;
	HeroTypeID getGemSorceressID() const;
	HeroTypeID getMutareDrakeID() const;
	bool restrictedGarrisonsForAI() const;

	const CampaignRegions & getRegions() const;
	const std::shared_ptr<TextLocalizationContainer> & getTexts();

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
		h & restrictGarrisonsAI;
		h & filename;
		h & modName;
		h & music;
		h & encoding;
		h & *textContainer;
		h & loadingBackground;
		h & videoRim;
		h & introVideo;
		h & outroVideo;
		h & yogWizardID;
		h & gemSorceressID;

		if(h.hasFeature(Handler::Version::MUTARE_DRAKE_OVERRIDE))
			h & mutareDrakeID;
		else if(!h.saving)
			mutareDrakeID = HeroTypeID();
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

#ifdef ENABLE_EDITOR
	friend class ::CampaignEditor;
	friend class ::CampaignProperties;
	friend class ::ScenarioProperties;
#endif

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

#ifdef ENABLE_EDITOR
	friend class ::CampaignEditor;
	friend class ::CampaignProperties;
	friend class ::ScenarioProperties;
#endif

	using ScenarioPoolType = std::vector<JsonNode>;
	using CampaignPoolType = std::map<CampaignScenarioID, ScenarioPoolType>;
	using GlobalPoolType = std::map<HeroTypeID, JsonNode>;

	/// List of all maps completed by player, in order of their completion
	std::vector<CampaignScenarioID> mapsConquered;

	std::map<CampaignScenarioID, std::shared_ptr<TextLocalizationContainer>> mapTranslations;

	std::map<CampaignScenarioID, std::vector<uint8_t> > mapPieces; //binary h3ms, scenario number -> map data
	std::map<CampaignScenarioID, ui8> chosenCampaignBonuses;
	std::optional<CampaignScenarioID> currentMap;

	/// Heroes from specific scenario, ordered by descending strength
	CampaignPoolType scenarioHeroPool;

	/// Pool of heroes currently reserved for usage in campaign
	GlobalPoolType globalHeroPool;

	/// Script variables carried over between scenarios (only those declared as persistent)
	ScriptVariablesStorage persistentScriptVariables;
	std::time_t startTime = 0;
	std::string saveDirectory;

public:
	CampaignState() = default;

	/// Copies persist-flagged script variables of the given map into the campaign state.
	void savePersistentVariables(const CMap & map);
	/// Seeds import-flagged script variables of the given map from the campaign state.
	void seedPersistentVariables(CMap & map) const;

	/// Returns last completed scenario, if any
	std::optional<CampaignScenarioID> lastScenario() const;

	std::optional<CampaignScenarioID> currentScenario() const;
	/// Texts of every scenario loaded so far. They stay resolvable for the whole campaign
	/// so that heroes transferred out of a finished scenario keep their names
	const std::map<CampaignScenarioID, std::shared_ptr<TextLocalizationContainer>> & getScenarioTexts() const { return mapTranslations; }

	std::set<CampaignScenarioID> conqueredScenarios() const;
	std::time_t getStartTime() const;
	void setStartTime(std::time_t value);
	const std::string & getSaveDirectory() const;
	void setSaveDirectory(const std::string & value);

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

	std::unique_ptr<CMap> getMap(CampaignScenarioID scenarioId, IGameInfoCallback * cb);
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

	/// Scenario texts are stored by value - the pointers exist only so that the rendering side
	/// can share ownership of them, which is not something a save needs to know about
	template <typename Handler> void serializeTranslations(Handler & h)
	{
		std::map<CampaignScenarioID, TextLocalizationContainer> translations;

		for(const auto & entry : mapTranslations)
			translations[entry.first] = *entry.second;

		h & translations;

		if(!h.saving)
			for(auto & entry : translations)
				mapTranslations[entry.first] = std::make_shared<TextLocalizationContainer>(std::move(entry.second));
	}

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
		serializeTranslations(h);
		h & highscoreParameters;

		if(h.hasFeature(Handler::Version::SCRIPT_VARIABLES))
			h & persistentScriptVariables;

		if(h.hasFeature(Handler::Version::GAME_SESSION_DIRECTORY))
		{
			h & startTime;
			h & saveDirectory;
		}
		else if(!h.saving)
		{
			startTime = 0;
			saveDirectory.clear();
		}
	}
};
