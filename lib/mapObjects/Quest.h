/*
 * Quest.h, part of VCMI engine
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
	/// before returning. No in-memory Quest ever holds this value.
	HOTA_MULTI_PLACEHOLDER = 10,
	// end of H3 missions

	KEYMASTER = 11,
	HOTA_HERO_CLASS = 12,
	HOTA_REACH_DATE = 13,
	HOTA_GAME_DIFFICULTY = 14,
	HOTA_SCRIPTED = 15,
};

class DLL_LINKAGE Quest final : public Serializeable
{
public:

	static const std::string & missionName(EQuestMission index);
	static const std::string & missionState(int index);
	
	EQuestMission missionKind = EQuestMission::NONE;

	si32 lastDay = -1; //after this day (first day is 0) mission cannot be completed; if -1 - no limit
	Rewardable::Limiter mission;
	bool repeatedQuest = false;
	bool isCompleted = false;
	std::set<PlayerColor> activeForPlayers;

	// following fields are used only for kill creature/hero missions, the original
	// objects became inaccessible after their removal, so we need to store info
	// needed for messages / hover text
	ui8 textOption = 0;
	ui8 completedOption = 0;
	CreatureID stackToKill;
	ui8 stackDirection = 0;
	std::string heroName; //backup of hero name
	HeroTypeID heroPortrait;

	MetaString firstVisitText;
	MetaString nextVisitText;
	MetaString completedText;

	/// Reward granted on completing this quest; the active quest's reward is
	/// mirrored into the owning object's configuration.info at runtime.
	std::optional<Rewardable::VisitInfo> reward;

	static bool checkMissionArmy(const Quest * q, const CCreatureSet * army);
	bool checkQuest(const CGHeroInstance * h) const; //determines whether the quest is complete or not
	void getVisitText(const IGameInfoCallback * cb, MetaString &text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	void getCompletionText(const IGameInfoCallback * cb, MetaString &text) const;
	void getRolloverText (const IGameInfoCallback * cb, MetaString &text, bool onHover) const; //hover or quest log entry
	/// Removes the consumable goods the limiter demands (artifacts / creatures /
	/// resources) from the hero — the "cost" of the quest. Does NOT mark the quest
	/// completed; callers handle completion/removal separately.
	void takeRequirements(IGameEventCallback & gameEvents, const CGHeroInstance * h, bool allowFullArmyRemoval) const;
	void addTextReplacements(const IGameInfoCallback * cb, MetaString &out, std::vector<Component> & components) const;
	void addKillTargetReplacements(MetaString &out) const;
	void defineQuestName();

	/// A quest is a "toll" when satisfying it surrenders consumable goods
	/// (resources / artifacts / creatures) — exactly what takeRequirements removes.
	/// Checked against the limiter because map quests store the cost there; if a
	/// separate "taken reward" is ever modelled, the check should move to it.
	bool isToll() const;

	template <typename Handler> void serialize(Handler &h)
	{
		if(!h.hasFeature(Handler::Version::QUEST_REWORK))
		{
			si32 legacyQuestInstanceID = 0; // removed Quest::qid
			h & legacyQuestInstanceID;
		}
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
		if(!h.hasFeature(Handler::Version::QUEST_REWORK))
		{
			// legacy single kill target; QUEST_REWORK stores the full
			// mission.destroyedObjects vector in the limiter instead
			ObjectInstanceID killTarget;
			h & killTarget;
			if(killTarget.hasValue())
				mission.destroyedObjects.push_back(killTarget);
		}
		if(h.hasFeature(Handler::Version::QUEST_REWORK))
			h & reward;
		if(!h.saving)
			defineQuestName();
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);
};

/// Abstract base for rewardable objects that gate a reward behind a Quest.
class DLL_LINKAGE QuestSource : public CRewardableObject
{
	// Seer Huts may carry several quests; only one is active at a time and it is
	// the same for every player. Other quest sources hold exactly one.
	std::vector<std::shared_ptr<Quest>> quests = { std::make_shared<Quest>() };
	si32 currentQuestIndex = 0;
public:
	using CRewardableObject::CRewardableObject;

	const Quest & getQuest() const { return *quests.at(currentQuestIndex); }
	Quest & getQuest() { return *quests.at(currentQuestIndex); }

	/// All quests this source owns (loader / setup use).
	const std::vector<std::shared_ptr<Quest>> & allQuests() const { return quests; }
	/// Appends a fresh quest and returns it (loader use).
	Quest & addQuest();

	virtual bool checkQuest(const CGHeroInstance * h) const;

	// Per-colour keymaster key state, shared by keymaster tents and border guards/gates.
	// TODO: review whether QuestSource is the right home/form for these.
	static bool hasVisitedKeymaster(const CGObjectInstance * keyObject, PlayerColor player);
	static std::string keymasterVisitedText(const CGObjectInstance * keyObject, PlayerColor player);

	/// The quest currently relevant for visiting / quest-log display.
	virtual const Quest & activeQuest() const { return getQuest(); }
	const Quest * activeQuestForLog() const override { return &activeQuest(); }

	void getVisitText(MetaString & text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CRewardableObject&>(*this);
		if(h.hasFeature(Handler::Version::QUEST_REWORK))
		{
			h & quests;
			h & currentQuestIndex;
			h & advancePending;
		}
		else
		{
			// legacy single-quest layout: wrap into a one-element vector
			quests.resize(1);
			h & quests[0];
			currentQuestIndex = 0;
		}
	}
protected:
	// the active quest finished; advance to the next one on the following visit / turn
	// (deferred so configuration.info is never rebuilt while a reward grant is pending)
	bool advancePending = false;

	/// True when no quest can currently be offered (all one-shots done / expired).
	bool isSpent() const;
	/// Offerable now: not expired by deadline or difficulty, not a consumed one-shot.
	bool isQuestAvailable(const Quest & q) const;
	/// Move the active quest to the next offerable one (loops within repeatables).
	void advanceToNextQuest();
	/// Pick the first offerable quest as active.
	void selectInitialQuest();
	/// Mirror the active quest's reward into configuration.info.
	void syncActiveReward();
};

class DLL_LINKAGE SeerHut : public QuestSource
{
public:
	using QuestSource::QuestSource;

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
	void getRolloverText (MetaString &text, bool onHover) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<QuestSource&>(*this);
		h & seerName;
	}
protected:
	bool allowsFullArmyRemoval() const;
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;

	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE QuestGuard : public SeerHut
{
public:
	using SeerHut::SeerHut;

	void init(vstd::RNG & rand) override;
	
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	void blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer) const override;
	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<SeerHut&>(*this);
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

/// Reads the pre-QUEST_REWORK CGBorderGuard/CGBorderGate byte layout
/// (a Quest pointer followed by the CGObjectInstance base) into a quest source,
/// synthesising the requiredKeys limiter from the object's colour subID.
template<typename Handler>
void loadLegacyBorderGuard(Handler & h, QuestSource & object)
{
	std::shared_ptr<Quest> quest;
	h & quest;
	h & static_cast<CGObjectInstance&>(object);
	object.getQuest() = *quest;
	object.getQuest().mission.requiredKeys.push_back(object.subID);
}

/// Key/toll gate: stays in place, passable for a player once its limiter is met
/// (border gates require the matching keymaster key).
class DLL_LINKAGE QuestGate : public QuestSource
{
public:
	using QuestSource::QuestSource;

	void initObj(IGameRandomizer & gameRandomizer) override;
	void onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const override;
	bool passableFor(PlayerColor color) const override;
	bool passableFor(const CGHeroInstance * hero) const override;

	template <typename Handler> void serialize(Handler & h)
	{
		// type id 12 served the legacy CGBorderGate; pre-QUEST_REWORK saves carry its layout
		if(h.hasFeature(Handler::Version::QUEST_REWORK))
			h & static_cast<QuestSource&>(*this);
		else
			loadLegacyBorderGuard(h, *this);
	}
};

class DLL_LINKAGE KeymasterTent : public CGObjectInstance
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
