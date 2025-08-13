/*
 * AIUtility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AIUtility.h"
#include "AIGateway.h"
#include "Goals/Goals.h"

#include "../../lib/UnlockGuard.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/bonuses/Limiters.h"
#include "../../lib/bonuses/Propagators.h"

#include <vcmi/CreatureService.h>

namespace NKAI
{

const CGObjectInstance * ObjectIdRef::operator->() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::operator const CGObjectInstance *() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::operator bool() const
{
	return cb->getObj(id, false);
}

ObjectIdRef::ObjectIdRef(ObjectInstanceID _id)
	: id(_id)
{

}

ObjectIdRef::ObjectIdRef(const CGObjectInstance * obj)
	: id(obj->id)
{

}

bool ObjectIdRef::operator<(const ObjectIdRef & rhs) const
{
	return id < rhs.id;
}

HeroPtr::HeroPtr(const CGHeroInstance * H)
{
	if(!H)
	{
		//init from nullptr should equal to default init
		*this = HeroPtr();
		return;
	}

	h = H;
	hid = H->id;
//	infosCount[ai->playerID][hid]++;
}

HeroPtr::HeroPtr()
{
	h = nullptr;
	hid = ObjectInstanceID();
}

HeroPtr::~HeroPtr()
{
//	if(hid >= 0)
//		infosCount[ai->playerID][hid]--;
}

bool HeroPtr::operator<(const HeroPtr & rhs) const
{
	return hid < rhs.hid;
}

std::string HeroPtr::name() const
{
	if (h)
		return h->getNameTextID();
	else
		return "<NO HERO>";
}

const CGHeroInstance * HeroPtr::get(bool doWeExpectNull) const
{
	return get(cb, doWeExpectNull);
}

const CGHeroInstance * HeroPtr::get(const CPlayerSpecificInfoCallback * cb, bool doWeExpectNull) const
{
	//TODO? check if these all assertions every time we get info about hero affect efficiency
	//
	//behave terribly when attempting unauthorized access to hero that is not ours (or was lost)
	assert(doWeExpectNull || h);

	if(h)
	{
		auto obj = cb->getObj(hid);
		//const bool owned = obj && obj->tempOwner == ai->playerID;

		if(doWeExpectNull && !obj)
		{
			return nullptr;
		}
		else
		{
			assert(obj);
			//assert(owned);
		}
	}

	return h;
}

const CGHeroInstance * HeroPtr::operator->() const
{
	return get();
}

bool HeroPtr::validAndSet() const
{
	return get(true);
}

const CGHeroInstance * HeroPtr::operator*() const
{
	return get();
}

bool HeroPtr::operator==(const HeroPtr & rhs) const
{
	return h == rhs.get(true);
}

bool isSafeToVisit(const CGHeroInstance * h, const CCreatureSet * heroArmy, uint64_t dangerStrength, float safeAttackRatio)
{
	const ui64 heroStrength = h->getHeroStrength() * heroArmy->getArmyStrength();

	if(dangerStrength)
	{
		return heroStrength > dangerStrength * safeAttackRatio;
	}

	return true; //there's no danger
}

bool isSafeToVisit(const CGHeroInstance * h, uint64_t dangerStrength, float safeAttackRatio)
{
	return isSafeToVisit(h, h, dangerStrength, safeAttackRatio);
}

bool isObjectRemovable(const CGObjectInstance * obj)
{
	//FIXME: move logic to object property!
	switch (obj->ID)
	{
	case Obj::MONSTER:
	case Obj::RESOURCE:
	case Obj::CAMPFIRE:
	case Obj::TREASURE_CHEST:
	case Obj::ARTIFACT:
	case Obj::BORDERGUARD:
	case Obj::FLOTSAM:
	case Obj::PANDORAS_BOX:
	case Obj::OCEAN_BOTTLE:
	case Obj::SEA_CHEST:
	case Obj::SHIPWRECK_SURVIVOR:
	case Obj::SPELL_SCROLL:
		return true;
		break;
	default:
		return false;
		break;
	}

}

bool canBeEmbarkmentPoint(const TerrainTile * t, bool fromWater)
{
	// TODO: Such information should be provided by pathfinder
	// Tile must be free or with unoccupied boat
	if(!t->blocked())
	{
		return true;
	}
	else if(!fromWater) // do not try to board when in water sector
	{
		if(t->visitableObjects.size() == 1 && cb->getObjInstance(t->topVisitableObj())->ID == Obj::BOAT)
			return true;
	}
	return false;
}

bool isObjectPassable(const Nullkiller * ai, const CGObjectInstance * obj)
{
	return isObjectPassable(obj, ai->playerID, ai->cb->getPlayerRelations(obj->tempOwner, ai->playerID));
}

// Pathfinder internal helper
bool isObjectPassable(const CGObjectInstance * obj, PlayerColor playerColor, PlayerRelations objectRelations)
{
	if((obj->ID == Obj::GARRISON || obj->ID == Obj::GARRISON2)
		&& objectRelations != PlayerRelations::ENEMIES)
		return true;

	if(obj->ID == Obj::BORDER_GATE)
	{
		auto quest = dynamic_cast<const CGKeys *>(obj);

		if(quest->wasMyColorVisited(playerColor))
			return true;
	}

	return false;
}

bool isBlockVisitObj(const int3 & pos)
{
	if(auto obj = cb->getTopObj(pos))
	{
		if(obj->isBlockedVisitable()) //we can't stand on that object
			return true;
	}

	return false;
}

creInfo infoFromDC(const dwellingContent & dc)
{
	creInfo ci;
	ci.count = dc.first;
	ci.creID = dc.second.size() ? dc.second.back() : CreatureID(-1); //should never be accessed
	if (ci.creID != CreatureID::NONE)
	{
		ci.level = ci.creID.toCreature()->getLevel(); //this is creature tier, while tryRealize expects dwelling level. Ignore.
	}
	else
	{
		ci.level = 0;
	}
	return ci;
}

bool compareHeroStrength(const CGHeroInstance * h1, const CGHeroInstance * h2)
{
	return h1->getTotalStrength() < h2->getTotalStrength();
}

bool compareArmyStrength(const CArmedInstance * a1, const CArmedInstance * a2)
{
	return a1->getArmyStrength() < a2->getArmyStrength();
}

double getArtifactBonusRelevance(const CGHeroInstance * hero, const std::shared_ptr<Bonus> & bonus)
{
	if (bonus->propagator && bonus->limiter && bonus->propagator->getPropagatorType() == BonusNodeType::BATTLE_WIDE)
	{
		// assume that this is battle wide / other side propagator+limiter
		// consider it as fully relevant since we don't know about future combat when equipping artifacts
		return 1.0;
	}

	const auto & getArmyRatioAffectedByLimiter = [&]()
	{
		if (!bonus->limiter)
			return 1.0;

		uint64_t totalStrength = 0;
		uint64_t affectedStrength = 0;

		const BonusList stillUndecided;

		for (const auto & slot : hero->Slots())
		{
			const auto allBonuses = slot.second->getAllBonuses(Selector::all);
			BonusLimitationContext context = {*bonus, *slot.second, *allBonuses, stillUndecided};

			uint64_t unitStrength = slot.second->getPower();

			if (bonus->limiter->limit(context) == ILimiter::EDecision::ACCEPT)
				affectedStrength += unitStrength;
			totalStrength += unitStrength;
		}

		if (totalStrength == 0)
			return 0.0;

		return static_cast<double>(affectedStrength) / totalStrength;
	};

	const auto & getArmyPercentageWithBonus = [&](BonusType type)
	{
		uint64_t totalStrength = 0;
		uint64_t affectedStrength = 0;

		for (const auto & slot : hero->Slots())
		{
			uint64_t unitStrength = slot.second->getPower();
			if (slot.second->hasBonusOfType(type))
				affectedStrength += unitStrength;
			totalStrength += unitStrength;
		}

		if (totalStrength == 0)
			return 0.0;

		return static_cast<double>(affectedStrength) / totalStrength;
	};

	const auto & getSpellSchoolKnownSpellsFactor = [&](SpellSchool school)
	{
		uint64_t totalWeight = 0;
		uint64_t knownWeight = 0;

		for (auto spellID : LIBRARY->spellh->getDefaultAllowed())
		{
			auto spell = spellID.toEntity(LIBRARY);
			if (!spell->hasSchool(school))
				continue;

			uint64_t spellLevel = spell->getLevel();
			uint64_t spellWeight = spellLevel * spellLevel;
			if (!hero->spellbookContainsSpell(spellID))
				knownWeight += spellWeight;
			totalWeight += spellWeight;
		}
		if (totalWeight == 0)
			return 0.0;

		return static_cast<double>(knownWeight) / totalWeight;
	};

	const auto & getSpellLevelKnownSpellsFactor = [&](int level)
	{
		uint64_t totalWeight = 0;
		uint64_t knownWeight = 0;

		for (auto spellID : LIBRARY->spellh->getDefaultAllowed())
		{
			auto spell = spellID.toEntity(LIBRARY);
			if (spell->getLevel() != level)
				continue;

			if (!hero->spellbookContainsSpell(spellID))
				knownWeight += 1;
			totalWeight += 1;
		}
		if (totalWeight == 0)
			return 0.0;

		return static_cast<double>(knownWeight) / totalWeight;
	};

	constexpr double notRelevant = 0.0;  // artifact is not useful in current conditions
	constexpr double relevant = 1.0;
	constexpr double veryRelevant = 2.0; // for very situational artifacts, e.g. skill-specific, or army composition-specific

	switch (bonus->type)
	{
		case BonusType::MOVEMENT:
			if (hero->getBoat() && bonus->subtype == BonusCustomSubtype::heroMovementSea)
				return veryRelevant;

			if (!hero->getBoat() && bonus->subtype == BonusCustomSubtype::heroMovementLand)
				return relevant;
			return notRelevant;
		case BonusType::STACKS_SPEED:
		case BonusType::STACK_HEALTH:
			return getArmyRatioAffectedByLimiter();
		case BonusType::MORALE:
			return getArmyRatioAffectedByLimiter() * (1 - getArmyPercentageWithBonus(BonusType::UNDEAD)); // TODO: other unaffected, e.g. Golems
		case BonusType::LUCK:
			return getArmyRatioAffectedByLimiter(); // Do we have luck?
		case BonusType::PRIMARY_SKILL:
			if (bonus->subtype == PrimarySkill::ATTACK || bonus->subtype == PrimarySkill::DEFENSE)
				return getArmyRatioAffectedByLimiter(); // e.g. Vial of Dragonblood - consider only affected unit
			else
				return relevant; // spellpower / knowledge - always relevant
		case BonusType::WATER_WALKING:
		case BonusType::FLYING_MOVEMENT:
			return hero->getBoat() ? notRelevant : relevant; // boat can't fly
		case BonusType::WHIRLPOOL_PROTECTION:
			return hero->getBoat() ? relevant : notRelevant;
		case BonusType::UNDEAD_RAISE_PERCENTAGE:
			return hero->hasBonusOfType(BonusType::IMPROVED_NECROMANCY) ? veryRelevant : notRelevant;
		case BonusType::SPELL_DAMAGE:
		case BonusType::SPELL_DURATION:
			return hero->hasSpellbook() ? relevant : notRelevant;
		case BonusType::PERCENTAGE_DAMAGE_BOOST:
			if (bonus->subtype == BonusCustomSubtype::damageTypeRanged)
				return veryRelevant * getArmyPercentageWithBonus(BonusType::SHOOTER);
			if (bonus->subtype == BonusCustomSubtype::damageTypeMelee)
				return veryRelevant * (1 - getArmyPercentageWithBonus(BonusType::SHOOTER));
			return 0;
		case BonusType::MANA_PERCENTAGE_REGENERATION:
		case BonusType::MANA_REGENERATION:
			return hero->hasSpellbook() ? relevant : notRelevant;
		case BonusType::LEARN_BATTLE_SPELL_CHANCE:
			return hero->hasBonusOfType(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT) ? relevant : notRelevant;
		case BonusType::NO_DISTANCE_PENALTY:
		case BonusType::NO_WALL_PENALTY:
			return getArmyPercentageWithBonus(BonusType::SHOOTER) * veryRelevant;
		case BonusType::SPELLS_OF_SCHOOL:
			if (!hero->hasSpellbook())
				return notRelevant;
			return 1 - getSpellSchoolKnownSpellsFactor(bonus->subtype.as<SpellSchool>());
		case BonusType::SPELLS_OF_LEVEL:
			if (!hero->hasSpellbook())
				return notRelevant;
			return 1 - getSpellLevelKnownSpellsFactor(bonus->subtype.getNum());
// Potential TODO's
//		case BonusType::MAGIC_RESISTANCE:
//		case BonusType::FREE_SHIP_BOARDING:
//		case BonusType::GENERATE_RESOURCE:
//		case BonusType::CREATURE_GROWTH:
//
//		case BonusType::SPELLS_OF_LEVEL:
//		case BonusType::SIGHT_RADIUS:
	}

	return 1.0;
}

int32_t getArtifactBonusScoreImpl(const std::shared_ptr<Bonus> & bonus)
{
	switch (bonus->type)
	{
		case BonusType::MOVEMENT:
			if (bonus->subtype == BonusCustomSubtype::heroMovementLand)
				return bonus->val * 20;
			if (bonus->subtype == BonusCustomSubtype::heroMovementSea)
				return bonus->val * 10;
			return 0;
		case BonusType::STACKS_SPEED:
			return bonus->val * 8000;
		case BonusType::MORALE:
			return bonus->val * 1500;
		case BonusType::LUCK:
			return bonus->val * 1000;
		case BonusType::PRIMARY_SKILL:
			return bonus->val * 1000;
		case BonusType::SURRENDER_DISCOUNT:
			return 0; // irrelevant in gameplay
		case BonusType::WATER_WALKING:
			return 5000;
		case BonusType::FREE_SHIP_BOARDING:
			return 10000;
		case BonusType::WHIRLPOOL_PROTECTION:
			return 5000;
		case BonusType::FLYING_MOVEMENT:
			return 20000;
		case BonusType::UNDEAD_RAISE_PERCENTAGE:
			return bonus->val * 400;
		case BonusType::GENERATE_RESOURCE:
			return bonus->val * LIBRARY->objh->resVals.at(bonus->subtype.as<GameResID>().getNum()) * 10;
		case BonusType::SPELL_DURATION:
			return bonus->val * 200;
		case BonusType::MAGIC_RESISTANCE:
			return bonus->val * 400;
		case BonusType::PERCENTAGE_DAMAGE_BOOST:
			if (bonus->subtype == BonusCustomSubtype::damageTypeRanged)
				return bonus->val * 200;
			if (bonus->subtype == BonusCustomSubtype::damageTypeMelee)
				return bonus->val * 500;
			return 0;
		case BonusType::CREATURE_GROWTH:
			return (1+bonus->subtype.getNum()) * bonus->val * 400;
		case BonusType::MANA_PERCENTAGE_REGENERATION:
			return bonus->val * 150;
		case BonusType::MANA_REGENERATION:
			return bonus->val * 500;
		case BonusType::SPELLS_OF_SCHOOL:
			return 20000;
		case BonusType::SPELLS_OF_LEVEL:
			return bonus->subtype.getNum() * 6000;
		case BonusType::SPELL_DAMAGE:
			return bonus->val * 120;
		case BonusType::SIGHT_RADIUS:
			return bonus->val * 1000;
		case BonusType::LEARN_BATTLE_SPELL_CHANCE:
			return 0; // irrelevant in gameplay
		case BonusType::STACK_HEALTH:
			return bonus->val * 5000;
		case BonusType::NO_DISTANCE_PENALTY:
			return 10000;
		case BonusType::NO_WALL_PENALTY:
			return 5000;
	}
	return 0;
	// Additional bonuses to consider from H3 artifacts:
	// MIND_IMMUNITY
	// BLOCK_MAGIC_ABOVE
	// SPELL_IMMUNITY
	// NEGATE_ALL_NATURAL_IMMUNITIES
	// SPELL_RESISTANCE_AURA
	// SPELL
	// BATTLE_NO_FLEEING
	// BLOCK_ALL_MAGIC
	// NONEVIL_ALIGNMENT_MIX
	// OPENING_BATTLE_SPELL
	// IMPROVED_NECROMANCY
	// HP_REGENERATION
	// CREATURE_GROWTH_PERCENT
	// LEVEL_SPELL_IMMUNITY
	// FREE_SHOOTING
	// FULL_MANA_REGENERATION
}

int32_t getArtifactBonusScore(const std::shared_ptr<Bonus> & bonus)
{
	if (bonus->propagator && bonus->propagator->getPropagatorType() == BonusNodeType::BATTLE_WIDE)
	{
		if (bonus->limiter)
		{
			// assume that this is battle wide / other side propagator+limiter -> invert value
			return -getArtifactBonusScoreImpl(bonus);
		}
		else
		{
			return 0; // TODO? How to consider battle-wide bonuses that affect everyone?
		}
	}
	else
	{
		return getArtifactBonusScoreImpl(bonus);
	}

}

int64_t getPotentialArtifactScore(const CArtifact * type)
{
	int64_t totalScore = 0;

	for (const auto & bonus : type->getExportedBonusList())
		totalScore += getArtifactBonusScore(bonus);

	if (type->hasParts())
	{
		for (const auto & part : type->getConstituents())
		{
			for (const auto & bonus : part->getExportedBonusList())
				totalScore += getArtifactBonusScore(bonus);
		}
	}

	int64_t finalScore = std::max<int64_t>(type->getPrice() / 5, totalScore );

	return finalScore;
}

int64_t getArtifactScoreForHero(const CGHeroInstance * hero, const CArtifactInstance * artifact)
{
	if (artifact->isScroll())
	{
		auto spellID = artifact->getScrollSpellID();
		auto spell = spellID.toEntity(LIBRARY);

		if (hero->getSpellsInSpellbook().count(spellID))
			return 0;
		else
			return spell->getLevel() * 100;
	}

	const CArtifact * type = artifact->getType();
	int64_t totalScore = 0;

	if (type->getId() == ArtifactID::SPELLBOOK)
		return 0;

	for (const auto & bonus : type->getExportedBonusList())
		totalScore += getArtifactBonusRelevance(hero, bonus) * getArtifactBonusScore(bonus);

	if (type->hasParts())
	{
		for (const auto & part : type->getConstituents())
		{
			for (const auto & bonus : part->getExportedBonusList())
				totalScore += getArtifactBonusRelevance(hero, bonus) * getArtifactBonusScore(bonus);
		}
	}

	return totalScore;
}

bool isWeeklyRevisitable(const Nullkiller * ai, const CGObjectInstance * obj)
{
	if(!obj)
		return false;

	//TODO: allow polling of remaining creatures in dwelling
	if(const auto * rewardable = dynamic_cast<const CRewardableObject *>(obj))
		return rewardable->configuration.getResetDuration() == 7;

	if(dynamic_cast<const CGDwelling *>(obj))
		return true;

	switch(obj->ID)
	{
	case Obj::HILL_FORT:
		return true;
	case Obj::BORDER_GATE:
	case Obj::BORDERGUARD:
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID); //FIXME: they could be revisited sooner than in a week
	}
	return false;
}

uint64_t timeElapsed(std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
	auto end = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int getDuplicatingSlots(const CArmedInstance * army)
{
	int duplicatingSlots = 0;

	for(const auto & stack : army->Slots())
	{
		if(stack.second->getCreature() && army->getSlotFor(stack.second->getCreature()) != stack.first)
			duplicatingSlots++;
	}

	return duplicatingSlots;
}

// todo: move to obj manager
bool shouldVisit(const Nullkiller * ai, const CGHeroInstance * h, const CGObjectInstance * obj)
{
	auto relations = ai->cb->getPlayerRelations(obj->tempOwner, h->tempOwner);

	switch(obj->ID)
	{
	case Obj::TOWN:
	case Obj::HERO: //never visit our heroes at random
		return relations == PlayerRelations::ENEMIES; //do not visit our towns at random

	case Obj::BORDER_GATE:
	{
		for(auto q : ai->cb->getMyQuests())
		{
			if(q.obj == obj->id)
			{
				return false; // do not visit guards or gates when wandering
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::BORDERGUARD: //open borderguard if possible
		return (dynamic_cast<const CGKeys *>(obj))->wasMyColorVisited(ai->playerID);
	case Obj::SEER_HUT:
	{
		for(auto q : ai->cb->getMyQuests())
		{
			if(q.obj == obj->id)
			{
				if(q.getQuest(ai->cb.get())->checkQuest(h))
					return true; //we completed the quest
				else
					return false; //we can't complete this quest
			}
		}
		return true; //we don't have this quest yet
	}
	case Obj::CREATURE_GENERATOR1:
	{
		if(relations == PlayerRelations::ENEMIES)
			return true; //flag just in case

		if(relations == PlayerRelations::ALLIES)
			return false;

		const CGDwelling * d = dynamic_cast<const CGDwelling *>(obj);
		auto duplicatingSlotsCount = getDuplicatingSlots(h);

		for(auto level : d->creatures)
		{
			for(auto c : level.second)
			{
				if(level.first
					&& (h->getSlotFor(CreatureID(c)) != SlotID() || duplicatingSlotsCount > 0)
					&& ai->cb->getResourceAmount().canAfford(c.toCreature()->getFullRecruitCost()))
				{
					return true;
				}
			}
		}

		return false;
	}
	case Obj::HILL_FORT:
	{
		for(const auto & slot : h->Slots())
		{
			if(slot.second->getType()->hasUpgrades())
				return true; //TODO: check price?
		}
		return false;
	}
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	case Obj::MONOLITH_ONE_WAY_EXIT:
	case Obj::MONOLITH_TWO_WAY:
	case Obj::WHIRLPOOL:
		return false;
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
	{
		if(ai->getFreeGold() < 1000)
			return false;
		break;
	}
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		if(h->level < 10)
			return false;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
	{
		if(ai->heroManager->getHeroRole(h) == HeroRole::SCOUT)
			return false;

		TResources myRes = ai->getFreeResources();
		if(myRes[EGameResID::GOLD] < 2000 || myRes[EGameResID::GEMS] < 10)
			return false;
		break;
	}
	case Obj::MAGIC_WELL:
		return h->mana < h->manaLimit();
	case Obj::PRISON:
		return !ai->heroManager->heroCapReached();
	case Obj::TAVERN:
	case Obj::EYE_OF_MAGI:
	case Obj::BOAT:
	case Obj::SIGN:
		return false;
	}

	if(obj->wasVisited(h))
		return false;

	auto rewardable = dynamic_cast<const Rewardable::Interface *>(obj);

	if(rewardable && rewardable->getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT).empty())
	{
		return false;
	}

	return true;
}

bool townHasFreeTavern(const CGTownInstance * town)
{
	if(!town->hasBuilt(BuildingID::TAVERN)) return false;
	if(!town->getVisitingHero()) return true;

	bool canMoveVisitingHeroToGarrison = !town->getUpperArmy()->stacksCount();

	return canMoveVisitingHeroToGarrison;
}

uint64_t getHeroArmyStrengthWithCommander(const CGHeroInstance * hero, const CCreatureSet * heroArmy, int fortLevel)
{
	auto armyStrength = heroArmy->getArmyStrength(fortLevel);

	if(hero && hero->getCommander() && hero->getCommander()->alive)
	{
		armyStrength += 100 * hero->getCommander()->level;
	}

	return armyStrength;
}

}
