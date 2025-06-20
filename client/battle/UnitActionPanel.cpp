/*
 * UnitActionPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "UnitActionPanel.h"

#include "BattleInterface.h"
#include "BattleActionsController.h"

#include "../GameEngine.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../windows/CSpellWindow.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/spells/CSpellHandler.h"

UnitActionPanel::UnitActionPanel(BattleInterface & owner)
	: CIntObject(0)
	, owner(owner)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | INPUT_MODE_CHANGE);

	pos = Rect(0, 0, 52, 600);
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	rect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w + 1, pos.h + 1), ColorRGBA(0, 0, 0, 0), ColorRGBA(241, 216, 120, 255));
}

void UnitActionPanel::restoreAllActions()
{
	owner.actionsController->resetCurrentStackPossibleActions();
}

void UnitActionPanel::setActions(int buttonIndex, const std::vector<PossiblePlayerBattleAction> & filteredActions)
{
	for (const auto & button : buttons)
		if (button != buttons.at(buttonIndex))
			button->setSelectedSilent(false);

	owner.actionsController->setPriorityActions(filteredActions);
	if (filteredActions.front().spellcast())
		owner.actionsController->enterCreatureCastingMode();
	owner.actionsController->setPriorityActions(filteredActions);
}

void UnitActionPanel::testAndAddAction(const std::vector<PossiblePlayerBattleAction> & allActions, const std::vector<PossiblePlayerBattleAction::Actions> & actionFilter, const ImagePath & iconPath, const std::string & descriptionTextID)
{
	std::vector<PossiblePlayerBattleAction> filteredActions;

	for (const auto & action : allActions)
		if (vstd::contains(actionFilter, action.get()))
			filteredActions.push_back(action);

	if (filteredActions.empty())
		return;

	int index = buttons.size();

	const auto & callback = [this, filteredActions, index](bool isSelected){ if (isSelected) setActions(index, filteredActions); else restoreAllActions(); };

	MetaString tooltip;
	tooltip.appendTextID(descriptionTextID);

	auto button = std::make_shared<CToggleButton>(Point(2, 7 + 50 * index), AnimationPath::builtin("battleUnitAction"), CButton::tooltip(tooltip.toString()), callback);
	button->setOverlay(std::make_shared<CPicture>(iconPath));
	button->setHighlightedBorderColor(Colors::WHITE);
	button->setAllowDeselection(true);
	buttons.push_back(button);
}

void UnitActionPanel::testAndAddSpell(const std::vector<PossiblePlayerBattleAction> & allActions, const SpellID & spellFilter)
{
	std::vector<PossiblePlayerBattleAction> filteredActions;

	for (const auto & action : allActions)
		if (action.spellcast() && action.spell() == spellFilter)
			filteredActions.push_back(action);

	if (filteredActions.empty())
		return;

	int index = buttons.size();
	const auto & callback = [this, filteredActions, index](bool isSelected){ if (isSelected) setActions(index, filteredActions); else restoreAllActions();};

	MetaString tooltip;
	tooltip.appendTextID("core.genrltxt.26");
	tooltip.replaceName(spellFilter);

	std::string hoverText = tooltip.toString();
	std::string description = spellFilter.toSpell()->getDescriptionTranslated(0);


	auto button = std::make_shared<CToggleButton>(Point(2, 7 + 50 * index), AnimationPath::builtin("battleUnitAction"), CButton::tooltip(hoverText, description), callback);
	button->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("spellint"), spellFilter.getNum() + 1));
	button->setHighlightedBorderColor(Colors::WHITE);
	buttons.push_back(button);
}

void UnitActionPanel::setPossibleActions(const std::vector<PossiblePlayerBattleAction> & newActions)
{
	OBJECT_CONSTRUCTION;

	buttons.clear();

	static const std::vector actionsMove = { PossiblePlayerBattleAction::MOVE_STACK };
	static const std::vector actionsInfo = { PossiblePlayerBattleAction::CREATURE_INFO, PossiblePlayerBattleAction::HERO_INFO };
	static const std::vector actionsShoot = { PossiblePlayerBattleAction::SHOOT };
	static const std::vector actionsGenie = { PossiblePlayerBattleAction::RANDOM_GENIE_SPELL };
	static const std::vector actionsAttack = { PossiblePlayerBattleAction::ATTACK, PossiblePlayerBattleAction::WALK_AND_ATTACK };
	static const std::vector actionsReturn = { PossiblePlayerBattleAction::ATTACK_AND_RETURN };

	testAndAddAction(newActions, actionsMove, ImagePath::builtin("battle/actionMove"), "vcmi.battle.action.move");
	testAndAddAction(newActions, actionsReturn, ImagePath::builtin("battle/actionReturn"), "vcmi.battle.action.return");
	testAndAddAction(newActions, actionsAttack, ImagePath::builtin("battle/actionAttack"), "vcmi.battle.action.attack");
	testAndAddAction(newActions, actionsShoot, ImagePath::builtin("battle/actionShoot"), "vcmi.battle.action.shoot");
	testAndAddAction(newActions, actionsGenie, ImagePath::builtin("battle/actionGenie"), "vcmi.battle.action.genie");

	std::vector<SpellID> spells;

	for (const auto & action : newActions)
		if (action.spellcast())
			spells.push_back(action.spell());


	for (const auto & spell : spells)
		testAndAddSpell(newActions, spell);

	// Not really a unit action, so place it at the end
	testAndAddAction(newActions, actionsInfo, ImagePath::builtin("battle/actionInfo"), "vcmi.battle.action.info");

	redraw();
}

void UnitActionPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}
