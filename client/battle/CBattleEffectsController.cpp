/*
 * CBattleEffectsController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleEffectsController.h"

#include "CBattleAnimations.h"
#include "CBattleControlPanel.h"
#include "CBattleInterface.h"
#include "CBattleInterfaceClasses.h"
#include "CBattleStacksController.h"
#include "../gui/CAnimation.h"
#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStack.h"
#include "../../lib/IGameEventsReceiver.h"
#include "../../lib/CGeneralTextHandler.h"

CBattleEffectsController::CBattleEffectsController(CBattleInterface * owner):
	owner(owner)
{}

void CBattleEffectsController::displayEffect(EBattleEffect::EBattleEffect effect, const BattleHex & destTile)
{
	std::string customAnim = graphics->battleACToDef[effect][0];

	owner->stacksController->addNewAnim(new CEffectAnimation(owner, customAnim, destTile));
}

void CBattleEffectsController::displayEffect(EBattleEffect::EBattleEffect effect, uint32_t soundID, const BattleHex & destTile)
{
	displayEffect(effect, destTile);
	if(soundBase::soundID(soundID) != soundBase::invalid )
		CCS->soundh->playSound(soundBase::soundID(soundID));
}

void CBattleEffectsController::displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects)
{
	for(const CustomEffectInfo & one : customEffects)
	{
		if(one.sound != 0)
			CCS->soundh->playSound(soundBase::soundID(one.sound));
		const CStack * s = owner->curInt->cb->battleGetStackByID(one.stack, false);
		if(s && one.effect != 0)
			displayEffect(EBattleEffect::EBattleEffect(one.effect), s->getPosition());
	}
}

void CBattleEffectsController::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	const CStack * stack = owner->curInt->cb->battleGetStackByID(bte.stackID);
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
			owner->controlPanel->console->addText(hlp);
			break;
		}
		default:
			return;
	}
	//waitForAnims(); //fixme: freezes game :?
}


void CBattleEffectsController::startAction(const BattleAction* action)
{
	const CStack *stack = owner->curInt->cb->battleGetStackByID(action->stackNumber);

	int txtid = 0;
	switch(action->actionType)
	{
	case EActionType::WAIT:
		txtid = 136;
		break;
	case EActionType::BAD_MORALE:
		txtid = -34; //negative -> no separate singular/plural form
		displayEffect(EBattleEffect::BAD_MORALE, soundBase::BADMRLE, stack->getPosition());
		break;
	}

	if(txtid != 0)
		owner->controlPanel->console->addText(stack->formatGeneralMessage(txtid));

	//displaying special abilities
	auto actionTarget = action->getTarget(owner->curInt->cb.get());
	switch(action->actionType)
	{
		case EActionType::STACK_HEAL:
			displayEffect(EBattleEffect::REGENERATION, soundBase::REGENER, actionTarget.at(0).hexValue);
			break;
	}
}


void CBattleEffectsController::showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects)
{
	for (auto & elem : battleEffects)
	{
		int currentFrame = static_cast<int>(floor(elem->currentFrame));
		currentFrame %= elem->animation->size();

		auto img = elem->animation->getImage(currentFrame);

		SDL_Rect temp_rect = genRect(img->height(), img->width(), elem->x, elem->y);

		img->draw(to, &temp_rect, nullptr);
	}
}

void CBattleEffectsController::sortObjectsByHex(BattleObjectsByHex & sorted)
{
	for (auto & battleEffect : battleEffects)
	{
		if (battleEffect.position.isValid())
			sorted.hex[battleEffect.position].effects.push_back(&battleEffect);
		else
			sorted.afterAll.effects.push_back(&battleEffect);
	}
}
