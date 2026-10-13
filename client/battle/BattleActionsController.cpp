/*
 * BattleActionsController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionsController.h"

#include "BattleFieldController.h"
#include "BattleHero.h"
#include "BattleInterface.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "BattleWindow.h"
#include "ClientCommandEntries.h"
#include "LibActionEntry.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/CIntObject.h"
#include "../gui/CursorHandler.h"
#include "../gui/WindowHandler.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/actions/BattleActionType.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/battle/DamageEstimationTexts.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/effects/Effect.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/texts/CGeneralTextHandler.h"

static std::string formatWithStackName(const std::string & textID, const CStack * stack)
{
	MetaString result = MetaString::createFromTextID(textID);
	result.replaceName(stack->unitType()->getId(), stack->getCount());
	return result.toString(&GAME->translator());
}

static std::string formatPlural(int amount, const std::string & baseTextID)
{
	return DamageEstimationTexts::plural({amount, amount}, baseTextID).toString(&GAME->translator());
}

static std::string prepareSpellEffectText(int gnrlTextID, const spells::effects::SpellEffectValue & value,
										  const std::string & spellName, const std::string & targetName)
{
	auto templateText = MetaString::createFromTextID("core.genrltxt", gnrlTextID);
	if (!spellName.empty())
		templateText.replaceRawString(spellName);
	if (!targetName.empty())
		templateText.replaceRawString(targetName);

	std::string baseText = templateText.toString(&GAME->translator());

	if(value.unitsDelta > 0)
	{
		auto unitTypeName = value.unitsDelta == 1 ? value.unitType->getNameSingularTranslated()
												  : value.unitType->getNamePluralTranslated();
		return baseText +" (+ "+ std::to_string(value.unitsDelta) +" "+ unitTypeName +")";
	}

	if(value.hpDelta == 0)
		return baseText;

	std::string outputString;
	if(value.hpDelta > 0)
	{
		const std::string baseTextID = gnrlTextID == 549 ? "vcmi.battleWindow.sacrificeAcquiredHealth" : "vcmi.battleWindow.healValuePreview"; //sacrifice spell
		outputString = DamageEstimationTexts::healthGain(value.hpDelta, baseTextID).toString(&GAME->translator());
	}
	else
	{
		outputString = formatPlural(value.hpDelta * -1, "vcmi.battleWindow.damageEstimation.damage");
		if(value.unitsDelta < 0)
			outputString += ", "+ formatPlural(value.unitsDelta * -1, "vcmi.battleWindow.damageEstimation.kills");
	}

	return baseText +" ("+ outputString +")";
}

/// Entry of an action kind that the PossiblePlayerBattleAction switches of the controller handle
class BattleActionsController::LegacyEntry final : public IBattleActionEntry
{
	BattleActionsController & controller;

public:
	const PossiblePlayerBattleAction action;

	LegacyEntry(BattleActionsController & controller, const PossiblePlayerBattleAction & action)
		: controller(controller)
		, action(action)
	{
	}

	int getPriority(const CStack * actor, const CStack * target) const override
	{
		return controller.actionGetPriority(action, actor, target);
	}

	bool isLegal(const BattleHex & hex) const override
	{
		return controller.actionIsLegal(action, hex);
	}

	BattleActionPreview preview(const BattleHex & hex) const override
	{
		if(controller.actionIsLegal(action, hex))
			return {controller.actionGetCursor(action, hex), controller.actionGetStatusMessage(action, hex), {}, {}};
		return {"combatBlocked", controller.actionGetStatusMessageBlocked(action, hex), {}, {}};
	}

	void realize(const BattleHex & hex) const override
	{
		controller.actionRealize(action, hex);
	}

	SpellID getSpell() const override
	{
		return action.spell();
	}

	std::optional<UnitActionButton> getPanelButton() const override
	{
		if(action.spellcast())
			return UnitActionButton{6, action.spell(), {}, {}};

		switch(action.get())
		{
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
				return UnitActionButton{4, SpellID::NONE, ImagePath::builtin("battle/actionGenie"), "vcmi.battle.action.genie"};
			default:
				return std::nullopt;
		}
	}
};

bool BattleActionsController::isLegacyAction(const IBattleActionEntry & entry, PossiblePlayerBattleAction::Actions kind)
{
	const auto * legacy = dynamic_cast<const LegacyEntry *>(&entry);
	return legacy && legacy->action.get() == kind;
}

bool BattleActionsController::isLibAction(const IBattleActionEntry & entry, EActionType actionType)
{
	const auto * lib = dynamic_cast<const LibActionEntry *>(&entry);
	return lib && lib->getType() == BattleActionType::find(actionType);
}

BattleActionsController::BattleActionsController(BattleInterface & owner):
	owner(owner),
	selectedStack(nullptr),
	heroSpellToCast(nullptr)
{
}

void BattleActionsController::endCastingSpell()
{
	if(heroSpellToCast)
	{
		heroSpellToCast.reset();
		owner.windowObject->blockUI(false);
	}

	if(monsterCaster)
	{
		monsterCaster = nullptr;
		owner.stacksController->activateStack();
	}
	monsterSpellTargets.clear();

	if(owner.stacksController->getActiveStack())
	{
		setEntries(getPossibleActionsForStack(owner.stacksController->getActiveStack())); //restore actions after they were cleared
		owner.windowObject->setPossibleActions(possibleActions);
	}

	selectedStack = nullptr;
	cachedSelection.reset();
	ENGINE->fakeMouseMove();
}

bool BattleActionsController::isActiveStackSpellcaster() const
{
	const CStack * casterStack = owner.stacksController->getActiveStack();
	if (!casterStack)
		return false;

	bool spellcaster = casterStack->hasBonusOfType(BonusType::SPELLCASTER);
	return (spellcaster && casterStack->canCast());
}

void BattleActionsController::enterCreatureCastingMode()
{
	//silently check for possible errors
	if (owner.isInTacticsMode())
		return;

	//hero is casting a spell
	if (heroSpellToCast)
		return;

	if (!owner.stacksController->getActiveStack())
		return;

	if(owner.getBattle()->battleCanTargetEmptyHex(owner.stacksController->getActiveStack()))
	{
		auto actionFilterPredicate = [](const std::shared_ptr<const IBattleActionEntry> & x)
		{
			return !isLibAction(*x, EActionType::SHOOT);
		};

		BattleActionEntries entries = possibleActions;
		vstd::erase_if(entries, actionFilterPredicate);
		setEntries(entries);
		ENGINE->fakeMouseMove();
		return;
	}

	if (!isActiveStackSpellcaster())
		return;

	for(const auto & action : possibleActions)
	{
		if (!isLegacyAction(*action, PossiblePlayerBattleAction::NO_LOCATION))
			continue;

		const spells::Caster * caster = owner.stacksController->getActiveStack();
		const CSpell * spell = action->getSpell().toSpell();

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(owner.getBattle().get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		const bool isCastingPossible = m->canBeCastAt(target);

		if (isCastingPossible)
		{
			owner.giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, spell->getId());
			selectedStack = nullptr;

			ENGINE->cursor().set(Cursor::Combat::POINTER);
		}
		return;
	}

	BattleActionEntries entries = getPossibleActionsForStack(owner.stacksController->getActiveStack());

	auto actionFilterPredicate = [](const std::shared_ptr<const IBattleActionEntry> & x)
	{
		return x->getSpell() == SpellID::NONE;
	};

	vstd::erase_if(entries, actionFilterPredicate);
	setEntries(entries);
	ENGINE->fakeMouseMove();
}

BattleActionEntries BattleActionsController::getPossibleActionsForStack(const CStack *stack)
{
	BattleClientInterfaceData data; //hard to get rid of these things so for now they're required data to pass

	for(const auto & spell : creatureSpells)
		data.creatureSpellsToCast.push_back(spell->id);

	data.tacticsMode = owner.isInTacticsMode();

	BattleActionEntries entries;
	for(const auto & action : owner.getBattle()->getClientActionsForStack(stack, data))
		entries.push_back(std::make_shared<LegacyEntry>(*this, action));

	std::vector<ActionOption> options;
	BattleActionType::collectAllOptions(*owner.getBattle(), *stack, options);
	for(const auto & option : options)
		entries.push_back(std::make_shared<LibActionEntry>(owner, *stack, option));

	if(data.tacticsMode)
		entries.push_back(std::make_shared<TacticsUnitSelectionEntry>(owner));

	entries.push_back(std::make_shared<HeroInfoEntry>(owner));
	entries.push_back(std::make_shared<CreatureInfoEntry>(owner));

	return entries;
}

int BattleActionsController::actionGetPriority(PossiblePlayerBattleAction item, const CStack * stack, const CStack * targetStack) const
{
	switch(item.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::NO_LOCATION:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::SACRIFICE:
			if(!stack->hasBonusOfType(BonusType::NO_SPELLCAST_BY_DEFAULT) && targetStack != nullptr)
			{
				PlayerColor stackOwner = owner.getBattle()->battleGetOwner(targetStack);
				bool enemyTargetingPositiveSpellcast = item.spell().toSpell()->isPositive() && stackOwner != owner.curInt->playerID;
				bool friendTargetingNegativeSpellcast = item.spell().toSpell()->isNegative() && stackOwner == owner.curInt->playerID;

				if(!enemyTargetingPositiveSpellcast && !friendTargetingNegativeSpellcast)
					return 1;
			}
			return 100; //bottom priority

			break;
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return 2;
			break;
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
			return 9;
			break;
		case PossiblePlayerBattleAction::TELEPORT:
			return 15;
			break;
		default:
			assert(0);
			return 200;
			break;
	}
}

void BattleActionsController::castThisSpell(SpellID spellID)
{
	heroSpellToCast = std::make_shared<BattleAction>();
	heroSpellToCast->actionType = EActionType::HERO_SPELL;
	heroSpellToCast->spell = spellID;
	heroSpellToCast->stackNumber = -1;
	heroSpellToCast->side = owner.curInt->cb->getBattle(owner.getBattleID())->battleGetMySide();

	//choosing possible targets
	const CGHeroInstance *castingHero = (owner.attackingHeroInstance->tempOwner == owner.curInt->playerID) ? owner.attackingHeroInstance : owner.defendingHeroInstance;
	assert(castingHero); // code below assumes non-null hero
	PossiblePlayerBattleAction spellSelMode = owner.getBattle()->getCasterAction(spellID.toSpell(), castingHero, spells::Mode::HERO);

	if (spellSelMode.get() == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
	{
		heroSpellToCast->aimToHex(BattleHex::INVALID);
		owner.curInt->cb->battleMakeSpellAction(owner.getBattleID(), *heroSpellToCast);
		endCastingSpell();
	}
	else
	{
		setEntries({std::make_shared<LegacyEntry>(*this, spellSelMode)}); //only this one action can be performed at the moment
		ENGINE->fakeMouseMove();//update cursor
	}

	owner.windowObject->blockUI(true);
}

const CSpell * BattleActionsController::getHeroSpellToCast( ) const
{
	if (heroSpellToCast)
		return heroSpellToCast->spell.toSpell();
	return nullptr;
}

const CSpell * BattleActionsController::getStackSpellToCast(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return nullptr;

	if (!owner.stacksController->getActiveStack())
		return nullptr;

	if (!hoveredHex.isValid())
		return nullptr;

	if(owner.stacksController->getActiveStack()->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
	{
		auto bonus = owner.stacksController->getActiveStack()->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
		return bonus->subtype.as<SpellID>().toSpell();
	}

	const auto & entry = getSelection(hoveredHex).entry;

	if (!entry || entry->getSpell() == SpellID::NONE)
		return nullptr;

	return entry->getSpell().toSpell();
}

const CSpell * BattleActionsController::getCurrentSpell(const BattleHex & hoveredHex)
{
	if (getHeroSpellToCast())
		return getHeroSpellToCast();
	return getStackSpellToCast(hoveredHex);
}

const CStack * BattleActionsController::getStackForHex(const BattleHex & hoveredHex)
{
	const CStack * shere = owner.getBattle()->battleGetStackByPos(hoveredHex, true);
	if(shere)
		return shere;
	return owner.getBattle()->battleGetStackByPos(hoveredHex, false);
}

std::string BattleActionsController::actionGetCursor(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
			return "castSpell";

		case PossiblePlayerBattleAction::TELEPORT:
			if(!selectedStack)
				return "castSpell";
			else
				return "combatTeleport";

		case PossiblePlayerBattleAction::SACRIFICE:
			if(!selectedStack)
				return "castSpell";
			else
				return "combatSacrifice";
	}
	assert(0);
	return "combatBlocked";
}

std::string BattleActionsController::actionGetStatusMessage(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		{
			const CSpell * spell = action.spell().toSpell();

			auto spellEffectValue =
					owner.getBattle()->getSpellEffectValue(spell, getCurrentSpellcaster(), getCurrentCastMode(), targetHex);

			// "Cast %s on %s" plus dmg and kills info or how many units are risen/summoned
			return prepareSpellEffectText(27, *spellEffectValue, spell->getNameTranslated(), targetStack->getName());
		}

		case PossiblePlayerBattleAction::ANY_LOCATION:
		{
			const CSpell * spell = action.spell().toSpell();
			if(!spell)
				return {};

			auto spellEffectValue =
					owner.getBattle()->getSpellEffectValue(spell, getCurrentSpellcaster(), getCurrentCastMode(), targetHex);

			// "Cast %s" plus dmg and kills info
			return prepareSpellEffectText(26, *spellEffectValue, spell->getNameTranslated(), "");
		}

		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
		{
			const CSpell * spell = getStackSpellToCast(targetHex);
			assert(spell);

			auto spellEffectValue =
					owner.getBattle()->getSpellEffectValue(spell, getCurrentSpellcaster(), getCurrentCastMode(), targetHex);

			// "Cast %s on %s" plus dmg and kills info
			return prepareSpellEffectText(27, *spellEffectValue, spell->getNameTranslated(), targetStack->getName());
		}

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
			return formatWithStackName("core.genrltxt.301", targetStack); //Cast a spell on %s

		case PossiblePlayerBattleAction::TELEPORT:
		{
			if(!selectedStack) // Phase 1: hovering over unit to teleport
			{
				const CSpell * spell = action.spell().toSpell();
				if(!spell || !targetStack)
					return {};
				auto spellEffectValue = owner.getBattle()->getSpellEffectValue(spell, getCurrentSpellcaster(), getCurrentCastMode(), targetHex);
				return prepareSpellEffectText(27, *spellEffectValue, spell->getNameTranslated(), targetStack->getName());
			}
			return LIBRARY->generaltexth->allTexts[25]; //Teleport Here
		}

		case PossiblePlayerBattleAction::OBSTACLE:
			return LIBRARY->generaltexth->allTexts[550];

		case PossiblePlayerBattleAction::SACRIFICE:
		{
			const CSpell * spell = action.spell().toSpell();
			if(!spell)
				return {};

			spells::Target aim;
			if(selectedStack)
				aim.emplace_back(selectedStack);
			aim.emplace_back(targetHex);
			auto spellEffectValue =
					owner.getBattle()->getSpellEffectValue(spell, getCurrentSpellcaster(), getCurrentCastMode(), aim);

			if(!selectedStack) // Phase 1: hovering over unit to heal or resurrect
				return prepareSpellEffectText(27, *spellEffectValue, spell->getNameTranslated(), targetStack ? targetStack->getName() : "");

			//sacrifice the %s
			return prepareSpellEffectText(549, *spellEffectValue, "", targetStack->getName());
		}

		case PossiblePlayerBattleAction::FREE_LOCATION:
		{
			MetaString text = MetaString::createFromTextID("core.genrltxt.26"); //Cast %s
			text.replaceName(action.spell());
			return text.toString(&GAME->translator());
		}
	}
	assert(0);
	return "";
}

std::string BattleActionsController::actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	switch (action.get())
	{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return LIBRARY->generaltexth->allTexts[23];
			break;
		case PossiblePlayerBattleAction::TELEPORT:
			if(!selectedStack)
				return LIBRARY->generaltexth->allTexts[23];
			return LIBRARY->generaltexth->allTexts[24]; //Invalid Teleport Destination
			break;
		case PossiblePlayerBattleAction::SACRIFICE:
			if(!selectedStack)
				return LIBRARY->generaltexth->allTexts[23];
			return LIBRARY->generaltexth->allTexts[543]; //choose army to sacrifice
			break;
		case PossiblePlayerBattleAction::FREE_LOCATION:
		{
			MetaString text = MetaString::createFromTextID("core.genrltxt.181"); //No room to place %s here
			text.replaceName(action.spell());
			return text.toString(&GAME->translator());
		}
		default:
			return "";
	}
}

bool BattleActionsController::actionIsLegal(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);
	bool targetStackOwned = targetStack && targetStack->unitOwner() == owner.curInt->playerID;

	switch (action.get())
	{
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
			{
				const CStack * currentStack = owner.stacksController->getActiveStack();
				if (!currentStack || !targetStack)
					return false;

				if (targetStack == currentStack)
					return false;

				return owner.getBattle()->battleCanAttackHex(currentStack, targetHex) && isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);
			}
		case PossiblePlayerBattleAction::NO_LOCATION:
			return false;

		case PossiblePlayerBattleAction::ANY_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);

		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			return !selectedStack && targetStack && isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);

		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			if(targetStack && targetStackOwned && targetStack != owner.stacksController->getActiveStack() && targetStack->alive()) //only positive spells for other allied creatures
			{
				SpellID spellID = owner.getBattle()->getRandomBeneficialSpell(CRandomGenerator::getDefault(), owner.stacksController->getActiveStack(), targetStack);
				return spellID != SpellID::NONE;
			}
			return false;

		case PossiblePlayerBattleAction::TELEPORT:
		case PossiblePlayerBattleAction::SACRIFICE:
			if(!selectedStack)
				return targetStack && isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);
			return isCastingPossibleHere(action.spell().toSpell(), selectedStack, targetHex);

		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
			return isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);
	}

	assert(0);
	return false;
}

void BattleActionsController::actionRealize(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
		{
			auto stack = owner.stacksController->getActiveStack();
			BattleHex attackFromHex = owner.getBattle()->fromWhichHexAttack(stack, targetHex, owner.fieldController->selectAttackDirection(targetHex));
			if (attackFromHex.isValid())
			{
				BattleAction command = BattleAction::makeWalkAndCast(stack, attackFromHex, targetStack, getStackSpellToCast(targetHex)->id);
				owner.sendCommand(command, stack);
			}
			return;
		}

		case PossiblePlayerBattleAction::SACRIFICE:
		{
			if(!selectedStack)
			{
				// Phase 1: select dead unit to resurrect
				monsterCaster = owner.stacksController->getActiveStack();
				owner.windowObject->blockUI(true);
				owner.stacksController->deactivateStack();
				if(heroSpellToCast)
					heroSpellToCast->aimToHex(targetHex);
				else
					monsterSpellTargets.push_back(targetHex);
				selectedStack = targetStack;
				return;
			}
			[[fallthrough]];
		}
		case PossiblePlayerBattleAction::TELEPORT:
		{
			if(!selectedStack)
			{
				// Phase 1: select unit to teleport
				monsterCaster = owner.stacksController->getActiveStack();
				owner.windowObject->blockUI(true);
				owner.stacksController->deactivateStack();
				if(heroSpellToCast)
					heroSpellToCast->aimToUnit(targetStack);
				else
					monsterSpellTargets.push_back(targetHex);
				selectedStack = targetStack;
				return;
			}
			[[fallthrough]];
		}
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
		case PossiblePlayerBattleAction::OBSTACLE:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		{
			if(action.get() == PossiblePlayerBattleAction::AIMED_SPELL_CREATURE)
			{
				monsterCaster = owner.stacksController->getActiveStack();
				owner.windowObject->blockUI(true);
				owner.stacksController->deactivateStack();
			}

			if (!heroSpellcastingModeActive())
			{
				if(monsterCaster)
					owner.stacksController->activateStack();

				if (action.spell().hasValue())
				{
					monsterSpellTargets.push_back(targetHex);
					owner.giveCommand(EActionType::MONSTER_SPELL, monsterSpellTargets, action.spell());
				}
				else //unknown random spell
				{
					monsterSpellTargets.push_back(targetHex);
					owner.giveCommand(EActionType::MONSTER_SPELL, monsterSpellTargets);
				}
				endCastingSpell();
			}
			else
			{
				assert(getHeroSpellToCast());
				if(action.get() == PossiblePlayerBattleAction::SACRIFICE)
					heroSpellToCast->aimToUnit(targetStack); //victim
				else
					heroSpellToCast->aimToHex(targetHex);
				owner.curInt->cb->battleMakeSpellAction(owner.getBattleID(), *heroSpellToCast);
				endCastingSpell();
			}
			selectedStack = nullptr;
			return;
		}
	}
	assert(0);
	return;
}

std::shared_ptr<const IBattleActionEntry> BattleActionsController::selectEntry(const BattleHex & targetHex)
{
	auto currentStack = monsterCaster ? monsterCaster : owner.stacksController->getActiveStack();
	assert(currentStack != nullptr);
	assert(targetHex.isValid());

	// creature spellcasting mode of a unit with an area shot and no SHOOT entry leaves no entries
	if(currentStack == nullptr || possibleActions.empty())
		return nullptr;

	const CStack * targetStack = getStackForHex(targetHex);

	std::vector<std::pair<int, std::shared_ptr<const IBattleActionEntry>>> ordered;
	for(const auto & entry : possibleActions)
		ordered.emplace_back(entry->getPriority(currentStack, targetStack), entry);

	std::stable_sort(ordered.begin(), ordered.end(), [](const auto & lhs, const auto & rhs){ return lhs.first < rhs.first; });

	for(const auto & entry : ordered)
	{
		if(entry.second->isLegal(targetHex))
			return entry.second;
	}
	return ordered.front().second;
}

const BattleActionsController::HexSelection & BattleActionsController::getSelection(const BattleHex & hex)
{
	if(!cachedSelection || cachedSelection->hex != hex)
	{
		auto entry = selectEntry(hex);
		BattleActionPreview preview = entry ? entry->preview(hex) : BattleActionPreview{"combatBlocked", "", {}, {}};
		cachedSelection = HexSelection{hex, std::move(entry), std::move(preview)};
	}
	return *cachedSelection;
}

void BattleActionsController::setEntries(BattleActionEntries entries)
{
	possibleActions = std::move(entries);
	cachedSelection.reset();
}

void BattleActionsController::onHexHovered(const BattleHex & hoveredHex)
{
	if (owner.openingPlaying())
	{
		currentConsoleMsg = LIBRARY->generaltexth->translate("vcmi.battleWindow.pressKeyToSkipIntro");
		ENGINE->statusbar()->write(currentConsoleMsg);
		return;
	}

	if (owner.stacksController->getActiveStack() == nullptr && monsterCaster == nullptr)
		return;

	if (hoveredHex == BattleHex::INVALID)
	{
		if (!currentConsoleMsg.empty())
			ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

		currentConsoleMsg.clear();
		ENGINE->cursor().set(Cursor::Combat::BLOCKED);
		return;
	}

	// the battle may have changed since the last hover, e.g. on a fake mouse move after an action
	cachedSelection.reset();
	BattleActionPreview preview = getSelection(hoveredHex).preview;

	if (owner.siegeController && owner.siegeController->isTowerHex(hoveredHex))
	{
		preview.cursor = "combatQuery"; // question cursor over a siege tower
		preview.statusText = LIBRARY->generaltexth->translate("core.genrltxt.156"); // "View arrow tower info."
	}

	ENGINE->cursor().set(preview.cursor);
	const std::string & newConsoleMsg = preview.statusText;

	if (!currentConsoleMsg.empty())
		ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

	if (!newConsoleMsg.empty())
		ENGINE->statusbar()->write(newConsoleMsg);

	currentConsoleMsg = newConsoleMsg;
}

void BattleActionsController::onHexRightHovered(const BattleHex & hoveredHex)
{
	onHoverEnded();
	currentConsoleMsg = secondaryActionText(hoveredHex, true);
	if(!currentConsoleMsg.empty())
	{
		ENGINE->cursor().set(Cursor::Combat::QUERY);
		ENGINE->statusbar()->write(currentConsoleMsg);
	}
}

void BattleActionsController::onHoverEnded()
{
	ENGINE->cursor().set(Cursor::Combat::POINTER);

	if (!currentConsoleMsg.empty())
		ENGINE->statusbar()->clearIfMatching(currentConsoleMsg);

	currentConsoleMsg.clear();
}

std::string BattleActionsController::primaryActionText(const BattleHex & hex)
{
	if(!hex.isAvailable() || (!owner.stacksController->getActiveStack() && !monsterCaster))
		return {};
	const auto & [selectedHex, entry, preview] = getSelection(hex);
	if(!entry || !entry->isLegal(hex))
		return {};

	std::string key;
	if(preview.cursor == "combatMove" || preview.cursor == "combatFly")
		key = "move";
	else if(preview.cursor.starts_with("combatHit"))
		key = "attack";
	else if(preview.cursor == "combatShoot" || preview.cursor == "combatShootPenalty")
		key = "shoot";
	else if(dynamic_cast<const CreatureInfoEntry *>(entry.get()))
		key = "inspect";
	else
		return preview.statusText;
	return LIBRARY->generaltexth->translate("vcmi.controllerPrompt." + key);
}

std::string BattleActionsController::secondaryActionText(const BattleHex & hex, bool persistentInfo) const
{
	if(!hex.isValid())
		return {};
	if(heroSpellcastingModeActive() || creatureSpellcastingModeActive())
		return LIBRARY->generaltexth->translate("vcmi.controllerPrompt.cancel");
	if(owner.getBattle()->battleGetStackByPos(hex, true))
		return LIBRARY->generaltexth->translate(persistentInfo ? "vcmi.controllerPrompt.inspect" : "vcmi.controllerPrompt.quickInspect");
	if(owner.siegeController && owner.siegeController->isTowerHex(hex))
		return LIBRARY->generaltexth->translate(persistentInfo ? "vcmi.controllerPrompt.inspect" : "vcmi.controllerPrompt.quickInspect");
	return {};
}

void BattleActionsController::onHexLeftClicked(const BattleHex & clickedHex, const std::function<void(const CStack *)> & showStackInfo)
{
	if (owner.stacksController->getActiveStack() == nullptr && monsterCaster == nullptr)
		return;

	const auto entry = selectEntry(clickedHex);

	if (!entry || !entry->isLegal(clickedHex))
		return;

	const auto * infoEntry = dynamic_cast<const CreatureInfoEntry *>(entry.get());
	if(infoEntry && showStackInfo)
		showStackInfo(getStackForHex(clickedHex));
	else
		entry->realize(clickedHex);
	cachedSelection.reset();
	ENGINE->statusbar()->clear();
}

void BattleActionsController::tryActivateStackSpellcasting(const CStack * casterStack)
{
	creatureSpells.clear();
	TConstBonusListPtr bl = casterStack->getBonusesOfType(BonusType::SPELLCASTER);

	if(casterStack->canCast() && !bl->empty())
	{
		// faerie dragon can cast only one, randomly selected spell until their next move
		//TODO: faerie dragon type spell should be selected by server
		const auto spellToCast = owner.getBattle()->getRandomCastedSpell(CRandomGenerator::getDefault(), casterStack);

		if(spellToCast.hasValue())
			creatureSpells.push_back(spellToCast.toSpell());
	}

	for(const auto & bonus : *bl)
	{
		if(!bonus->parameters && bonus->subtype.as<SpellID>().hasValue())
			creatureSpells.push_back(bonus->subtype.as<SpellID>().toSpell());
	}
}

const spells::Caster * BattleActionsController::getCurrentSpellcaster() const
{
	if (heroSpellToCast)
		return owner.currentHero();
	else if(monsterCaster)
		return monsterCaster;
	else
		return owner.stacksController->getActiveStack();
}

spells::Mode BattleActionsController::getCurrentCastMode() const
{
	if(heroSpellToCast)
		return spells::Mode::HERO;
	else
		return spells::Mode::CREATURE_ACTIVE;
}

bool BattleActionsController::isCastingPossibleHere(const CSpell * currentSpell, const CStack *targetStack, const BattleHex & targetHex)
{
	assert(currentSpell);
	if (!currentSpell)
		return false;

	auto caster = getCurrentSpellcaster();

	const spells::Mode mode = heroSpellToCast ? spells::Mode::HERO : spells::Mode::CREATURE_ACTIVE;

	spells::Target target;
	if(targetStack)
		target.emplace_back(targetStack);
	target.emplace_back(targetHex);

	spells::BattleCast cast(owner.getBattle().get(), caster, mode, currentSpell);

	auto m = currentSpell->battleMechanics(&cast);
	spells::detail::ProblemImpl problem; //todo: display problem in status bar

	return m->canBeCastAt(target, problem);
}

void BattleActionsController::activateStack()
{
	const CStack * s = owner.stacksController->getActiveStack();
	if(s)
	{
		tryActivateStackSpellcasting(s);

		setEntries(getPossibleActionsForStack(s));
		owner.windowObject->setPossibleActions(possibleActions);
	}
}

void BattleActionsController::onHexRightClicked(const BattleHex & clickedHex, const std::function<void(const CStack *)> & showStackInfo, const std::function<void(const std::string &)> & showTextInfo)
{
	const auto presentText = [&showTextInfo](const std::string & text)
	{
		if(showTextInfo)
			showTextInfo(text);
		else
			CRClickPopup::createAndPush(text);
	};

	bool isCurrentStackInSpellcastMode = creatureSpellcastingModeActive();

	if (heroSpellcastingModeActive() || isCurrentStackInSpellcastMode)
	{
		endCastingSpell();
		presentText(LIBRARY->generaltexth->translate("core.genrltxt.731")); // spell cancelled
		return;
	}

	auto selectedStack = owner.getBattle()->battleGetStackByPos(clickedHex, true);

	if (selectedStack != nullptr)
	{
		if(showStackInfo)
			showStackInfo(selectedStack);
		else
			ENGINE->windows().createAndPushWindow<CStackWindow>(selectedStack, true);
	}
	else if (owner.siegeController && owner.siegeController->isTowerHex(clickedHex))
		presentText(owner.siegeController->getTowersInfoText());

	if (clickedHex == BattleHex::HERO_ATTACKER && owner.attackingHero)
		owner.attackingHero->heroRightClicked();

	if (clickedHex == BattleHex::HERO_DEFENDER && owner.defendingHero)
		owner.defendingHero->heroRightClicked();
}

bool BattleActionsController::heroSpellcastingModeActive() const
{
	return heroSpellToCast != nullptr;
}

bool BattleActionsController::creatureSpellcastingModeActive() const
{
	auto spellcastModePredicate = [](const std::shared_ptr<const IBattleActionEntry> & entry)
	{
		return entry->getSpell() != SpellID::NONE || isLibAction(*entry, EActionType::SHOOT); //for hotkey-eligible SPELL_LIKE_ATTACK creature should have only SHOOT action
	};

	return !possibleActions.empty() && std::all_of(possibleActions.begin(), possibleActions.end(), spellcastModePredicate);
}

bool BattleActionsController::currentActionSpellcasting(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return true;

	if (!owner.stacksController->getActiveStack())
		return false;

	const auto & entry = getSelection(hoveredHex).entry;

	return entry && entry->getSpell() != SpellID::NONE;
}

bool BattleActionsController::currentActionWalkAndCast(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return false;

	if (!owner.stacksController->getActiveStack())
		return false;

	const auto & entry = getSelection(hoveredHex).entry;

	return entry && isLegacyAction(*entry, PossiblePlayerBattleAction::WALK_AND_SPELLCAST);
}

bool BattleActionsController::currentActionHasAttackDirection(const BattleHex & hoveredHex)
{
	if(heroSpellToCast || !owner.stacksController->getActiveStack())
		return false;

	const auto & selection = getSelection(hoveredHex);
	return selection.entry && selection.entry->isLegal(hoveredHex) && selection.preview.cursor.starts_with("combatHit");
}

BattleHexArray BattleActionsController::getShadedHexes(const BattleHex & hoveredHex)
{
	return getSelection(hoveredHex).preview.shadedHexes;
}

std::vector<uint32_t> BattleActionsController::getAffectedUnits(const BattleHex & hoveredHex)
{
	return getSelection(hoveredHex).preview.affectedUnits;
}

void BattleActionsController::setPriorityActions(const BattleActionEntries & actions)
{
	setEntries(actions);
}

void BattleActionsController::resetCurrentStackPossibleActions()
{
	setEntries(getPossibleActionsForStack(owner.stacksController->getActiveStack()));
}
