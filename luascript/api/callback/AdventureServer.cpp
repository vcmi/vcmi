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
#include "../../LuaStack.h"

#include "../LuaComponent.h"
#include "../../../lib/GameLibrary.h"
#include "../LuaMetaString.h"

#include <vcmi/Artifact.h>
#include <vcmi/Creature.h>
#include <vcmi/ResourceType.h>
#include <vcmi/Skill.h>
#include <vcmi/spells/Spell.h>

#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGObjectInstance.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapObjects/Quest.h"
#include "../../../lib/mapObjects/army/CArmedInstance.h"
#include "../../../lib/mapObjects/army/CCreatureSet.h"
#include "../../../lib/mapObjects/army/CStackBasicDescriptor.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/networkPacks/ArtifactLocation.h"
#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/StackLocation.h"

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
	R.function<&AdventureServerProxy::finishQuestOrRemoveObject>("finishQuestOrRemoveObject",
		{{"target", "The quest source (seer hut / quest guard) that just finished its quest."}}, {},
		"Ends the current quest of the given object. A seer hut's active quest is marked complete and cleared, "
		"without removing the hut itself; a quest guard is removed from the map. Errors when the object is not "
		"a quest source - use removeObject for plain events and pandoras.");
	R.function<&AdventureServerProxy::markQuestProposed>("markQuestProposed",
		{
			{"target", "The quest source (seer hut / quest guard) to mark."},
			{"player", "Player who has now seen the quest proposed."}
		}, {},
		"Internal plumbing for registerQuest: remembers that a player has already been offered this quest, "
		"so a later visit shows the progression text instead of the proposal text again.");
	R.function<&AdventureServerProxy::addToQuestLog>("addToQuestLog",
		{
			{"target", "The quest source (seer hut / quest guard) to add."},
			{"player", "Player whose quest log gains the entry."}
		}, {},
		"Adds the object's active quest to a player's in-game quest log. Calling it again for a quest already "
		"in the log does nothing.");
	R.function<&AdventureServerProxy::setQuestHintText>("setQuestHintText",
		{
			{"target", "The quest source (seer hut / quest guard) whose hint changes."},
			{"text", "New hover / quest-log text for the object's active quest."}
		}, {},
		"Internal plumbing for the setQuestHint helper. Scripts should call setQuestHint instead.");
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
	R.function<&AdventureServerProxy::giveResource>("giveResource",
		{
			{"player",   "Player whose treasury changes."},
			{"resource", "Resource to change, as returned by Services:getResourceByName."},
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
			{"spell", "Spell to teach, as returned by Services:getSpellByName."}
		}, {},
		"Teaches a spell to a hero, writing it into the hero's spellbook. The hero needs a spellbook for the spell to be "
		"usable in combat. Teaching a spell the hero already knows does nothing.");
	R.function<&AdventureServerProxy::takeSpell>("takeSpell",
		{
			{"hero",  "Hero that forgets the spell."},
			{"spell", "Spell to remove, as returned by Services:getSpellByName."}
		}, {},
		"Removes a spell from a hero's spellbook. Does nothing if the hero did not know the spell.");
	R.function<&AdventureServerProxy::grantPrimarySkill>("grantPrimarySkill",
		{
			{"hero",   "Hero whose skill changes."},
			{"skill",  "Primary skill to change; use ENUM.PrimarySkill."},
			{"amount", "How many points to add. Use a negative number to reduce the skill; it is clamped at zero and never goes negative."}
		}, {},
		"Permanently raises or lowers one of a hero's four primary skills.");
	R.function<&AdventureServerProxy::grantSecondarySkill>("grantSecondarySkill",
		{
            {"hero",  "Hero that learns the skill."},
			{"skill", "Secondary skill to grant, as returned by Services:getSecondarySkillByName."},
			{"level", "Mastery to move to: 1 = basic, 2 = advanced, 3 = expert."}
		}, {},
        "Teaches a secondary skill to a hero, or changes it to the given mastery. A hero who has no free skill slots left will not learn a brand-new skill.");
	R.function<&AdventureServerProxy::grantArtifact>("grantArtifact",
		{
			{"hero",     "Hero that receives the artifact."},
			{"artifact", "Artifact to give, as returned by Services:getArtifactByName."}
		}, {},
		"Gives an artifact to a hero. It is equipped in a matching free slot, or placed in the backpack when no suitable "
		"slot is free. The hero gains the artifact's bonuses only while it is equipped.");
	R.function<&AdventureServerProxy::grantScroll>("grantScroll",
		{
			{"hero",  "Hero that receives the scroll."},
			{"spell", "Spell written on the scroll, as returned by Services:getSpellByName."}
		}, {},
		"Gives a spell scroll to a hero. While the scroll is carried the hero may cast that spell even without a spellbook. "
		"The scroll occupies an artifact slot like any other artifact.");
	R.function<&AdventureServerProxy::takeArtifact>("takeArtifact",
		{
			{"hero",     "Hero that loses the artifact."},
			{"artifact", "Artifact to remove, as returned by Services:getArtifactByName."}
		}, {},
		"Removes an artifact from a hero, whether it is equipped or sitting in the backpack. If the artifact is a part of an "
		"assembled combination artifact, the combination is taken apart first and the remaining parts stay with the hero. "
		"Does nothing if the hero does not own the artifact.");
	R.function<&AdventureServerProxy::grantCreatures>("grantCreatures",
		{
			{"hero",     "Hero whose army grows."},
			{"creature", "Creature to add, as returned by Services:getCreatureByName."},
			{"count",    "How many creatures to add. They join an existing stack of the same creature, or take a new army slot; if the army is full of other creatures the new ones are lost."}
		}, {},
		"Adds creatures to a hero's army.");
	R.function<&AdventureServerProxy::takeCreatures>("takeCreatures",
		{
			{"hero",     "Hero whose army shrinks."},
			{"creature", "Creature to remove, as returned by Services:getCreatureByName."},
			{"count",    "How many to remove. If the hero has fewer, all of them are removed. Emptied stacks disappear."}
		}, {},
		"Removes creatures of one type from a hero's army.");
	R.function<&AdventureServerProxy::grantWarMachine>("grantWarMachine",
		{
			{"hero",    "Hero that receives the war machine."},
			{"machine", "War machine to give, as returned by Services:getArtifactByName."}
		}, {},
		"Gives a war machine to a hero, placing it in its dedicated equipment slot. Has no effect if the hero already "
		"carries a war machine in that slot.");
	R.function<&AdventureServerProxy::takeWarMachine>("takeWarMachine",
		{
			{"hero",    "Hero that loses the war machine."},
			{"machine", "War machine to remove, as returned by Services:getArtifactByName."}
		}, {},
		"Removes a war machine from a hero. Does nothing if the hero did not carry it.");
	R.function<&AdventureServerProxy::grantSpellbook>("grantSpellbook",
		{
			{"hero", "Hero that receives the spellbook."}
		}, {},
		"Gives a spellbook to a hero, without which learned spells cannot be cast in combat. Does nothing if the hero "
		"already has a spellbook.");
	R.function<&AdventureServerProxy::takeSpellbook>("takeSpellbook",
		{
			{"hero", "Hero that loses the spellbook."}
		}, {},
		"Removes the hero's spellbook. The hero keeps the list of learned spells but can no longer cast them until given a "
		"spellbook again. Does nothing if the hero had no spellbook.");
	R.function<&AdventureServerProxy::grantMorale>("grantMorale",
		{
			{"hero",   "Hero that receives the morale change."},
			{"amount", "Morale points to add; use a negative number to lower morale."}
		}, {},
		"Gives a hero a temporary morale bonus that lasts until the end of the hero's next battle. "
		"Repeated calls each add another separate bonus rather than replacing the previous one.");
	R.function<&AdventureServerProxy::grantLuck>("grantLuck",
		{
			{"hero",   "Hero that receives the luck change."},
			{"amount", "Luck points to add; use a negative number to lower luck."}
		}, {},
		"Gives a hero a temporary luck bonus that lasts until the end of the hero's next battle. "
		"Repeated calls each add another separate bonus rather than replacing the previous one.");
	R.function<&AdventureServerProxy::grantSpellPoints>("grantSpellPoints",
		{
			{"hero",   "Hero whose spell points change."},
			{"amount", "Spell points involved in the change."},
			{"mode",   "0 adds the amount, 1 subtracts it (clamped at zero), 2 sets the total to the amount."}
		}, {},
		"Changes a hero's remaining spell points. The mode selects whether the amount is added, subtracted or set as the new total.");
	R.function<&AdventureServerProxy::grantMovementPoints>("grantMovementPoints",
		{
			{"hero",   "Hero whose movement points change."},
			{"amount", "Movement points involved in the change."},
			{"mode",   "0 adds the amount, 1 subtracts it (clamped at zero), 2 sets the total to the amount."}
		}, {},
		"Changes a hero's remaining movement points for the current turn. The mode selects whether the amount is added, subtracted or set.");
	R.function<&AdventureServerProxy::grantCreaturesToHire>("grantCreaturesToHire",
		{
			{"town",  "Town whose pool of creatures available to hire changes."},
			{"level", "Creature tier (0-7) whose available pool is modified."},
			{"count", "How many extra creatures become available to hire at that tier. Use a negative number to reduce the pool; it is clamped at zero."}
		}, {},
		"Changes the number of creatures available to hire at one tier of a town on the map.");
	R.function<&AdventureServerProxy::constructBuilding>("constructBuilding",
		{
			{"town",     "Town that gains the building."},
			{"building", "Identifier of the building to erect."}
		}, {},
		"Erects a building in a town for free, ignoring the usual cost and prerequisites.");
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
	R.function<&AdventureServerProxy::spawnDialog>("spawnDialog",
		{
			{"player",     "Player who must answer the dialog."},
			{"text",       "The dialog text, built with MetaString."},
			{"mode",       "0 shows a plain acknowledge box; any other value shows a yes/no question."},
			{"components", "Optional icons shown under the text."}
		}, {},
		"Internal plumbing for the blocking showQuestion / showRewardsMessage helpers: shows a modal dialog and "
		"registers the query whose reply resumes the paused script. Scripts should call showQuestion / showRewardsMessage instead.");
	R.function<&AdventureServerProxy::spawnCombat>("spawnCombat",
		{
			{"host", "The visited event/pandora whose garrison is replaced with the opposing army."},
			{"hero", "Hero that fights the army."},
			{"army", "List of {count, creatureKey} pairs describing the opposing army."}
		}, {},
		"Internal plumbing for the startCombat helper: replaces the host object's garrison with the given "
		"creatures and starts a battle against the visiting hero. Scripts should call startCombat instead.");
}

void AdventureServerProxy::setMapVariable(IGameEventCallback & object, const std::string & name, const JsonNode & value)
{
	object.setScriptVariable(ModScope::scopeMap(), name, value);
}

void AdventureServerProxy::removeObject(IGameEventCallback & object, const CGObjectInstance & target)
{
	object.removeObject(&target, target.getOwner());
}

void AdventureServerProxy::finishQuestOrRemoveObject(IGameEventCallback & object, const CGObjectInstance & target)
{
	if(dynamic_cast<const QuestGate *>(&target))
	{
		// TODO: decide what finishing a quest means for a passage gate; refuse rather than guess
		logScript->warn("finishQuestOrRemoveObject: Quest Gate '%s' is not yet supported, ignoring", target.getObjectNameTextID());
		return;
	}
	if(dynamic_cast<const QuestGuard *>(&target))
	{
		object.removeObject(&target, target.getOwner());
		return;
	}
	if(dynamic_cast<const SeerHut *>(&target))
	{
		// switches the seer hut to its "empty" state (no active quest) rather than removing the hut itself
		object.setObjPropertyValue(target.id, ObjProperty::SEERHUT_COMPLETE, true);
		return;
	}

	throw LuaApiException("finishQuestOrRemoveObject: object '" + target.getObjectNameTextID() + "' is not a quest source");
}

void AdventureServerProxy::markQuestProposed(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor player)
{
	object.setObjPropertyID(target.id, ObjProperty::SEERHUT_VISITED, player);
}

void AdventureServerProxy::addToQuestLog(IGameEventCallback & object, const CGObjectInstance & target, PlayerColor player)
{
	const auto * questSource = dynamic_cast<const IQuestSource *>(&target);
	if(!questSource)
	{
		logScript->error("addToQuestLog: object '%s' is not a quest source", target.getObjectNameTextID());
		return;
	}
	object.addQuest(player, questSource->getQuestIdentity());
}

void AdventureServerProxy::setQuestHintText(IGameEventCallback & object, const CGObjectInstance & target, const LuaMetaString & text)
{
	object.setQuestHintText(target.id, text.toMetaString());
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

void AdventureServerProxy::giveResource(IGameEventCallback & object, PlayerColor player, const ResourceType & resource, int amount)
{
	object.giveResource(player, resource.getId(), amount);
}

void AdventureServerProxy::grantSpell(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell)
{
	object.changeSpells(&hero, true, {spell.getId()});
}

void AdventureServerProxy::takeSpell(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell)
{
	object.changeSpells(&hero, false, {spell.getId()});
}

void AdventureServerProxy::grantPrimarySkill(IGameEventCallback & object, const CGHeroInstance & hero, PrimarySkill skill, int amount)
{
	object.changePrimSkill(&hero, skill, amount, ChangeValueMode::RELATIVE);
}

void AdventureServerProxy::grantSecondarySkill(IGameEventCallback & object, const CGHeroInstance & hero, const Skill & skill, int level)
{
    object.changeSecSkill(&hero, skill.getId(), level, ChangeValueMode::ABSOLUTE);
}

void AdventureServerProxy::grantArtifact(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & artifact)
{
	object.giveHeroNewArtifact(&hero, artifact.getId(), ArtifactPosition::FIRST_AVAILABLE);
}

void AdventureServerProxy::grantScroll(IGameEventCallback & object, const CGHeroInstance & hero, const spells::Spell & spell)
{
	object.giveHeroNewScroll(&hero, spell.getId(), ArtifactPosition::FIRST_AVAILABLE);
}

namespace
{
// Removes one artifact from a hero, disassembling any combination it is part of first.
void removeHeroArtifact(IGameEventCallback & object, const CGHeroInstance & hero, const ArtifactID & artifact)
{
	if(!hero.hasArt(artifact))
	{
		const auto * assembly = hero.getCombinedArtWithPart(artifact);
		if(assembly)
		{
			DisassembledArtifact da;
			da.al = ArtifactLocation(hero.id, hero.getArtPos(assembly));
			object.sendAndApply(da);
		}
	}
	if(hero.hasArt(artifact))
		object.removeArtifact(ArtifactLocation(hero.id, hero.getArtPos(artifact, false)));
}
}

void AdventureServerProxy::takeArtifact(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & artifact)
{
	removeHeroArtifact(object, hero, artifact.getId());
}

void AdventureServerProxy::grantCreatures(IGameEventCallback & object, const CGHeroInstance & hero, const Creature & creature, int count)
{
	CCreatureSet army;
	army.addToSlot(army.getFreeSlot(), creature.getId(), count);
	object.giveCreatures(&hero, army);
}

void AdventureServerProxy::takeCreatures(IGameEventCallback & object, const CGHeroInstance & hero, const Creature & creature, int count)
{
	object.takeCreatures(hero.id, {CStackBasicDescriptor(creature.getId(), count)});
}

void AdventureServerProxy::grantWarMachine(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & machine)
{
	object.giveHeroNewArtifact(&hero, machine.getId(), ArtifactPosition::FIRST_AVAILABLE);
}

void AdventureServerProxy::takeWarMachine(IGameEventCallback & object, const CGHeroInstance & hero, const Artifact & machine)
{
	removeHeroArtifact(object, hero, machine.getId());
}

void AdventureServerProxy::grantSpellbook(IGameEventCallback & object, const CGHeroInstance & hero)
{
	object.giveHeroNewArtifact(&hero, ArtifactID::SPELLBOOK, ArtifactPosition::FIRST_AVAILABLE);
}

void AdventureServerProxy::takeSpellbook(IGameEventCallback & object, const CGHeroInstance & hero)
{
	removeHeroArtifact(object, hero, ArtifactID::SPELLBOOK);
}

void AdventureServerProxy::grantMorale(IGameEventCallback & object, const CGHeroInstance & hero, int amount)
{
	Bonus bonus(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OTHER, amount, BonusSourceID());
	GiveBonus gb(GiveBonus::ETarget::OBJECT, hero.id, bonus);
	object.giveHeroBonus(&gb);
}

void AdventureServerProxy::grantLuck(IGameEventCallback & object, const CGHeroInstance & hero, int amount)
{
	Bonus bonus(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OTHER, amount, BonusSourceID());
	GiveBonus gb(GiveBonus::ETarget::OBJECT, hero.id, bonus);
	object.giveHeroBonus(&gb);
}

void AdventureServerProxy::grantSpellPoints(IGameEventCallback & object, const CGHeroInstance & hero, int amount, int mode)
{
	int result = amount; // mode 2: set the total directly
	if(mode == 0)
		result = hero.mana + amount;
	else if(mode == 1)
		result = std::max(0, hero.mana - amount);

	object.setManaPoints(hero.id, result);
}

void AdventureServerProxy::grantMovementPoints(IGameEventCallback & object, const CGHeroInstance & hero, int amount, int mode)
{
	int current = hero.movementPointsRemaining();
	int result = amount; // mode 2: set the total directly
	if(mode == 0)
		result = current + amount;
	else if(mode == 1)
		result = std::max(0, current - amount);

	object.setMovePoints(hero.id, result);
}

void AdventureServerProxy::grantCreaturesToHire(IGameEventCallback & object, const CGTownInstance & town, int level, int count)
{
	if(level < 0 || level >= static_cast<int>(town.creatures.size()))
	{
		logScript->error("grantCreaturesToHire: town '%s' has no creature tier %d", town.getNameTextID(), level);
		return;
	}

	SetAvailableCreatures pack;
	pack.tid = town.id;
	pack.creatures = town.creatures;
	pack.creatures[level].first = std::max(0, static_cast<int>(pack.creatures[level].first) + count);

	object.sendAndApply(pack);
}

void AdventureServerProxy::constructBuilding(IGameEventCallback & object, const CGTownInstance & town, BuildingID building)
{
	object.buildStructureForced(town.id, building);
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

void AdventureServerProxy::spawnDialog(IGameEventCallback & object, PlayerColor player, const LuaMetaString & text,
	int mode, const std::optional<std::vector<LuaComponent>> & components)
{
	// mode 0 is an acknowledge-only reward box; anything else is a yes/no question.
	BlockingDialog bd(mode != 0, false);
	bd.player = player;
	bd.text = text.toMetaString();
	if(components)
		for(const auto & component : *components)
			bd.components.push_back(component.toComponent());

	object.showScriptDialog(&bd);
}

void AdventureServerProxy::spawnCombat(IGameEventCallback & object, const CGObjectInstance & host, const CGHeroInstance & hero, const JsonNode & army)
{
	// The visited event/pandora is reused as the opposing army: its own garrison is discarded and the
	// event's creatures are placed into it, then the hero fights it in place.
	const auto * armedHost = dynamic_cast<const CArmedInstance *>(&host);
	if(!armedHost)
	{
		logScript->error("startCombat: host object '%s' can not hold an army", host.getObjectNameTextID());
		return;
	}

	for(int guard = 0; guard < GameConstants::ARMY_SIZE && !armedHost->Slots().empty(); ++guard)
		object.eraseStack(StackLocation(armedHost->id, armedHost->Slots().begin()->first), true);

	int slotIndex = 0;
	for(const auto & slot : army.Vector())
	{
		if(slotIndex >= GameConstants::ARMY_SIZE)
			break;

		const auto & entry = slot.Vector();
		if(entry.size() >= 2 && entry[0].Integer() > 0)
		{
			CreatureID creature = CreatureID::decode(entry[1].String());
			if(creature.hasValue())
				object.insertNewStack(StackLocation(armedHost->id, SlotID(slotIndex)), creature.toCreature(), entry[0].Integer());
		}
		++slotIndex;
	}

	object.startBattle(&hero, armedHost);
}

}
