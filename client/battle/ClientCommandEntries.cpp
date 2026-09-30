/*
 * ClientCommandEntries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ClientCommandEntries.h"

#include "BattleHero.h"
#include "BattleInterface.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../windows/CCreatureWindow.h"

#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

// Shared by the unit and hero info entries. Not really a unit action, so placed at the end of the panel
static UnitActionButton infoButton()
{
	return UnitActionButton{100, SpellID::NONE, ImagePath::builtin("battle/actionInfo"), "vcmi.battle.action.info"};
}

ClientCommandEntry::ClientCommandEntry(BattleInterface & owner)
	: owner(owner)
{
}

const CStack * ClientCommandEntry::getUnit(const BattleHex & hex) const
{
	return owner.getBattle()->battleGetStackByPos(hex, true);
}

SpellID ClientCommandEntry::getSpell() const
{
	return SpellID::NONE;
}

int CreatureInfoEntry::getPriority(const CStack * actor, const CStack * target) const
{
	return 13;
}

bool CreatureInfoEntry::isLegal(const BattleHex & hex) const
{
	return getUnit(hex) != nullptr;
}

BattleActionPreview CreatureInfoEntry::preview(const BattleHex & hex) const
{
	if(!isLegal(hex))
		return {"combatBlocked", "", {}};

	MetaString text = MetaString::createFromTextID("core.genrltxt.297"); //View %s info.
	getUnit(hex)->addNameReplacement(text);
	return {"combatQuery", text.toString(&GAME->translator()), {}};
}

void CreatureInfoEntry::realize(const BattleHex & hex) const
{
	ENGINE->windows().createAndPushWindow<CStackWindow>(getUnit(hex), false);
}

std::optional<UnitActionButton> CreatureInfoEntry::getPanelButton() const
{
	return infoButton();
}

int HeroInfoEntry::getPriority(const CStack * actor, const CStack * target) const
{
	return 14;
}

bool HeroInfoEntry::isLegal(const BattleHex & hex) const
{
	if(hex == BattleHex::HERO_ATTACKER)
		return owner.attackingHero != nullptr;

	if(hex == BattleHex::HERO_DEFENDER)
		return owner.defendingHero != nullptr;

	return false;
}

BattleActionPreview HeroInfoEntry::preview(const BattleHex & hex) const
{
	if(!isLegal(hex))
		return {"combatBlocked", "", {}};

	return {"combatHero", LIBRARY->generaltexth->translate("core.genrltxt.417"), {}}; // "View Hero Stats"
}

void HeroInfoEntry::realize(const BattleHex & hex) const
{
	if(hex == BattleHex::HERO_ATTACKER)
		owner.attackingHero->heroLeftClicked();

	if(hex == BattleHex::HERO_DEFENDER)
		owner.defendingHero->heroLeftClicked();
}

std::optional<UnitActionButton> HeroInfoEntry::getPanelButton() const
{
	return infoButton();
}

int TacticsUnitSelectionEntry::getPriority(const CStack * actor, const CStack * target) const
{
	return 12;
}

bool TacticsUnitSelectionEntry::isLegal(const BattleHex & hex) const
{
	const CStack * unit = getUnit(hex);
	return unit && unit->unitOwner() == owner.curInt->playerID && unit->getMovementRange() > 0;
}

BattleActionPreview TacticsUnitSelectionEntry::preview(const BattleHex & hex) const
{
	if(!isLegal(hex))
		return {"combatBlocked", "", {}};

	MetaString text = MetaString::createFromTextID("core.genrltxt.481"); //Select %s
	getUnit(hex)->addNameReplacement(text);
	return {"combatPointer", text.toString(&GAME->translator()), {}};
}

void TacticsUnitSelectionEntry::realize(const BattleHex & hex) const
{
	owner.stackActivated(getUnit(hex));
}

std::optional<UnitActionButton> TacticsUnitSelectionEntry::getPanelButton() const
{
	return std::nullopt;
}
