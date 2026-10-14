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
#include "BattleActionsController.h"

#include "BattleFieldController.h"
#include "BattleInterface.h"

#include "../CPlayerInterface.h"

#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/battle/actions/BattleActionType.h"
#include "../../lib/callback/CCallback.h"

LibActionEntry::LibActionEntry(BattleInterface & owner, const CStack * actor, const ActionOption & option)
	: owner(owner)
	, actor(actor)
	, option(option)
{
}

ActionContext LibActionEntry::getContext(const BattleHex & hex) const
{
	ActionContext context{actor, hex, std::nullopt, owner.actionsController->getPartialTarget(), owner.getBattle()->battleGetMySide()};

	const auto approaches = getApproaches(hex);
	if(!approaches.empty())
		context.approach = owner.fieldController->selectApproach(hex, approaches);

	return context;
}

std::vector<ActionApproach> LibActionEntry::getApproaches(const BattleHex & hex) const
{
	if(!actor)
		return {};
	return option.type->getApproaches(*owner.getBattle(), option, *actor, hex);
}

int LibActionEntry::getPriority(const battle::Unit * target) const
{
	// an option of the hero is the only entry while it is offered
	if(!actor)
		return 0;

	return option.type->getPriority(*owner.getBattle(), option, *actor, target);
}

bool LibActionEntry::isLegal(const BattleHex & hex) const
{
	const ActionContext context = getContext(hex);
	return option.type->buildTargetStep(*owner.getBattle(), option, context).has_value()
		|| option.type->isLegal(*owner.curInt->cb, *owner.getBattle(), option, context);
}

ActionPreview LibActionEntry::preview(const BattleHex & hex) const
{
	return option.type->preview(*owner.curInt->cb, *owner.getBattle(), option, getContext(hex));
}

void LibActionEntry::realize(const BattleHex & hex) const
{
	const ActionContext context = getContext(hex);

	if(const auto step = option.type->buildTargetStep(*owner.getBattle(), option, context))
	{
		owner.actionsController->addTargetStep(*this, *step);
		return;
	}

	owner.actionsController->sendAction(option.type->build(*owner.getBattle(), option, context).value(), actor);
}

SpellID LibActionEntry::getSpell() const
{
	return option.spell;
}

bool LibActionEntry::needsNoTarget() const
{
	return option.spell.hasValue() && option.type->build(*owner.getBattle(), option, getContext(BattleHex::INVALID)).has_value();
}

std::optional<UnitActionButton> LibActionEntry::getPanelButton() const
{
	return option.button;
}

const BattleActionType * LibActionEntry::getType() const
{
	return option.type;
}
