/*
 * CQuest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CRewardableObject.h"
#include "../ResourceSet.h"
#include "../serializer/Serializeable.h"
#include "../texts/MetaString.h"

class CGCreature;

enum class EQuestMission {
	NONE = 0,
	LEVEL = 1,
	PRIMARY_SKILL = 2,
	KILL_HERO = 3,
	KILL_CREATURE = 4,
	ARTIFACT = 5,
	ARMY = 6,
	RESOURCES = 7,
	HERO = 8,
	PLAYER = 9,
	/// Parse-time only: appears solely in CMapLoaderH3M::readQuest, which rewrites it
	/// to HOTA_HERO_CLASS / HOTA_REACH_DATE / HOTA_GAME_DIFFICULTY / HOTA_SCRIPTED
	/// before returning. No in-memory CQuest ever holds this value.
	HOTA_MULTI_PLACEHOLDER = 10,
	// end of H3 missions

	KEYMASTER = 11,
	HOTA_HERO_CLASS = 12,
	HOTA_REACH_DATE = 13,
	HOTA_GAME_DIFFICULTY = 14,
	HOTA_SCRIPTED = 15,
};

class DLL_LINKAGE CQuest final : public Serializeable
{
public:

	static const std::string & missionName(EQuestMission index);
	static const std::string & missionState(int index);
	
	EQuestMission missionKind;

	QuestInstanceID qid;

	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit
	Rewardable::Limiter mission;
	bool repeatedQuest;
	bool isCompleted;
	std::set<PlayerColor> activeForPlayers;

	// following fields are used only for kill creature/hero missions, the original
	// objects became inaccessible after their removal, so we need to store info
	// needed for messages / hover text
	ui8 textOption;
	ui8 completedOption;
	CreatureID stackToKill;
	ui8 stackDirection;
	std::string heroName; //backup of hero name
	HeroTypeID heroPortrait;

	MetaString firstVisitText;
	MetaString nextVisitText;
	MetaString completedText;

	CQuest(); //TODO: Remove constructor

	static bool checkMissionArmy(const CQuest * q, const CCreatureSet * army);
	bool checkQuest(const CGHeroInstance * h) const; //determines whether the quest is complete or not
	void getVisitText(const IGameInfoCallback * cb, MetaString &text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	void getCompletionText(const IGameInfoCallback * cb, MetaString &text) const;
	void getRolloverText (const IGameInfoCallback * cb, MetaString &text, bool onHover) const; //hover or quest log entry
	void completeQuest(IGameEventCallback & gameEvents, const CGHeroInstance * h, bool allowFullArmyRemoval) const;
	void addTextReplacements(const IGameInfoCallback * cb, MetaString &out, std::vector<Component> & components) const;
	void addKillTargetReplacements(MetaString &out) const;
	void defineQuestName();

	bool operator== (const CQuest & quest) const
	{
		return (quest.qid == qid);
	}

	template <typename Handler> void serialize(Handler &h)
	{
		h & qid;
		h & isCompleted;
		h & activeForPlayers;
		h & lastDay;
		h & textOption;
		h & stackToKill;
		h & stackDirection;
		h & heroName;
		h & heroPortrait;
		h & firstVisitText;
		h & nextVisitText;
		h & completedText;
		// legacy "text was customized" flags; now derived on the fly from text
		// emptiness in initObj. Kept on the wire for save compatibility.
		bool isCustomFirst = !firstVisitText.empty();
		bool isCustomNext = !nextVisitText.empty();
		bool isCustomComplete = !completedText.empty();
		h & isCustomFirst;
		h & isCustomNext;
		h & isCustomComplete;
		h & completedOption;
		// legacy serialized field; in memory the kind is the derived missionKind enum,
		// recomputed from the limiter below. Kept on the wire for save compatibility.
		std::string questName;
		if(h.saving)
			questName = missionName(missionKind);
		h & questName;
		h & mission;
		// legacy single kill target; now stored in mission.destroyedObjects. Kept at this
		// wire position for save compatibility (formalised in the QUEST_REWORK pass).
		ObjectInstanceID killTarget;
		if(h.saving)
			killTarget = mission.destroyedObjects.empty() ? ObjectInstanceID::NONE : mission.destroyedObjects.front();
		h & killTarget;
		if(!h.saving && killTarget.hasValue())
			mission.destroyedObjects.push_back(killTarget);
		if(!h.saving)
			defineQuestName();
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);
};

/// Abstract base for rewardable objects that gate a reward behind a CQuest.
class DLL_LINKAGE CGQuestSource : public CRewardableObject
{
	std::shared_ptr<CQuest> quest = std::make_shared<CQuest>(); // TODO: not actually shared, replace with unique_ptr once 1.6 save compat is not needed
public:
	using CRewardableObject::CRewardableObject;

	const CQuest & getQuest() const { return *quest; }
	CQuest & getQuest() { return *quest; }
	virtual bool checkQuest(const CGHeroInstance * h) const;

	// Per-colour keymaster key state, shared by keymaster tents and border guards/gates.
	// TODO: review whether CGQuestSource is the right home/form for these.
	static bool hasVisitedKeymaster(const CGObjectInstance * keyObject, PlayerColor player);
	static std::string keymasterVisitedText(const CGObjectInstance * keyObject, PlayerColor player);

	/// The quest currently relevant for visiting / quest-log display.
	virtual const CQuest & activeQuest() const { return getQuest(); }
	const CQuest * activeQuestForLog() const override { return &activeQuest(); }

	void getVisitText(MetaString & text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CRewardableObject&>(*this);
		h & quest;
	}
};

class DLL_LINKAGE CGSeerHut : public CGQuestSource
{
public:
	using CGQuestSource::CGQuestSource;

	std::string seerName;

	void initObj(IGameRandomizer & gameRandomizer) override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;
	std::vector<Component> getPopupComponents(const CGHeroInstance * hero) const override;
	void newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const override;

	virtual void init(vstd::RNG & rand);
	void setObjToKill(); //remember creatures / heroes to kill after they are initialized
	const CGHeroInstance *getHeroToKill(bool allowNull) const;
	const CGCreature *getCreatureToKill(bool allowNull) const;
	void getRolloverText (MetaString &text, bool onHover) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGQuestSource&>(*this);
		h & seerName;
	}
protected:
	bool allowsFullArmyRemoval() const;
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;

	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGQuestGuard : public CGSeerHut
{
public:
	using CGSeerHut::CGSeerHut;

	void init(vstd::RNG & rand) override;
	
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer) const override;
	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGSeerHut&>(*this);
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

/// Key/toll gate: stays in place, passable for a player once its limiter is met
/// (border gates require the matching keymaster key).
class DLL_LINKAGE CGQuestGate : public CGQuestSource
{
public:
	using CGQuestSource::CGQuestSource;

	void initObj(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CGQuestSource&>(*this);
	}
};

class DLL_LINKAGE CGKeymasterTent : public CGObjectInstance
{
public:
	using CGObjectInstance::CGObjectInstance;

	bool wasMyColorVisited(const PlayerColor & player) const;
	bool wasVisited(PlayerColor player) const override;

	std::string getObjectName() const override;
	std::string getObjectDescription(PlayerColor player) const;
	std::string getHoverText(PlayerColor player) const override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};
