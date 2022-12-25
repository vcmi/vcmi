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
#include "BattleWindow.h"
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
#include "../../lib/filesystem/ResourceID.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStack.h"
#include "../../lib/IGameEventsReceiver.h"
#include "../../lib/CGeneralTextHandler.h"

BattleEffectsController::BattleEffectsController(BattleInterface & owner):
	owner(owner)
{
	loadColorMuxers();
}

void BattleEffectsController::displayEffect(EBattleEffect effect, const BattleHex & destTile)
{
	displayEffect(effect, "", destTile);
}

void BattleEffectsController::displayEffect(EBattleEffect effect, std::string soundFile, const BattleHex & destTile)
{
	size_t effectID = static_cast<size_t>(effect);

	std::string customAnim = graphics->battleACToDef[effectID][0];

	CCS->soundh->playSound( soundFile );

	owner.stacksController->addNewAnim(new EffectAnimation(owner, customAnim, destTile));
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
		case Bonus::HP_REGENERATION:
			displayEffect(EBattleEffect::REGENERATION, "REGENER", stack->getPosition());
			break;
		case Bonus::MANA_DRAIN:
			displayEffect(EBattleEffect::MANA_DRAIN, "MANADRAI", stack->getPosition());
			break;
		case Bonus::POISON:
			displayEffect(EBattleEffect::POISON, "POISON", stack->getPosition());
			break;
		case Bonus::FEAR:
			displayEffect(EBattleEffect::FEAR, "FEAR", stack->getPosition());
			break;
		case Bonus::MORALE:
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(EBattleEffect::GOOD_MORALE, "GOODMRLE", stack->getPosition());
			owner.appendBattleLog(hlp);
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
		owner.appendBattleLog(stack->formatGeneralMessage(136));
		break;
	case EActionType::BAD_MORALE:
		owner.appendBattleLog(stack->formatGeneralMessage(-34));
		displayEffect(EBattleEffect::BAD_MORALE, "BADMRLE", stack->getPosition());
		break;
	}

	//displaying special abilities
	auto actionTarget = action->getTarget(owner.curInt->cb.get());
	switch(action->actionType)
	{
		case EActionType::STACK_HEAL:
			displayEffect(EBattleEffect::REGENERATION, "REGENER", actionTarget.at(0).hexValue);
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

void BattleEffectsController::loadColorMuxers()
{
	const JsonNode config(ResourceID("config/battleEffects.json"));

	for(auto & muxer : config["colorMuxers"].Struct())
	{
		ColorMuxerEffect effect;
		std::string identifier = muxer.first;

		for (const JsonNode & entry : muxer.second.Vector() )
		{
			effect.timePoints.push_back(entry["time"].Float());
			effect.filters.push_back(ColorFilter::genFromJson(entry));
		}
		colorMuxerEffects[identifier] = effect;
	}
}

const ColorMuxerEffect & BattleEffectsController::getMuxerEffect(const std::string & name)
{
	static const ColorMuxerEffect emptyEffect;

	if (colorMuxerEffects.count(name))
		return colorMuxerEffects[name];

	logAnim->error("Failed to find color muxer effect named '%s'!", name);
	return emptyEffect;
}
