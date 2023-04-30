/*
 * Limiters.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Limiters.h"

#include "../VCMI_Lib.h"
#include "../spells/CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CCreatureSet.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CSkillHandler.h"
#include "../CStack.h"
#include "../CArtHandler.h"
#include "../CModHandler.h"
#include "../TerrainHandler.h"
#include "../StringConstants.h"
#include "../battle/BattleInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::map<std::string, TLimiterPtr> bonusLimiterMap =
{
	{"SHOOTER_ONLY", std::make_shared<HasAnotherBonusLimiter>(BonusType::SHOOTER)},
	{"DRAGON_NATURE", std::make_shared<HasAnotherBonusLimiter>(BonusType::DRAGON_NATURE)},
	{"IS_UNDEAD", std::make_shared<HasAnotherBonusLimiter>(BonusType::UNDEAD)},
	{"CREATURE_NATIVE_TERRAIN", std::make_shared<CreatureTerrainLimiter>()},
	{"CREATURE_FACTION", std::make_shared<AllOfLimiter>(std::initializer_list<TLimiterPtr>{std::make_shared<CreatureLevelLimiter>(), std::make_shared<FactionLimiter>()})},
	{"SAME_FACTION", std::make_shared<FactionLimiter>()},
	{"CREATURES_ONLY", std::make_shared<CreatureLevelLimiter>()},
	{"OPPOSITE_SIDE", std::make_shared<OppositeSideLimiter>()},
};

static const CStack * retrieveStackBattle(const CBonusSystemNode * node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_BATTLE:
		return dynamic_cast<const CStack *>(node);
	default:
		return nullptr;
	}
}

static const CStackInstance * retrieveStackInstance(const CBonusSystemNode * node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_INSTANCE:
		return (dynamic_cast<const CStackInstance *>(node));
	case CBonusSystemNode::STACK_BATTLE:
		return (dynamic_cast<const CStack *>(node))->base;
	default:
		return nullptr;
	}
}

static const CCreature * retrieveCreature(const CBonusSystemNode *node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::CREATURE:
		return (dynamic_cast<const CCreature *>(node));
	case CBonusSystemNode::STACK_BATTLE:
		return (dynamic_cast<const CStack *>(node))->unitType();
	default:
		const CStackInstance * csi = retrieveStackInstance(node);
		if(csi)
			return csi->type;
		return nullptr;
	}
}

ILimiter::EDecision ILimiter::limit(const BonusLimitationContext &context) const /*return true to drop the bonus */
{
	return ILimiter::EDecision::ACCEPT;
}

std::string ILimiter::toString() const
{
	return typeid(*this).name();
}

JsonNode ILimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	root["type"].String() = toString();
	return root;
}

ILimiter::EDecision CCreatureTypeLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	if(!c)
		return ILimiter::EDecision::DISCARD;
	
	auto accept =  c->getId() == creature->getId() || (includeUpgrades && creature->isMyUpgrade(c));
	return accept ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD;
	//drop bonus if it's not our creature and (we don`t check upgrades or its not our upgrade)
}

CCreatureTypeLimiter::CCreatureTypeLimiter(const CCreature & creature_, bool IncludeUpgrades)
	: creature(&creature_), includeUpgrades(IncludeUpgrades)
{
}

void CCreatureTypeLimiter::setCreature(const CreatureID & id)
{
	creature = VLC->creh->objects[id];
}

std::string CCreatureTypeLimiter::toString() const
{
	boost::format fmt("CCreatureTypeLimiter(creature=%s, includeUpgrades=%s)");
	fmt % creature->getJsonKey() % (includeUpgrades ? "true" : "false");
	return fmt.str();
}

JsonNode CCreatureTypeLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_TYPE_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(creature->getJsonKey()));
	root["parameters"].Vector().push_back(JsonUtils::boolNode(includeUpgrades));

	return root;
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( BonusType bonus )
	: type(bonus), subtype(0), isSubtypeRelevant(false), isSourceRelevant(false), isSourceIDRelevant(false)
{
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( BonusType bonus, TBonusSubtype _subtype )
	: type(bonus), subtype(_subtype), isSubtypeRelevant(true), isSourceRelevant(false), isSourceIDRelevant(false)
{
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter(BonusType bonus, BonusSource src)
	: type(bonus), source(src), isSubtypeRelevant(false), isSourceRelevant(true), isSourceIDRelevant(false)
{
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter(BonusType bonus, TBonusSubtype _subtype, BonusSource src)
	: type(bonus), subtype(_subtype), isSubtypeRelevant(true), source(src), isSourceRelevant(true), isSourceIDRelevant(false)
{
}

ILimiter::EDecision HasAnotherBonusLimiter::limit(const BonusLimitationContext &context) const
{
	//TODO: proper selector config with parsing of JSON
	auto mySelector = Selector::type()(type);

	if(isSubtypeRelevant)
		mySelector = mySelector.And(Selector::subtype()(subtype));
	if(isSourceRelevant && isSourceIDRelevant)
		mySelector = mySelector.And(Selector::source(source, sid));
	else if (isSourceRelevant)
		mySelector = mySelector.And(Selector::sourceTypeSel(source));

	//if we have a bonus of required type accepted, limiter should accept also this bonus
	if(context.alreadyAccepted.getFirst(mySelector))
		return ILimiter::EDecision::ACCEPT;

	//if there are no matching bonuses pending, we can (and must) reject right away
	if(!context.stillUndecided.getFirst(mySelector))
		return ILimiter::EDecision::DISCARD;

	//do not accept for now but it may change if more bonuses gets included
	return ILimiter::EDecision::NOT_SURE;
}

std::string HasAnotherBonusLimiter::toString() const
{
	std::string typeName = vstd::findKey(bonusNameMap, type);
	if(isSubtypeRelevant)
	{
		boost::format fmt("HasAnotherBonusLimiter(type=%s, subtype=%d)");
		fmt % typeName % subtype;
		return fmt.str();
	}
	else
	{
		boost::format fmt("HasAnotherBonusLimiter(type=%s)");
		fmt % typeName;
		return fmt.str();
	}
}

JsonNode HasAnotherBonusLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	std::string typeName = vstd::findKey(bonusNameMap, type);
	auto sourceTypeName = vstd::findKey(bonusSourceMap, source);

	root["type"].String() = "HAS_ANOTHER_BONUS_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(typeName));
	if(isSubtypeRelevant)
		root["parameters"].Vector().push_back(JsonUtils::intNode(subtype));
	if(isSourceRelevant)
		root["parameters"].Vector().push_back(JsonUtils::stringNode(sourceTypeName));

	return root;
}

ILimiter::EDecision UnitOnHexLimiter::limit(const BonusLimitationContext &context) const
{
	const auto * stack = retrieveStackBattle(&context.node);
	if(!stack)
		return ILimiter::EDecision::DISCARD;

	auto accept = false;
	for (const auto & hex : stack->getHexes())
		accept |= !!applicableHexes.count(hex);

	return accept ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD;
}

UnitOnHexLimiter::UnitOnHexLimiter(const std::set<BattleHex> & applicableHexes):
	applicableHexes(applicableHexes)
{
}

JsonNode UnitOnHexLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "UNIT_ON_HEXES";
	for(const auto & hex : applicableHexes)
		root["parameters"].Vector().push_back(JsonUtils::intNode(hex));

	return root;
}

CreatureTerrainLimiter::CreatureTerrainLimiter()
	: terrainType(ETerrainId::NATIVE_TERRAIN)
{
}

CreatureTerrainLimiter::CreatureTerrainLimiter(TerrainId terrain):
	terrainType(terrain)
{
}

ILimiter::EDecision CreatureTerrainLimiter::limit(const BonusLimitationContext &context) const
{
	const CStack *stack = retrieveStackBattle(&context.node);
	if(stack)
	{
		if (terrainType == ETerrainId::NATIVE_TERRAIN && stack->isOnNativeTerrain())//terrainType not specified = native
			return ILimiter::EDecision::ACCEPT;

		if(terrainType != ETerrainId::NATIVE_TERRAIN && stack->isOnTerrain(terrainType))
			return ILimiter::EDecision::ACCEPT;

	}
	return ILimiter::EDecision::DISCARD;
	//TODO neutral creatues
}

std::string CreatureTerrainLimiter::toString() const
{
	boost::format fmt("CreatureTerrainLimiter(terrainType=%s)");
	auto terrainName = VLC->terrainTypeHandler->getById(terrainType)->getJsonKey();
	fmt % (terrainType == ETerrainId::NATIVE_TERRAIN ? "native" : terrainName);
	return fmt.str();
}

JsonNode CreatureTerrainLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_TERRAIN_LIMITER";
	auto terrainName = VLC->terrainTypeHandler->getById(terrainType)->getJsonKey();
	root["parameters"].Vector().push_back(JsonUtils::stringNode(terrainName));

	return root;
}

FactionLimiter::FactionLimiter(FactionID creatureFaction)
	: faction(creatureFaction)
{
}

ILimiter::EDecision FactionLimiter::limit(const BonusLimitationContext &context) const
{
	const auto * bearer = dynamic_cast<const INativeTerrainProvider*>(&context.node);

	if(bearer)
	{
		if(faction != FactionID::DEFAULT)
			return bearer->getFaction() == faction ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD;

		switch(context.b.source)
		{
			case BonusSource::CREATURE_ABILITY:
				return bearer->getFaction() == CreatureID(context.b.sid).toCreature()->getFaction() ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD;
			
			case BonusSource::TOWN_STRUCTURE:
				return bearer->getFaction() == FactionID(Bonus::getHighFromSid32(context.b.sid)) ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD;

			//TODO: other sources of bonuses
		}
	}
	return ILimiter::EDecision::DISCARD; //Discard by default
}

std::string FactionLimiter::toString() const
{
	boost::format fmt("FactionLimiter(faction=%s)");
	fmt % VLC->factions()->getByIndex(faction)->getJsonKey();
	return fmt.str();
}

JsonNode FactionLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "FACTION_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(VLC->factions()->getByIndex(faction)->getJsonKey()));

	return root;
}

CreatureLevelLimiter::CreatureLevelLimiter(uint32_t minLevel, uint32_t maxLevel) :
	minLevel(minLevel),
	maxLevel(maxLevel)
{
}

ILimiter::EDecision CreatureLevelLimiter::limit(const BonusLimitationContext &context) const
{
	const auto *c = retrieveCreature(&context.node);
	auto accept = c && (c->getLevel() < maxLevel && c->getLevel() >= minLevel);
	return accept ? ILimiter::EDecision::ACCEPT : ILimiter::EDecision::DISCARD; //drop bonus for non-creatures or non-native residents
}

std::string CreatureLevelLimiter::toString() const
{
	boost::format fmt("CreatureLevelLimiter(minLevel=%d,maxLevel=%d)");
	fmt % minLevel % maxLevel;
	return fmt.str();
}

JsonNode CreatureLevelLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_LEVEL_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::intNode(minLevel));
	root["parameters"].Vector().push_back(JsonUtils::intNode(maxLevel));

	return root;
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter(EAlignment Alignment)
	: alignment(Alignment)
{
}

ILimiter::EDecision CreatureAlignmentLimiter::limit(const BonusLimitationContext &context) const
{
	const auto * c = retrieveCreature(&context.node);
	if(c) {
		if(alignment == EAlignment::GOOD && c->isGood())
			return ILimiter::EDecision::ACCEPT;
		if(alignment == EAlignment::EVIL && c->isEvil())
			return ILimiter::EDecision::ACCEPT;
		if(alignment == EAlignment::NEUTRAL && !c->isEvil() && !c->isGood())
			return ILimiter::EDecision::ACCEPT;
	}

	return ILimiter::EDecision::DISCARD;
}

std::string CreatureAlignmentLimiter::toString() const
{
	boost::format fmt("CreatureAlignmentLimiter(alignment=%s)");
	fmt % GameConstants::ALIGNMENT_NAMES[vstd::to_underlying(alignment)];
	return fmt.str();
}

JsonNode CreatureAlignmentLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_ALIGNMENT_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(GameConstants::ALIGNMENT_NAMES[vstd::to_underlying(alignment)]));

	return root;
}

RankRangeLimiter::RankRangeLimiter(ui8 Min, ui8 Max)
	:minRank(Min), maxRank(Max)
{
}

RankRangeLimiter::RankRangeLimiter()
{
	minRank = maxRank = -1;
}

ILimiter::EDecision RankRangeLimiter::limit(const BonusLimitationContext &context) const
{
	const CStackInstance * csi = retrieveStackInstance(&context.node);
	if(csi)
	{
		if (csi->getNodeType() == CBonusSystemNode::COMMANDER) //no stack exp bonuses for commander creatures
			return ILimiter::EDecision::DISCARD;
		if (csi->getExpRank() > minRank && csi->getExpRank() < maxRank)
			return ILimiter::EDecision::ACCEPT;
	}
	return ILimiter::EDecision::DISCARD;
}

OppositeSideLimiter::OppositeSideLimiter(PlayerColor Owner):
	owner(std::move(Owner))
{
}

ILimiter::EDecision OppositeSideLimiter::limit(const BonusLimitationContext & context) const
{
	auto contextOwner = CBonusSystemNode::retrieveNodeOwner(& context.node);
	auto decision = (owner == contextOwner || owner == PlayerColor::CANNOT_DETERMINE) ? ILimiter::EDecision::DISCARD : ILimiter::EDecision::ACCEPT;
	return decision;
}

// Aggregate/Boolean Limiters

AggregateLimiter::AggregateLimiter(std::vector<TLimiterPtr> limiters):
	limiters(std::move(limiters))
{
}

void AggregateLimiter::add(const TLimiterPtr & limiter)
{
	if(limiter)
		limiters.push_back(limiter);
}

JsonNode AggregateLimiter::toJsonNode() const
{
	JsonNode result(JsonNode::JsonType::DATA_VECTOR);
	result.Vector().push_back(JsonUtils::stringNode(getAggregator()));
	for(const auto & l : limiters)
		result.Vector().push_back(l->toJsonNode());
	return result;
}

const std::string AllOfLimiter::aggregator = "allOf";
const std::string & AllOfLimiter::getAggregator() const
{
	return aggregator;
}

AllOfLimiter::AllOfLimiter(std::vector<TLimiterPtr> limiters):
	AggregateLimiter(limiters)
{
}

ILimiter::EDecision AllOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(const auto & limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::EDecision::DISCARD)
			return result;
		if(result == ILimiter::EDecision::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::EDecision::NOT_SURE : ILimiter::EDecision::ACCEPT;
}

const std::string AnyOfLimiter::aggregator = "anyOf";
const std::string & AnyOfLimiter::getAggregator() const
{
	return aggregator;
}

AnyOfLimiter::AnyOfLimiter(std::vector<TLimiterPtr> limiters):
	AggregateLimiter(limiters)
{
}

ILimiter::EDecision AnyOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(const auto & limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::EDecision::ACCEPT)
			return result;
		if(result == ILimiter::EDecision::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::EDecision::NOT_SURE : ILimiter::EDecision::DISCARD;
}

const std::string NoneOfLimiter::aggregator = "noneOf";
const std::string & NoneOfLimiter::getAggregator() const
{
	return aggregator;
}

NoneOfLimiter::NoneOfLimiter(std::vector<TLimiterPtr> limiters):
	AggregateLimiter(limiters)
{
}

ILimiter::EDecision NoneOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(const auto & limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::EDecision::ACCEPT)
			return ILimiter::EDecision::DISCARD;
		if(result == ILimiter::EDecision::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::EDecision::NOT_SURE : ILimiter::EDecision::ACCEPT;
}

VCMI_LIB_NAMESPACE_END
