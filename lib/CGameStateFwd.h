/*
 * CGameStateFwd.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CCreatureSet.h"
#include "int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class CQuest;
class CGObjectInstance;
class CHeroClass;
class CTown;

//numbers of creatures are exact numbers if detailed else they are quantity ids (1 - a few, 2 - several and so on; additionally 0 - unknown)
struct ArmyDescriptor : public std::map<SlotID, CStackBasicDescriptor>
{
	bool isDetailed;
	DLL_LINKAGE ArmyDescriptor(const CArmedInstance *army, bool detailed); //not detailed -> quantity ids as count
	DLL_LINKAGE ArmyDescriptor();

	DLL_LINKAGE int getStrength() const;
};

struct DLL_LINKAGE InfoAboutArmy
{
	PlayerColor owner;
	std::string name;

	ArmyDescriptor army;

	InfoAboutArmy();
	InfoAboutArmy(const CArmedInstance *Army, bool detailed);

	void initFromArmy(const CArmedInstance *Army, bool detailed);
};

struct DLL_LINKAGE InfoAboutHero : public InfoAboutArmy
{
private:
	void assign(const InfoAboutHero & iah);

public:
	struct DLL_LINKAGE Details
	{
		std::vector<si32> primskills;
		si32 mana, manaLimit, luck, morale;
	} *details;

	const CHeroClass *hclass;
	int portrait;

	enum EInfoLevel
	{
		BASIC,
		DETAILED,
		INBATTLE
	};

	InfoAboutHero();
	InfoAboutHero(const InfoAboutHero & iah);
	InfoAboutHero(const CGHeroInstance *h, EInfoLevel infoLevel);
	~InfoAboutHero();

	InfoAboutHero & operator=(const InfoAboutHero & iah);

	void initFromHero(const CGHeroInstance *h, EInfoLevel infoLevel);
};

/// Struct which holds a int information about a town
struct DLL_LINKAGE InfoAboutTown : public InfoAboutArmy
{
	struct DLL_LINKAGE Details
	{
		si32 hallLevel, goldIncome;
		bool customRes;
		bool garrisonedHero;

	} *details;

	const CTown *tType;

	si32 built;
	si32 fortLevel; //0 - none

	InfoAboutTown();
	InfoAboutTown(const CGTownInstance *t, bool detailed);
	~InfoAboutTown();
	void initFromTown(const CGTownInstance *t, bool detailed);
};

class DLL_LINKAGE EVictoryLossCheckResult
{
public:
	static EVictoryLossCheckResult victory(std::string toSelf, std::string toOthers)
	{
		return EVictoryLossCheckResult(VICTORY, toSelf, toOthers);
	}

	static EVictoryLossCheckResult defeat(std::string toSelf, std::string toOthers)
	{
		return EVictoryLossCheckResult(DEFEAT, toSelf, toOthers);
	}

	EVictoryLossCheckResult():
	intValue(0)
	{
	}

	bool operator==(EVictoryLossCheckResult const & other) const
	{
		return intValue == other.intValue;
	}

	bool operator!=(EVictoryLossCheckResult const & other) const
	{
		return intValue != other.intValue;
	}

	bool victory() const
	{
		return intValue == VICTORY;
	}
	bool loss() const
	{
		return intValue == DEFEAT;
	}

	EVictoryLossCheckResult invert()
	{
		return EVictoryLossCheckResult(-intValue, messageToOthers, messageToSelf);
	}

	std::string messageToSelf;
	std::string messageToOthers;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & intValue;
		h & messageToSelf;
		h & messageToOthers;
	}
private:
	enum EResult
	{
		DEFEAT = -1,
		INGAME =  0,
		VICTORY= +1
	};

	EVictoryLossCheckResult(si32 intValue, std::string toSelf, std::string toOthers):
		messageToSelf(toSelf),
		messageToOthers(toOthers),
		intValue(intValue)
	{
	}

	si32 intValue; // uses EResult
};

/*static std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult)
{
	os << victoryLossCheckResult.messageToSelf;
	return os;
}*/

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	const CQuest * quest;
	const CGObjectInstance * obj; //related object, most likely Seer Hut
	int3 tile;

	QuestInfo()
		: quest(nullptr), obj(nullptr), tile(-1,-1,-1)
	{};
	QuestInfo (const CQuest * Quest, const CGObjectInstance * Obj, int3 Tile) :
		quest (Quest), obj (Obj), tile (Tile){};

	QuestInfo (const QuestInfo &qi) : quest(qi.quest), obj(qi.obj), tile(qi.tile)
	{};

	const QuestInfo& operator= (const QuestInfo &qi)
	{
		quest = qi.quest;
		obj = qi.obj;
		tile = qi.tile;
		return *this;
	}

	bool operator== (const QuestInfo & qi) const
	{
		return (quest == qi.quest && obj == qi.obj);
	}

	//std::vector<std::string> > texts //allow additional info for quest log?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest;
		h & obj;
		h & tile;
	}
};

VCMI_LIB_NAMESPACE_END
