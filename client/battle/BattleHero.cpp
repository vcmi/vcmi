/*
 * BattleHero.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleHero.h"

#include "BattleActionsController.h"
#include "BattleFieldController.h"
#include "BattleInterface.h"
#include "BattleRenderer.h"
#include "HeroInfoWindow.h"

#include "../GameEngine.h"
#include "../gui/CursorHandler.h"
#include "../gui/WindowHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../windows/CSpellWindow.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

const CGHeroInstance * BattleHero::instance() const
{
	return hero;
}

void BattleHero::tick(uint32_t msPassed)
{
	size_t groupIndex = static_cast<size_t>(phase);

	float timePassed = msPassed / 1000.f;

	flagCurrentFrame += currentSpeed * timePassed;
	currentFrame += currentSpeed * timePassed;

	if(flagCurrentFrame >= flagAnimation->size(0))
		flagCurrentFrame -= flagAnimation->size(0);

	if(currentFrame >= animation->size(groupIndex))
	{
		currentFrame -= animation->size(groupIndex);
		switchToNextPhase();
	}
}

void BattleHero::render(Canvas & canvas)
{
	size_t groupIndex = static_cast<size_t>(phase);

	auto flagFrame = flagAnimation->getImage(flagCurrentFrame, 0, true);
	auto heroFrame = animation->getImage(currentFrame, groupIndex, true);

	Point heroPosition = pos.center() - parent->pos.topLeft() - heroFrame->dimensions() / 2;
	Point flagPosition = pos.center() - parent->pos.topLeft() - flagFrame->dimensions() / 2;

	if(defender)
		flagPosition += Point(-4, -41);
	else
		flagPosition += Point(4, -41);

	canvas.draw(flagFrame, flagPosition);
	canvas.draw(heroFrame, heroPosition);
}

void BattleHero::pause()
{
	currentSpeed = 0.f;
}

void BattleHero::play()
{
	//H3 speed: 10 fps ( 100 ms per frame)
	currentSpeed = 10.f;
}

float BattleHero::getFrame() const
{
	return currentFrame;
}

void BattleHero::collectRenderableObjects(BattleRenderer & renderer)
{
	auto hex = defender ? BattleHex(GameConstants::BFIELD_WIDTH-1) : BattleHex(0);

	renderer.insert(EBattleFieldLayer::HEROES, hex, [this](BattleRenderer::RendererRef canvas)
	{
		render(canvas);
	});
}

void BattleHero::onPhaseFinished(const std::function<void()> & callback)
{
	phaseFinishedCallback = callback;
}

void BattleHero::setPhase(EHeroAnimType newPhase)
{
	nextPhase = newPhase;
	switchToNextPhase(); //immediately switch to next phase and then restore idling phase
	nextPhase = EHeroAnimType::HOLDING;
}

void BattleHero::heroLeftClicked()
{
	if(owner.actionsController->heroSpellcastingModeActive()) //we are casting a spell
		return;

	if(!hero || !owner.makingTurn())
		return;

	if(owner.getBattle()->battleCanCastSpell(hero, spells::Mode::HERO) == ESpellCastProblem::OK) //check conditions
	{
		ENGINE->cursor().set(Cursor::Map::POINTER);
		ENGINE->windows().createAndPushWindow<CSpellWindow>(hero, owner.getCurrentPlayerInterface());
	}
}

void BattleHero::heroRightClicked() const
{
	if(settings["battle"]["stickyHeroInfoWindows"].Bool())
		return;

	Point windowPosition;
	if(ENGINE->screenDimensions().x < 1000)
	{
		windowPosition.x = (!defender) ? owner.fieldController->pos.left() + 1 : owner.fieldController->pos.right() - 79;
		windowPosition.y = owner.fieldController->pos.y + 135;
	}
	else
	{
		windowPosition.x = (!defender) ? owner.fieldController->pos.left() - 93 : owner.fieldController->pos.right() + 15;
		windowPosition.y = owner.fieldController->pos.y;
	}

	InfoAboutHero targetHero;
	if(owner.makingTurn() || settings["session"]["spectate"].Bool())
	{
		const auto * h = defender ? owner.defendingHeroInstance : owner.attackingHeroInstance;
		targetHero.initFromHero(h, InfoAboutHero::EInfoLevel::INBATTLE);
		ENGINE->windows().createAndPushWindow<HeroInfoWindow>(targetHero, &windowPosition);
	}
}

void BattleHero::switchToNextPhase()
{
	phase = nextPhase;
	currentFrame = 0.f;

	auto copy = phaseFinishedCallback;
	phaseFinishedCallback.clear();
	copy();
}

BattleHero::BattleHero(const BattleInterface & owner, const CGHeroInstance * hero, bool defender)
	: defender(defender)
	, hero(hero)
	, owner(owner)
	, phase(EHeroAnimType::HOLDING)
	, nextPhase(EHeroAnimType::HOLDING)
	, currentSpeed(0.f)
	, currentFrame(0.f)
	, flagCurrentFrame(0.f)
{
	AnimationPath animationPath;

	if(!hero->getHeroType()->battleImage.empty())
		animationPath = hero->getHeroType()->battleImage;
	else if(hero->gender == EHeroGender::FEMALE)
		animationPath = hero->getHeroClass()->imageBattleFemale;
	else
		animationPath = hero->getHeroClass()->imageBattleMale;

	animation = ENGINE->renderHandler().loadAnimation(animationPath, EImageBlitMode::WITH_SHADOW);

	pos.w = 64;
	pos.h = 136;
	pos.x = owner.fieldController->pos.x + (defender ? (owner.fieldController->pos.w - pos.w) : 0);
	pos.y = owner.fieldController->pos.y;

	if(defender)
		animation->verticalFlip();

	if(defender)
		flagAnimation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CMFLAGR"), EImageBlitMode::COLORKEY);
	else
		flagAnimation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CMFLAGL"), EImageBlitMode::COLORKEY);

	flagAnimation->playerColored(hero->tempOwner);

	switchToNextPhase();
	play();

	addUsedEvents(TIME);
}
