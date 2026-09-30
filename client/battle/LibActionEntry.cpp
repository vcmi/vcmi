/*
 * LibActionEntry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LibActionEntry.h"

#include "BattleInterface.h"

#include "../CPlayerInterface.h"
#include "../GameInstance.h"

#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/battle/actions/BattleActionType.h"
#include "../../lib/callback/CCallback.h"

LibActionEntry::LibActionEntry(BattleInterface & owner, const CStack & actor, const ActionOption & option)
	: owner(owner)
	, actor(actor)
	, option(option)
{
}

ActionContext LibActionEntry::getContext(const BattleHex & hex) const
{
	return {&actor, hex};
}

int LibActionEntry::getPriority(const CStack *, const CStack * target) const
{
	return option.type->getPriority(*owner.getBattle(), option, actor, target);
}

bool LibActionEntry::isLegal(const BattleHex & hex) const
{
	return option.type->isLegal(*owner.curInt->cb, *owner.getBattle(), option, getContext(hex));
}

BattleActionPreview LibActionEntry::preview(const BattleHex & hex) const
{
	const ActionPreview preview = option.type->preview(*owner.curInt->cb, *owner.getBattle(), option, getContext(hex));
	return {preview.cursor, preview.statusText.toString(&GAME->translator())};
}

BattleHexArray LibActionEntry::getShadedHexes(const BattleHex & hex) const
{
	return option.type->preview(*owner.curInt->cb, *owner.getBattle(), option, getContext(hex)).shadedHexes;
}

void LibActionEntry::realize(const BattleHex & hex) const
{
	owner.sendCommand(option.type->build(*owner.getBattle(), option, getContext(hex)), &actor);
}

SpellID LibActionEntry::getSpell() const
{
	// no lib action type casts spells yet
	return SpellID::NONE;
}

std::optional<UnitActionButton> LibActionEntry::getPanelButton() const
{
	return option.button;
}
