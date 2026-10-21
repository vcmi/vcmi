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

#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/actions/BattleActionType.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/AdvancedClass.h"
#include "../../lib/texts/CGeneralTextHandler.h"

bool BattleActionsController::isLibAction(const IBattleActionEntry & entry, EActionType actionType)
{
	const auto * lib = dynamic_cast<const LibActionEntry *>(&entry);
	return lib && lib->getType() == BattleActionType::find(actionType);
}

BattleActionsController::BattleActionsController(BattleInterface & owner):
	owner(owner)
{
}

void BattleActionsController::endCastingSpell()
{
	if(heroSpellcasting)
	{
		heroSpellcasting = false;
		owner.windowObject->blockUI(false);
	}

	if(suspendedStack)
	{
		suspendedStack = nullptr;
		owner.stacksController->activateStack();
	}
	partialTarget.clear();

	if(owner.stacksController->getActiveStack())
	{
		setEntries(getPossibleActionsForStack(owner.stacksController->getActiveStack())); //restore actions after they were cleared
		owner.windowObject->setPossibleActions(possibleActions);
	}

	cachedSelection.reset();
	ENGINE->fakeMouseMove();
}

const battle::Target & BattleActionsController::getPartialTarget() const
{
	return partialTarget;
}

void BattleActionsController::addTargetStep(const IBattleActionEntry & entry, const battle::Destination & destination)
{
	if(partialTarget.empty())
	{
		const auto chosen = std::find_if(possibleActions.begin(), possibleActions.end(), [&entry](const auto & candidate){ return candidate.get() == &entry; });
		if(chosen == possibleActions.end())
			throw std::runtime_error("Target step added by a battle action entry that is not offered");

		// the remaining destinations are chosen for this entry alone
		const auto kept = *chosen;
		setEntries({kept});

		suspendedStack = owner.stacksController->getActiveStack();
		owner.windowObject->blockUI(true);
		owner.stacksController->deactivateStack();
	}

	partialTarget.push_back(destination);
	cachedSelection.reset();
}

void BattleActionsController::sendAction(const BattleAction & action, const CStack * actor)
{
	const bool choosingSpellTarget = heroSpellcasting || !partialTarget.empty();

	if(actor)
	{
		// so that BattleStacksController does not activate the suspended unit again after its action is sent
		if(suspendedStack)
			owner.stacksController->activateStack();
		owner.sendCommand(action, actor);
	}
	else
	{
		owner.curInt->cb->battleMakeSpellAction(owner.getBattleID(), action);
	}

	if(choosingSpellTarget)
		endCastingSpell();
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
	if (heroSpellcasting)
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

	// a spell without a target is cast at once
	for(const auto & entry : possibleActions)
	{
		const auto * spellEntry = dynamic_cast<const LibActionEntry *>(entry.get());
		if(!spellEntry || !spellEntry->needsNoTarget())
			continue;

		if(spellEntry->isLegal(BattleHex::INVALID))
		{
			spellEntry->realize(BattleHex::INVALID);
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
	const bool tacticsMode = owner.isInTacticsMode();

	BattleActionEntries entries;
	std::vector<ActionOption> options;
	BattleActionType::collectAllOptions(*owner.getBattle(), *stack, options);
	for(const auto & option : options)
		entries.push_back(std::make_shared<LibActionEntry>(owner, stack, option));

	const auto * hero = owner.currentHero();
	if(hero && GAME->interface()->cb->getSettings().getBoolean(EGameSettings::MODULE_ADVANCED_CLASSES))
	{
		const auto side = owner.getBattle()->battleGetMySide();
		for(size_t slot = 0; slot < GameConstants::ADVANCED_CLASS_ABILITY_SLOTS; ++slot)
		{
			const auto * definition = LIBRARY->advancedClassHandler->getAbility(*hero, slot);
			const bool cooldownReady = definition && definition->isAbilityReady(
				owner.getBattle()->battleGetRound(), owner.getBattle()->battleGetClassAbilityLastUse(side, slot));
			if(definition && cooldownReady)
			{
				const auto option = BattleActionType::makeHeroAbilityOption(slot, definition->ability);
				entries.push_back(std::make_shared<LibActionEntry>(owner, nullptr, option));
			}
		}
	}

	if(rolledCreatureSpell.hasValue() && !tacticsMode && stack->canCast())
		entries.push_back(std::make_shared<LibActionEntry>(owner, stack, BattleActionType::makeUnitSpellOption(rolledCreatureSpell)));

	if(tacticsMode)
		entries.push_back(std::make_shared<TacticsUnitSelectionEntry>(owner));

	entries.push_back(std::make_shared<HeroInfoEntry>(owner));
	entries.push_back(std::make_shared<CreatureInfoEntry>(owner));

	return entries;
}

void BattleActionsController::castThisSpell(SpellID spellID)
{
	heroSpellcasting = true;

	const auto entry = std::make_shared<LibActionEntry>(owner, nullptr, BattleActionType::makeHeroSpellOption(spellID));

	if(entry->needsNoTarget())
	{
		entry->realize(BattleHex::INVALID);
	}
	else
	{
		setEntries({entry}); //only this one action can be performed at the moment
		ENGINE->fakeMouseMove();//update cursor
	}

	owner.windowObject->blockUI(true);
}

std::shared_ptr<const IBattleActionEntry> BattleActionsController::selectEntry(const BattleHex & targetHex)
{
	auto currentStack = suspendedStack ? suspendedStack : owner.stacksController->getActiveStack();
	assert(currentStack != nullptr);
	assert(targetHex.isValid());

	// creature spellcasting mode of a unit with an area shot and no SHOOT entry leaves no entries
	if(currentStack == nullptr || possibleActions.empty())
		return nullptr;

	const battle::Unit * targetStack = owner.getBattle()->battleGetUnitOrCorpseByPos(targetHex);

	std::vector<std::pair<int, std::shared_ptr<const IBattleActionEntry>>> ordered;
	for(const auto & entry : possibleActions)
		ordered.emplace_back(entry->getPriority(targetStack), entry);

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
		const auto entry = selectEntry(hex);
		cachedSelection = HexSelection{hex, entry ? entry->preview(hex) : ActionPreview{"combatBlocked", {}, {}, {}}};
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

	if (owner.stacksController->getActiveStack() == nullptr && suspendedStack == nullptr)
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
	ActionPreview preview = getSelection(hoveredHex).preview;

	if (owner.siegeController && owner.siegeController->isTowerHex(hoveredHex))
	{
		preview.cursor = "combatQuery"; // question cursor over a siege tower
		preview.statusText = MetaString::createFromTextID("core.genrltxt.156"); // "View arrow tower info."
	}

	ENGINE->cursor().set(preview.cursor);
	const std::string newConsoleMsg = preview.statusText.toString(&GAME->translator());

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
	if(!hex.isAvailable() || (!owner.stacksController->getActiveStack() && !suspendedStack))
		return {};
	const ActionPreview & preview = getSelection(hex).preview;
	if(preview.cursor == "combatBlocked")
		return {};

	std::string key;
	if(preview.cursor == "combatMove" || preview.cursor == "combatFly")
		key = "move";
	else if(preview.cursor.starts_with("combatHit"))
		key = "attack";
	else if(preview.cursor == "combatShoot" || preview.cursor == "combatShootPenalty")
		key = "shoot";
	else if(preview.cursor == "combatQuery")
		key = "inspect";
	else
		return preview.statusText.toString(&GAME->translator());
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
	if (owner.stacksController->getActiveStack() == nullptr && suspendedStack == nullptr)
		return;

	const auto entry = selectEntry(clickedHex);

	if (!entry || !entry->isLegal(clickedHex))
		return;

	const auto * infoEntry = dynamic_cast<const CreatureInfoEntry *>(entry.get());
	if(infoEntry && showStackInfo)
		showStackInfo(owner.getBattle()->battleGetStackByPos(clickedHex, true));
	else
		entry->realize(clickedHex);
	cachedSelection.reset();
	ENGINE->statusbar()->clear();
}

void BattleActionsController::tryActivateStackSpellcasting(const CStack * casterStack)
{
	rolledCreatureSpell = SpellID::NONE;

	// faerie dragon can cast only one, randomly selected spell until their next move
	//TODO: faerie dragon type spell should be selected by server
	if(casterStack->canCast() && casterStack->hasBonusOfType(BonusType::SPELLCASTER))
		rolledCreatureSpell = owner.getBattle()->getRandomCastedSpell(CRandomGenerator::getDefault(), casterStack);
}

void BattleActionsController::activateStack()
{
	// a target that was being chosen when the turn ended some other way, e.g. by the turn timer
	partialTarget.clear();
	suspendedStack = nullptr;

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
	return heroSpellcasting;
}

bool BattleActionsController::creatureSpellcastingModeActive() const
{
	auto spellcastModePredicate = [](const std::shared_ptr<const IBattleActionEntry> & entry)
	{
		return entry->getSpell() != SpellID::NONE || isLibAction(*entry, EActionType::SHOOT); //for hotkey-eligible SPELL_LIKE_ATTACK creature should have only SHOOT action
	};

	return !possibleActions.empty() && std::all_of(possibleActions.begin(), possibleActions.end(), spellcastModePredicate);
}

std::vector<ActionApproach> BattleActionsController::getApproaches(const BattleHex & hoveredHex)
{
	const auto entry = selectEntry(hoveredHex);
	const auto * libEntry = dynamic_cast<const LibActionEntry *>(entry.get());
	if(!libEntry || !libEntry->isLegal(hoveredHex))
		return {};
	return libEntry->getApproaches(hoveredHex);
}

BattleHexArray BattleActionsController::getShadedHexes(const BattleHex & hoveredHex)
{
	if(!owner.stacksController->getActiveStack() && !suspendedStack)
		return {};

	return getSelection(hoveredHex).preview.shadedHexes;
}

std::vector<uint32_t> BattleActionsController::getAffectedUnits(const BattleHex & hoveredHex)
{
	if(!owner.stacksController->getActiveStack() && !suspendedStack)
		return {};

	return getSelection(hoveredHex).preview.affectedUnits;
}

void BattleActionsController::resetCurrentStackPossibleActions()
{
	setEntries(getPossibleActionsForStack(owner.stacksController->getActiveStack()));
}
