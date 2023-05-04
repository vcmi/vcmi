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
	{"BATTLE_WIDE", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::BATTLE)},
	{"VISITED_TOWN_AND_VISITOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::TOWN_AND_VISITOR)},
	{"PLAYER_PROPAGATOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::PLAYER)},
	{"HERO", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::HERO)},
	{"TEAM_PROPAGATOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::TEAM)}, //untested
	{"GLOBAL_EFFECT", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::GLOBAL_EFFECTS)}
}; //untested

bool IPropagator::shouldBeAttached(CBonusSystemNode *dest)
{
	return false;
}

CBonusSystemNode::ENodeTypes IPropagator::getPropagatorType() const
{
	return CBonusSystemNode::ENodeTypes::NONE;
}

CPropagatorNodeType::CPropagatorNodeType(CBonusSystemNode::ENodeTypes NodeType)
	: nodeType(NodeType)
{
}

CBonusSystemNode::ENodeTypes CPropagatorNodeType::getPropagatorType() const
{
	return nodeType;
}

bool CPropagatorNodeType::shouldBeAttached(CBonusSystemNode *dest)
{
	return nodeType == dest->getNodeType();
}

VCMI_LIB_NAMESPACE_END