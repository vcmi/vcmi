/*
 * AdventureServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AdventureServer.h"

#include "../../LuaCallWrapper.h"

#include "../LuaComponent.h"
#include "../LuaMetaString.h"

#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGObjectInstance.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/networkPacks/PacksForClient.h"

#include <vstd/RNG.h>

namespace scripting::api
{

void AdventureServerProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&AdventureServerProxy::setMapVariable>("setMapVariable",
		{
			{"name",  "Name of the variable to store under."},
			{"value", "Value to store; any value that survives as JSON - number, string, boolean, or a table of those. Overwrites any previous value."}
		}, {},
		"Stores a named value that persists in the save file and can be read back later with Game:getMapVariable. "
		"Use it to remember progress across visits, such as whether a one-time reward was already handed out.");
	R.function<&AdventureServerProxy::removeObject>("removeObject",
		{{"target", "Map object to remove."}}, {},
		"Permanently removes a map object from the adventure map. The object disappears for every player; "
		"if it was a town or a hero, ownership and garrison are lost as well. There is no undo.");
	R.function<&AdventureServerProxy::random>("random",
		{
			{"lower", "Smallest value that may be returned."},
			{"upper", "Largest value that may be returned."}
		},
		{"A whole number between lower and upper, both ends included."},
		"Returns a random whole number in the given range. Draws from the game's own random generator, so the "
		"result stays consistent with saved games and network play - do not use Lua's math.random for gameplay decisions.");
	R.function<&AdventureServerProxy::giveExperience>("giveExperience",
		{
			{"hero",   "Hero that gains the experience."},
			{"amount", "Experience points to add. The hero levels up automatically if the total crosses a level threshold."}
		}, {},
		"Awards experience points to a hero, triggering any level-ups (and the level-up dialog for a human player) that result.");
	R.method<&IGameEventCallback::giveResource>("giveResource",
		{
			{"player",   "Player whose treasury changes."},
			{"resource", "Resource to change, given as its JSON key (\"wood\", \"ore\", \"mercury\", \"sulfur\", \"crystal\", \"gems\", \"gold\")."},
			{"amount",   "How much to add. Use a negative number to take resources away; the treasury is clamped at zero and never goes negative."}
		}, {},
		"Adds or removes a single resource for one player.");
	R.function<&AdventureServerProxy::setOwner>("setOwner",
		{
			{"object", "Map object whose owner changes, such as a town, mine or dwelling."},
			{"owner",  "New owner. Pass the neutral player to make the object unowned."}
		}, {},
		"Transfers ownership of a map object to another player. For a town or mine this immediately moves its income "
		"and control; it does not move any garrisoned army or visiting hero.");
	R.function<&AdventureServerProxy::grantSpell>("grantSpell",
		{
			{"hero",  "Hero that learns the spell."},
			{"spell", "Spell to teach, given as its JSON key."}
		}, {},
		"Teaches a spell to a hero, writing it into the hero's spellbook. The hero needs a spellbook for the spell to be "
		"usable in combat. Teaching a spell the hero already knows does nothing.");
	R.function<&AdventureServerProxy::takeSpell>("takeSpell",
		{
			{"hero",  "Hero that forgets the spell."},
			{"spell", "Spell to remove, given as its JSON key."}
		}, {},
		"Removes a spell from a hero's spellbook. Does nothing if the hero did not know the spell.");
	R.function<&AdventureServerProxy::grantPrimarySkill>("grantPrimarySkill",
		{
			{"hero",   "Hero whose skill changes."},
			{"skill",  "Primary skill to change, given as its JSON key (\"attack\", \"defence\", \"spellpower\", \"knowledge\")."},
			{"amount", "How many points to add. Use a negative number to reduce the skill; it is clamped at zero and never goes negative."}
		}, {},
		"Permanently raises or lowers one of a hero's four primary skills.");
	R.function<&AdventureServerProxy::grantSecondarySkill>("grantSecondarySkill",
		{
			{"hero",  "Hero that learns or improves the skill."},
			{"skill", "Secondary skill to grant, given as its JSON key."},
			{"level", "Mastery to move to: 1 = basic, 2 = advanced, 3 = expert."}
		}, {},
		"Teaches a secondary skill to a hero, or raises it to the given mastery. If the hero already knows the skill at an "
		"equal or higher mastery it is left unchanged; a hero who has no free skill slots left will not learn a brand-new skill.");
	R.function<&AdventureServerProxy::grantArtifact>("grantArtifact",
		{
			{"hero",     "Hero that receives the artifact."},
			{"artifact", "Artifact to give, given as its JSON key."}
		}, {},
		"Gives an artifact to a hero. It is equipped in a matching free slot, or placed in the backpack when no suitable "
		"slot is free. The hero gains the artifact's bonuses only while it is equipped.");
	R.function<&AdventureServerProxy::grantScroll>("grantScroll",
		{
			{"hero",  "Hero that receives the scroll."},
			{"spell", "Spell written on the scroll, given as its JSON key."}
		}, {},
		"Gives a spell scroll to a hero. While the scroll is carried the hero may cast that spell even without a spellbook. "
		"The scroll occupies an artifact slot like any other artifact.");
	R.function<&AdventureServerProxy::showMessage>("showMessage",
		{
			{"player",     "Player who should see the message. Other players see nothing."},
			{"text",       "The message text, built with MetaString so it can be translated and can embed names and numbers."},
			{"components", "Optional icons shown under the text, such as awarded resources or artifacts. Omit for a plain text box."},
			{"soundID",    "Optional sound to play when the message opens. Omit for the default."},
			{"windowType", "Optional look of the window. Omit to let the engine pick automatically based on the contents."}
		}, {},
		"Pops up a message box for one player. This call does not wait for the player to react - the script continues "
		"immediately and the box is shown at the next opportunity. Use it for notifications, not for questions.");
}

void AdventureServerProxy::setMapVariable(IGameEventCallback & object, const std::string & name, const JsonNode & value)
{
	object.setScriptVariable(ModScope::scopeMap(), name, value);
}

void AdventureServerProxy::removeObject(IGameEventCallback & object, const CGObjectInstance & target)
{
	object.removeObject(&target, target.getOwner());
}

int AdventureServerProxy::random(IGameEventCallback & object, int lower, int upper)
{
	return object.getRandomGenerator().nextInt(lower, upper);
}

void AdventureServerProxy::giveExperience(IGameEventCallback & object, const CGHeroInstance & hero, int64_t amount)
{
	object.giveExperience(&hero, amount);
}

void AdventureServerProxy::setOwner(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor owner)
{
	object.setOwner(&target, owner);
}

void AdventureServerProxy::grantSpell(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell)
{
	object.changeSpells(&hero, true, {spell});
}

void AdventureServerProxy::takeSpell(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell)
{
	object.changeSpells(&hero, false, {spell});
}

void AdventureServerProxy::grantPrimarySkill(IGameEventCallback & object, const CGHeroInstance & hero, PrimarySkill skill, int amount)
{
	object.changePrimSkill(&hero, skill, amount, ChangeValueMode::RELATIVE);
}

void AdventureServerProxy::grantSecondarySkill(IGameEventCallback & object, const CGHeroInstance & hero, SecondarySkill skill, int level)
{
	object.changeSecSkill(&hero, skill, level, ChangeValueMode::RELATIVE);
}

void AdventureServerProxy::grantArtifact(IGameEventCallback & object, const CGHeroInstance & hero, ArtifactID artifact)
{
	object.giveHeroNewArtifact(&hero, artifact, ArtifactPosition::FIRST_AVAILABLE);
}

void AdventureServerProxy::grantScroll(IGameEventCallback & object, const CGHeroInstance & hero, SpellID spell)
{
	object.giveHeroNewScroll(&hero, spell, ArtifactPosition::FIRST_AVAILABLE);
}

void AdventureServerProxy::showMessage(IGameEventCallback & object, PlayerColor player, const LuaMetaString & text,
	const std::optional<std::vector<LuaComponent>> & components, const std::optional<int> & soundID, const std::optional<int> & windowType)
{
	InfoWindow iw;
	iw.player = player;
	iw.text = text.toMetaString();
	if(components)
		for(const auto & component : *components)
			iw.components.push_back(component.toComponent());
	if(soundID)
		iw.soundID = static_cast<ui16>(*soundID);
	if(windowType)
		iw.type = static_cast<EInfoWindowMode>(*windowType);

	object.showInfoDialog(&iw);
}

}
