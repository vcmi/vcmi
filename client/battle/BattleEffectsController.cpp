/*
 * BattleEffectsController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleEffectsController.h"

#include "BattleAnimationClasses.h"
#include "BattleControlPanel.h"
#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleFieldController.h"
#include "BattleStacksController.h"
#include "BattleRenderer.h"

#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/Canvas.h"

#include "../../CCallback.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStack.h"
#include "../../lib/IGameEventsReceiver.h"
#include "../../lib/CGeneralTextHandler.h"

BattleEffectsController::BattleEffectsController(BattleInterface & owner):
	owner(owner)
{}

void BattleEffectsController::displayEffect(EBattleEffect::EBattleEffect effect, const BattleHex & destTile)
{
	displayEffect(effect, soundBase::invalid, destTile);
}

void BattleEffectsController::displayEffect(EBattleEffect::EBattleEffect effect, uint32_t soundID, const BattleHex & destTile)
{
	std::string customAnim = graphics->battleACToDef[effect][0];

	owner.stacksController->addNewAnim(new CPointEffectAnimation(owner, soundBase::stringsList()[soundID], customAnim, destTile));
}

void BattleEffectsController::displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects)
{
	for(const CustomEffectInfo & one : customEffects)
	{
		const CStack * s = owner.curInt->cb->battleGetStackByID(one.stack, false);

		assert(s);
		assert(one.effect != 0);

		displayEffect(EBattleEffect::EBattleEffect(one.effect), soundBase::soundID(one.sound), s->getPosition());
	}
}

void BattleEffectsController::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

	const CStack * stack = owner.curInt->cb->battleGetStackByID(bte.stackID);
	if(!stack)
	{
		logGlobal->error("Invalid stack ID %d", bte.stackID);
		return;
	}
	//don't show animation when no HP is regenerated
	switch(bte.effect)
	{
		//TODO: move to bonus type handler
		case Bonus::HP_REGENERATION:
			displayEffect(EBattleEffect::REGENERATION, soundBase::REGENER, stack->getPosition());
			break;
		case Bonus::MANA_DRAIN:
			displayEffect(EBattleEffect::MANA_DRAIN, soundBase::MANADRAI, stack->getPosition());
			break;
		case Bonus::POISON:
			displayEffect(EBattleEffect::POISON, soundBase::POISON, stack->getPosition());
			break;
		case Bonus::FEAR:
			displayEffect(EBattleEffect::FEAR, soundBase::FEAR, stack->getPosition());
			break;
		case Bonus::MORALE:
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(EBattleEffect::GOOD_MORALE, soundBase::GOODMRLE, stack->getPosition());
			owner.controlPanel->console->addText(hlp);
			break;
		}
		default:
			return;
	}
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
}

void BattleEffectsController::startAction(const BattleAction* action)
{
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

	const CStack *stack = owner.curInt->cb->battleGetStackByID(action->stackNumber);

	switch(action->actionType)
	{
	case EActionType::WAIT:
		owner.controlPanel->console->addText(stack->formatGeneralMessage(136));
		break;
	case EActionType::BAD_MORALE:
		owner.controlPanel->console->addText(stack->formatGeneralMessage(-34));
		displayEffect(EBattleEffect::BAD_MORALE, soundBase::BADMRLE, stack->getPosition());
		break;
	}

	//displaying special abilities
	auto actionTarget = action->getTarget(owner.curInt->cb.get());
	switch(action->actionType)
	{
		case EActionType::STACK_HEAL:
			displayEffect(EBattleEffect::REGENERATION, soundBase::REGENER, actionTarget.at(0).hexValue);
			break;
	}

	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
}

void BattleEffectsController::collectRenderableObjects(BattleRenderer & renderer)
{
	for (auto & elem : battleEffects)
	{
		renderer.insert( EBattleFieldLayer::EFFECTS, elem.position, [&elem](BattleRenderer::RendererRef canvas)
		{
			int currentFrame = static_cast<int>(floor(elem.currentFrame));
			currentFrame %= elem.animation->size();

			auto img = elem.animation->getImage(currentFrame);

			canvas.draw(img, Point(elem.x, elem.y));
		});
	}
}
