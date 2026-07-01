#include "StdInc.h"
#include "JsonKeyExtractor.h"

#include "../entities/artifact/CArtHandler.h"
#include "../callback/IGameInfoCallback.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

JsonKeyExtractor::JsonKeyExtractor(IGameInfoCallback * cb) : cb(cb) {}

si32 JsonKeyExtractor::loadVariable(const std::string & variableGroup, const std::string & value, const Variables & variables, si32 defaultValue)
{
	if(value.empty() || value[0] != '@')
	{
		logMod->warn("Invalid syntax in load value! Can not load value from '%s'", value);
		return defaultValue;
	}

	std::string variableID = variableGroup + value;

	if(variables.count(variableID) == 0)
	{
		logMod->warn("Invalid syntax in load value! Unknown variable '%s'", value);
		return defaultValue;
	}
	return variables.at(variableID);
}

std::set<ArtifactID> JsonKeyExtractor::filterKeysTyped(const JsonNode & value, const std::set<ArtifactID> & valuesSet)
{
	assert(value.isStruct());

	std::set<EArtifactClass> allowedClasses;
	std::set<ArtifactPosition> allowedPositions;
	ui32 minValue = 0;
	ui32 maxValue = std::numeric_limits<ui32>::max();

	if(value["class"].getType() == JsonNode::JsonType::DATA_STRING)
		allowedClasses.insert(CArtHandler::stringToClass(value["class"].String()));
	else
		for(const auto & entry : value["class"].Vector())
			allowedClasses.insert(CArtHandler::stringToClass(entry.String()));

	if(value["slot"].getType() == JsonNode::JsonType::DATA_STRING)
		allowedPositions.insert(ArtifactPosition::decode(value["class"].String()));
	else
		for(const auto & entry : value["slot"].Vector())
			allowedPositions.insert(ArtifactPosition::decode(entry.String()));

	if(!value["minValue"].isNull())
		minValue = static_cast<ui32>(value["minValue"].Float());
	if(!value["maxValue"].isNull())
		maxValue = static_cast<ui32>(value["maxValue"].Float());

	std::set<ArtifactID> result;

	for(const auto & artID : valuesSet)
	{
		const CArtifact * art = artID.toArtifact();

		if(!vstd::iswithin(art->getPrice(), minValue, maxValue))
			continue;

		if(!allowedClasses.empty() && !allowedClasses.count(art->aClass))
			continue;

		if(!cb->isAllowed(art->getId()))
			continue;

		if(!allowedPositions.empty())
		{
			bool positionAllowed = false;
			for(const auto & pos : art->getPossibleSlots().at(ArtBearer::HERO))
			{
				if(allowedPositions.count(pos))
					positionAllowed = true;
			}

			if(!positionAllowed)
				continue;
		}

		result.insert(artID);
	}
	return result;
}

std::set<SecondarySkill> JsonKeyExtractor::filterKeysTyped(const JsonNode & value, const std::set<SecondarySkill> & valuesSet)
{
	std::set<SecondarySkill> result = valuesSet;

	if(!value["tag"].isNull())
	{
		std::string requiredTag = value["tag"].String();

		vstd::erase_if(
			result,
			[&requiredTag](const SecondarySkill & skill)
			{
				return !skill.toSkill()->hasTag(requiredTag);
			}
			);
	}
	return result;
}


std::set<SpellID> JsonKeyExtractor::filterKeysTyped(const JsonNode & value, const std::set<SpellID> & valuesSet)
{
	std::set<SpellID> result = valuesSet;

	if(!value["level"].isNull())
	{
		int32_t spellLevel = value["level"].Integer();

		vstd::erase_if(
			result,
			[=](const SpellID & spell)
			{
				return LIBRARY->spellh->getById(spell)->getLevel() != spellLevel;
			}
		);
	}

	if(!value["school"].isNull())
	{
		int32_t schoolID = LIBRARY->identifiers()->getIdentifier("spellSchool", value["school"]).value();

		vstd::erase_if(
			result,
			[=](const SpellID & spell)
			{
				return !LIBRARY->spellh->getById(spell)->hasSchool(SpellSchool(schoolID));
			}
		);
	}
	return result;
}

VCMI_LIB_NAMESPACE_END
