/*
* PriorityEvaluator.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include <limits>

#include "Nullkiller.h"
#include "../../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjectConstructors/CBankInstanceConstructor.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/StartInfo.h"
#include "../../../CCallback.h"
#include "../../../lib/filesystem/Filesystem.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/BuildThis.h"
#include "../Goals/StayAtTown.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "../Goals/DismissHero.h"
#include "../Markers/UnlockCluster.h"
#include "../Markers/HeroExchange.h"
#include "../Markers/ArmyUpgrade.h"
#include "../Markers/DefendTown.h"

namespace NKAI
{

#define MIN_AI_STRENGTH (0.5f) //lower when combat AI gets smarter
#define UNGUARDED_OBJECT (100.0f) //we consider unguarded objects 100 times weaker than us
const float MIN_CRITICAL_VALUE = 2.0f;

EvaluationContext::EvaluationContext(const Nullkiller * ai)
	: movementCost(0.0),
	manaCost(0),
	danger(0),
	closestWayRatio(1),
	movementCostByRole(),
	skillReward(0),
	goldReward(0),
	goldCost(0),
	armyReward(0),
	armyLossPersentage(0),
	heroRole(HeroRole::SCOUT),
	turn(0),
	strategicalValue(0),
	evaluator(ai),
	enemyHeroDangerRatio(0),
	armyGrowth(0)
{
}

void EvaluationContext::addNonCriticalStrategicalValue(float value)
{
	vstd::amax(strategicalValue, std::min(value, MIN_CRITICAL_VALUE));
}

PriorityEvaluator::~PriorityEvaluator()
{
	delete engine;
}

void PriorityEvaluator::initVisitTile()
{
	auto file = CResourceHandler::get()->load(ResourcePath("config/ai/nkai/object-priorities.txt"))->readAll();
	std::string str = std::string((char *)file.first.get(), file.second);
	engine = fl::FllImporter().fromString(str);
	armyLossPersentageVariable = engine->getInputVariable("armyLoss");
	armyGrowthVariable = engine->getInputVariable("armyGrowth");
	heroRoleVariable = engine->getInputVariable("heroRole");
	dangerVariable = engine->getInputVariable("danger");
	turnVariable = engine->getInputVariable("turn");
	mainTurnDistanceVariable = engine->getInputVariable("mainTurnDistance");
	scoutTurnDistanceVariable = engine->getInputVariable("scoutTurnDistance");
	goldRewardVariable = engine->getInputVariable("goldReward");
	armyRewardVariable = engine->getInputVariable("armyReward");
	skillRewardVariable = engine->getInputVariable("skillReward");
	rewardTypeVariable = engine->getInputVariable("rewardType");
	closestHeroRatioVariable = engine->getInputVariable("closestHeroRatio");
	strategicalValueVariable = engine->getInputVariable("strategicalValue");
	goldPressureVariable = engine->getInputVariable("goldPressure");
	goldCostVariable = engine->getInputVariable("goldCost");
	fearVariable = engine->getInputVariable("fear");
	value = engine->getOutputVariable("Value");
}

bool isAnotherAi(const CGObjectInstance * obj, const CPlayerSpecificInfoCallback & cb)
{
	return obj->getOwner().isValidPlayer()
		&& cb.getStartInfo()->getIthPlayersSettings(obj->getOwner()).isControlledByAI();
}

int32_t estimateTownIncome(CCallback * cb, const CGObjectInstance * target, const CGHeroInstance * hero)
{
	auto relations = cb->getPlayerRelations(hero->tempOwner, target->tempOwner);

	if(relations != PlayerRelations::ENEMIES)
		return 0; // if we already own it, no additional reward will be received by just visiting it

	auto booster = isAnotherAi(target, *cb) ? 1 : 2;

	auto town = cb->getTown(target->id);
	auto fortLevel = town->fortLevel();

	if(town->hasCapitol())
		return booster * 2000;

	// probably well developed town will have city hall
	if(fortLevel == CGTownInstance::CASTLE) return booster * 750;
	
	return booster * (town->hasFort() && town->tempOwner != PlayerColor::NEUTRAL  ? booster * 500 : 250);
}

TResources getCreatureBankResources(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	//Fixme: unused variable hero

	auto objectInfo = target->getObjectHandler()->getObjectInfo(target->appearance);
	CBankInfo * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());
	auto resources = bankInfo->getPossibleResourcesReward();
	TResources result = TResources();
	int sum = 0;

	for(auto & reward : resources)
	{
		result += reward.data * reward.chance;
		sum += reward.chance;
	}

	return sum > 1 ? result / sum : result;
}

int32_t getResourcesGoldReward(const TResources & res)
{
	int32_t result = 0;

	for(EGameResID r = EGameResID(0); r < EGameResID::COUNT; r.advance(1))
	{
		if(res[r] > 0)
		{
			result += r == EGameResID::GOLD ? res[r] : res[r] * 100;
		}
	}

	return result;
}

uint64_t getCreatureBankArmyReward(const CGObjectInstance * target, const CGHeroInstance * hero)
{
	auto objectInfo = target->getObjectHandler()->getObjectInfo(target->appearance);
	CBankInfo * bankInfo = dynamic_cast<CBankInfo *>(objectInfo.get());
	auto creatures = bankInfo->getPossibleCreaturesReward(target->cb);
	uint64_t result = 0;

	const auto& slots = hero->Slots();
	ui64 weakestStackPower = 0;
	int duplicatingSlots = getDuplicatingSlots(hero);

	if (slots.size() >= GameConstants::ARMY_SIZE)
	{
		//No free slot, we might discard our weakest stack
		weakestStackPower = std::numeric_limits<ui64>().max();
		for (const auto & stack : slots)
		{
			vstd::amin(weakestStackPower, stack.second->getPower());
		}
	}

	for (auto c : creatures)
	{
		//Only if hero has slot for this creature in the army
		auto ccre = dynamic_cast<const CCreature*>(c.data.type);
		if (hero->getSlotFor(ccre).validSlot() || duplicatingSlots > 0)
		{
			result += (c.data.type->getAIValue() * c.data.count) * c.chance;
		}
		/*else
		{
			//we will need to discard the weakest stack
			result += (c.data.type->getAIValue() * c.data.count - weakestStackPower) * c.chance;
		}*/
	}
	result /= 100; //divide by total chance

	return result;
}

uint64_t getDwellingArmyValue(CCallback * cb, const CGObjectInstance * target, bool checkGold)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	uint64_t score = 0;
	
	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.first && creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();
			auto creaturesAreFree = creature->getLevel() == 1;
			if(!creaturesAreFree && checkGold && !cb->getResourceAmount().canAfford(creature->getFullRecruitCost() * creLevel.first))
				continue;

			score += creature->getAIValue() * creLevel.first;
		}
	}

	return score;
}

uint64_t getDwellingArmyGrowth(CCallback * cb, const CGObjectInstance * target, PlayerColor myColor)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	uint64_t score = 0;

	if(dwelling->getOwner() == myColor)
		return 0;

	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();

			score += creature->getAIValue() * creature->getGrowth();
		}
	}

	return score;
}

int getDwellingArmyCost(const CGObjectInstance * target)
{
	auto dwelling = dynamic_cast<const CGDwelling *>(target);
	int cost = 0;

	for(auto & creLevel : dwelling->creatures)
	{
		if(creLevel.first && creLevel.second.size())
		{
			auto creature = creLevel.second.back().toCreature();
			auto creaturesAreFree = creature->getLevel() == 1;
			if(!creaturesAreFree)
				cost += creature->getRecruitCost(EGameResID::GOLD) * creLevel.first;
		}
	}

	return cost;
}

static uint64_t evaluateArtifactArmyValue(const CArtifact * art)
{
	if(art->getId() == ArtifactID::SPELL_SCROLL)
		return 1500;

	auto statsValue =
		10 * art->valOfBonuses(BonusType::MOVEMENT, BonusCustomSubtype::heroMovementLand)
		+ 1200 * art->valOfBonuses(BonusType::STACKS_SPEED)
		+ 700 * art->valOfBonuses(BonusType::MORALE)
		+ 700 * art->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK))
		+ 700 * art->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE))
		+ 700 * art->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::KNOWLEDGE))
		+ 700 * art->valOfBonuses(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::SPELL_POWER))
		+ 500 * art->valOfBonuses(BonusType::LUCK);

	auto classValue = 0;

	switch(art->aClass)
	{
	case CArtifact::EartClass::ART_MINOR:
		classValue = 1000;
		break;

	case CArtifact::EartClass::ART_MAJOR:
		classValue = 3000;
		break;
	case CArtifact::EartClass::ART_RELIC:
	case CArtifact::EartClass::ART_SPECIAL:
		classValue = 8000;
		break;
	}

	return statsValue > classValue ? statsValue : classValue;
}

uint64_t RewardEvaluator::getArmyReward(
	const CGObjectInstance * target,
	const CGHeroInstance * hero,
	const CCreatureSet * army,
	bool checkGold) const
{
	const float enemyArmyEliminationRewardRatio = 0.5f;

	auto relations = ai->cb->getPlayerRelations(target->tempOwner, ai->playerID);

	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::HILL_FORT:
		return ai->armyManager->calculateCreaturesUpgrade(army, target, ai->cb->getResourceAmount()).upgradeValue;
	case Obj::CREATURE_BANK:
		return getCreatureBankArmyReward(target, hero);
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR2:
	case Obj::CREATURE_GENERATOR3:
	case Obj::CREATURE_GENERATOR4:
		return getDwellingArmyValue(ai->cb.get(), target, checkGold);
	case Obj::CRYPT:
	case Obj::SHIPWRECK:
	case Obj::SHIPWRECK_SURVIVOR:
	case Obj::WARRIORS_TOMB:
		return 1000;
	case Obj::ARTIFACT:
		return evaluateArtifactArmyValue(dynamic_cast<const CGArtifact *>(target)->storedArtifact->artType);
	case Obj::DRAGON_UTOPIA:
		return 10000;
	case Obj::HERO:
		return  relations == PlayerRelations::ENEMIES
			? enemyArmyEliminationRewardRatio * dynamic_cast<const CGHeroInstance *>(target)->getArmyStrength()
			: 0;
	case Obj::PANDORAS_BOX:
		return 5000;
	case Obj::MAGIC_WELL:
	case Obj::MAGIC_SPRING:
		return getManaRecoveryArmyReward(hero);
	default:
		break;
	}

	auto rewardable = dynamic_cast<const Rewardable::Interface *>(target);

	if(rewardable)
	{
		auto totalValue = 0;
		
		for(int index : rewardable->getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT))
		{
			auto & info = rewardable->configuration.info[index];

			auto rewardValue = 0;

			if(!info.reward.artifacts.empty())
			{
				for(auto artID : info.reward.artifacts)
				{
					const CArtifact * art = dynamic_cast<const CArtifact *>(VLC->artifacts()->getById(artID));

					rewardValue += evaluateArtifactArmyValue(art);
				}
			}

			if(!info.reward.creatures.empty())
			{
				for(auto stackInfo : info.reward.creatures)
				{
					rewardValue += stackInfo.getType()->getAIValue() * stackInfo.getCount();
				}
			}

			totalValue += rewardValue > 0 ? rewardValue / (info.reward.artifacts.size() + info.reward.creatures.size()) : 0;
		}

		return totalValue;
	}

	return 0;
}

uint64_t RewardEvaluator::getArmyGrowth(
	const CGObjectInstance * target,
	const CGHeroInstance * hero,
	const CCreatureSet * army) const
{
	if(!target)
		return 0;

	auto relations = ai->cb->getPlayerRelations(target->tempOwner, hero->tempOwner);

	if(relations != PlayerRelations::ENEMIES)
		return 0;

	switch(target->ID)
	{
	case Obj::TOWN:
	{
		auto town = dynamic_cast<const CGTownInstance *>(target);
		auto fortLevel = town->fortLevel();
		auto neutral = !town->getOwner().isValidPlayer();
		auto booster = isAnotherAi(town, *ai->cb) ||  neutral ? 1 : 2;

		if(fortLevel < CGTownInstance::CITADEL)
			return town->hasFort() ? booster * 500 : 0;
		else
			return booster * (fortLevel == CGTownInstance::CASTLE ? 5000 : 2000);
	}

	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR2:
	case Obj::CREATURE_GENERATOR3:
	case Obj::CREATURE_GENERATOR4:
		return getDwellingArmyGrowth(ai->cb.get(), target, hero->getOwner());
	case Obj::ARTIFACT:
		// it is not supported now because hero will not sit in town on 7th day but later parts of legion may be counted as army growth as well.
		return 0;
	default:
		return 0;
	}
}

int RewardEvaluator::getGoldCost(const CGObjectInstance * target, const CGHeroInstance * hero, const CCreatureSet * army) const
{
	if(!target)
		return 0;
	
	if(auto * m = dynamic_cast<const IMarket *>(target))
	{
		if(m->allowsTrade(EMarketMode::RESOURCE_SKILL))
			return 2000;
	}

	switch(target->ID)
	{
	case Obj::HILL_FORT:
		return ai->armyManager->calculateCreaturesUpgrade(army, target, ai->cb->getResourceAmount()).upgradeCost[EGameResID::GOLD];
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
		return 1000;
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR2:
	case Obj::CREATURE_GENERATOR3:
	case Obj::CREATURE_GENERATOR4:
		return getDwellingArmyCost(target);
	default:
		return 0;
	}
}

float RewardEvaluator::getEnemyHeroStrategicalValue(const CGHeroInstance * enemy) const
{
	auto objectsUnderTreat = ai->dangerHitMap->getOneTurnAccessibleObjects(enemy);
	float objectValue = 0;

	for(auto obj : objectsUnderTreat)
	{
		vstd::amax(objectValue, getStrategicalValue(obj));
	}

	/*
	  1. If an enemy hero can attack nearby object, it's not useful to capture the object on our own.
	  Killing the hero is almost as important (0.9) as capturing the object itself.

	  2. The formula quickly approaches 1.0 as hero level increases,
	  but higher level always means higher value and the minimal value for level 1 hero is 0.5
	*/
	return std::min(1.5f, objectValue * 0.9f + (1.5f - (1.5f / (1 + enemy->level))));
}

float RewardEvaluator::getResourceRequirementStrength(int resType) const
{
	TResources requiredResources = ai->buildAnalyzer->getResourcesRequiredNow();
	TResources dailyIncome = ai->buildAnalyzer->getDailyIncome();

	if(requiredResources[resType] == 0)
		return 0;

	if(dailyIncome[resType] == 0)
		return 1.0f;

	float ratio = (float)requiredResources[resType] / dailyIncome[resType] / 2;

	return std::min(ratio, 1.0f);
}

float RewardEvaluator::getTotalResourceRequirementStrength(int resType) const
{
	TResources requiredResources = ai->buildAnalyzer->getTotalResourcesRequired();
	TResources dailyIncome = ai->buildAnalyzer->getDailyIncome();

	if(requiredResources[resType] == 0)
		return 0;

	float ratio = dailyIncome[resType] == 0
		? (float)requiredResources[resType] / 10.0f
		: (float)requiredResources[resType] / dailyIncome[resType] / 20.0f;

	return std::min(ratio, 2.0f);
}

uint64_t RewardEvaluator::townArmyGrowth(const CGTownInstance * town) const
{
	uint64_t result = 0;

	for(auto creatureInfo : town->creatures)
	{
		if(creatureInfo.second.empty())
			continue;

		auto creature = creatureInfo.second.back().toCreature();
		result += creature->getAIValue() * town->getGrowthInfo(creature->getLevel() - 1).totalGrowth();
	}

	return result;
}

uint64_t RewardEvaluator::getManaRecoveryArmyReward(const CGHeroInstance * hero) const
{
	return ai->heroManager->getMagicStrength(hero) * 10000 * (1.0f - std::sqrt(static_cast<float>(hero->mana) / hero->manaLimit()));
}

float RewardEvaluator::getResourceRequirementStrength(const TResources & res) const
{
	float sum = 0.0f;

	for(TResources::nziterator it(res); it.valid(); it++)
	{
		//Evaluate resources used for construction. Gold is evaluated separately.
		if(it->resType != EGameResID::GOLD)
		{
			sum += 0.1f * it->resVal * getResourceRequirementStrength(it->resType)
				+ 0.05f * it->resVal * getTotalResourceRequirementStrength(it->resType);
		}
	}

	return sum;
}

float RewardEvaluator::getStrategicalValue(const CGObjectInstance * target, const CGHeroInstance * hero) const
{
	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::MINE:
	{
		auto mine = dynamic_cast<const CGMine *>(target);
		return mine->producedResource == EGameResID::GOLD
			? 0.5f 
			: 0.4f * getTotalResourceRequirementStrength(mine->producedResource) + 0.1f * getResourceRequirementStrength(mine->producedResource);
	}

	case Obj::RESOURCE:
	{
		auto resource = dynamic_cast<const CGResource *>(target);
		TResources res;
		res[resource->resourceID()] = resource->amount;
		
		return getResourceRequirementStrength(res);
	}

	case Obj::CREATURE_BANK:
	{
		auto resourceReward = getCreatureBankResources(target, nullptr);
		
		return getResourceRequirementStrength(resourceReward);
	}

	case Obj::TOWN:
	{
		if(ai->buildAnalyzer->getDevelopmentInfo().empty())
			return 10.0f;

		auto town = dynamic_cast<const CGTownInstance *>(target);

		if(town->getOwner() == ai->playerID)
		{
			auto armyIncome = townArmyGrowth(town);
			auto dailyIncome = town->dailyIncome()[EGameResID::GOLD];

			return std::min(1.0f, std::sqrt(armyIncome / 40000.0f)) + std::min(0.3f, dailyIncome / 10000.0f);
		}

		auto fortLevel = town->fortLevel();
		auto booster = isAnotherAi(town, *ai->cb) ? 0.4f : 1.0f;

		if(town->hasCapitol())
			return booster * 1.5;

		if(fortLevel < CGTownInstance::CITADEL)
			return booster * (town->hasFort() ? 1.0 : 0.8);
		else
			return booster * (fortLevel == CGTownInstance::CASTLE ? 1.4 : 1.2);
	}

	case Obj::HERO:
		return ai->cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? getEnemyHeroStrategicalValue(dynamic_cast<const CGHeroInstance *>(target))
			: 0;

	case Obj::KEYMASTER:
		return 0.6f;

	default:
		break;
	}

	auto rewardable = dynamic_cast<const Rewardable::Interface *>(target);

	if(rewardable && hero)
	{
		auto resourceReward = 0.0f;

		for(int index : rewardable->getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT))
		{
			resourceReward += getResourceRequirementStrength(rewardable->configuration.info[index].reward.resources);
		}

		return resourceReward;
	}

	return 0;
}

float RewardEvaluator::evaluateWitchHutSkillScore(const CGObjectInstance * hut, const CGHeroInstance * hero, HeroRole role) const
{
	auto rewardable = dynamic_cast<const CRewardableObject *>(hut);
	assert(rewardable);

	auto skill = SecondarySkill(*rewardable->configuration.getVariable("secondarySkill", "gainedSkill"));

	if(!hut->wasVisited(hero->tempOwner))
		return role == HeroRole::SCOUT ? 2 : 0;

	if(hero->getSecSkillLevel(skill) != MasteryLevel::NONE
		|| hero->secSkills.size() >= GameConstants::SKILL_PER_HERO)
		return 0;

	auto score = ai->heroManager->evaluateSecSkill(skill, hero);

	return score >= 2 ? (role == HeroRole::MAIN ? 10 : 4) : score;
}

float RewardEvaluator::getSkillReward(const CGObjectInstance * target, const CGHeroInstance * hero, HeroRole role) const
{
	const float enemyHeroEliminationSkillRewardRatio = 0.5f;

	if(!target)
		return 0;

	switch(target->ID)
	{
	case Obj::STAR_AXIS:
	case Obj::SCHOLAR:
	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
	case Obj::GARDEN_OF_REVELATION:
	case Obj::MARLETTO_TOWER:
	case Obj::MERCENARY_CAMP:
	case Obj::TREE_OF_KNOWLEDGE:
		return 1;
	case Obj::LEARNING_STONE:
		return 1.0f / std::sqrt(hero->level);
	case Obj::ARENA:
		return 2;
	case Obj::SHRINE_OF_MAGIC_INCANTATION:
		return 0.25f;
	case Obj::SHRINE_OF_MAGIC_GESTURE:
		return 1.0f;
	case Obj::SHRINE_OF_MAGIC_THOUGHT:
		return 2.0f;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		return 8;
	case Obj::WITCH_HUT:
		return evaluateWitchHutSkillScore(target, hero, role);
	case Obj::PANDORAS_BOX:
		//Can contains experience, spells, or skills (only on custom maps)
		return 2.5f;
	case Obj::PYRAMID:
		return 6.0f;
	case Obj::HERO:
		return ai->cb->getPlayerRelations(target->tempOwner, ai->playerID) == PlayerRelations::ENEMIES
			? enemyHeroEliminationSkillRewardRatio * dynamic_cast<const CGHeroInstance *>(target)->level
			: 0;

	default:
		break;
	}

	auto rewardable = dynamic_cast<const Rewardable::Interface *>(target);

	if(rewardable)
	{
		auto totalValue = 0.0f;

		for(int index : rewardable->getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT))
		{
			auto & info = rewardable->configuration.info[index];

			auto rewardValue = 0.0f;

			if(!info.reward.spells.empty())
			{
				for(auto spellID : info.reward.spells)
				{
					const spells::Spell * spell = VLC->spells()->getById(spellID);
						
					if(hero->canLearnSpell(spell) && !hero->spellbookContainsSpell(spellID))
					{
						rewardValue += std::sqrt(spell->getLevel()) / 4.0f;
					}
				}

				totalValue += rewardValue / info.reward.spells.size();
			}

			if(!info.reward.primary.empty())
			{
				for(auto value : info.reward.primary)
				{
					totalValue += value;
				}
			}
		}

		return totalValue;
	}

	return 0;
}

const HitMapInfo & RewardEvaluator::getEnemyHeroDanger(const int3 & tile, uint8_t turn) const
{
	auto & treatNode = ai->dangerHitMap->getTileThreat(tile);

	if(treatNode.maximumDanger.danger == 0)
		return HitMapInfo::NoThreat;

	if(treatNode.maximumDanger.turn <= turn)
		return treatNode.maximumDanger;

	return treatNode.fastestDanger.turn <= turn ? treatNode.fastestDanger : HitMapInfo::NoThreat;
}

int32_t getArmyCost(const CArmedInstance * army)
{
	int32_t value = 0;

	for(auto stack : army->Slots())
	{
		value += stack.second->getCreatureID().toCreature()->getRecruitCost(EGameResID::GOLD) * stack.second->count;
	}

	return value;
}

int32_t RewardEvaluator::getGoldReward(const CGObjectInstance * target, const CGHeroInstance * hero) const
{
	if(!target)
		return 0;

	auto relations = ai->cb->getPlayerRelations(target->tempOwner, hero->tempOwner);

	const int dailyIncomeMultiplier = 5;
	const float enemyArmyEliminationGoldRewardRatio = 0.2f;
	const int32_t heroEliminationBonus = GameConstants::HERO_GOLD_COST / 2;

	switch(target->ID)
	{
	case Obj::RESOURCE:
	{
		auto * res = dynamic_cast<const CGResource*>(target);
		return res && res->resourceID() == GameResID::GOLD ? 600 : 100;
	}
	case Obj::TREASURE_CHEST:
		return 1500;
	case Obj::WATER_WHEEL:
		return 1000;
	case Obj::TOWN:
		return dailyIncomeMultiplier * estimateTownIncome(ai->cb.get(), target, hero);
	case Obj::MINE:
	case Obj::ABANDONED_MINE:
	{
		auto * mine = dynamic_cast<const CGMine*>(target);
		return dailyIncomeMultiplier * (mine->producedResource == GameResID::GOLD ? 1000 : 75);
	}
	case Obj::MYSTICAL_GARDEN:
	case Obj::WINDMILL:
		return 100;
	case Obj::CAMPFIRE:
		return 800;
	case Obj::WAGON:
		return 100;
	case Obj::CREATURE_BANK:
		return getResourcesGoldReward(getCreatureBankResources(target, hero));
	case Obj::CRYPT:
	case Obj::DERELICT_SHIP:
		return 3000;
	case Obj::DRAGON_UTOPIA:
		return 10000;
	case Obj::SEA_CHEST:
		return 1500;
	case Obj::PANDORAS_BOX:
		return 2500;
	case Obj::PRISON:
		//Objectively saves us 2500 to hire hero
		return GameConstants::HERO_GOLD_COST;
	case Obj::HERO:
		return relations == PlayerRelations::ENEMIES
			? heroEliminationBonus + enemyArmyEliminationGoldRewardRatio * getArmyCost(dynamic_cast<const CGHeroInstance *>(target))
			: 0;
	default:
		break;
	}

	auto rewardable = dynamic_cast<const Rewardable::Interface *>(target);

	if(rewardable)
	{
		auto goldReward = 0;

		for(int index : rewardable->getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT))
		{
			auto & info = rewardable->configuration.info[index];

			goldReward += getResourcesGoldReward(info.reward.resources);
		}

		return goldReward;
	}

	return 0;
}

class HeroExchangeEvaluator : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::HERO_EXCHANGE)
			return;

		Goals::HeroExchange & heroExchange = dynamic_cast<Goals::HeroExchange &>(*task);

		uint64_t armyStrength = heroExchange.getReinforcementArmyStrength(evaluationContext.evaluator.ai);

		evaluationContext.addNonCriticalStrategicalValue(2.0f * armyStrength / (float)heroExchange.hero->getArmyStrength());
		evaluationContext.heroRole = evaluationContext.evaluator.ai->heroManager->getHeroRole(heroExchange.hero);
	}
};

class ArmyUpgradeEvaluator : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::ARMY_UPGRADE)
			return;

		Goals::ArmyUpgrade & armyUpgrade = dynamic_cast<Goals::ArmyUpgrade &>(*task);

		uint64_t upgradeValue = armyUpgrade.getUpgradeValue();

		evaluationContext.armyReward += upgradeValue;
		evaluationContext.addNonCriticalStrategicalValue(upgradeValue / (float)armyUpgrade.hero->getArmyStrength());
	}
};

class ExplorePointEvaluator : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::EXPLORATION_POINT)
			return;

		int tilesDiscovered = task->value;

		evaluationContext.addNonCriticalStrategicalValue(0.03f * tilesDiscovered);
	}
};

class StayAtTownManaRecoveryEvaluator : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::STAY_AT_TOWN)
			return;

		Goals::StayAtTown & stayAtTown = dynamic_cast<Goals::StayAtTown &>(*task);

		evaluationContext.armyReward += evaluationContext.evaluator.getManaRecoveryArmyReward(stayAtTown.getHero());
		evaluationContext.movementCostByRole[evaluationContext.heroRole] += stayAtTown.getMovementWasted();
		evaluationContext.movementCost += stayAtTown.getMovementWasted();
	}
};

void addTileDanger(EvaluationContext & evaluationContext, const int3 & tile, uint8_t turn, uint64_t ourStrength)
{
	HitMapInfo enemyDanger = evaluationContext.evaluator.getEnemyHeroDanger(tile, turn);

	if(enemyDanger.danger)
	{
		auto dangerRatio = enemyDanger.danger / (double)ourStrength;
		auto enemyHero = evaluationContext.evaluator.ai->cb->getObj(enemyDanger.hero.hid, false);
		bool isAI = enemyHero && isAnotherAi(enemyHero, *evaluationContext.evaluator.ai->cb);

		if(isAI)
		{
			dangerRatio *= 1.5; // lets make AI bit more afraid of other AI.
		}

		vstd::amax(evaluationContext.enemyHeroDangerRatio, dangerRatio);
	}
}

class DefendTownEvaluator : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::DEFEND_TOWN)
			return;

		Goals::DefendTown & defendTown = dynamic_cast<Goals::DefendTown &>(*task);
		const CGTownInstance * town = defendTown.town;
		auto & treat = defendTown.getTreat();

		auto strategicalValue = evaluationContext.evaluator.getStrategicalValue(town);

		float multiplier = 1;

		if(treat.turn < defendTown.getTurn())
			multiplier /= 1 + (defendTown.getTurn() - treat.turn);

		multiplier /= 1.0f + treat.turn / 5.0f;

		if(defendTown.getTurn() > 0 && defendTown.isCounterAttack())
		{
			auto ourSpeed = defendTown.hero->movementPointsLimit(true);
			auto enemySpeed = treat.hero.get(evaluationContext.evaluator.ai->cb.get())->movementPointsLimit(true);

			if(enemySpeed > ourSpeed) multiplier *= 0.7f;
		}

		auto dailyIncome = town->dailyIncome()[EGameResID::GOLD];
		auto armyGrowth = evaluationContext.evaluator.townArmyGrowth(town);

		evaluationContext.armyGrowth += armyGrowth * multiplier;
		evaluationContext.goldReward += dailyIncome * 5 * multiplier;

		if(evaluationContext.evaluator.ai->buildAnalyzer->getDevelopmentInfo().size() == 1)
			vstd::amax(evaluationContext.strategicalValue, 2.5f * multiplier * strategicalValue);
		else
			evaluationContext.addNonCriticalStrategicalValue(1.7f * multiplier * strategicalValue);

		vstd::amax(evaluationContext.danger, defendTown.getTreat().danger);
		addTileDanger(evaluationContext, town->visitablePos(), defendTown.getTurn(), defendTown.getDefenceStrength());
	}
};

class ExecuteHeroChainEvaluationContextBuilder : public IEvaluationContextBuilder
{
private:
	const Nullkiller * ai;

public:
	ExecuteHeroChainEvaluationContextBuilder(const Nullkiller * ai) : ai(ai) {}

	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::EXECUTE_HERO_CHAIN)
			return;

		Goals::ExecuteHeroChain & chain = dynamic_cast<Goals::ExecuteHeroChain &>(*task);
		const AIPath & path = chain.getPath();

		vstd::amax(evaluationContext.danger, path.getTotalDanger());
		evaluationContext.movementCost += path.movementCost();
		evaluationContext.closestWayRatio = chain.closestWayRatio;

		std::map<const CGHeroInstance *, float> costsPerHero;

		for(auto & node : path.nodes)
		{
			vstd::amax(costsPerHero[node.targetHero], node.cost);
		}

		for(auto pair : costsPerHero)
		{
			auto role = evaluationContext.evaluator.ai->heroManager->getHeroRole(pair.first);

			evaluationContext.movementCostByRole[role] += pair.second;
		}

		auto hero = task->hero;
		bool checkGold = evaluationContext.danger == 0;
		auto army = path.heroArmy;

		const CGObjectInstance * target = ai->cb->getObj((ObjectInstanceID)task->objid, false);
		auto heroRole = evaluationContext.evaluator.ai->heroManager->getHeroRole(hero);

		if(heroRole == HeroRole::MAIN)
			evaluationContext.heroRole = heroRole;

		if (target)
		{
			evaluationContext.goldReward += evaluationContext.evaluator.getGoldReward(target, hero);
			evaluationContext.armyReward += evaluationContext.evaluator.getArmyReward(target, hero, army, checkGold);
			evaluationContext.armyGrowth += evaluationContext.evaluator.getArmyGrowth(target, hero, army);
			evaluationContext.skillReward += evaluationContext.evaluator.getSkillReward(target, hero, heroRole);
			evaluationContext.addNonCriticalStrategicalValue(evaluationContext.evaluator.getStrategicalValue(target));
			evaluationContext.goldCost += evaluationContext.evaluator.getGoldCost(target, hero, army);
		}

		vstd::amax(evaluationContext.armyLossPersentage, path.getTotalArmyLoss() / (double)path.getHeroStrength());
		addTileDanger(evaluationContext, path.targetTile(), path.turn(), path.getHeroStrength());
		vstd::amax(evaluationContext.turn, path.turn());
	}
};

class ClusterEvaluationContextBuilder : public IEvaluationContextBuilder
{
public:
	ClusterEvaluationContextBuilder(const Nullkiller * ai) {}

	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::UNLOCK_CLUSTER)
			return;

		Goals::UnlockCluster & clusterGoal = dynamic_cast<Goals::UnlockCluster &>(*task);
		std::shared_ptr<ObjectCluster> cluster = clusterGoal.getCluster();

		auto hero = clusterGoal.hero;
		auto role = evaluationContext.evaluator.ai->heroManager->getHeroRole(hero);

		std::vector<std::pair<ObjectInstanceID, ClusterObjectInfo>> objects(cluster->objects.begin(), cluster->objects.end());

		std::sort(objects.begin(), objects.end(), [](std::pair<ObjectInstanceID, ClusterObjectInfo> o1, std::pair<ObjectInstanceID, ClusterObjectInfo> o2) -> bool
		{
			return o1.second.priority > o2.second.priority;
		});

		int boost = 1;

		for(auto & objInfo : objects)
		{
			auto target = evaluationContext.evaluator.ai->cb->getObj(objInfo.first);
			bool checkGold = objInfo.second.danger == 0;
			auto army = hero;

			evaluationContext.goldReward += evaluationContext.evaluator.getGoldReward(target, hero) / boost;
			evaluationContext.armyReward += evaluationContext.evaluator.getArmyReward(target, hero, army, checkGold) / boost;
			evaluationContext.skillReward += evaluationContext.evaluator.getSkillReward(target, hero, role) / boost;
			evaluationContext.addNonCriticalStrategicalValue(evaluationContext.evaluator.getStrategicalValue(target) / boost);
			evaluationContext.goldCost += evaluationContext.evaluator.getGoldCost(target, hero, army) / boost;
			evaluationContext.movementCostByRole[role] += objInfo.second.movementCost / boost;
			evaluationContext.movementCost += objInfo.second.movementCost / boost;

			vstd::amax(evaluationContext.turn, objInfo.second.turn / boost);

			boost <<= 1;

			if(boost > 8)
				break;
		}
	}
};

class ExchangeSwapTownHeroesContextBuilder : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::EXCHANGE_SWAP_TOWN_HEROES)
			return;

		Goals::ExchangeSwapTownHeroes & swapCommand = dynamic_cast<Goals::ExchangeSwapTownHeroes &>(*task);
		const CGHeroInstance * garrisonHero = swapCommand.getGarrisonHero();

		if(garrisonHero && swapCommand.getLockingReason() == HeroLockedReason::DEFENCE)
		{
			auto defenderRole = evaluationContext.evaluator.ai->heroManager->getHeroRole(garrisonHero);
			auto mpLeft = garrisonHero->movementPointsRemaining() / (float)garrisonHero->movementPointsLimit(true);

			evaluationContext.movementCost += mpLeft;
			evaluationContext.movementCostByRole[defenderRole] += mpLeft;
			evaluationContext.heroRole = defenderRole;
		}
	}
};

class DismissHeroContextBuilder : public IEvaluationContextBuilder
{
private:
	const Nullkiller * ai;

public:
	DismissHeroContextBuilder(const Nullkiller * ai) : ai(ai) {}

	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::DISMISS_HERO)
			return;

		Goals::DismissHero & dismissCommand = dynamic_cast<Goals::DismissHero &>(*task);
		const CGHeroInstance * dismissedHero = dismissCommand.getHero();

		auto role = ai->heroManager->getHeroRole(dismissedHero);
		auto mpLeft = dismissedHero->movementPointsRemaining();
			
		evaluationContext.movementCost += mpLeft;
		evaluationContext.movementCostByRole[role] += mpLeft;
		evaluationContext.goldCost += GameConstants::HERO_GOLD_COST + getArmyCost(dismissedHero);
	}
};

class BuildThisEvaluationContextBuilder : public IEvaluationContextBuilder
{
public:
	void buildEvaluationContext(EvaluationContext & evaluationContext, Goals::TSubgoal task) const override
	{
		if(task->goalType != Goals::BUILD_STRUCTURE)
			return;

		Goals::BuildThis & buildThis = dynamic_cast<Goals::BuildThis &>(*task);
		auto & bi = buildThis.buildingInfo;
		
		evaluationContext.goldReward += 7 * bi.dailyIncome[EGameResID::GOLD] / 2; // 7 day income but half we already have
		evaluationContext.heroRole = HeroRole::MAIN;
		evaluationContext.movementCostByRole[evaluationContext.heroRole] += bi.prerequisitesCount;
		evaluationContext.goldCost += bi.buildCostWithPrerequisites[EGameResID::GOLD];
		evaluationContext.closestWayRatio = 1;

		if(bi.creatureID != CreatureID::NONE)
		{
			evaluationContext.addNonCriticalStrategicalValue(buildThis.townInfo.armyStrength / 50000.0);

			if(bi.baseCreatureID == bi.creatureID)
			{
				evaluationContext.addNonCriticalStrategicalValue((0.5f + 0.1f * bi.creatureLevel) / (float)bi.prerequisitesCount);
				evaluationContext.armyReward += bi.armyStrength;
			}
			else
			{
				auto potentialUpgradeValue = evaluationContext.evaluator.getUpgradeArmyReward(buildThis.town, bi);
				
				evaluationContext.addNonCriticalStrategicalValue(potentialUpgradeValue / 10000.0f / (float)bi.prerequisitesCount);
				evaluationContext.armyReward += potentialUpgradeValue / (float)bi.prerequisitesCount;
			}
		}
		else if(bi.id == BuildingID::CITADEL || bi.id == BuildingID::CASTLE)
		{
			evaluationContext.addNonCriticalStrategicalValue(buildThis.town->creatures.size() * 0.2f);
			evaluationContext.armyReward += buildThis.townInfo.armyStrength / 2;
		}
		else if(bi.id >= BuildingID::MAGES_GUILD_1 && bi.id <= BuildingID::MAGES_GUILD_5)
		{
			evaluationContext.skillReward += 2 * (bi.id - BuildingID::MAGES_GUILD_1);
		}
		
		if(evaluationContext.goldReward)
		{
			auto goldPressure = evaluationContext.evaluator.ai->buildAnalyzer->getGoldPressure();

			evaluationContext.addNonCriticalStrategicalValue(evaluationContext.goldReward * goldPressure / 3500.0f / bi.prerequisitesCount);
		}

		if(bi.notEnoughRes && bi.prerequisitesCount == 1)
		{
			evaluationContext.strategicalValue /= 3;
			evaluationContext.movementCostByRole[evaluationContext.heroRole] += 5;
			evaluationContext.turn += 5;
		}
	}
};

uint64_t RewardEvaluator::getUpgradeArmyReward(const CGTownInstance * town, const BuildingInfo & bi) const
{
	if(ai->buildAnalyzer->hasAnyBuilding(town->getFaction(), bi.id))
		return 0;

	auto creaturesToUpgrade = ai->armyManager->getTotalCreaturesAvailable(bi.baseCreatureID);
	auto upgradedPower = ai->armyManager->evaluateStackPower(bi.creatureID.toCreature(), creaturesToUpgrade.count);

	return upgradedPower - creaturesToUpgrade.power;
}

PriorityEvaluator::PriorityEvaluator(const Nullkiller * ai)
	:ai(ai)
{
	initVisitTile();
	evaluationContextBuilders.push_back(std::make_shared<ExecuteHeroChainEvaluationContextBuilder>(ai));
	evaluationContextBuilders.push_back(std::make_shared<BuildThisEvaluationContextBuilder>());
	evaluationContextBuilders.push_back(std::make_shared<ClusterEvaluationContextBuilder>(ai));
	evaluationContextBuilders.push_back(std::make_shared<HeroExchangeEvaluator>());
	evaluationContextBuilders.push_back(std::make_shared<ArmyUpgradeEvaluator>());
	evaluationContextBuilders.push_back(std::make_shared<DefendTownEvaluator>());
	evaluationContextBuilders.push_back(std::make_shared<ExchangeSwapTownHeroesContextBuilder>());
	evaluationContextBuilders.push_back(std::make_shared<DismissHeroContextBuilder>(ai));
	evaluationContextBuilders.push_back(std::make_shared<StayAtTownManaRecoveryEvaluator>());
	evaluationContextBuilders.push_back(std::make_shared<ExplorePointEvaluator>());
}

EvaluationContext PriorityEvaluator::buildEvaluationContext(Goals::TSubgoal goal) const
{
	Goals::TGoalVec parts;
	EvaluationContext context(ai);

	if(goal->goalType == Goals::COMPOSITION)
	{
		parts = goal->decompose(ai);
	}
	else
	{
		parts.push_back(goal);
	}

	for(auto subgoal : parts)
	{
		context.goldCost += subgoal->goldCost;

		for(auto builder : evaluationContextBuilders)
		{
			builder->buildEvaluationContext(context, subgoal);
		}
	}

	return context;
}

float PriorityEvaluator::evaluate(Goals::TSubgoal task)
{
	auto evaluationContext = buildEvaluationContext(task);

	int rewardType = (evaluationContext.goldReward > 0 ? 1 : 0) 
		+ (evaluationContext.armyReward > 0 ? 1 : 0)
		+ (evaluationContext.skillReward > 0 ? 1 : 0)
		+ (evaluationContext.strategicalValue > 0 ? 1 : 0);

	float goldRewardPerTurn = evaluationContext.goldReward / std::log2f(2 + evaluationContext.movementCost * 10);
	
	double result = 0;

	try
	{
		armyLossPersentageVariable->setValue(evaluationContext.armyLossPersentage);
		heroRoleVariable->setValue(evaluationContext.heroRole);
		mainTurnDistanceVariable->setValue(evaluationContext.movementCostByRole[HeroRole::MAIN]);
		scoutTurnDistanceVariable->setValue(evaluationContext.movementCostByRole[HeroRole::SCOUT]);
		goldRewardVariable->setValue(goldRewardPerTurn);
		armyRewardVariable->setValue(evaluationContext.armyReward);
		armyGrowthVariable->setValue(evaluationContext.armyGrowth);
		skillRewardVariable->setValue(evaluationContext.skillReward);
		dangerVariable->setValue(evaluationContext.danger);
		rewardTypeVariable->setValue(rewardType);
		closestHeroRatioVariable->setValue(evaluationContext.closestWayRatio);
		strategicalValueVariable->setValue(evaluationContext.strategicalValue);
		goldPressureVariable->setValue(ai->buildAnalyzer->getGoldPressure());
		goldCostVariable->setValue(evaluationContext.goldCost / ((float)ai->getFreeResources()[EGameResID::GOLD] + (float)ai->buildAnalyzer->getDailyIncome()[EGameResID::GOLD] + 1.0f));
		turnVariable->setValue(evaluationContext.turn);
		fearVariable->setValue(evaluationContext.enemyHeroDangerRatio);

		engine->process();

		result = value->getValue();
	}
	catch(fl::Exception & fe)
	{
		logAi->error("evaluate VisitTile: %s", fe.getWhat());
	}

#if NKAI_TRACE_LEVEL >= 2
	logAi->trace("Evaluated %s, loss: %f, turn: %d, turns main: %f, scout: %f, gold: %f, cost: %d, army gain: %f, danger: %d, role: %s, strategical value: %f, cwr: %f, fear: %f, result %f",
		task->toString(),
		evaluationContext.armyLossPersentage,
		(int)evaluationContext.turn,
		evaluationContext.movementCostByRole[HeroRole::MAIN],
		evaluationContext.movementCostByRole[HeroRole::SCOUT],
		goldRewardPerTurn,
		evaluationContext.goldCost,
		evaluationContext.armyReward,
		evaluationContext.danger,
		evaluationContext.heroRole == HeroRole::MAIN ? "main" : "scout",
		evaluationContext.strategicalValue,
		evaluationContext.closestWayRatio,
		evaluationContext.enemyHeroDangerRatio,
		result);
#endif

	return result;
}

}
