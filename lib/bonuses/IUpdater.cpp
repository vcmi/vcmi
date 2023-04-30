/*
 * IUpdater.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "IUpdater.h"
#include "ILimiter.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../CStack.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::map<std::string, TUpdaterPtr> bonusUpdaterMap =
{
	{"TIMES_HERO_LEVEL", std::make_shared<TimesHeroLevelUpdater>()},
	{"TIMES_STACK_LEVEL", std::make_shared<TimesStackLevelUpdater>()},
	{"ARMY_MOVEMENT", std::make_shared<ArmyMovementUpdater>()},
	{"BONUS_OWNER_UPDATER", std::make_shared<OwnerUpdater>()}
};

std::shared_ptr<Bonus> IUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	return b;
}

std::string IUpdater::toString() const
{
	return typeid(*this).name();
}

JsonNode IUpdater::toJsonNode() const
{
	return JsonNode(JsonNode::JsonType::DATA_NULL);
}

GrowsWithLevelUpdater::GrowsWithLevelUpdater(int valPer20, int stepSize) : valPer20(valPer20), stepSize(stepSize)
{
}

std::shared_ptr<Bonus> GrowsWithLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = dynamic_cast<const CGHeroInstance &>(context).level;
		int steps = stepSize ? level / stepSize : level;
		//rounding follows format for HMM3 creature specialty bonus
		int newVal = (valPer20 * steps + 19) / 20;
		//return copy of bonus with updated val
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
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
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "GROWS_WITH_LEVEL";
	root["parameters"].Vector().push_back(JsonUtils::intNode(valPer20));
	if(stepSize > 1)
		root["parameters"].Vector().push_back(JsonUtils::intNode(stepSize));

	return root;
}

std::shared_ptr<Bonus> TimesHeroLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = dynamic_cast<const CGHeroInstance &>(context).level;
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
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
	return JsonUtils::stringNode("TIMES_HERO_LEVEL");
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

std::shared_ptr<Bonus> ArmyMovementUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(b->type == Bonus::MOVEMENT && context.getNodeType() == CBonusSystemNode::HERO)
	{
		auto speed = static_cast<const CGHeroInstance &>(context).getLowestCreatureSpeed();
		si32 armySpeed = speed * base / divider;
		auto counted = armySpeed * multiplier;
		auto newBonus = std::make_shared<Bonus>(*b);
		newBonus->source = Bonus::ARMY;
		newBonus->val += vstd::amin(counted, max);
		return newBonus;
	}
	if(b->type != Bonus::MOVEMENT)
		logGlobal->error("ArmyMovementUpdater should only be used for MOVEMENT bonus!");
	return b;
}

std::string ArmyMovementUpdater::toString() const
{
	return "ArmyMovementUpdater";
}

JsonNode ArmyMovementUpdater::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "ARMY_MOVEMENT";
	root["parameters"].Vector().push_back(JsonUtils::intNode(base));
	root["parameters"].Vector().push_back(JsonUtils::intNode(divider));
	root["parameters"].Vector().push_back(JsonUtils::intNode(multiplier));
	root["parameters"].Vector().push_back(JsonUtils::intNode(max));

	return root;
}
std::shared_ptr<Bonus> TimesStackLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE)
	{
		int level = dynamic_cast<const CStackInstance &>(context).getLevel();
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level;
		return newBonus;
	}
	else if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const auto & stack = dynamic_cast<const CStack &>(context);
		//only update if stack doesn't have an instance (summons, war machines)
		//otherwise we'd end up multiplying twice
		if(stack.base == nullptr)
		{
			int level = stack.unitType()->getLevel();
			std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
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
	return JsonUtils::stringNode("TIMES_STACK_LEVEL");
}

std::string OwnerUpdater::toString() const
{
	return "OwnerUpdater";
}

JsonNode OwnerUpdater::toJsonNode() const
{
	return JsonUtils::stringNode("BONUS_OWNER_UPDATER");
}

std::shared_ptr<Bonus> OwnerUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	auto owner = CBonusSystemNode::retrieveNodeOwner(&context);

	if(owner == PlayerColor::UNFLAGGABLE)
		owner = PlayerColor::NEUTRAL;

	std::shared_ptr<Bonus> updated =
		std::make_shared<Bonus>(static_cast<Bonus::BonusDuration>(b->duration), b->type, b->source, b->val, b->sid, b->subtype, b->valType);
	updated->limiter = std::make_shared<OppositeSideLimiter>(owner);
	return updated;
}

VCMI_LIB_NAMESPACE_END