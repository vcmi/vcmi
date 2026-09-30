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
#include "events/InputHandler.h"
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
#include "../../lib/spells/CSpell.h"
#include "../GameInstance.h"

UnitActionPanel::UnitActionPanel(BattleInterface & owner)
	: CIntObject(0)
	, owner(owner)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | SHOW_POPUP | MOVE);

	pos = Rect(0, 0, 52, 600);
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	rect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w + 1, pos.h + 1), ColorRGBA(0, 0, 0, 0), ColorRGBA(241, 216, 120, 255));
}

void UnitActionPanel::restoreAllActions()
{
	owner.actionsController->resetCurrentStackPossibleActions();
}

void UnitActionPanel::setActions(int buttonIndex, const BattleActionEntries & filteredActions)
{
	for (const auto & button : buttons)
		if (button != buttons.at(buttonIndex))
			button->setSelectedSilent(false);

	owner.actionsController->setPriorityActions(filteredActions);
	if (filteredActions.front()->getSpell() != SpellID::NONE)
		owner.actionsController->enterCreatureCastingMode();
	owner.actionsController->setPriorityActions(filteredActions);
}

void UnitActionPanel::addButton(const UnitActionButton & button, const BattleActionEntries & entries)
{
	int index = buttons.size();

	const auto & callback = [this, entries, index](bool isSelected){ if (isSelected) setActions(index, entries); else restoreAllActions(); };

	std::shared_ptr<CToggleButton> toggle;

	if (button.spell == SpellID::NONE)
	{
		MetaString tooltip;
		tooltip.appendTextID(button.tooltipTextID);

		toggle = std::make_shared<CToggleButton>(Point(2, 7 + 50 * index), AnimationPath::builtin("battleUnitAction"), CButton::tooltip(tooltip.toString(&GAME->translator())), callback);
		toggle->setOverlay(std::make_shared<CPicture>(button.icon));
		toggle->setAllowDeselection(true);
	}
	else
	{
		MetaString tooltip;
		tooltip.appendTextID("core.genrltxt.26");
		tooltip.replaceName(button.spell);

		std::string hoverText = tooltip.toString(&GAME->translator());
		std::string description = button.spell.toSpell()->getDescriptionTranslated(0);

		toggle = std::make_shared<CToggleButton>(Point(2, 7 + 50 * index), AnimationPath::builtin("battleUnitAction"), CButton::tooltip(hoverText, description), callback);
		toggle->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("spellint"), button.spell.getNum() + 1));
	}

	toggle->setHighlightedBorderColor(Colors::WHITE);
	buttons.push_back(toggle);
}

void UnitActionPanel::setPossibleActions(const BattleActionEntries & newActions)
{
	OBJECT_CONSTRUCTION;

	buttons.clear();

	std::vector<std::pair<UnitActionButton, BattleActionEntries>> groups;

	for (const auto & entry : newActions)
	{
		const auto button = entry->getPanelButton();
		if (!button)
			continue;

		auto group = std::find_if(groups.begin(), groups.end(), [&button](const auto & group){ return group.first == *button; });
		if (group == groups.end())
			groups.emplace_back(*button, BattleActionEntries{entry});
		else
			group->second.push_back(entry);
	}

	std::stable_sort(groups.begin(), groups.end(), [](const auto & lhs, const auto & rhs){ return lhs.first.order < rhs.first.order; });

	for (const auto & [button, entries] : groups)
		addButton(button, entries);

	redraw();
}

void UnitActionPanel::show(Canvas & to)
{
	showAll(to);
	CIntObject::show(to);
}
