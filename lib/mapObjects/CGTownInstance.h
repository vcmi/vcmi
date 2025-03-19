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

#include "IMarket.h"
#include "CGDwelling.h"
#include "../ConstTransitivePtr.h"
#include "../entities/faction/CFaction.h" // TODO: remove
#include "../entities/faction/CTown.h" // TODO: remove

VCMI_LIB_NAMESPACE_BEGIN

class CCastleEvent;
class CTown;
class TownBuildingInstance;
struct TownFortifications;
class TownRewardableBuildingInstance;
struct DamageRange;

template<typename ContainedClass>
class LogicalExpression;

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
	int handicapPercentage;
};

class DLL_LINKAGE CGTownInstance : public CGDwelling, public IShipyard, public IMarket, public INativeTerrainProvider, public ICreatureUpgrader
{
	friend class CTownInstanceConstructor;
	std::string nameTextId; // name of town

	std::map<BuildingID, TownRewardableBuildingInstance*> convertOldBuildings(std::vector<TownRewardableBuildingInstance*> oldVector);
	std::set<BuildingID> builtBuildings;

public:
	enum EFortLevel {NONE = 0, FORT = 1, CITADEL = 2, CASTLE = 3};

	CTownAndVisitingHero townAndVis;
	si32 built; //how many buildings has been built this turn
	si32 destroyed; //how many buildings has been destroyed this turn
	ConstTransitivePtr<CGHeroInstance> garrisonHero, visitingHero;
	ui32 identifier; //special identifier from h3m (only > RoE maps)
	PlayerColor alignmentToPlayer; // if set to non-neutral, random town will have same faction as specified player
	std::set<BuildingID> forbiddenBuildings;
	std::map<BuildingID, TownRewardableBuildingInstance*> rewardableBuildings;
	std::vector<SpellID> possibleSpells, obligatorySpells;
	std::vector<std::vector<SpellID> > spells; //spells[level] -> vector of spells, first will be available in guild
	std::vector<CCastleEvent> events;
	std::pair<si32, si32> bonusValue;//var to store town bonuses (rampart = resources from mystic pond, factory = save debts);
	int spellResearchCounterDay;
	int spellResearchAcceptedCounter;
	bool spellResearchAllowed;

	//////////////////////////////////////////////////////////////////////////
	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CGDwelling&>(*this);
		h & nameTextId;
		h & built;
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
		h & spellResearchCounterDay;
		h & spellResearchAcceptedCounter;
		h & spellResearchAllowed;
		h & rewardableBuildings;
		h & townAndVis;
		BONUS_TREE_DESERIALIZATION_FIX

		if(!h.saving)
			postDeserialize();
	}
	//////////////////////////////////////////////////////////////////////////

	CBonusSystemNode & whatShouldBeAttached() override;
	std::string nodeName() const override;
	void updateMoraleBonusFromArmy() override;
	void deserializationFix();
	void postDeserialize();
	void recreateBuildingsBonuses();
	void setVisitingHero(CGHeroInstance *h);
	void setGarrisonedHero(CGHeroInstance *h);
	const CArmedInstance *getUpperArmy() const; //garrisoned hero if present or the town itself

	std::string getNameTranslated() const;
	std::string getNameTextID() const;
	void setNameTextId(const std::string & newName);

	//////////////////////////////////////////////////////////////////////////

	bool passableFor(PlayerColor color) const override;
	//int3 getSightCenter() const override; //"center" tile from which the sight distance is calculated
	int getSightRadius() const override; //returns sight distance
	BoatId getBoatType() const override; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	void getOutOffsets(std::vector<int3> &offsets) const override; //offsets to obj pos when we boat can be placed. Parameter will be cleared
	EGeneratorState shipyardStatus() const override;
	const IObjectInterface * getObject() const override;
	int getMarketEfficiency() const override; //=market count
	std::set<EMarketMode> availableModes() const override;
	std::vector<TradeItemBuy> availableItemsIds(EMarketMode mode) const override;
	ObjectInstanceID getObjInstanceID() const override;
	void updateAppearance();

	//////////////////////////////////////////////////////////////////////////

	bool needsLastStack() const override;
	CGTownInstance::EFortLevel fortLevel() const;
	TownFortifications fortificationsLevel() const;
	int hallLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int mageGuildLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int getHordeLevel(const int & HID) const; //HID - 0 or 1; returns creature level or -1 if that horde structure is not present
	int creatureGrowth(const int & level) const;
	GrowthInfo getGrowthInfo(int level) const;
	bool hasFort() const;
	bool hasCapitol() const;
	bool hasBuiltSomeTradeBuilding() const;
	//checks if special building with type buildingID is constructed
	bool hasBuilt(BuildingSubID::EBuildingSubID buildingID) const;
	//checks if building is constructed and town has same subID
	bool hasBuilt(const BuildingID & buildingID) const;
	bool hasBuilt(const BuildingID & buildingID, FactionID townID) const;
	void addBuilding(const BuildingID & buildingID);
	void removeBuilding(const BuildingID & buildingID);
	void removeAllBuildings();
	std::set<BuildingID> getBuildings() const;

	TResources getBuildingCost(const BuildingID & buildingID) const;
	ResourceSet dailyIncome() const override;
	std::vector<CreatureID> providedCreatures() const override;

	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)
	bool armedGarrison() const; //true if town has creatures in garrison or garrisoned hero
	int getTownLevel() const;

	LogicalExpression<BuildingID> genBuildingRequirements(const BuildingID & build, bool deep = false) const;

	void mergeGarrisonOnSiege() const; // merge garrison into army of visiting hero
	void removeCapitols(const PlayerColor & owner) const;
	void clearArmy() const;
	void addHeroToStructureVisitors(const CGHeroInstance *h, si64 structureInstanceID) const; //hero must be visiting or garrisoned in town
	void deleteTownBonus(BuildingID bid);

	/// Returns damage range for secondary towers of this town
	DamageRange getTowerDamageRange() const;

	/// Returns damage range for central tower(keep) of this town
	DamageRange getKeepDamageRange() const;

	const CTown * getTown() const;
	const CFaction * getFaction() const;

	/// INativeTerrainProvider
	FactionID getFactionID() const override;
	TerrainId getNativeTerrain() const override;

	/// Returns ID of war machine that is produced by specified building or NONE if this is not built or if building does not produce war machines
	ArtifactID getWarMachineInBuilding(BuildingID) const;
	/// Returns true if provided war machine is available in any of built buildings of this town
	bool isWarMachineAvailable(ArtifactID) const;

	CGTownInstance(IGameCallback *cb);
	virtual ~CGTownInstance();

	///IObjectInterface overrides
	void newTurn(vstd::RNG & rand) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void onHeroLeave(const CGHeroInstance * h) const override;
	void initObj(vstd::RNG & rand) override;
	void pickRandomObject(vstd::RNG & rand) override;
	void battleFinished(const CGHeroInstance * hero, const BattleResult & result) const override;
	std::string getObjectName() const override;

	void fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const override;

	void afterAddToMap(CMap * map) override;
	void afterRemoveFromMap(CMap * map) override;

	inline bool isBattleOutsideTown(const CGHeroInstance * defendingHero) const
	{
		return defendingHero && garrisonHero && defendingHero != garrisonHero;
	}
protected:
	void setPropertyDer(ObjProperty what, ObjPropertyID identifier) override;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

private:
	FactionID randomizeFaction(vstd::RNG & rand);
	void setOwner(const PlayerColor & owner) const;
	void onTownCaptured(const PlayerColor & winner) const;
	int getDwellingBonus(const std::vector<CreatureID>& creatureIds, const std::vector<const CGObjectInstance* >& dwellings) const;
	bool townEnvisagesBuilding(BuildingSubID::EBuildingSubID bid) const;
	void initializeConfigurableBuildings(vstd::RNG & rand);
	void initializeNeutralTownGarrison(vstd::RNG & rand);
};

VCMI_LIB_NAMESPACE_END
