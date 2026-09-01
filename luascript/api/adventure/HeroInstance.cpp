/*
 * HeroInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroInstance.h"

#include "../Registry.h"
#include "../library/BonusBearerBindings.h"

#include "StackInstance.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/entities/artifact/CArtifactInstance.h"

namespace scripting::api
{

void HeroInstanceProxy::registerMethods(MethodRegistrar & R)
{
	BonusBearerBindings<CGHeroInstance>::registerMethods(R);

	R.method<&CCreatureSet::getStackPtr, CGHeroInstance>("getStack",
		{{"slot", "Army slot to query (1-based)."}}, {},
		"Returns the stack instance in the given army slot, or nil if the slot is empty.");
	R.method<&CGObjectInstance::getOwner, CGHeroInstance>("getOwner", {},
		"Returns the player color that owns this hero.");
	R.method<&CGHeroInstance::getNameTextID>("getNameTextID", {},
		"Returns the text ID of the hero's name.");
	R.function<&HeroInstanceProxy::isMale>("isMale", {},
		"True if the hero's gender is male.");
	R.function<&HeroInstanceProxy::isFemale>("isFemale", {},
		"True if the hero's gender is female.");
	R.function<&HeroInstanceProxy::getLevel>("getLevel",
		{"Current level of the hero."},
		"Returns the hero's current level.");
	R.function<&HeroInstanceProxy::getExperience>("getExperience",
		{"Total experience points of the hero."},
		"Returns the hero's total experience points.");
	R.method<&CGHeroInstance::getPrimSkillLevel>("getPrimarySkill",
		{{"skill", "Primary skill JSON key (`attack`, `defence`, `spellpower`, `knowledge`)."}},
		{"Current value of the primary skill."},
		"Returns the value of one of the hero's primary skills.");
	R.method<&CGHeroInstance::getSecSkillLevel>("getSecondarySkill",
		{{"skill", "Secondary skill JSON key."}},
		{"Mastery level (0 = none, 1 = basic, 2 = advanced, 3 = expert)."},
		"Returns the hero's mastery of the given secondary skill.");
	R.function<&HeroInstanceProxy::hasArtifact>("hasArtifact",
		{{"artifact", "Artifact JSON key."}},
		{"True if the hero owns the artifact."},
		"Returns whether the hero owns the given artifact, either as equipped or in the backpack.");
	R.function<&HeroInstanceProxy::ownedArtifacts>("ownedArtifacts",
		{{"artifact", "Artifact JSON key."}},
		{"Number of copies of that artifact the hero carries."},
		"Returns how many copies of the given artifact the hero owns, counting both equipped slots and the backpack.");
	R.function<&HeroInstanceProxy::creatureCountInArmy>("creatureCountInArmy",
		{{"creature", "Creature JSON key."}},
		{"Total number of that creature across the hero's army."},
		"Returns how many creatures of the given type are in the hero's army across all unit stacks.");
}

bool HeroInstanceProxy::isMale(const CGHeroInstance & hero)
{
	return hero.gender == EHeroGender::MALE;
}

bool HeroInstanceProxy::isFemale(const CGHeroInstance & hero)
{
	return hero.gender == EHeroGender::FEMALE;
}

int HeroInstanceProxy::getLevel(const CGHeroInstance & hero)
{
	return hero.level;
}

int64_t HeroInstanceProxy::getExperience(const CGHeroInstance & hero)
{
	return hero.exp;
}

bool HeroInstanceProxy::hasArtifact(const CGHeroInstance & hero, ArtifactID artifact)
{
	return hero.hasArt(artifact);
}

int HeroInstanceProxy::ownedArtifacts(const CGHeroInstance & hero, ArtifactID artifact)
{
	int total = 0;
	for(const auto & [pos, slot] : hero.artifactsWorn)
		if(slot.getArt() && slot.getArt()->getTypeId() == artifact)
			++total;
	for(const auto & slot : hero.artifactsInBackpack)
		if(slot.getArt() && slot.getArt()->getTypeId() == artifact)
			++total;
	return total;
}

int HeroInstanceProxy::creatureCountInArmy(const CGHeroInstance & hero, CreatureID creature)
{
	int total = 0;
	for(const auto & slot : hero.Slots())
	{
		const CCreature * stackCreature = hero.getCreature(slot.first);
		if(stackCreature && stackCreature->getId() == creature)
			total += hero.getStackCount(slot.first);
	}
	return total;
}

}
