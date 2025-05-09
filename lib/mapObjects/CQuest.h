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

VCMI_LIB_NAMESPACE_BEGIN

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
	HOTA_MULTI = 10,
	// end of H3 missions

	KEYMASTER = 11,
	HOTA_HERO_CLASS = 12,
	HOTA_REACH_DATE = 13,
	HOTA_GAME_DIFFICULTY = 13,
};

class DLL_LINKAGE CQuest final : public Serializeable
{
public:

	static const std::string & missionName(EQuestMission index);
	static const std::string & missionState(int index);
	
	std::string questName;

	QuestInstanceID qid;

	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit
	ObjectInstanceID killTarget;
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
	bool isCustomFirst;
	bool isCustomNext;
	bool isCustomComplete;

	CQuest(); //TODO: Remove constructor

	static bool checkMissionArmy(const CQuest * q, const CCreatureSet * army);
	bool checkQuest(const CGHeroInstance * h) const; //determines whether the quest is complete or not
	void getVisitText(const CGameInfoCallback * cb, MetaString &text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	void getCompletionText(const CGameInfoCallback * cb, MetaString &text) const;
	void getRolloverText (const CGameInfoCallback * cb, MetaString &text, bool onHover) const; //hover or quest log entry
	void completeQuest(IGameCallback *, const CGHeroInstance * h, bool allowFullArmyRemoval) const;
	void addTextReplacements(const CGameInfoCallback * cb, MetaString &out, std::vector<Component> & components) const;
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
		h & isCustomFirst;
		h & isCustomNext;
		h & isCustomComplete;
		h & completedOption;
		h & questName;
		h & mission;
		h & killTarget;
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);
};

class DLL_LINKAGE IQuestObject
{
	std::shared_ptr<CQuest> quest; // TODO: not actually shared, replace with unique_ptr once 1.6 save compat is not needed
public:
	IQuestObject();
	virtual ~IQuestObject();
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h = nullptr) const = 0;
	virtual bool checkQuest (const CGHeroInstance * h) const;
	virtual const CQuest & getQuest() const { return *quest; }
	virtual CQuest & getQuest() { return *quest; }

	template <typename Handler> void serialize(Handler &h)
	{
		h & quest;
	}
};

class DLL_LINKAGE CGSeerHut : public CRewardableObject, public IQuestObject
{
public:
	using CRewardableObject::CRewardableObject;

	std::string seerName;

	void initObj(vstd::RNG & rand) override;
	std::string getHoverText(PlayerColor player) const override;
	std::string getHoverText(const CGHeroInstance * hero) const override;
	std::string getPopupText(PlayerColor player) const override;
	std::string getPopupText(const CGHeroInstance * hero) const override;
	std::vector<Component> getPopupComponents(PlayerColor player) const override;
	std::vector<Component> getPopupComponents(const CGHeroInstance * hero) const override;
	void newTurn(vstd::RNG & rand) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;
	void getVisitText (MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h = nullptr) const override;

	virtual void init(vstd::RNG & rand);
	int checkDirection() const; //calculates the region of map where monster is placed
	void setObjToKill(); //remember creatures / heroes to kill after they are initialized
	const CGHeroInstance *getHeroToKill(bool allowNull) const;
	const CGCreature *getCreatureToKill(bool allowNull) const;
	void getRolloverText (MetaString &text, bool onHover) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CRewardableObject&>(*this);
		h & static_cast<IQuestObject&>(*this);
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
	
	void onHeroVisit(const CGHeroInstance * h) const override;
	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGSeerHut&>(*this);
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGKeys : public CGObjectInstance //Base class for Keymaster and guards
{
public:
	using CGObjectInstance::CGObjectInstance;

	bool wasMyColorVisited(const PlayerColor & player) const;

	std::string getObjectName() const override;
	std::string getObjectDescription(PlayerColor player) const;
	std::string getHoverText(PlayerColor player) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGKeymasterTent : public CGKeys
{
public:
	using CGKeys::CGKeys;

	bool wasVisited (PlayerColor player) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBorderGuard : public CGKeys, public IQuestObject
{
	QuestInstanceID qid;
public:
	using CGKeys::CGKeys;

	void initObj(vstd::RNG & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;

	void getVisitText (MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h = nullptr) const override;
	void getRolloverText (MetaString &text, bool onHover) const;
	bool checkQuest (const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<IQuestObject&>(*this);
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBorderGate : public CGBorderGuard
{
public:
	using CGBorderGuard::CGBorderGuard;

	void onHeroVisit(const CGHeroInstance * h) const override;

	bool passableFor(PlayerColor color) const override;
};

VCMI_LIB_NAMESPACE_END
