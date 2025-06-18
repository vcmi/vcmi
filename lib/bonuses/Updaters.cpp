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
	return JsonNode();
}

GrowsWithLevelUpdater::GrowsWithLevelUpdater(int valPer20, int stepSize)
	: valPer20(valPer20)
	, stepSize(stepSize)
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

std::shared_ptr<Bonus> TimesHeroLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = dynamic_cast<const CGHeroInstance &>(context).level;
		auto newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level / stepSize;
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

std::shared_ptr<Bonus> TimesHeroLevelDivideStackLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		auto newBonus = TimesHeroLevelUpdater::createUpdatedBonus(b, context);
		newBonus->updater = divideStackLevel;
		return newBonus;
	}
	return b;
}

std::string TimesHeroLevelDivideStackLevelUpdater::toString() const
{
	return "TimesHeroLevelDivideStackLevelUpdater";
}

JsonNode TimesHeroLevelDivideStackLevelUpdater::toJsonNode() const
{
	return JsonNode("TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL");
}

std::shared_ptr<Bonus> TimesStackSizeUpdater::apply(const std::shared_ptr<Bonus> & b, int count) const
{
	auto newBonus = std::make_shared<Bonus>(*b);
	newBonus->val *= std::clamp(count / stepSize, minimum, maximum);
	newBonus->updater = nullptr; // prevent double-apply
	return newBonus;
}

std::shared_ptr<Bonus> TimesStackSizeUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE || context.getNodeType() == CBonusSystemNode::COMMANDER)
	{
		int count = dynamic_cast<const CStackInstance &>(context).getCount();
		return apply(b, count);
	}

	if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const auto & stack = dynamic_cast<const CStack &>(context);
		return apply(b, stack.getCount());
	}
	return b;
}

std::string TimesStackSizeUpdater::toString() const
{
	return "TimesStackSizeUpdater";
}

JsonNode TimesStackSizeUpdater::toJsonNode() const
{
	return JsonNode("TIMES_STACK_SIZE");
}

std::shared_ptr<Bonus> TimesStackLevelUpdater::apply(const std::shared_ptr<Bonus> & b, int level) const
{
	auto newBonus = std::make_shared<Bonus>(*b);
	newBonus->val *= level;
	newBonus->updater = nullptr; // prevent double-apply
	return newBonus;
}

std::shared_ptr<Bonus> TimesStackLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE || context.getNodeType() == CBonusSystemNode::COMMANDER)
	{
		int level = dynamic_cast<const CStackInstance &>(context).getLevel();
		return apply(b, level);
	}

	if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const auto & stack = dynamic_cast<const CStack &>(context);
		//update if stack doesn't have an instance (summons, war machines)
		if(stack.base == nullptr)
			return apply(b, stack.unitType()->getLevel());

		// If these are not handled here, the final outcome may potentially be incorrect.
		int level = dynamic_cast<const CStackInstance*>(stack.base)->getLevel();
		return apply(b, level);
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

std::shared_ptr<Bonus> DivideStackLevelUpdater::apply(const std::shared_ptr<Bonus> & b, int level) const
{
	if (level == 0)
		return b; // e.g. war machines & other special units

	auto newBonus = std::make_shared<Bonus>(*b);
	level = std::max(1, level);
	newBonus->val /= level;
	newBonus->updater = nullptr; // prevent double-apply
	return newBonus;
}

std::shared_ptr<Bonus> DivideStackLevelUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE || context.getNodeType() == CBonusSystemNode::COMMANDER)
	{
		int level = dynamic_cast<const CStackInstance &>(context).getLevel();
		return apply(b, level);
	}

	if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const auto & stack = dynamic_cast<const CStack &>(context);
		//update if stack doesn't have an instance (summons, war machines)
		if(stack.base == nullptr)
			return apply(b, stack.unitType()->getLevel());

		// If these are not handled here, the final outcome may potentially be incorrect.
		int level = dynamic_cast<const CStackInstance*>(stack.base)->getLevel();
		return apply(b, level);
	}
	return b;
}

std::string DivideStackLevelUpdater::toString() const
{
	return "DivideStackLevelUpdater";
}

JsonNode DivideStackLevelUpdater::toJsonNode() const
{
	return JsonNode("DIVIDE_STACK_LEVEL");
}

std::string OwnerUpdater::toString() const
{
	return "OwnerUpdater";
}

JsonNode OwnerUpdater::toJsonNode() const
{
	return JsonNode("BONUS_OWNER_UPDATER");
}

std::shared_ptr<Bonus> OwnerUpdater::createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const
{
	PlayerColor owner = context.getOwner();
	
	if(owner == PlayerColor::UNFLAGGABLE)
		owner = PlayerColor::NEUTRAL;

	std::shared_ptr<Bonus> updated =
		std::make_shared<Bonus>(*b);
	updated->bonusOwner = owner;
	return updated;
}

VCMI_LIB_NAMESPACE_END
