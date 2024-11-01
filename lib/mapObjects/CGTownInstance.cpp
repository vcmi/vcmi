/*
 * CGTownInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGTownInstance.h"

#include "TownBuildingInstance.h"
#include "../spells/CSpellHandler.h"
#include "../bonuses/Bonus.h"
#include "../battle/IBattleInfoCallback.h"
#include "../battle/BattleLayout.h"
#include "../CConfigHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"
#include "../StartInfo.h"
#include "../TerrainHandler.h"
#include "../entities/building/CBuilding.h"
#include "../entities/faction/CTownHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../modding/ModScope.h"
#include "../networkPacks/StackLocation.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

int CGTownInstance::getSightRadius() const //returns sight distance
{
	auto ret = CBuilding::HEIGHT_NO_TOWER;

	for(const auto & bid : builtBuildings)
	{
		auto height = town->buildings.at(bid)->height;
		if(ret < height)
			ret = height;
	}
	return ret;
}

void CGTownInstance::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
///this is freakin' overcomplicated solution
	switch (what)
	{
		case ObjProperty::STRUCTURE_ADD_VISITING_HERO:
			rewardableBuildings.at(identifier.getNum())->setProperty(ObjProperty::VISITORS, visitingHero->id);
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			rewardableBuildings.at(identifier.getNum())->setProperty(ObjProperty::STRUCTURE_CLEAR_VISITORS, NumericID(0));
			break;
		case ObjProperty::STRUCTURE_ADD_GARRISONED_HERO: //add garrisoned hero to visitors
			rewardableBuildings.at(identifier.getNum())->setProperty(ObjProperty::VISITORS, garrisonHero->id);
			break;
		case ObjProperty::BONUS_VALUE_FIRST:
			bonusValue.first = identifier.getNum();
			break;
		case ObjProperty::BONUS_VALUE_SECOND:
			bonusValue.second = identifier.getNum();
			break;
	}
}
CGTownInstance::EFortLevel CGTownInstance::fortLevel() const //0 - none, 1 - fort, 2 - citadel, 3 - castle
{
	if (hasBuilt(BuildingID::CASTLE))
		return CASTLE;
	if (hasBuilt(BuildingID::CITADEL))
		return CITADEL;
	if (hasBuilt(BuildingID::FORT))
		return FORT;
	return NONE;
}

int CGTownInstance::hallLevel() const // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
{
	if (hasBuilt(BuildingID::CAPITOL))
		return 3;
	if (hasBuilt(BuildingID::CITY_HALL))
		return 2;
	if (hasBuilt(BuildingID::TOWN_HALL))
		return 1;
	if (hasBuilt(BuildingID::VILLAGE_HALL))
		return 0;
	return -1;
}

int CGTownInstance::mageGuildLevel() const
{
	if (hasBuilt(BuildingID::MAGES_GUILD_5))
		return 5;
	if (hasBuilt(BuildingID::MAGES_GUILD_4))
		return 4;
	if (hasBuilt(BuildingID::MAGES_GUILD_3))
		return 3;
	if (hasBuilt(BuildingID::MAGES_GUILD_2))
		return 2;
	if (hasBuilt(BuildingID::MAGES_GUILD_1))
		return 1;
	return 0;
}

int CGTownInstance::getHordeLevel(const int & HID)  const//HID - 0 or 1; returns creature level or -1 if that horde structure is not present
{
	return town->hordeLvl.at(HID);
}

int CGTownInstance::creatureGrowth(const int & level) const
{
	return getGrowthInfo(level).totalGrowth();
}

GrowthInfo CGTownInstance::getGrowthInfo(int level) const
{
	GrowthInfo ret;

	if (level<0 || level >=town->creatures.size())
		return ret;
	if (creatures[level].second.empty())
		return ret; //no dwelling

	const Creature *creature = creatures[level].second.back().toEntity(VLC);
	const int base = creature->getGrowth();
	int castleBonus = 0;

	if(tempOwner.isValidPlayer())
	{
		auto * playerSettings = cb->getPlayerSettings(tempOwner);
		ret.handicapPercentage = playerSettings->handicap.percentGrowth;
	}
	else
		ret.handicapPercentage = 100;

	ret.entries.emplace_back(VLC->generaltexth->allTexts[590], base); // \n\nBasic growth %d"

	if (hasBuilt(BuildingID::CASTLE))
		ret.entries.emplace_back(subID, BuildingID::CASTLE, castleBonus = base);
	else if (hasBuilt(BuildingID::CITADEL))
		ret.entries.emplace_back(subID, BuildingID::CITADEL, castleBonus = base / 2);

	if(town->hordeLvl.at(0) == level)//horde 1
		if(hasBuilt(BuildingID::HORDE_1))
			ret.entries.emplace_back(subID, BuildingID::HORDE_1, creature->getHorde());

	if(town->hordeLvl.at(1) == level)//horde 2
		if(hasBuilt(BuildingID::HORDE_2))
			ret.entries.emplace_back(subID, BuildingID::HORDE_2, creature->getHorde());

	//statue-of-legion-like bonus: % to base+castle
	TConstBonusListPtr bonuses2 = getBonuses(Selector::type()(BonusType::CREATURE_GROWTH_PERCENT));
	for(const auto & b : *bonuses2)
	{
		const auto growth = b->val * (base + castleBonus) / 100;
		if (growth)
		{
			ret.entries.emplace_back(growth, b->Description(growth));
		}
	}

	//other *-of-legion-like bonuses (%d to growth cumulative with grail)
	// Note: bonus uses 1-based levels (Pikeman is level 1), town list uses 0-based (Pikeman in 0-th creatures entry)
	TConstBonusListPtr bonuses = getBonuses(Selector::typeSubtype(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(level+1)));
	for(const auto & b : *bonuses)
		ret.entries.emplace_back(b->val, b->Description());

	int dwellingBonus = 0;
	if(const PlayerState *p = cb->getPlayerState(tempOwner, false))
	{
		dwellingBonus = getDwellingBonus(creatures[level].second, p->getOwnedObjects());
	}
	if(dwellingBonus)
		ret.entries.emplace_back(VLC->generaltexth->allTexts[591], dwellingBonus); // \nExternal dwellings %+d

	if(hasBuilt(BuildingID::GRAIL)) //grail - +50% to ALL (so far added) growth
		ret.entries.emplace_back(subID, BuildingID::GRAIL, ret.totalGrowth() / 2);

	return ret;
}

int CGTownInstance::getDwellingBonus(const std::vector<CreatureID>& creatureIds, const std::vector<const CGObjectInstance * >& dwellings) const
{
	int totalBonus = 0;
	for (const auto& dwelling : dwellings)
	{
		const auto & dwellingCreatures = dwelling->asOwnable()->providedCreatures();
		bool hasMatch = false;
		for (const auto& creature : dwellingCreatures)
			hasMatch = vstd::contains(creatureIds, creature);

		if (hasMatch)
			totalBonus += 1;
	}
	return totalBonus;
}

TResources CGTownInstance::dailyIncome() const
{
	TResources ret;
	for(const auto & p : town->buildings)
	{
		BuildingID buildingUpgrade;

		for(const auto & p2 : town->buildings)
		{
			if (p2.second->upgrade == p.first)
			{
				buildingUpgrade = p2.first;
			}
		}
		if (!hasBuilt(buildingUpgrade)&&(hasBuilt(p.first)))
		{
			ret += p.second->produce;
		}
	}

	if (!getOwner().isValidPlayer())
		return ret;

	const auto & playerSettings = cb->getPlayerSettings(getOwner());
	ret.applyHandicap(playerSettings->handicap.percentIncome);
	return ret;
}

std::vector<CreatureID> CGTownInstance::providedCreatures() const
{
	return {};
}

bool CGTownInstance::hasFort() const
{
	return hasBuilt(BuildingID::FORT);
}

bool CGTownInstance::hasCapitol() const
{
	return hasBuilt(BuildingID::CAPITOL);
}

TownFortifications CGTownInstance::fortificationsLevel() const
{
	auto result = town->fortifications;

	for (auto const	& buildingID : builtBuildings)
		result += town->buildings.at(buildingID)->fortifications;

	if (result.wallsHealth == 0)
		return TownFortifications();

	return result;
}

CGTownInstance::CGTownInstance(IGameCallback *cb):
	CGDwelling(cb),
	town(nullptr),
	built(0),
	destroyed(0),
	identifier(0),
	alignmentToPlayer(PlayerColor::NEUTRAL)
{
	this->setNodeType(CBonusSystemNode::TOWN);
}

CGTownInstance::~CGTownInstance()
{
	for (auto & elem : rewardableBuildings)
		delete elem.second;
}

int CGTownInstance::spellsAtLevel(int level, bool checkGuild) const
{
	if(checkGuild && mageGuildLevel() < level)
		return 0;
	int ret = 6 - level; //how many spells are available at this level

	if (hasBuilt(BuildingSubID::LIBRARY))
		ret++;

	return ret;
}

bool CGTownInstance::needsLastStack() const
{
	return garrisonHero != nullptr;
}

void CGTownInstance::setOwner(const PlayerColor & player) const
{
	removeCapitols(player);
	cb->setOwner(this, player);
}

void CGTownInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(cb->gameState()->getPlayerRelations( getOwner(), h->getOwner() ) == PlayerRelations::ENEMIES)
	{
		if(armedGarrison() || visitingHero)
		{
			const CGHeroInstance * defendingHero = visitingHero ? visitingHero : garrisonHero;
			const CArmedInstance * defendingArmy = defendingHero ? (CArmedInstance *)defendingHero : this;
			const bool isBattleOutside = isBattleOutsideTown(defendingHero);

			if(!isBattleOutside && visitingHero && defendingHero == visitingHero)
			{
				//we have two approaches to merge armies: mergeGarrisonOnSiege() and used in the CGameHandler::garrisonSwap(ObjectInstanceID tid)
				auto * nodeSiege = defendingHero->whereShouldBeAttachedOnSiege(isBattleOutside);

				if(nodeSiege == (CBonusSystemNode *)this)
					cb->swapGarrisonOnSiege(this->id);

				const_cast<CGHeroInstance *>(defendingHero)->inTownGarrison = false; //hack to return visitor from garrison after battle
			}
			cb->startBattle(h, defendingArmy, getSightCenter(), h, defendingHero, BattleLayout::createDefaultLayout(cb, h, defendingArmy), (isBattleOutside ? nullptr : this));
		}
		else
		{
			auto heroColor = h->getOwner();
			onTownCaptured(heroColor);

			if(cb->gameState()->getPlayerStatus(heroColor) == EPlayerStatus::WINNER)
			{
				return; //we just won game, we do not need to perform any extra actions
				//TODO: check how does H3 behave, visiting town on victory can affect campaigns (spells learned, +1 stat building visited)
			}
			cb->heroVisitCastle(this, h);
		}
	}
	else
	{
		assert(h->visitablePos() == this->visitablePos());
		bool commander_recover = h->commander && !h->commander->alive;
		if (commander_recover) // rise commander from dead
		{
			SetCommanderProperty scp;
			scp.heroid = h->id;
			scp.which = SetCommanderProperty::ALIVE;
			scp.amount = 1;
			cb->sendAndApply(&scp);
		}
		cb->heroVisitCastle(this, h);
		// TODO(vmarkovtsev): implement payment for rising the commander
		if (commander_recover) // info window about commander
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.appendRawString(h->commander->getName());
			iw.components.emplace_back(ComponentType::CREATURE, h->commander->getId(), h->commander->getCount());
			cb->showInfoDialog(&iw);
		}
	}
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	//FIXME: find out why this issue appears on random maps
	if(visitingHero == h)
	{
		cb->stopHeroVisitCastle(this, h);
		logGlobal->trace("%s correctly left town %s", h->getNameTranslated(), getNameTranslated());
	}
	else
		logGlobal->warn("Warning, %s tries to leave the town %s but hero is not inside.", h->getNameTranslated(), getNameTranslated());
}

std::string CGTownInstance::getObjectName() const
{
	return getNameTranslated() + ", " + town->faction->getNameTranslated();
}

bool CGTownInstance::townEnvisagesBuilding(BuildingSubID::EBuildingSubID subId) const
{
	return town->getBuildingType(subId) != BuildingID::NONE;
}

void CGTownInstance::initializeConfigurableBuildings(vstd::RNG & rand)
{
	for(const auto & kvp : town->buildings)
	{
		if(!kvp.second->rewardableObjectInfo.getParameters().isNull())
			rewardableBuildings[kvp.first] = new TownRewardableBuildingInstance(this, kvp.second->bid, rand);
	}
}

DamageRange CGTownInstance::getTowerDamageRange() const
{
	// http://heroes.thelazy.net/wiki/Arrow_tower
	// base damage, irregardless of town level
	static constexpr int baseDamage = 6;
	// extra damage, for each building in town
	static constexpr int extraDamage = 1;

	const int minDamage = baseDamage + extraDamage * getTownLevel();

	return {
		minDamage,
		minDamage * 2
	};
}

DamageRange CGTownInstance::getKeepDamageRange() const
{
	// http://heroes.thelazy.net/wiki/Arrow_tower
	// base damage, irregardless of town level
	static constexpr int baseDamage = 10;
	// extra damage, for each building in town
	static constexpr int extraDamage = 2;

	const int minDamage = baseDamage + extraDamage * getTownLevel();

	return {
		minDamage,
		minDamage * 2
	};
}

FactionID CGTownInstance::randomizeFaction(vstd::RNG & rand)
{
	if(getOwner().isValidPlayer())
		return cb->gameState()->scenarioOps->getIthPlayersSettings(getOwner()).castle;

	if(alignmentToPlayer.isValidPlayer())
		return cb->gameState()->scenarioOps->getIthPlayersSettings(alignmentToPlayer).castle;

	std::vector<FactionID> potentialPicks;

	for (FactionID faction(0); faction < FactionID(VLC->townh->size()); ++faction)
		if (VLC->factions()->getById(faction)->hasTown())
			potentialPicks.push_back(faction);

	assert(!potentialPicks.empty());
	return *RandomGeneratorUtil::nextItem(potentialPicks, rand);
}

void CGTownInstance::pickRandomObject(vstd::RNG & rand)
{
	assert(ID == MapObjectID::TOWN || ID == MapObjectID::RANDOM_TOWN);
	if (ID == MapObjectID::RANDOM_TOWN)
	{
		ID = MapObjectID::TOWN;
		subID = randomizeFaction(rand);
	}

	assert(ID == Obj::TOWN); // just in case
	setType(ID, subID);
	town = (*VLC->townh)[getFaction()]->town;
	randomizeArmy(getFaction());
	updateAppearance();
}

void CGTownInstance::initObj(vstd::RNG & rand) ///initialize town structures
{
	blockVisit = true;

	if(townEnvisagesBuilding(BuildingSubID::PORTAL_OF_SUMMONING)) //Dungeon for example
		creatures.resize(town->creatures.size() + 1);
	else
		creatures.resize(town->creatures.size());

	for (int level = 0; level < town->creatures.size(); level++)
	{
		BuildingID buildID = BuildingID(BuildingID::getDwellingFromLevel(level, 0));
		int upgradeNum = 0;

		for (; town->buildings.count(buildID); upgradeNum++, BuildingID::advanceDwelling(buildID))
		{
			if (hasBuilt(buildID) && town->creatures.at(level).size() > upgradeNum)
				creatures[level].second.push_back(town->creatures[level][upgradeNum]);
		}
	}
	initializeConfigurableBuildings(rand);
	initializeNeutralTownGarrison(rand);
	recreateBuildingsBonuses();
	updateAppearance();
}

void CGTownInstance::initializeNeutralTownGarrison(vstd::RNG & rand)
{
	struct RandomGuardsInfo{
		int tier;
		int chance;
		int min;
		int max;
	};

	constexpr std::array<RandomGuardsInfo, 4> randomGuards = {
		RandomGuardsInfo{ 0, 33, 8, 15 },
		RandomGuardsInfo{ 1, 33, 5,  7 },
		RandomGuardsInfo{ 2, 20, 3,  5 },
		RandomGuardsInfo{ 3, 14, 1,  3 },
	};

	// Only neutral towns may get initial garrison
	if (getOwner().isValidPlayer())
		return;

	// Only towns with garrison not set in map editor may get initial garrison
	// FIXME: H3 editor allow explicitly empty garrison, but vcmi loses this flag on load
	if (stacksCount() > 0)
		return;

	for (auto const & guard : randomGuards)
	{
		if (rand.nextInt(99) >= guard.chance)
			continue;

		CreatureID guardID = getTown()->creatures[guard.tier].at(0);
		int guardSize = rand.nextInt(guard.min, guard.max);

		putStack(getFreeSlot(), new CStackInstance(guardID, guardSize));
	}
}

void CGTownInstance::newTurn(vstd::RNG & rand) const
{
	for(const auto & building : rewardableBuildings)
		building.second->newTurn(rand);
		
	if(hasBuilt(BuildingSubID::BANK) && bonusValue.second > 0)
	{
		TResources res;
		res[EGameResID::GOLD] = -500;
		cb->giveResources(getOwner(), res);
		cb->setObjPropertyValue(id, ObjProperty::BONUS_VALUE_SECOND, bonusValue.second - 500);
	}
}

bool CGTownInstance::passableFor(PlayerColor color) const
{
	if (!armedGarrison())//empty castle - anyone can visit
		return true;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return false;

	return cb->getPlayerRelations(tempOwner, color) != PlayerRelations::ENEMIES;
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets = {int3(-1,2,0), int3(+1,2,0)};
}

CGTownInstance::EGeneratorState CGTownInstance::shipyardStatus() const
{
	if (!hasBuilt(BuildingID::SHIPYARD))
		return EGeneratorState::UNKNOWN;

	return IShipyard::shipyardStatus();
}

const IObjectInterface * CGTownInstance::getObject() const
{
	return this;
}

void CGTownInstance::mergeGarrisonOnSiege() const
{
	auto getWeakestStackSlot = [&](ui64 powerLimit)
	{
		std::vector<SlotID> weakSlots;
		auto stacksList = visitingHero->stacks;
		std::pair<SlotID, CStackInstance *> pair;
		while(!stacksList.empty())
		{
			pair = *vstd::minElementByFun(stacksList, [&](const std::pair<SlotID, CStackInstance *> & elem) { return elem.second->getPower(); });
			if(powerLimit > pair.second->getPower() &&
				(weakSlots.empty() || pair.second->getPower() == visitingHero->getStack(weakSlots.front()).getPower()))
			{
				weakSlots.push_back(pair.first);
				stacksList.erase(pair.first);
			}
			else
				break;
		}

		if(!weakSlots.empty())
			return *std::max_element(weakSlots.begin(), weakSlots.end());

		return SlotID();
	};

	auto count = static_cast<int>(stacks.size());

	for(int i = 0; i < count; i++)
	{
		auto pair = *vstd::maxElementByFun(stacks, [&](const std::pair<SlotID, CStackInstance *> & elem)
		{
			ui64 power = elem.second->getPower();
			auto dst = visitingHero->getSlotFor(elem.second->getCreatureID());
			if(dst.validSlot() && visitingHero->hasStackAtSlot(dst))
				power += visitingHero->getStack(dst).getPower();

			return power;
		});
		auto dst = visitingHero->getSlotFor(pair.second->getCreatureID());
		if(dst.validSlot())
			cb->moveStack(StackLocation(this, pair.first), StackLocation(visitingHero, dst), -1);
		else
		{
			dst = getWeakestStackSlot(static_cast<int>(pair.second->getPower()));
			if(dst.validSlot())
				cb->swapStacks(StackLocation(this, pair.first), StackLocation(visitingHero, dst));
		}
	}
}

void CGTownInstance::removeCapitols(const PlayerColor & owner) const
{
	if (hasCapitol()) // search if there's an older capitol
	{
		PlayerState* state = cb->gameState()->getPlayerState(owner); //get all towns owned by player
		for (const auto & town : state->getTowns())
		{
			if (town != this && town->hasCapitol())
			{
				RazeStructures rs;
				rs.tid = id;
				rs.bid.insert(BuildingID::CAPITOL);
				rs.destroyed = destroyed;
				cb->sendAndApply(&rs);
				return;
			}
		}
	}
}

void CGTownInstance::clearArmy() const
{
	while(!stacks.empty())
	{
		cb->eraseStack(StackLocation(this, stacks.begin()->first));
	}
}

BoatId CGTownInstance::getBoatType() const
{
	return town->faction->boatType;
}

int CGTownInstance::getMarketEfficiency() const
{
	if(!hasBuiltSomeTradeBuilding())
		return 0;

	const PlayerState *p = cb->getPlayerState(tempOwner);
	assert(p);

	int marketCount = 0;
	for(const CGTownInstance *t : p->getTowns())
		if(t->hasBuiltSomeTradeBuilding())
			marketCount++;

	return marketCount;
}

std::vector<TradeItemBuy> CGTownInstance::availableItemsIds(EMarketMode mode) const
{
	if(mode == EMarketMode::RESOURCE_ARTIFACT)
	{
		std::vector<TradeItemBuy> ret;
		for(const CArtifact *a : cb->gameState()->map->townMerchantArtifacts)
			if(a)
				ret.push_back(a->getId());
			else
				ret.push_back(ArtifactID{});
		return ret;
	}
	else if ( mode == EMarketMode::RESOURCE_SKILL )
	{
		return cb->gameState()->map->townUniversitySkills;
	}
	else
		return IMarket::availableItemsIds(mode);
}

ObjectInstanceID CGTownInstance::getObjInstanceID() const
{
	return id;
}

void CGTownInstance::updateAppearance()
{
	auto terrain = cb->gameState()->getTile(visitablePos())->terType->getId();
	//FIXME: not the best way to do this
	auto app = getObjectHandler()->getOverride(terrain, this);
	if (app)
		appearance = app;
}

std::string CGTownInstance::nodeName() const
{
	return "Town (" + (town ? town->faction->getNameTranslated() : "unknown") + ") of " + getNameTranslated();
}

void CGTownInstance::deserializationFix()
{
	attachTo(townAndVis);

	//Hero is already handled by CGameState::attachArmedObjects

// 	if(visitingHero)
// 		visitingHero->attachTo(&townAndVis);
// 	if(garrisonHero)
// 		garrisonHero->attachTo(this);
}

void CGTownInstance::updateMoraleBonusFromArmy()
{
	auto b = getExportedBonusList().getFirst(Selector::sourceType()(BonusSource::ARMY).And(Selector::type()(BonusType::MORALE)));
	if(!b)
	{
		b = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::ARMY, 0, BonusSourceID());
		addNewBonus(b);
	}

	if (garrisonHero)
	{
		b->val = 0;
		CBonusSystemNode::treeHasChanged();
	}
	else
		CArmedInstance::updateMoraleBonusFromArmy();
}

void CGTownInstance::recreateBuildingsBonuses()
{
	BonusList bl;
	getExportedBonusList().getBonuses(bl, Selector::sourceType()(BonusSource::TOWN_STRUCTURE));

	for(const auto & b : bl)
		removeBonus(b);



	for(const auto & bid : builtBuildings)
	{
		bool bonusesReplacedByUpgrade = false;

		for(const auto & upgradeID : builtBuildings)
		{
			const auto & upgrade = town->buildings.at(upgradeID);
			if (upgrade->getBase() == bid && upgrade->upgradeReplacesBonuses)
				bonusesReplacedByUpgrade = true;
		}

		// bonuses from this building are disabled and replaced by bonuses from an upgrade
		if (bonusesReplacedByUpgrade)
			continue;

		auto building = town->buildings.at(bid);

		if(building->buildingBonuses.empty())
			continue;

		for(auto & bonus : building->buildingBonuses)
			addNewBonus(bonus);
	}
}

void CGTownInstance::setVisitingHero(CGHeroInstance *h)
{
	if(visitingHero.get() == h)
		return;
	
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayerState(h->tempOwner);
		assert(p);
		h->detachFrom(*p);
		h->attachTo(townAndVis);
		visitingHero = h;
		h->visitedTown = this;
		h->inTownGarrison = false;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayerState(visitingHero->tempOwner);
		visitingHero->visitedTown = nullptr;
		visitingHero->detachFrom(townAndVis);
		visitingHero->attachTo(*p);
		visitingHero = nullptr;
	}
}

void CGTownInstance::setGarrisonedHero(CGHeroInstance *h)
{
	if(garrisonHero.get() == h)
		return;
	
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayerState(h->tempOwner);
		assert(p);
		h->detachFrom(*p);
		h->attachTo(*this);
		garrisonHero = h;
		h->visitedTown = this;
		h->inTownGarrison = true;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayerState(garrisonHero->tempOwner);
		garrisonHero->visitedTown = nullptr;
		garrisonHero->inTownGarrison = false;
		garrisonHero->detachFrom(*this);
		garrisonHero->attachTo(*p);
		garrisonHero = nullptr;
	}
	updateMoraleBonusFromArmy(); //avoid giving morale bonus for same army twice
}

bool CGTownInstance::armedGarrison() const
{
	return !stacks.empty() || garrisonHero;
}

const CTown * CGTownInstance::getTown() const
{
    if(ID == Obj::RANDOM_TOWN)
		return VLC->townh->randomTown;
	else
	{
		if(nullptr == town)
		{
			return (*VLC->townh)[getFaction()]->town;
		}
		else
			return town;
	}
}

int CGTownInstance::getTownLevel() const
{
	// count all buildings that are not upgrades
	int level = 0;

	for(const auto & bid : builtBuildings)
	{
		if(town->buildings.at(bid)->upgrade == BuildingID::NONE)
			level++;
	}
	return level;
}

CBonusSystemNode & CGTownInstance::whatShouldBeAttached()
{
	return townAndVis;
}

std::string CGTownInstance::getNameTranslated() const
{
	return VLC->generaltexth->translate(nameTextId);
}

std::string CGTownInstance::getNameTextID() const
{
	return nameTextId;
}

void CGTownInstance::setNameTextId( const std::string & newName )
{
	nameTextId = newName;
}

const CArmedInstance * CGTownInstance::getUpperArmy() const
{
	if(garrisonHero)
		return garrisonHero;
	return this;
}

bool CGTownInstance::hasBuiltSomeTradeBuilding() const
{
	return availableModes().empty() ? false : true;
}

bool CGTownInstance::hasBuilt(BuildingSubID::EBuildingSubID buildingID) const
{
	for(const auto & bid : builtBuildings)
	{
		if(town->buildings.at(bid)->subId == buildingID)
			return true;
	}
	return false;
}

bool CGTownInstance::hasBuilt(const BuildingID & buildingID) const
{
	return vstd::contains(builtBuildings, buildingID);
}

bool CGTownInstance::hasBuilt(const BuildingID & buildingID, FactionID townID) const
{
	if (townID == town->faction->getId() || townID == FactionID::ANY)
		return hasBuilt(buildingID);
	return false;
}

void CGTownInstance::addBuilding(const BuildingID & buildingID)
{
	if(buildingID == BuildingID::NONE)
		return;

	builtBuildings.insert(buildingID);
}

std::set<EMarketMode> CGTownInstance::availableModes() const
{
	std::set<EMarketMode> result;
	for (const auto & buildingID : builtBuildings)
	{
		const auto * buildingPtr = town->buildings.at(buildingID).get();
		result.insert(buildingPtr->marketModes.begin(), buildingPtr->marketModes.end());
	}

	return result;
}

void CGTownInstance::removeBuilding(const BuildingID & buildingID)
{
	if(!vstd::contains(builtBuildings, buildingID))
		return;

	builtBuildings.erase(buildingID);
}

void CGTownInstance::removeAllBuildings()
{
	builtBuildings.clear();
}

std::set<BuildingID> CGTownInstance::getBuildings() const
{
	return builtBuildings;
}

TResources CGTownInstance::getBuildingCost(const BuildingID & buildingID) const
{
	if (vstd::contains(town->buildings, buildingID))
		return town->buildings.at(buildingID)->resources;
	else
	{
		logGlobal->error("Town %s at %s has no possible building %d!", getNameTranslated(), pos.toString(), buildingID.toEnum());
		return TResources();
	}

}

CBuilding::TRequired CGTownInstance::genBuildingRequirements(const BuildingID & buildID, bool deep) const
{
	const CBuilding * building = town->buildings.at(buildID);

	//TODO: find better solution to prevent infinite loops
	std::set<BuildingID> processed;

	std::function<CBuilding::TRequired::Variant(const BuildingID &)> dependTest =
	[&](const BuildingID & id) -> CBuilding::TRequired::Variant
	{
		if (town->buildings.count(id) == 0)
		{
			logMod->error("Invalid building ID %d in building dependencies!", id.getNum());
			return CBuilding::TRequired::OperatorAll();
		}

		const CBuilding * build = town->buildings.at(id);
		CBuilding::TRequired::OperatorAll requirements;

		if (!hasBuilt(id))
		{
			if (deep)
				requirements.expressions.emplace_back(id);
			else
				return id;
		}

		if(!vstd::contains(processed, id))
		{
			processed.insert(id);
			if (build->upgrade != BuildingID::NONE)
				requirements.expressions.push_back(dependTest(build->upgrade));

			requirements.expressions.push_back(build->requirements.morph(dependTest));
		}
		return requirements;
	};

	CBuilding::TRequired::OperatorAll requirements;
	if (building->upgrade != BuildingID::NONE)
	{
		const CBuilding * upgr = town->buildings.at(building->upgrade);

		requirements.expressions.push_back(dependTest(upgr->bid));
		processed.clear();
	}
	requirements.expressions.push_back(building->requirements.morph(dependTest));

	CBuilding::TRequired::Variant variant(requirements);
	CBuilding::TRequired ret(variant);
	ret.minimize();
	return ret;
}

void CGTownInstance::addHeroToStructureVisitors(const CGHeroInstance *h, si64 structureInstanceID ) const
{
	if(visitingHero == h)
		cb->setObjPropertyValue(id, ObjProperty::STRUCTURE_ADD_VISITING_HERO, structureInstanceID); //add to visitors
	else if(garrisonHero == h)
		cb->setObjPropertyValue(id, ObjProperty::STRUCTURE_ADD_GARRISONED_HERO, structureInstanceID); //then it must be garrisoned hero
	else
	{
		//should never ever happen
		logGlobal->error("Cannot add hero %s to visitors of structure # %d", h->getNameTranslated(), structureInstanceID);
		throw std::runtime_error("unexpected hero in CGTownInstance::addHeroToStructureVisitors");
	}
}

void CGTownInstance::battleFinished(const CGHeroInstance * hero, const BattleResult & result) const
{
	if(result.winner == BattleSide::ATTACKER)
	{
		clearArmy();
		onTownCaptured(hero->getOwner());
	}
}

void CGTownInstance::onTownCaptured(const PlayerColor & winner) const
{
	setOwner(winner);
	cb->changeFogOfWar(getSightCenter(), getSightRadius(), winner, ETileVisibility::REVEALED);
}

void CGTownInstance::afterAddToMap(CMap * map)
{
	map->towns.emplace_back(this);
}

void CGTownInstance::afterRemoveFromMap(CMap * map)
{
	vstd::erase_if_present(map->towns, this);
}

void CGTownInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGObjectInstance::serializeJsonOwner(handler);
	if(!handler.saving)
		handler.serializeEnum("tightFormation", formation, NArmyFormation::names); //for old format
	CArmedInstance::serializeJsonOptions(handler);
	handler.serializeString("name", nameTextId);

	{
		auto decodeBuilding = [this](const std::string & identifier) -> si32
		{
			auto rawId = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), getTown()->getBuildingScope(), identifier);

			if(rawId)
				return rawId.value();
			else
				return -1;
		};

		auto encodeBuilding = [this](si32 index) -> std::string
		{
			return getTown()->buildings.at(BuildingID(index))->getJsonKey();
		};

		const std::set<si32> standard = getTown()->getAllBuildings();//by default all buildings are allowed
		JsonSerializeFormat::LICSet buildingsLIC(standard, decodeBuilding, encodeBuilding);

		if(handler.saving)
		{
			bool customBuildings = false;

			boost::logic::tribool hasFort(false);

			for(const BuildingID & id : forbiddenBuildings)
			{
				buildingsLIC.none.insert(id.getNum());
				customBuildings = true;
			}

			for(const BuildingID & id : builtBuildings)
			{
				if(id == BuildingID::DEFAULT)
					continue;

				const CBuilding * building = getTown()->buildings.at(id);

				if(building->mode == CBuilding::BUILD_AUTO)
					continue;

				if(id == BuildingID::FORT)
					hasFort = true;

				buildingsLIC.all.insert(id.getNum());
				customBuildings = true;
			}

			if(customBuildings)
				handler.serializeLIC("buildings", buildingsLIC);
			else
				handler.serializeBool("hasFort",hasFort);
		}
		else
		{
			handler.serializeLIC("buildings", buildingsLIC);

			addBuilding(BuildingID::VILLAGE_HALL);

			if(buildingsLIC.none.empty() && buildingsLIC.all.empty())
			{
				addBuilding(BuildingID::DEFAULT);

				bool hasFort = false;
				handler.serializeBool("hasFort",hasFort);
				if(hasFort)
					addBuilding(BuildingID::FORT);
			}
			else
			{
				for(const si32 item : buildingsLIC.none)
					forbiddenBuildings.insert(BuildingID(item));
				for(const si32 item : buildingsLIC.all)
					addBuilding(BuildingID(item));
			}
		}
	}

	{
		handler.serializeIdArray( "possibleSpells", possibleSpells);
		handler.serializeIdArray( "obligatorySpells", obligatorySpells);
	}

	{
		auto eventsHandler = handler.enterArray("events");
		eventsHandler.syncSize(events, JsonNode::JsonType::DATA_VECTOR);
		eventsHandler.serializeStruct(events);
	}
}

FactionID CGTownInstance::getFaction() const
{
	return  FactionID(subID.getNum());
}

TerrainId CGTownInstance::getNativeTerrain() const
{
	return town->faction->getNativeTerrain();
}

ArtifactID CGTownInstance::getWarMachineInBuilding(BuildingID building) const
{
	if (builtBuildings.count(building) == 0)
		return ArtifactID::NONE;

	if (building == BuildingID::BLACKSMITH && town->warMachineDeprecated.hasValue())
		return town->warMachineDeprecated.toCreature()->warMachine;

	return town->buildings.at(building)->warMachine;
}

bool CGTownInstance::isWarMachineAvailable(ArtifactID warMachine) const
{
	for (auto const & buildingID : builtBuildings)
		if (town->buildings.at(buildingID)->warMachine == warMachine)
			return true;

	if (builtBuildings.count(BuildingID::BLACKSMITH) &&
	   town->warMachineDeprecated.hasValue() &&
	   town->warMachineDeprecated.toCreature()->warMachine == warMachine)
		return true;

	return false;
}

GrowthInfo::Entry::Entry(const std::string &format, int _count)
	: count(_count)
{
	MetaString formatter;
	formatter.appendRawString(format);
	formatter.replacePositiveNumber(count);

	description = formatter.toString();
}

GrowthInfo::Entry::Entry(int subID, const BuildingID & building, int _count): count(_count)
{
	MetaString formatter;
	formatter.appendRawString("%s %+d");
	formatter.replaceRawString((*VLC->townh)[subID]->town->buildings.at(building)->getNameTranslated());
	formatter.replacePositiveNumber(count);

	description = formatter.toString();
}

GrowthInfo::Entry::Entry(int _count, std::string fullDescription):
	count(_count),
	description(std::move(fullDescription))
{
}

CTownAndVisitingHero::CTownAndVisitingHero()
{
	setNodeType(TOWN_AND_VISITOR);
}

int GrowthInfo::totalGrowth() const
{
	int ret = 0;
	for(const Entry &entry : entries)
		ret += entry.count;

	// always round up income - we don't want buildings to always produce zero if handicap in use
	return vstd::divideAndCeil(ret * handicapPercentage, 100);
}

void CGTownInstance::fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const
{
	for(const CGTownInstance::TCreaturesSet::value_type & dwelling : creatures)
	{
		if (vstd::contains(dwelling.second, stack.type->getId())) //Dwelling with our creature
		{
			for(const auto & upgrID : dwelling.second)
			{
				if(vstd::contains(stack.type->upgrades, upgrID)) //possible upgrade
				{
					info.newID.push_back(upgrID);
					info.cost.push_back(upgrID.toCreature()->getFullRecruitCost() - stack.type->getFullRecruitCost());
				}
			}
		}
	}
}

void CGTownInstance::postDeserialize()
{
	setNodeType(CBonusSystemNode::TOWN);
	for(auto & building : rewardableBuildings)
		building.second->town = this;
}

std::map<BuildingID, TownRewardableBuildingInstance*> CGTownInstance::convertOldBuildings(std::vector<TownRewardableBuildingInstance*> oldVector)
{
	std::map<BuildingID, TownRewardableBuildingInstance*> result;

	for(auto & building : oldVector)
	{
		result[building->getBuildingType()] = new TownRewardableBuildingInstance(*building);
		delete building;
	}

	return result;
}

VCMI_LIB_NAMESPACE_END
