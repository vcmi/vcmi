/*
 * AdvancedClass.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "../IHandlerBase.h"
#include "../constants/EntityIdentifiers.h"

struct Bonus;
class CGHeroInstance;
class JsonNode;

enum class AdvancedClassSelectionProblem
{
	NONE,
	INVALID_PROGRESS,
	UNKNOWN_CLASS,
	INVALID_DEFINITION,
	PROGRESSION_COMPLETE,
	WRONG_TIER,
	LEVEL_REQUIREMENT,
	POINT_REQUIREMENT,
	WRONG_PATH
};

DLL_LINKAGE std::string_view advancedClassSelectionProblemName(AdvancedClassSelectionProblem problem);

struct DLL_LINKAGE AdvancedClassProgress
{
	std::array<std::string, 4> classes;

	bool valid() const;
	int nextTier() const;

	template <typename Handler> void serialize(Handler & h)
	{
		h & classes;
	}
};

struct DLL_LINKAGE AdvancedClassSkillWeight
{
	SecondarySkill skill;
	std::array<int, 3> values;
};

struct DLL_LINKAGE AdvancedClassDefinition
{
	std::string id;
	std::string nameTextId;
	std::string descriptionTextId;
	int tier = 0;
	int requiredLevel = 0;
	int requiredPoints = 0;
	std::vector<std::string> parents;
	std::vector<std::vector<AdvancedClassSkillWeight>> skillGroups;
	std::vector<std::shared_ptr<Bonus>> bonuses;
	SpellID ability = SpellID::NONE;
	bool abilityConfigured = false;
	int abilityCooldown = 0;
	bool valid = true;

	int calculatePoints(const std::function<int(SecondarySkill)> & mastery) const;
	bool follows(const AdvancedClassProgress & progress) const;
	bool isAbilityReady(int currentRound, int lastUseRound) const;
};

class DLL_LINKAGE AdvancedClassHandler final : public IHandlerBase
{
	std::vector<std::shared_ptr<AdvancedClassDefinition>> objects;
	std::map<std::string, std::shared_ptr<AdvancedClassDefinition>> objectsById;

	static std::string qualify(const std::string & scope, const std::string & identifier);
public:
	static constexpr size_t ABILITY_SLOTS = 2;

	std::vector<JsonNode> loadLegacyData() override;
	void loadObject(const std::string & scope, const std::string & name, const JsonNode & data) override;
	void loadObject(const std::string & scope, const std::string & name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	const AdvancedClassDefinition * find(const std::string & id) const;
	const std::vector<std::shared_ptr<AdvancedClassDefinition>> & getAll() const;
	const AdvancedClassDefinition * getRootClass(const AdvancedClassDefinition & definition) const;
	const AdvancedClassDefinition * getAbility(const CGHeroInstance & hero, size_t slot) const;
	int pointsFor(const CGHeroInstance & hero, const AdvancedClassDefinition & definition) const;
	AdvancedClassSelectionProblem selectionProblem(const CGHeroInstance & hero, const std::string & id) const;
	bool canSelect(const CGHeroInstance & hero, const std::string & id) const;
};
