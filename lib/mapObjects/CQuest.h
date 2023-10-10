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
#include "../MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGCreature;

class DLL_LINKAGE CQuest final
{
public:

	enum EProgress {
		NOT_ACTIVE,
		IN_PROGRESS,
		COMPLETE
	};

	static const std::string & missionName(int index);
	static const std::string & missionState(int index);
	
	std::string questName;

	si32 qid; //unique quest id for serialization / identification

	EProgress progress;
	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit
	ObjectInstanceID killTarget;
	Rewardable::Limiter mission;
	bool repeatedQuest;

	// following fields are used only for kill creature/hero missions, the original
	// objects became inaccessible after their removal, so we need to store info
	// needed for messages / hover text
	ui8 textOption;
	ui8 completedOption;
	CStackBasicDescriptor stackToKill;
	ui8 stackDirection;
	std::string heroName; //backup of hero name
	HeroTypeID heroPortrait;

	MetaString firstVisitText, nextVisitText, completedText;
	bool isCustomFirst;
	bool isCustomNext;
	bool isCustomComplete;

	CQuest(); //TODO: Remove constructor

	static bool checkMissionArmy(const CQuest * q, const CCreatureSet * army);
	virtual bool checkQuest(const CGHeroInstance * h) const; //determines whether the quest is complete or not
	virtual void getVisitText(MetaString &text, std::vector<Component> & components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual void getCompletionText(MetaString &text) const;
	virtual void getRolloverText (MetaString &text, bool onHover) const; //hover or quest log entry
	virtual void completeQuest(IGameCallback *, const CGHeroInstance * h) const;
	virtual void addTextReplacements(MetaString &out, std::vector<Component> & components) const;
	virtual void addKillTargetReplacements(MetaString &out) const;
	void defineQuestName();

	bool operator== (const CQuest & quest) const
	{
		return (quest.qid == qid);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid;
		h & progress;
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
public:
	CQuest * quest = new CQuest();

	///Information about quest should remain accessible even if IQuestObject removed from map
	///All CQuest objects are freed in CMap destructor
	virtual ~IQuestObject() = default;
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual bool checkQuest (const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest;
	}
protected:
	void afterAddToMapCommon(CMap * map) const;
};

class DLL_LINKAGE CGSeerHut : public CRewardableObject, public IQuestObject
{
public:
	std::string seerName;

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

	void afterAddToMap(CMap * map) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CRewardableObject&>(*this);
		h & static_cast<IQuestObject&>(*this);
		h & seerName;
	}
protected:
	static constexpr int OBJPROP_VISITED = 10;

	void setPropertyDer(ui8 what, ui32 val) override;

	void serializeJsonOptions(JsonSerializeFormat & handler) override;
};

class DLL_LINKAGE CGQuestGuard : public CGSeerHut
{
public:
	void init(CRandomGenerator & rand) override;

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

	bool wasMyColorVisited(const PlayerColor & player) const;

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
	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void getVisitText (MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h = nullptr) const override;
	void getRolloverText (MetaString &text, bool onHover) const;
	bool checkQuest (const CGHeroInstance * h) const override;

	void afterAddToMap(CMap * map) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IQuestObject&>(*this);
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBorderGate : public CGBorderGuard
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;

	bool passableFor(PlayerColor color) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGBorderGuard&>(*this); //need to serialize or object will be empty
	}
};

VCMI_LIB_NAMESPACE_END
