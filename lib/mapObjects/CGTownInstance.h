/*
 * CGTownInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CObjectHandler.h"
#include "CGMarket.h" // For IMarket interface
#include "CArmedInstance.h"
#include "CGDwelling.h"
#include "CGTownBuilding.h"

#include "../CTownHandler.h" // For CTown

VCMI_LIB_NAMESPACE_BEGIN

class CCastleEvent;
struct DamageRange;


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
		Entry(int subID, const BuildingID & building, int _count);
		Entry(int _count, std::string fullDescription);
	};

	std::vector<Entry> entries;
	int totalGrowth() const;
};

class DLL_LINKAGE CGTownInstance : public CGDwelling, public IShipyard, public IMarket, public INativeTerrainProvider
{
	std::string name; // name of town
public:
	enum EFortLevel {NONE = 0, FORT = 1, CITADEL = 2, CASTLE = 3};

	CTownAndVisitingHero townAndVis;
	const CTown * town;
	si32 builded; //how many buildings has been built this turn
	si32 destroyed; //how many buildings has been destroyed this turn
	ConstTransitivePtr<CGHeroInstance> garrisonHero, visitingHero;
	ui32 identifier; //special identifier from h3m (only > RoE maps)
	PlayerColor alignmentToPlayer; // if set to non-neutral, random town will have same faction as specified player
	std::set<BuildingID> forbiddenBuildings;
	std::set<BuildingID> builtBuildings;
	std::set<BuildingID> overriddenBuildings; ///buildings which bonuses are overridden and should not be applied
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
		h & name;
		h & builded;
		h & destroyed;
		h & identifier;
		h & garrisonHero;
		h & visitingHero;
		h & alignmentToPlayer;
		h & forbiddenBuildings;
		h & builtBuildings;
		h & bonusValue;
		h & possibleSpells;
		h & obligatorySpells;
		h & spells;
		h & events;
		h & bonusingBuildings;
		
		for(auto * bonusingBuilding : bonusingBuildings)
			bonusingBuilding->town = this;
		
		h & town;
		h & townAndVis;
		BONUS_TREE_DESERIALIZATION_FIX

		if(town)
		{
			vstd::erase_if(builtBuildings, [this](BuildingID building) -> bool
			{
				if(!town->buildings.count(building) || !town->buildings.at(building))
				{
					logGlobal->error("#1444-like issue in CGTownInstance::serialize. From town %s at %s removing the bogus builtBuildings item %s", name, pos.toString(), building);
					return true;
				}
				return false;
			});
		}

		h & overriddenBuildings;

		if(!h.saving)
			this->setNodeType(CBonusSystemNode::TOWN);
	}
	//////////////////////////////////////////////////////////////////////////

	CBonusSystemNode & whatShouldBeAttached() override;
	std::string nodeName() const override;
	void updateMoraleBonusFromArmy() override;
	void deserializationFix();
	void recreateBuildingsBonuses();
	void setVisitingHero(CGHeroInstance *h);
	void setGarrisonedHero(CGHeroInstance *h);
	const CArmedInstance *getUpperArmy() const; //garrisoned hero if present or the town itself

	std::string getNameTranslated() const;
	void setNameTranslated(const std::string & newName);

	//////////////////////////////////////////////////////////////////////////

	bool passableFor(PlayerColor color) const override;
	//int3 getSightCenter() const override; //"center" tile from which the sight distance is calculated
	int getSightRadius() const override; //returns sight distance
	BoatId getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
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
	std::vector<const CGTownBuilding *> getBonusingBuildings(BuildingSubID::EBuildingSubID subId) const;
	bool hasBuiltSomeTradeBuilding() const;
	//checks if special building with type buildingID is constructed
	bool hasBuilt(BuildingSubID::EBuildingSubID buildingID) const;
	//checks if building is constructed and town has same subID
	bool hasBuilt(const BuildingID & buildingID) const;
	bool hasBuilt(const BuildingID & buildingID, int townID) const;

	TResources getBuildingCost(const BuildingID & buildingID) const;
	TResources dailyIncome() const; //calculates daily income of this town
	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)
	bool armedGarrison() const; //true if town has creatures in garrison or garrisoned hero
	int getTownLevel() const;

	CBuilding::TRequired genBuildingRequirements(const BuildingID & build, bool deep = false) const;

	void mergeGarrisonOnSiege() const; // merge garrison into army of visiting hero
	void removeCapitols(const PlayerColor & owner) const;
	void clearArmy() const;
	void addHeroToStructureVisitors(const CGHeroInstance *h, si64 structureInstanceID) const; //hero must be visiting or garrisoned in town
	void deleteTownBonus(BuildingID::EBuildingID bid);

	/// Returns damage range for secondary towers of this town
	DamageRange getTowerDamageRange() const;

	/// Returns damage range for central tower(keep) of this town
	DamageRange getKeepDamageRange() const;

	const CTown * getTown() const;

	/// INativeTerrainProvider
	FactionID getFaction() const override;
	TerrainId getNativeTerrain() const override;

	CGTownInstance();
	virtual ~CGTownInstance();

	///IObjectInterface overrides
	void newTurn(CRandomGenerator & rand) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void onHeroLeave(const CGHeroInstance * h) const override;
	void initObj(CRandomGenerator & rand) override;
	void battleFinished(const CGHeroInstance * hero, const BattleResult & result) const override;
	std::string getObjectName() const override;

	void afterAddToMap(CMap * map) override;
	void afterRemoveFromMap(CMap * map) override;
	static void reset();

	inline bool isBattleOutsideTown(const CGHeroInstance * defendingHero) const
	{
		return defendingHero && garrisonHero && defendingHero != garrisonHero;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

private:
	void setOwner(const PlayerColor & owner) const;
	void onTownCaptured(const PlayerColor & winner) const;
	int getDwellingBonus(const std::vector<CreatureID>& creatureIds, const std::vector<ConstTransitivePtr<CGDwelling> >& dwellings) const;
	bool townEnvisagesBuilding(BuildingSubID::EBuildingSubID bid) const;
	bool isBonusingBuildingAdded(BuildingID::EBuildingID bid) const;
	void initOverriddenBids();
	void addTownBonuses(CRandomGenerator & rand);
};

VCMI_LIB_NAMESPACE_END
