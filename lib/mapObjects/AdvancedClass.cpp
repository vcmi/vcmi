/*
 * AdvancedClass.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#include "StdInc.h"
#include "AdvancedClass.h"

#include "CGHeroInstance.h"

#include "../GameLibrary.h"
#include "../bonuses/Bonus.h"
#include "../json/JsonBonus.h"
#include "../json/JsonNode.h"
#include "../modding/IdentifierStorage.h"
#include "../texts/CGeneralTextHandler.h"

bool AdvancedClassProgress::empty() const
{
	return std::ranges::all_of(classes, [](const auto & value) { return value.empty(); });
}

bool AdvancedClassProgress::valid() const
{
	bool encounteredEmptyTier = false;
	for(const auto & value : classes)
	{
		if(value.empty())
			encounteredEmptyTier = true;
		else if(encounteredEmptyTier)
			return false;
	}
	return true;
}

int AdvancedClassProgress::nextTier() const
{
	for(size_t index = 0; index < classes.size(); ++index)
		if(classes[index].empty())
			return static_cast<int>(index) + 1;
	return 5;
}

int AdvancedClassDefinition::calculatePoints(const std::function<int(SecondarySkill)> & mastery) const
{
	if(skillGroups.empty())
		return 0;

	int result = std::numeric_limits<int>::max();
	for(const auto & group : skillGroups)
	{
		int groupPoints = 0;
		for(const auto & weight : group)
		{
			const int level = mastery(weight.skill);
			if(level >= 1 && level <= 3)
				groupPoints += weight.values.at(level - 1);
		}
		result = std::min(result, groupPoints);
	}
	return result;
}

bool AdvancedClassDefinition::follows(const AdvancedClassProgress & progress) const
{
	if(tier == 1)
		return parents.empty();
	if(tier < 1 || tier > 4)
		return false;
	return std::ranges::find(parents, progress.classes.at(tier - 2)) != parents.end();
}

bool AdvancedClassDefinition::isAbilityReady(int currentRound, int lastUseRound) const
{
	return abilityConfigured && abilityCooldown >= 0 && currentRound - lastUseRound >= abilityCooldown;
}

namespace
{
bool fail(std::string * error, const std::string & message)
{
	if(error)
		*error = message;
	return false;
}
}

std::string AdvancedClassHandler::qualify(const std::string & scope, const std::string & identifier)
{
	return identifier.find(':') == std::string::npos ? scope + ":" + identifier : identifier;
}

std::vector<JsonNode> AdvancedClassHandler::loadLegacyData()
{
	return {};
}

void AdvancedClassHandler::loadObject(const std::string & scope, const std::string & name, const JsonNode & data)
{
	auto definition = std::make_shared<AdvancedClassDefinition>();
	definition->id = qualify(scope, name);
	definition->modScope = scope;
	definition->nameTextId = "advancedClass." + scope + "." + name + ".name";
	definition->tier = static_cast<int>(data["tier"].Integer());
	definition->requiredLevel = static_cast<int>(data["requiredLevel"].Integer());
	definition->requiredPoints = static_cast<int>(data["requiredPoints"].Integer());

	LIBRARY->generaltexth->registerString(scope, definition->nameTextId, data["name"]);

	for(const auto & parent : data["parents"].Vector())
		definition->parents.push_back(qualify(scope, parent.String()));

	for(const auto & groupNode : data["skillGroups"].Vector())
	{
		definition->skillGroups.emplace_back();
		auto & group = definition->skillGroups.back();
		for(const auto & [skillName, valuesNode] : groupNode.Struct())
		{
			if(valuesNode.Vector().size() != 3)
				continue;
			AdvancedClassSkillWeight weight;
			for(size_t i = 0; i < weight.values.size(); ++i)
				weight.values[i] = static_cast<int>(valuesNode.Vector().at(i).Integer());
			group.push_back(weight);
			LIBRARY->identifiers()->requestIdentifier(valuesNode.getModScope(), "skill", skillName, [definition, groupIndex = definition->skillGroups.size() - 1, weightIndex = group.size() - 1](si32 skillId)
			{
				definition->skillGroups.at(groupIndex).at(weightIndex).skill = SecondarySkill(skillId);
			});
		}
	}

	for(const auto & [bonusName, bonusNode] : data["bonuses"].Struct())
	{
		auto bonus = JsonUtils::parseBonus(bonusNode, TextIdentifier(definition->nameTextId, "bonus", bonusName));
		if(bonus)
			definition->bonuses.push_back(std::move(bonus));
	}

	if(!data["ability"].isNull())
	{
		definition->abilityConfigured = true;
		definition->abilityCooldown = static_cast<int>(data["ability"]["cooldown"].Integer());
		LIBRARY->identifiers()->requestIdentifier("spell", data["ability"]["spell"], [definition](si32 spellId)
		{
			definition->ability = SpellID(spellId);
		});
	}

	objects.push_back(definition);
	objectsById[definition->id] = definition;
	registerObject(scope, "advancedClass", name, data, static_cast<si32>(objects.size() - 1));
}

void AdvancedClassHandler::loadObject(const std::string & scope, const std::string & name, const JsonNode & data, size_t index)
{
	logMod->error("Advanced class '%s:%s' uses reserved index %d; explicit indices are not supported", scope, name, index);
	loadObject(scope, name, data);
}

const AdvancedClassDefinition * AdvancedClassHandler::find(const std::string & id) const
{
	auto found = objectsById.find(id);
	return found == objectsById.end() ? nullptr : found->second.get();
}

const std::vector<std::shared_ptr<AdvancedClassDefinition>> & AdvancedClassHandler::getAll() const
{
	return objects;
}

const AdvancedClassDefinition * AdvancedClassHandler::rootClass(const AdvancedClassDefinition & definition) const
{
	const AdvancedClassDefinition * current = &definition;
	while(current->tier > 1)
	{
		if(current->parents.empty())
			return nullptr;
		current = find(current->parents.front());
		if(!current)
			return nullptr;
	}
	return current;
}

int AdvancedClassHandler::pointsFor(const CGHeroInstance & hero, const AdvancedClassDefinition & definition) const
{
	const auto * root = definition.tier == 1 ? &definition : find(hero.advancedClass.classes.front());
	if(!root)
		root = rootClass(definition);
	if(!root || root->skillGroups.empty())
		return 0;

	return root->calculatePoints([&hero](SecondarySkill skill)
	{
		return hero.getSecSkillLevel(skill);
	});
}

bool AdvancedClassHandler::canSelect(const CGHeroInstance & hero, const std::string & id, std::string * error) const
{
	if(!hero.advancedClass.valid())
		return fail(error, "Advanced class progression is invalid");
	const auto * definition = find(id);
	if(!definition)
		return fail(error, "Unknown advanced class");
	if(!definition->valid)
		return fail(error, "Advanced class definition is invalid");
	const int nextTier = hero.advancedClass.nextTier();
	if(nextTier > 4)
		return fail(error, "Advanced class progression is complete");
	if(definition->tier != nextTier)
		return fail(error, "Advanced class tier is not available");
	if(hero.level < definition->requiredLevel)
		return fail(error, "Hero level requirement is not met");
	if(pointsFor(hero, *definition) < definition->requiredPoints)
		return fail(error, "Advanced class path point requirement is not met");
	if(!definition->follows(hero.advancedClass))
		return fail(error, "Advanced class does not continue the selected path");
	return true;
}

void AdvancedClassHandler::afterLoadFinalization()
{
	for(const auto & definition : objects)
	{
		if(definition->tier < 1 || definition->tier > 4)
		{
			logMod->error("Advanced class '%s' has invalid tier %d", definition->id, definition->tier);
			definition->valid = false;
		}
		if(definition->requiredLevel < 0 || definition->requiredPoints < 0)
		{
			logMod->error("Advanced class '%s' has negative requirements", definition->id);
			definition->valid = false;
		}
		if(definition->tier == 1 && definition->skillGroups.empty())
		{
			logMod->error("Tier-one advanced class '%s' has no skillGroups", definition->id);
			definition->valid = false;
		}
		if(definition->abilityConfigured && !definition->ability.hasValue())
		{
			logMod->error("Advanced class '%s' references an unknown ability spell", definition->id);
			definition->valid = false;
		}
		if(definition->abilityConfigured && (definition->tier < 2 || definition->tier > 3))
		{
			logMod->error("Advanced class '%s' defines a battle ability outside tiers 2 and 3", definition->id);
			definition->valid = false;
		}
		if(definition->abilityConfigured && definition->abilityCooldown < 0)
		{
			logMod->error("Advanced class '%s' defines a negative ability cooldown", definition->id);
			definition->valid = false;
		}
		if(std::ranges::any_of(definition->bonuses, [](const auto & bonus) { return !bonus || bonus->type == BonusType::NONE; }))
		{
			logMod->error("Advanced class '%s' contains an invalid bonus", definition->id);
			definition->valid = false;
		}
		for(const auto & group : definition->skillGroups)
		{
			if(group.empty() || std::ranges::any_of(group, [](const auto & weight) { return !weight.skill.hasValue(); }))
			{
				logMod->error("Advanced class '%s' contains an empty group or unknown skill", definition->id);
				definition->valid = false;
			}
		}
		if((definition->tier == 1) != definition->parents.empty())
		{
			logMod->error("Advanced class '%s' has invalid parent count for tier %d", definition->id, definition->tier);
			definition->valid = false;
		}
		for(const auto & parentId : definition->parents)
		{
			const auto * parent = find(parentId);
			if(!parent)
			{
				logMod->error("Advanced class '%s' references unknown parent '%s'", definition->id, parentId);
				definition->valid = false;
			}
			else if(parent->tier + 1 != definition->tier)
			{
				logMod->error("Advanced class '%s' parent '%s' is not from the previous tier", definition->id, parentId);
				definition->valid = false;
			}
		}
	}

	bool invalidated;
	do
	{
		invalidated = false;
		for(const auto & definition : objects)
		{
			if(!definition->valid)
				continue;
			for(const auto & parentId : definition->parents)
			{
				const auto * parent = find(parentId);
				if(parent && !parent->valid)
				{
					logMod->error("Advanced class '%s' depends on invalid parent '%s'", definition->id, parentId);
					definition->valid = false;
					invalidated = true;
					break;
				}
			}
		}
	} while(invalidated);
}
