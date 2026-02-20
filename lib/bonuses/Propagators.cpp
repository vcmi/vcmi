/*
 * Propagators.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Propagators.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::map<std::string, TPropagatorPtr> bonusPropagatorMap =
{
	{"BATTLE_WIDE", std::make_shared<CPropagatorNodeType>(BonusNodeType::BATTLE_WIDE)},
	{"TOWN_AND_VISITOR", std::make_shared<CPropagatorNodeType>(BonusNodeType::TOWN_AND_VISITOR)},
	{"PLAYER", std::make_shared<CPropagatorNodeType>(BonusNodeType::PLAYER)},
	{"HERO", std::make_shared<CPropagatorNodeType>(BonusNodeType::HERO)},
	{"TOWN", std::make_shared<CPropagatorNodeType>(BonusNodeType::TOWN)},
	{"ARMY", std::make_shared<CPropagatorNodeType>(BonusNodeType::ARMY)},
	{"TEAM", std::make_shared<CPropagatorNodeType>(BonusNodeType::TEAM)},
	{"GLOBAL_EFFECT", std::make_shared<CPropagatorNodeType>(BonusNodeType::GLOBAL_EFFECTS)},

	// deprecated, for compatibility
	{"VISITED_TOWN_AND_VISITOR", std::make_shared<CPropagatorNodeType>(BonusNodeType::TOWN_AND_VISITOR)},
	{"PLAYER_PROPAGATOR", std::make_shared<CPropagatorNodeType>(BonusNodeType::PLAYER)},
	{"TEAM_PROPAGATOR", std::make_shared<CPropagatorNodeType>(BonusNodeType::TEAM)},

};

bool IPropagator::shouldBeAttached(CBonusSystemNode *dest) const
{
	return false;
}

BonusNodeType IPropagator::getPropagatorType() const
{
	return BonusNodeType::NONE;
}

CPropagatorNodeType::CPropagatorNodeType(BonusNodeType NodeType)
	: nodeType(NodeType)
{
}

BonusNodeType CPropagatorNodeType::getPropagatorType() const
{
	return nodeType;
}

bool CPropagatorNodeType::shouldBeAttached(CBonusSystemNode *dest) const
{
	if (nodeType == dest->getNodeType())
		return true;

	if (nodeType == BonusNodeType::ARMY)
		return dest->getNodeType() == BonusNodeType::HERO || dest->getNodeType() == BonusNodeType::TOWN;

	return false;
}

VCMI_LIB_NAMESPACE_END
