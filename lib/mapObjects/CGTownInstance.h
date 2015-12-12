#pragma once

#include "CObjectHandler.h"
#include "CGMarket.h" // For IMarket interface
#include "CArmedInstance.h"

#include "../CTownHandler.h" // For CTown

/*
 * CGTownInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCastleEvent;
class CGTownInstance;

class DLL_LINKAGE CSpecObjInfo
{
public:
	virtual ~CSpecObjInfo(){};
	PlayerColor player; //owner
};

class DLL_LINKAGE CCreGenAsCastleInfo : public virtual CSpecObjInfo
{
public:
	bool asCastle;
	ui32 identifier;
	ui8 castles[2]; //allowed castles
};

class DLL_LINKAGE CCreGenLeveledInfo : public virtual CSpecObjInfo
{
public:
	ui8 minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};

class DLL_LINKAGE CCreGenLeveledCastleInfo : public CCreGenAsCastleInfo, public CCreGenLeveledInfo
{
};

class DLL_LINKAGE CGDwelling : public CArmedInstance
{
public:
	typedef std::vector<std::pair<ui32, std::vector<CreatureID> > > TCreaturesSet;

	CSpecObjInfo * info; //h3m info about dewlling
	TCreaturesSet creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

private:
	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void newTurn() const override;
	void setPropertyDer(ui8 what, ui32 val) override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void updateGuards() const;
	void heroAcceptsCreatures(const CGHeroInstance *h) const;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this) & creatures;
	}
};

class DLL_LINKAGE CGTownBuilding : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	BuildingID ID; //from buildig list
	si32 id; //identifies its index on towns vector
	CGTownInstance *town;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & id;
	}
};
class DLL_LINKAGE COPWBonus : public CGTownBuilding
{///used for OPW bonusing structures
public:
	std::set<si32> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	COPWBonus (BuildingID index, CGTownInstance *TOWN);
	COPWBonus (){ID = BuildingID::NONE; town = nullptr;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownBonus : public CGTownBuilding
{
///used for one-time bonusing structures
///feel free to merge inheritance tree
public:
	std::set<ObjectInstanceID> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	CTownBonus (BuildingID index, CGTownInstance *TOWN);
	CTownBonus (){ID = BuildingID::NONE; town = nullptr;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownAndVisitingHero : public CBonusSystemNode
{
public:
	CTownAndVisitingHero();
};

struct DLL_LINKAGE GrowthInfo
{
	struct Entry
	{
		int count;
		std::string description;
		Entry(const std::string &format, int _count);
		Entry(int subID, BuildingID building, int _count);
		Entry(int _count, const std::string &fullDescription);
	};

	std::vector<Entry> entries;
	int totalGrowth() const;
};

class DLL_LINKAGE CGTownInstance : public CGDwelling, public IShipyard, public IMarket
{
public:
	enum EFortLevel {NONE = 0, FORT = 1, CITADEL = 2, CASTLE = 3};

	CTownAndVisitingHero townAndVis;
	const CTown * town;
	std::string name; // name of town
	si32 builded; //how many buildings has been built this turn
	si32 destroyed; //how many buildings has been destroyed this turn
	ConstTransitivePtr<CGHeroInstance> garrisonHero, visitingHero;
	ui32 identifier; //special identifier from h3m (only > RoE maps)
	si32 alignment;
	std::set<BuildingID> forbiddenBuildings, builtBuildings;
	std::vector<CGTownBuilding*> bonusingBuildings;
	std::vector<SpellID> possibleSpells, obligatorySpells;
	std::vector<std::vector<SpellID> > spells; //spells[level] -> vector of spells, first will be available in guild
	std::list<CCastleEvent> events;
	std::pair<si32, si32> bonusValue;//var to store town bonuses (rampart = resources from mystic pond);

	//////////////////////////////////////////////////////////////////////////
	static std::vector<const CArtifact *> merchantArtifacts; //vector of artifacts available at Artifact merchant, NULLs possible (for making empty space when artifact is bought)
	static std::vector<int> universitySkills;//skills for university of magic

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGDwelling&>(*this);
		h & static_cast<IShipyard&>(*this);
		h & static_cast<IMarket&>(*this);
		h & name & builded & destroyed & identifier;
		h & garrisonHero & visitingHero;
		h & alignment & forbiddenBuildings & builtBuildings & bonusValue
			& possibleSpells & obligatorySpells & spells & /*strInfo & */events & bonusingBuildings;

		for (std::vector<CGTownBuilding*>::iterator i = bonusingBuildings.begin(); i!=bonusingBuildings.end(); i++)
			(*i)->town = this;

		h & town & townAndVis;
		BONUS_TREE_DESERIALIZATION_FIX

		vstd::erase_if(builtBuildings, [this](BuildingID building) -> bool
		{
			if(!town->buildings.count(building) ||  !town->buildings.at(building))
			{
				logGlobal->errorStream() << boost::format("#1444-like issue in CGTownInstance::serialize. From town %s at %s removing the bogus builtBuildings item %s")
					% name % pos % building;
				return true;
			}
			return false;
		});
	}
	//////////////////////////////////////////////////////////////////////////

	CBonusSystemNode *whatShouldBeAttached() override;
	std::string nodeName() const override;
	void updateMoraleBonusFromArmy() override;
	void deserializationFix();
	void recreateBuildingsBonuses();
	bool addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, TPropagatorPtr &prop, int subtype = -1); //returns true if building is built and bonus has been added
	bool addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, int subtype = -1); //convienence version of above
	void setVisitingHero(CGHeroInstance *h);
	void setGarrisonedHero(CGHeroInstance *h);
	const CArmedInstance *getUpperArmy() const; //garrisoned hero if present or the town itself

	//////////////////////////////////////////////////////////////////////////

	bool passableFor(PlayerColor color) const override;
	//int3 getSightCenter() const override; //"center" tile from which the sight distance is calculated
	int getSightRadious() const override; //returns sight distance
	int getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed. Parameter will be cleared
	int getMarketEfficiency() const override; //=market count
	bool allowsTrade(EMarketMode::EMarketMode mode) const override;
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;

	void setType(si32 ID, si32 subID) override;
	void updateAppearance();

	//////////////////////////////////////////////////////////////////////////

	bool needsLastStack() const override;
	CGTownInstance::EFortLevel fortLevel() const;
	int hallLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int mageGuildLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int getHordeLevel(const int & HID) const; //HID - 0 or 1; returns creature level or -1 if that horde structure is not present
	int creatureGrowth(const int & level) const;
	GrowthInfo getGrowthInfo(int level) const;
	bool hasFort() const;
	bool hasCapitol() const;
	//checks if building is constructed and town has same subID
	bool hasBuilt(BuildingID buildingID) const;
	bool hasBuilt(BuildingID buildingID, int townID) const;
	TResources dailyIncome() const; //calculates daily income of this town
	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)
	bool armedGarrison() const; //true if town has creatures in garrison or garrisoned hero
	int getTownLevel() const;

	CBuilding::TRequired genBuildingRequirements(BuildingID build, bool includeUpgrade=true) const;

	void removeCapitols (PlayerColor owner) const;
	void addHeroToStructureVisitors(const CGHeroInstance *h, si32 structureInstanceID) const; //hero must be visiting or garrisoned in town

	CGTownInstance();
	virtual ~CGTownInstance();

	///IObjectInterface overrides
	void newTurn() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void onHeroLeave(const CGHeroInstance * h) const override;
	void initObj() override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	std::string getObjectName() const override;
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};
