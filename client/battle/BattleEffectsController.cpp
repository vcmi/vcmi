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
#include "BattleFieldController.h"
#include "BattleInterface.h"
#include "BattleRenderer.h"
#include "BattleStacksController.h"
#include "BattleWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../media/ISoundPlayer.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/Graphics.h"

#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/texts/CGeneralTextHandler.h"

BattleEffectsController::BattleEffectsController(BattleInterface & owner):
	owner(owner)
{
	loadColorMuxers();
}

void BattleEffectsController::displayEffect(EBattleEffect effect, const BattleHex & destTile)
{
	displayEffect(effect, AudioPath(), destTile);
}

void BattleEffectsController::displayEffect(EBattleEffect effect, const AudioPath & soundFile, const BattleHex & destTile, float transparencyFactor)
{
	size_t effectID = static_cast<size_t>(effect);

	AnimationPath customAnim = AnimationPath::builtinTODO(graphics->battleACToDef[effectID][0]);

	ENGINE->sound().playSound( soundFile );

	owner.stacksController->addNewAnim(new EffectAnimation(owner, customAnim, destTile, 0, transparencyFactor));
}

void BattleEffectsController::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	owner.checkForAnimations();

	const CStack * stack = owner.getBattle()->battleGetStackByID(bte.stackID);
	if(!stack)
	{
		logGlobal->error("Invalid stack ID %d", bte.stackID);
		return;
	}
	//don't show animation when no HP is regenerated
	switch(bte.effect)
	{
		case BonusType::HP_REGENERATION:
			displayEffect(EBattleEffect::REGENERATION, AudioPath::builtin("REGENER"), stack->getPosition(), 0.5);
			break;
		case BonusType::MANA_DRAIN:
			displayEffect(EBattleEffect::MANA_DRAIN, AudioPath::builtin("MANADRAI"), stack->getPosition());
			break;
		case BonusType::POISON:
			displayEffect(EBattleEffect::POISON, AudioPath::builtin("POISON"), stack->getPosition());
			break;
		case BonusType::FEARFUL:
			displayEffect(EBattleEffect::FEAR, AudioPath::builtin("FEAR"), stack->getPosition(), 0.5);
			break;
		case BonusType::MORALE:
		{
			std::string hlp = LIBRARY->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(EBattleEffect::GOOD_MORALE, AudioPath::builtin("GOODMRLE"), stack->getPosition());
			owner.appendBattleLog(hlp);
			break;
		}
		default:
			return;
	}
	owner.waitForAnimations();
}

void BattleEffectsController::startAction(const BattleAction & action)
{
	owner.checkForAnimations();

	const CStack *stack = owner.getBattle()->battleGetStackByID(action.stackNumber);

	switch(action.actionType)
	{
	case EActionType::WAIT:
		owner.appendBattleLog(stack->formatGeneralMessage(136));
		break;
	case EActionType::BAD_MORALE:
		owner.appendBattleLog(stack->formatGeneralMessage(-34));
		displayEffect(EBattleEffect::BAD_MORALE, AudioPath::builtin("BADMRLE"), stack->getPosition());
		break;
	}

	owner.waitForAnimations();
}

void BattleEffectsController::collectRenderableObjects(BattleRenderer & renderer)
{
	for (auto & elem : battleEffects)
	{
		renderer.insert( EBattleFieldLayer::EFFECTS, elem.tile, [&elem](BattleRenderer::RendererRef canvas)
		{
			int currentFrame = static_cast<int>(floor(elem.currentFrame));
			currentFrame %= elem.animation->size();

			auto img = elem.animation->getImage(currentFrame, static_cast<size_t>(elem.type));
			img->setAlpha(255 * elem.transparencyFactor);

			canvas.draw(img, elem.pos);
		});
	}
}

void BattleEffectsController::loadColorMuxers()
{
	const JsonNode config(JsonPath::builtin("config/battleEffects.json"));

	for(auto & muxer : config["colorMuxers"].Struct())
	{
		ColorMuxerEffect effect;
		std::string identifier = muxer.first;

		for (const JsonNode & entry : muxer.second.Vector() )
		{
			effect.timePoints.push_back(entry["time"].Float());
			effect.effectColors.push_back(ColorRGBA(255*entry["color"][0].Float(), 255*entry["color"][1].Float(), 255*entry["color"][2].Float(), 255*entry["color"][3].Float()));
			effect.transparency.push_back(entry["alpha"].Float() * 255);
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
