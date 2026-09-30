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
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/effects/Effect.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/texts/CGeneralTextHandler.h"

struct TextReplacement
{
	std::string placeholder;
	std::string replacement;
};

using TextReplacementList = std::vector<TextReplacement>;

static std::string replacePlaceholders(const std::string & input, const TextReplacementList & format )
{
	MetaString result = MetaString::createFromRawString(input);
	for(const auto & entry : format)
		result.replaceTokenRawString(entry.placeholder, entry.replacement);

	return result.toString(&GAME->translator());
}

static std::string formatWithStackName(const std::string & textID, const CStack * stack)
{
	MetaString result = MetaString::createFromTextID(textID);
	result.replaceName(stack->unitType()->getId(), stack->getCount());
	return result.toString(&GAME->translator());
}

static std::string translatePlural(int amount, const std::string& baseTextID)
{
	if(amount == 1)
		return LIBRARY->generaltexth->translate(baseTextID + ".1");
	return LIBRARY->generaltexth->translate(baseTextID);
}

static std::string formatPluralImpl(int amount, const std::string & amountString, const std::string & baseTextID)
{
	std::string baseString = translatePlural(amount, baseTextID);
	TextReplacementList replacements {
		{ "%d", amountString }
	};

	return replacePlaceholders(baseString, replacements);
}

static std::string formatPlural(int amount, const std::string & baseTextID)
{
	return formatPluralImpl(amount, std::to_string(amount), baseTextID);
}

static std::string formatPlural(DamageRange range, const std::string & baseTextID)
{
	if (range.min == range.max)
		return formatPlural(range.min, baseTextID);

	std::string rangeString = std::to_string(range.min) + " - " + std::to_string(range.max);

	return formatPluralImpl(range.max, rangeString, baseTextID);
}

static std::string formatAttack(const DamageEstimation & estimation, const std::string & creatureName, const std::string & baseTextID, int shotsLeft)
{
	TextReplacementList replacements = {
		{ "%CREATURE", creatureName },
		{ "%DAMAGE", formatPlural(estimation.damage, "vcmi.battleWindow.damageEstimation.damage") },
		{ "%SHOTS", formatPlural(shotsLeft, "vcmi.battleWindow.damageEstimation.shots") },
		{ "%KILLS", formatPlural(estimation.kills, "vcmi.battleWindow.damageEstimation.kills") },
	};

	return replacePlaceholders(LIBRARY->generaltexth->translate(baseTextID), replacements);
}

static std::string formatMeleeAttack(const DamageEstimation & estimation, const std::string & creatureName)
{
	std::string baseTextID = estimation.kills.max == 0 ?
		"vcmi.battleWindow.damageEstimation.melee" :
		"vcmi.battleWindow.damageEstimation.meleeKills";

	return formatAttack(estimation, creatureName, baseTextID, 0);
}

static std::string formatRangedAttack(const DamageEstimation & estimation, const std::string & creatureName, int shotsLeft)
{
	std::string baseTextID = estimation.kills.max == 0 ?
		"vcmi.battleWindow.damageEstimation.ranged" :
		"vcmi.battleWindow.damageEstimation.rangedKills";

	return formatAttack(estimation, creatureName, baseTextID, shotsLeft);
}

static std::string formatRetaliation(const DamageEstimation & estimation, bool mayBeKilled)
{
	if (estimation.damage.max == 0)
		return LIBRARY->generaltexth->translate("vcmi.battleWindow.damageRetaliation.never");

	std::string baseTextID = estimation.kills.max == 0 ?
								 "vcmi.battleWindow.damageRetaliation.damage" :
								 "vcmi.battleWindow.damageRetaliation.damageKills";

	std::string prefixTextID = mayBeKilled ?
		"vcmi.battleWindow.damageRetaliation.may" :
		"vcmi.battleWindow.damageRetaliation.will";

	return LIBRARY->generaltexth->translate(prefixTextID) + formatAttack(estimation, "", baseTextID, 0);
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
		auto val = value.hpDelta;
		TextReplacementList replacements {{ "%d", std::to_string(val) }};
		int64_t correctPluralIndex = val > 3 ? 0 : std::clamp(val, int64_t(1), int64_t(2));
		std::string textTemplateKey;
		if(gnrlTextID == 549) //sacrifice spell
			textTemplateKey = "vcmi.battleWindow.sacrificeAcquiredHealth.";
		else
			textTemplateKey = "vcmi.battleWindow.healValuePreview.";
		outputString = LIBRARY->generaltexth->translate(textTemplateKey + std::to_string(correctPluralIndex));
		outputString = replacePlaceholders(outputString, replacements);
	}
	else
	{
		outputString = formatPlural(value.hpDelta * -1, "vcmi.battleWindow.damageEstimation.damage");
		if(value.unitsDelta < 0)
			outputString += ", "+ formatPlural(value.unitsDelta * -1, "vcmi.battleWindow.damageEstimation.kills");
	}

	return baseText +" ("+ outputString +")";
}

static BattleHex findAttackFromHex(const BattleInterface & owner, const CStack * attacker, const BattleHex & targetHex, bool allowLongWeapon)
{
	if(!attacker || !targetHex.isValid())
		return BattleHex::INVALID;

	const auto preferredDirection = owner.fieldController->selectAttackDirection(targetHex);
	BattleHex attackFromHex = owner.getBattle()->fromWhichHexAttack(attacker, targetHex, preferredDirection, allowLongWeapon);

	if(attackFromHex.isValid())
		return attackFromHex;

	for(int direction = 0; direction < 8; ++direction)
	{
		attackFromHex = owner.getBattle()->fromWhichHexAttack(attacker, targetHex, static_cast<BattleHex::EDir>(direction), allowLongWeapon);
		if(attackFromHex.isValid())
			return attackFromHex;
	}

	return BattleHex::INVALID;
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
			return {controller.actionGetCursor(action, hex), controller.actionGetStatusMessage(action, hex)};
		return {"combatBlocked", controller.actionGetStatusMessageBlocked(action, hex)};
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
			case PossiblePlayerBattleAction::MOVE_STACK:
				return UnitActionButton{0, SpellID::NONE, ImagePath::builtin("battle/actionMove"), "vcmi.battle.action.move"};
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
				return UnitActionButton{1, SpellID::NONE, ImagePath::builtin("battle/actionReturn"), "vcmi.battle.action.return"};
			case PossiblePlayerBattleAction::ATTACK:
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
				return UnitActionButton{2, SpellID::NONE, ImagePath::builtin("battle/actionAttack"), "vcmi.battle.action.attack"};
			case PossiblePlayerBattleAction::SHOOT:
				return UnitActionButton{3, SpellID::NONE, ImagePath::builtin("battle/actionShoot"), "vcmi.battle.action.shoot"};
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
				return UnitActionButton{4, SpellID::NONE, ImagePath::builtin("battle/actionGenie"), "vcmi.battle.action.genie"};
			case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
				return UnitActionButton{5, SpellID::NONE, ImagePath::builtin("battle/actionLongWeapon"), "vcmi.battle.action.attackLongWeapon"};
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
		possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack()); //restore actions after they were cleared
		owner.windowObject->setPossibleActions(possibleActions);
	}

	selectedStack = nullptr;
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
			return !isLegacyAction(*x, PossiblePlayerBattleAction::SHOOT);
		};

		vstd::erase_if(possibleActions, actionFilterPredicate);
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

	possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());

	auto actionFilterPredicate = [](const std::shared_ptr<const IBattleActionEntry> & x)
	{
		return x->getSpell() == SpellID::NONE;
	};

	vstd::erase_if(possibleActions, actionFilterPredicate);
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
		case PossiblePlayerBattleAction::SHOOT:
			if(targetStack == nullptr || targetStack->unitSide() == stack->unitSide() || !targetStack->alive())
				return 100; //bottom priority

			return 4;
			break;
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			return 5;
			break;
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
			return 6;
			break;
		case PossiblePlayerBattleAction::ATTACK:
			return 7;
			break;
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			return 8;
			break;
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
			return 9;
			break;
		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			return 10;
			break;
		case PossiblePlayerBattleAction::CATAPULT:
			return 11;
			break;
		case PossiblePlayerBattleAction::HEAL:
			return 12;
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
		possibleActions.clear();
		possibleActions.push_back(std::make_shared<LegacyEntry>(*this, spellSelMode)); //only this one action can be performed at the moment
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

	const auto entry = selectEntry(hoveredHex);

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
		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (owner.stacksController->getActiveStack()->hasBonusOfType(BonusType::FLYING))
				return "combatFly";
			else
				return "combatMove";

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
		{
			static const std::map<BattleHex::EDir, std::string> sectorCursor = {
				{BattleHex::TOP_LEFT,     "combatHitSouthEast"},
				{BattleHex::TOP_RIGHT,    "combatHitSouthWest"},
				{BattleHex::RIGHT,        "combatHitWest"     },
				{BattleHex::BOTTOM_RIGHT, "combatHitNorthWest"},
				{BattleHex::BOTTOM_LEFT,  "combatHitNorthEast"},
				{BattleHex::LEFT,         "combatHitEast"     },
				{BattleHex::TOP,          "combatHitSouth"    },
				{BattleHex::BOTTOM,       "combatHitNorth"    }
			};

			auto direction = owner.fieldController->selectAttackDirection(targetHex);

			// selectAttackDirection logs an error and returns NONE if the hex can't be attacked from any direction
			assert(sectorCursor.count(direction) > 0);
			if (!sectorCursor.count(direction))
				return "combatBlocked";

			return sectorCursor.at(direction);
		}

		case PossiblePlayerBattleAction::SHOOT:
			if (owner.getBattle()->battleHasShootingPenalty(owner.stacksController->getActiveStack(), targetHex))
				return "combatShootPenalty";
			else
				return "combatShoot";

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

		case PossiblePlayerBattleAction::HEAL:
			return "combatHeal";

		case PossiblePlayerBattleAction::CATAPULT:
			return "combatShootCatapult";
	}
	assert(0);
	return "combatBlocked";
}

std::string BattleActionsController::actionGetStatusMessage(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
		{
			const CStack * activeStack = owner.stacksController->getActiveStack();
			if (activeStack->hasBonusOfType(BonusType::FLYING))
				return formatWithStackName("core.genrltxt.295", activeStack); //Fly %s here
			else
				return formatWithStackName("core.genrltxt.294", activeStack); //Move %s here
		}

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
			{
				const auto * attacker = owner.stacksController->getActiveStack();
				bool allowLongWeapon = action.get() == PossiblePlayerBattleAction::LONG_WEAPON_ATTACK;
				BattleHex attackFromHex = findAttackFromHex(owner, attacker, targetHex, allowLongWeapon);
				assert(attackFromHex.isValid());
				if(!attackFromHex.isValid())
					return "";
				int distance = attacker->position.isValid() ? owner.getBattle()->battleGetDistances(attacker, attacker->getPosition())[attackFromHex.toInt()] : 0;
				DamageEstimation retaliation;
				BattleAttackInfo attackInfo(attacker, targetStack, distance, false);
				attackInfo.attackerPos = attackFromHex;
				DamageEstimation estimation = owner.getBattle()->battleEstimateDamage(attackInfo, &retaliation);
				estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
				estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());
				bool enemyMayBeKilled = estimation.kills.max == targetStack->getCount();

				// breath and other multi-hex attacks also strike extra units - add their kills to the prediction
				// (getAttackedBattleUnits excludes the directly-attacked hex, so the main target is handled above)
				for(const auto * splashTarget : owner.getBattle()->getAttackedCreatures(attacker, targetHex, false, attackFromHex).first)
				{
					if(splashTarget == targetStack || splashTarget == attacker)
						continue;
					BattleAttackInfo splashInfo(attacker, splashTarget, distance, false);
					splashInfo.attackerPos = attackFromHex;
					DamageEstimation splash = owner.getBattle()->battleEstimateDamage(splashInfo, nullptr);
					estimation.kills.min += std::min<int64_t>(splash.kills.min, splashTarget->getCount());
					estimation.kills.max += std::min<int64_t>(splash.kills.max, splashTarget->getCount());
				}

				return formatMeleeAttack(estimation, targetStack->getName()) + "\n" + formatRetaliation(retaliation, enemyMayBeKilled);
			}

		case PossiblePlayerBattleAction::SHOOT:
		{
			const auto * shooter = owner.stacksController->getActiveStack();

			if(targetStack == nullptr) //should be true only for spell-like attack
			{
				auto spellLikeAttackBonus = shooter->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
				assert(spellLikeAttackBonus != nullptr);
				const CSpell * spell = spellLikeAttackBonus->subtype.as<SpellID>().toSpell();

				DamageEstimation est = owner.getBattle()->estimateSpellLikeAttackDamage(shooter, spell, targetHex);
				return formatRangedAttack(est, spell->getNameTranslated(), shooter->shots.available());
			}

			DamageEstimation retaliation;
			BattleAttackInfo attackInfo(shooter, targetStack, 0, true );
			DamageEstimation estimation = owner.getBattle()->battleEstimateDamage(attackInfo, &retaliation);
			estimation.kills.max = std::min<int64_t>(estimation.kills.max, targetStack->getCount());
			estimation.kills.min = std::min<int64_t>(estimation.kills.min, targetStack->getCount());
			return formatRangedAttack(estimation, targetStack->getName(), shooter->shots.available());
		}

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

		case PossiblePlayerBattleAction::HEAL:
		{
			spells::effects::SpellEffectValue value = {};
			value.hpDelta = owner.getBattle()->getFirstAidHealValue(owner.currentHero(), targetStack);
			//Apply first aid to the %s plus heal value
			return prepareSpellEffectText(419, value, "", targetStack->getName());
		}

		case PossiblePlayerBattleAction::CATAPULT:
			return ""; // TODO
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
		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
			if (!(targetStack && targetStack->alive())) //we can walk on dead stacks
			{
				const CStack * currentStack = owner.stacksController->getActiveStack();
				return currentStack && owner.getBattle()->toWhichHexMove(currentStack, targetHex).isValid();
			}
			return false;

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			{
				const CStack * currentStack = owner.stacksController->getActiveStack();
				bool allowLongWeapon = action.get() == PossiblePlayerBattleAction::LONG_WEAPON_ATTACK;
				return currentStack &&
					owner.getBattle()->battleCanAttackUnit(currentStack, targetStack) &&
					owner.getBattle()->battleCanAttackHex(currentStack, targetHex) &&
					findAttackFromHex(owner, currentStack, targetHex, allowLongWeapon).isValid();
			}
		case PossiblePlayerBattleAction::WALK_AND_SPELLCAST:
			{
				const CStack * currentStack = owner.stacksController->getActiveStack();
				if (!currentStack || !targetStack)
					return false;

				if (targetStack == currentStack)
					return false;

				return owner.getBattle()->battleCanAttackHex(currentStack, targetHex) && isCastingPossibleHere(action.spell().toSpell(), nullptr, targetHex);
			}
		case PossiblePlayerBattleAction::SHOOT:
			{
				auto currentStack = owner.stacksController->getActiveStack();
				if(!owner.getBattle()->battleCanShoot(currentStack, targetHex))
					return false;

				if((targetStack == nullptr || targetStack->isInvincible()) && owner.getBattle()->battleCanTargetEmptyHex(currentStack))
				{
					auto spellLikeAttackBonus = currentStack->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
					const CSpell * spellDataToCheck = spellLikeAttackBonus->subtype.as<SpellID>().toSpell();
					return isCastingPossibleHere(spellDataToCheck, nullptr, targetHex);
				}

				return true;
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

		case PossiblePlayerBattleAction::CATAPULT:
			return owner.siegeController && owner.siegeController->isAttackableByCatapult(targetHex);

		case PossiblePlayerBattleAction::HEAL:
			return targetStack && targetStackOwned && targetStack->canBeHealed();
	}

	assert(0);
	return false;
}

void BattleActionsController::actionRealize(PossiblePlayerBattleAction action, const BattleHex & targetHex)
{
	const CStack * targetStack = getStackForHex(targetHex);

	switch (action.get()) //display console message, realize selected action
	{
		case PossiblePlayerBattleAction::MOVE_TACTICS:
		case PossiblePlayerBattleAction::MOVE_STACK:
		{
			const auto * activeStack = owner.stacksController->getActiveStack();
			auto toHex = owner.getBattle()->toWhichHexMove(activeStack, targetHex);
			assert(toHex.isValid());
			owner.giveCommand(EActionType::WALK, toHex);
			return;
		}

		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
		{
			bool returnAfterAttack = action.get() == PossiblePlayerBattleAction::ATTACK_AND_RETURN;
			bool allowLongWeapon = action.get() == PossiblePlayerBattleAction::LONG_WEAPON_ATTACK;
			auto attacker = owner.stacksController->getActiveStack();
			BattleHex attackFromHex = findAttackFromHex(owner, attacker, targetHex, allowLongWeapon);
			assert(attackFromHex.isValid());
			if(!attackFromHex.isValid())
				return;
			BattleAction command = BattleAction::makeMeleeAttack(attacker, targetHex, attackFromHex, returnAfterAttack);
			owner.sendCommand(command, attacker);
			return;
		}

		case PossiblePlayerBattleAction::SHOOT:
		{
			owner.giveCommand(EActionType::SHOOT, targetHex);
			return;
		}

		case PossiblePlayerBattleAction::HEAL:
		{
			owner.giveCommand(EActionType::STACK_HEAL, targetHex);
			return;
		};

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

		case PossiblePlayerBattleAction::CATAPULT:
		{
			owner.giveCommand(EActionType::CATAPULT, targetHex);
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

	const auto entry = selectEntry(hoveredHex);
	BattleActionPreview preview = entry ? entry->preview(hoveredHex) : BattleActionPreview{"combatBlocked", ""};

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
	const auto entry = selectEntry(hex);
	if(!entry || !entry->isLegal(hex))
		return {};

	const auto * legacy = dynamic_cast<const LegacyEntry *>(entry.get());
	std::string key;
	if(dynamic_cast<const CreatureInfoEntry *>(entry.get()))
		key = "inspect";
	else if(legacy)
	{
		switch(legacy->action.get())
		{
		case PossiblePlayerBattleAction::MOVE_STACK:
		case PossiblePlayerBattleAction::MOVE_TACTICS: key = "move"; break;
		case PossiblePlayerBattleAction::ATTACK:
		case PossiblePlayerBattleAction::LONG_WEAPON_ATTACK:
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN: key = "attack"; break;
		case PossiblePlayerBattleAction::SHOOT: key = "shoot"; break;
		default: break;
		}
	}
	if(key.empty())
		return entry->preview(hex).statusText;
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

		possibleActions = getPossibleActionsForStack(s);
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
		return entry->getSpell() != SpellID::NONE || isLegacyAction(*entry, PossiblePlayerBattleAction::SHOOT); //for hotkey-eligible SPELL_LIKE_ATTACK creature should have only SHOOT action
	};

	return !possibleActions.empty() && std::all_of(possibleActions.begin(), possibleActions.end(), spellcastModePredicate);
}

bool BattleActionsController::currentActionSpellcasting(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return true;

	if (!owner.stacksController->getActiveStack())
		return false;

	const auto entry = selectEntry(hoveredHex);

	return entry && entry->getSpell() != SpellID::NONE;
}

bool BattleActionsController::currentActionWalkAndCast(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return false;

	if (!owner.stacksController->getActiveStack())
		return false;

	const auto entry = selectEntry(hoveredHex);

	return entry && isLegacyAction(*entry, PossiblePlayerBattleAction::WALK_AND_SPELLCAST);
}

bool BattleActionsController::currentActionUsesLongWeapon(const BattleHex & hoveredHex)
{
	if (heroSpellToCast)
		return false;

	if (!owner.stacksController->getActiveStack())
		return true;

	const auto entry = selectEntry(hoveredHex);

	return entry && isLegacyAction(*entry, PossiblePlayerBattleAction::LONG_WEAPON_ATTACK);
}

bool BattleActionsController::currentActionHasAttackDirection(const BattleHex & hoveredHex)
{
	if(heroSpellToCast || !owner.stacksController->getActiveStack())
		return false;

	const auto entry = selectEntry(hoveredHex);
	if(!entry || !entry->isLegal(hoveredHex))
		return false;

	return isLegacyAction(*entry, PossiblePlayerBattleAction::ATTACK)
		|| isLegacyAction(*entry, PossiblePlayerBattleAction::LONG_WEAPON_ATTACK)
		|| isLegacyAction(*entry, PossiblePlayerBattleAction::WALK_AND_ATTACK)
		|| isLegacyAction(*entry, PossiblePlayerBattleAction::ATTACK_AND_RETURN);
}

void BattleActionsController::setPriorityActions(const BattleActionEntries & actions)
{
	possibleActions = actions;
}

void BattleActionsController::resetCurrentStackPossibleActions()
{
	possibleActions = getPossibleActionsForStack(owner.stacksController->getActiveStack());
}
