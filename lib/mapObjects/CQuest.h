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

#include "CObjectHandler.h"
#include "CArmedInstance.h"

#include "../CCreatureSet.h"
#include "../NetPacksBase.h"

class CGCreature;

class DLL_LINKAGE CQuest
{
public:
	enum Emission {MISSION_NONE = 0, MISSION_LEVEL = 1, MISSION_PRIMARY_STAT = 2, MISSION_KILL_HERO = 3, MISSION_KILL_CREATURE = 4,
		MISSION_ART = 5, MISSION_ARMY = 6, MISSION_RESOURCES = 7, MISSION_HERO = 8, MISSION_PLAYER = 9, MISSION_KEYMASTER = 10};
	enum Eprogress {NOT_ACTIVE, IN_PROGRESS, COMPLETE};

	si32 qid; //unique quest id for serialization / identification

	Emission missionType;
	Eprogress progress;
	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit

	ui32 m13489val;
	std::vector<ui32> m2stats;
	std::vector<ui16> m5arts; //artifacts id
	std::vector<CStackBasicDescriptor> m6creatures; //pair[cre id, cre count], CreatureSet info irrelevant
	std::vector<ui32> m7resources; //TODO: use resourceset?

	// following fields are used only for kill creature/hero missions, the original
	// objects became inaccessible after their removal, so we need to store info
	// needed for messages / hover text
	ui8 textOption;
	ui8 completedOption;
	CStackBasicDescriptor stackToKill;
	ui8 stackDirection;
	std::string heroName; //backup of hero name
	si32 heroPortrait;

	std::string firstVisitText, nextVisitText, completedText;
	bool isCustomFirst, isCustomNext, isCustomComplete;

	CQuest();
	virtual ~CQuest(){};

	virtual bool checkQuest (const CGHeroInstance * h) const; //determines whether the quest is complete or not
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual void getCompletionText (MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h = nullptr) const;
	virtual void getRolloverText (MetaString &text, bool onHover) const; //hover or quest log entry
	virtual void completeQuest (const CGHeroInstance * h) const {};
	virtual void addReplacements(MetaString &out, const std::string &base) const;

	bool operator== (const CQuest & quest) const
	{
		return (quest.qid == qid);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid;
		h & missionType;
		h & progress;
		h & lastDay;
		h & m13489val;
		h & m2stats;
		h & m5arts;
		h & m6creatures;
		h & m7resources;
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
		if(version >= 757)
		{
			h & completedOption;
		}
		else if(!h.saving)
		{
			completedOption = 1;
		}
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName);
};

class DLL_LINKAGE IQuestObject
{
public:
	CQuest * quest;

	IQuestObject();
	virtual ~IQuestObject();
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual bool checkQuest (const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest;
	}
protected:
	void afterAddToMapCommon(CMap * map);
};

class DLL_LINKAGE CGSeerHut : public CArmedInstance, public IQuestObject //army is used when giving reward
{
public:
	enum ERewardType {NOTHING, EXPERIENCE, MANA_POINTS, MORALE_BONUS, LUCK_BONUS, RESOURCES, PRIMARY_SKILL, SECONDARY_SKILL, ARTIFACT, SPELL, CREATURE};
	ERewardType rewardType;
	si32 rID; //reward ID
	si32 rVal; //reward value
	std::string seerName;

	CGSeerHut();
	void initObj(CRandomGenerator & rand) override;
	std::string getHoverText(PlayerColor player) const override;
	void newTurn(CRandomGenerator & rand) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	virtual void init(CRandomGenerator & rand);
	int checkDirection() const; //calculates the region of map where monster is placed
	void setObjToKill(); //remember creatures / heroes to kill after they are initialized
	const CGHeroInstance *getHeroToKill(bool allowNull = false) const;
	const CGCreature *getCreatureToKill(bool allowNull = false) const;
	void getRolloverText (MetaString &text, bool onHover) const;
	void getCompletionText(MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h = nullptr) const;
	void finishQuest (const CGHeroInstance * h, ui32 accept) const; //common for both objects
	virtual void completeQuest (const CGHeroInstance * h) const;

	void afterAddToMap(CMap * map) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<IQuestObject&>(*this);
		h & rewardType;
		h & rID;
		h & rVal;
		h & seerName;
	}
protected:
	static const int OBJPROP_VISITED = 10;

	void setPropertyDer(ui8 what, ui32 val) override;

	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGQuestGuard : public CGSeerHut
{
public:
	CGQuestGuard() : CGSeerHut(){};
	void init(CRandomGenerator & rand) override;
	void completeQuest (const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGSeerHut&>(*this);
	}
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGKeys : public CGObjectInstance //Base class for Keymaster and guards
{
public:
	static std::map <PlayerColor, std::set <ui8> > playerKeyMap; //[players][keysowned]
	//SubID 0 - lightblue, 1 - green, 2 - red, 3 - darkblue, 4 - brown, 5 - purple, 6 - white, 7 - black

	static void reset();

	bool wasMyColorVisited (PlayerColor player) const;

	std::string getObjectName() const override; //depending on color
	std::string getHoverText(PlayerColor player) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGKeymasterTent : public CGKeys
{
public:
	bool wasVisited (PlayerColor player) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBorderGuard : public CGKeys, public IQuestObject
{
public:
	CGBorderGuard() : IQuestObject(){};
	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const override;
	void getRolloverText (MetaString &text, bool onHover) const;
	bool checkQuest (const CGHeroInstance * h) const override;

	void afterAddToMap(CMap * map) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IQuestObject&>(*this);
		h & static_cast<CGObjectInstance&>(*this);
		h & blockVisit;
	}
};

class DLL_LINKAGE CGBorderGate : public CGBorderGuard
{
public:
	CGBorderGate() : CGBorderGuard(){};
	void onHeroVisit(const CGHeroInstance * h) const override;

	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGBorderGuard&>(*this); //need to serialize or object will be empty
	}
};
