/*
 * Updaters.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Updaters.h"
#include "Limiters.h"
#include "../json/JsonNode.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../CStack.h"

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<Bonus> IUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	return b;
}

std::string IUpdater::toString() const
{
	return typeid(*this).name();
}

JsonNode IUpdater::toJsonNode() const
{
	return JsonNode();
}

void IUpdater::visitLimiter(AggregateLimiter& limiter)
{
	for (auto& limit : limiter.limiters)
		limit->acceptUpdater(*this);
}

void IUpdater::visitLimiter(CCreatureTypeLimiter& limiter) {}
void IUpdater::visitLimiter(HasAnotherBonusLimiter& limiter) {}
void IUpdater::visitLimiter(CreatureTerrainLimiter& limiter) {}
void IUpdater::visitLimiter(CreatureLevelLimiter& limiter) {}
void IUpdater::visitLimiter(FactionLimiter& limiter) {}
void IUpdater::visitLimiter(CreatureAlignmentLimiter& limiter) {}
void IUpdater::visitLimiter(OppositeSideLimiter& limiter) {}
void IUpdater::visitLimiter(RankRangeLimiter& limiter) {}
void IUpdater::visitLimiter(UnitOnHexLimiter& limiter) {}

GrowsWithLevelUpdater::GrowsWithLevelUpdater(int valPer20, int stepSize) : valPer20(valPer20), stepSize(stepSize)
{
}

std::shared_ptr<Bonus> GrowsWithLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = dynamic_cast<const CGHeroInstance &>(context).level;
		int steps = stepSize ? level / stepSize : level;
		//rounding follows format for HMM3 creature specialty bonus
		int newVal = (valPer20 * steps + 19) / 20;
		//return copy of bonus with updated val
		auto newBonus = std::make_shared<Bonus>(*b);
		newBonus->val = newVal;
		return newBonus;
	}
	return b;
}

std::string GrowsWithLevelUpdater::toString() const
{
	return boost::str(boost::format("GrowsWithLevelUpdater(valPer20=%d, stepSize=%d)") % valPer20 % stepSize);
}

JsonNode GrowsWithLevelUpdater::toJsonNode() const
{
	JsonNode root;

	root["type"].String() = "GROWS_WITH_LEVEL";
	root["parameters"].Vector().emplace_back(valPer20);
	if(stepSize > 1)
		root["parameters"].Vector().emplace_back(stepSize);

	return root;
}

std::shared_ptr<Bonus> TimesHeroLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = dynamic_cast<const CGHeroInstance &>(context).level;
		auto newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level;
		return newBonus;
	}
	return b;
}

std::string TimesHeroLevelUpdater::toString() const
{
	return "TimesHeroLevelUpdater";
}

JsonNode TimesHeroLevelUpdater::toJsonNode() const
{
	return JsonNode("TIMES_HERO_LEVEL");
}

ArmyMovementUpdater::ArmyMovementUpdater():
	base(20),
	divider(3),
	multiplier(10),
	max(700)
{
}

ArmyMovementUpdater::ArmyMovementUpdater(int base, int divider, int multiplier, int max):
	base(base),
	divider(divider),
	multiplier(multiplier),
	max(max)
{
}

std::shared_ptr<Bonus> ArmyMovementUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	return b;
}

std::string ArmyMovementUpdater::toString() const
{
	return "ArmyMovementUpdater";
}

JsonNode ArmyMovementUpdater::toJsonNode() const
{
	JsonNode root;

	root["type"].String() = "ARMY_MOVEMENT";
	root["parameters"].Vector().emplace_back(base);
	root["parameters"].Vector().emplace_back(divider);
	root["parameters"].Vector().emplace_back(multiplier);
	root["parameters"].Vector().emplace_back(max);

	return root;
}
std::shared_ptr<Bonus> TimesStackLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE || context.getNodeType() == CBonusSystemNode::COMMANDER)
	{
		int level = dynamic_cast<const CStackInstance &>(context).getLevel();
		auto newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level;
		return newBonus;
	}
	else if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const auto & stack = dynamic_cast<const CStack &>(context);
		//update if stack doesn't have an instance (summons, war machines)
		if(stack.base == nullptr)
		{
			int level = stack.unitType()->getLevel();
			auto newBonus = std::make_shared<Bonus>(*b);
			newBonus->val *= level;
			return newBonus;
		}
		// If these are not handled here, the final outcome may potentially be incorrect.
		else
		{
			int level = dynamic_cast<const CStackInstance*>(stack.base)->getLevel();
			auto newBonus = std::make_shared<Bonus>(*b);
			newBonus->val *= level;
			return newBonus;
		}
	}
	return b;
}

std::string TimesStackLevelUpdater::toString() const
{
	return "TimesStackLevelUpdater";
}

JsonNode TimesStackLevelUpdater::toJsonNode() const
{
	return JsonNode("TIMES_STACK_LEVEL");
}

std::string OwnerUpdater::toString() const
{
	return "OwnerUpdater";
}

JsonNode OwnerUpdater::toJsonNode() const
{
	return JsonNode("BONUS_OWNER_UPDATER");
}

std::shared_ptr<Bonus> OwnerUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context)
{
	owner = context.getOwner();
	
	if(owner == PlayerColor::UNFLAGGABLE)
		owner = PlayerColor::NEUTRAL;

	std::shared_ptr<Bonus> updated =
		std::make_shared<Bonus>(*b);
	updated->limiter = b->limiter;
	updated->limiter->acceptUpdater(*this);
	return updated;
}

void OwnerUpdater::visitLimiter(OppositeSideLimiter& limiter)
{
	limiter.owner = owner;
}

VCMI_LIB_NAMESPACE_END
