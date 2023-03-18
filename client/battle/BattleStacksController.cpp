/*
 * BattleStacksController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleStacksController.h"

#include "BattleSiegeController.h"
#include "BattleInterfaceClasses.h"
#include "BattleInterface.h"
#include "BattleActionsController.h"
#include "BattleAnimationClasses.h"
#include "BattleFieldController.h"
#include "BattleEffectsController.h"
#include "BattleProjectileController.h"
#include "BattleWindow.h"
#include "BattleRenderer.h"
#include "CreatureAnimation.h"

#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../render/Colors.h"
#include "../render/Canvas.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../CCallback.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/CGameState.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"
#include "../../lib/TextOperations.h"

static void onAnimationFinished(const CStack *stack, std::weak_ptr<CreatureAnimation> anim)
{
	std::shared_ptr<CreatureAnimation> animation = anim.lock();
	if(!animation)
		return;

	if (!stack->isFrozen() && animation->getType() == ECreatureAnimType::FROZEN)
		animation->setType(ECreatureAnimType::HOLDING);

	if (animation->isIdle())
	{
		const CCreature *creature = stack->getCreature();

		if (stack->isFrozen())
			animation->setType(ECreatureAnimType::FROZEN);
		else
		if (animation->framesInGroup(ECreatureAnimType::MOUSEON) > 0)
		{
			if (CRandomGenerator::getDefault().nextDouble(99.0) < creature->animation.timeBetweenFidgets *10)
				animation->playOnce(ECreatureAnimType::MOUSEON);
			else
				animation->setType(ECreatureAnimType::HOLDING);
		}
		else
		{
			animation->setType(ECreatureAnimType::HOLDING);
		}
	}
	// always reset callback
	animation->onAnimationReset += std::bind(&onAnimationFinished, stack, anim);
}

BattleStacksController::BattleStacksController(BattleInterface & owner):
	owner(owner),
	activeStack(nullptr),
	stackToActivate(nullptr),
	selectedStack(nullptr),
	animIDhelper(0)
{
	//preparing graphics for displaying amounts of creatures
	amountNormal     = IImage::createFromFile("CMNUMWIN.BMP", EImageBlitMode::COLORKEY);
	amountPositive   = IImage::createFromFile("CMNUMWIN.BMP", EImageBlitMode::COLORKEY);
	amountNegative   = IImage::createFromFile("CMNUMWIN.BMP", EImageBlitMode::COLORKEY);
	amountEffNeutral = IImage::createFromFile("CMNUMWIN.BMP", EImageBlitMode::COLORKEY);

	static const auto shifterNormal   = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.6f, 0.2f, 1.0f );
	static const auto shifterPositive = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.2f, 1.0f, 0.2f );
	static const auto shifterNegative = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 0.2f, 0.2f );
	static const auto shifterNeutral  = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 1.0f, 0.2f );

	amountNormal->adjustPalette(shifterNormal, 0);
	amountPositive->adjustPalette(shifterPositive, 0);
	amountNegative->adjustPalette(shifterNegative, 0);
	amountEffNeutral->adjustPalette(shifterNeutral, 0);

	//Restore border color {255, 231, 132, 255} to its original state
	amountNormal->resetPalette(26);
	amountPositive->resetPalette(26);
	amountNegative->resetPalette(26);
	amountEffNeutral->resetPalette(26);

	std::vector<const CStack*> stacks = owner.curInt->cb->battleGetAllStacks(true);
	for(const CStack * s : stacks)
	{
		stackAdded(s, true);
	}
}

BattleHex BattleStacksController::getStackCurrentPosition(const CStack * stack) const
{
	if ( !stackAnimation.at(stack->ID)->isMoving())
		return stack->getPosition();

	if (stack->hasBonusOfType(Bonus::FLYING) && stackAnimation.at(stack->ID)->getType() == ECreatureAnimType::MOVING )
		return BattleHex::HEX_AFTER_ALL;

	for (auto & anim : currentAnimations)
	{
		// certainly ugly workaround but fixes quite annoying bug
		// stack position will be updated only *after* movement is finished
		// before this - stack is always at its initial position. Thus we need to find
		// its current position. Which can be found only in this class
		if (StackMoveAnimation *move = dynamic_cast<StackMoveAnimation*>(anim))
		{
			if (move->stack == stack)
				return std::max(move->prevHex, move->nextHex);
		}
	}
	return stack->getPosition();
}

void BattleStacksController::collectRenderableObjects(BattleRenderer & renderer)
{
	auto stacks = owner.curInt->cb->battleGetAllStacks(false);

	for (auto stack : stacks)
	{
		if (stackAnimation.find(stack->ID) == stackAnimation.end()) //e.g. for summoned but not yet handled stacks
			continue;

		//FIXME: hack to ignore ghost stacks
		if ((stackAnimation[stack->ID]->getType() == ECreatureAnimType::DEAD || stackAnimation[stack->ID]->getType() == ECreatureAnimType::HOLDING) && stack->isGhost())
			continue;

		auto layer = stackAnimation[stack->ID]->isDead() ? EBattleFieldLayer::CORPSES : EBattleFieldLayer::STACKS;
		auto location = getStackCurrentPosition(stack);

		renderer.insert(layer, location, [this, stack]( BattleRenderer::RendererRef renderer ){
			showStack(renderer, stack);
		});

		if (stackNeedsAmountBox(stack))
		{
			renderer.insert(EBattleFieldLayer::STACK_AMOUNTS, location, [this, stack]( BattleRenderer::RendererRef renderer ){
				showStackAmountBox(renderer, stack);
			});
		}
	}
}

void BattleStacksController::stackReset(const CStack * stack)
{
	owner.checkForAnimations();

	//reset orientation?
	//stackFacingRight[stack->ID] = stack->side == BattleSide::ATTACKER;

	auto iter = stackAnimation.find(stack->ID);

	if(iter == stackAnimation.end())
	{
		logGlobal->error("Unit %d have no animation", stack->ID);
		return;
	}

	auto animation = iter->second;

	if(stack->alive() && animation->isDeadOrDying())
	{
		owner.addToAnimationStage(EAnimationEvents::HIT, [=]()
		{
			addNewAnim(new ResurrectionAnimation(owner, stack));
		});
	}
}

void BattleStacksController::stackAdded(const CStack * stack, bool instant)
{
	// Tower shooters have only their upper half visible
	static const int turretCreatureAnimationHeight = 225;

	stackFacingRight[stack->ID] = stack->side == BattleSide::ATTACKER; // must be set before getting stack position

	Point coords = getStackPositionAtHex(stack->getPosition(), stack);

	if(stack->initialPosition < 0) //turret
	{
		assert(owner.siegeController);

		const CCreature *turretCreature = owner.siegeController->getTurretCreature();

		stackAnimation[stack->ID] = AnimationControls::getAnimation(turretCreature);
		stackAnimation[stack->ID]->pos.h = turretCreatureAnimationHeight;

		coords = owner.siegeController->getTurretCreaturePosition(stack->initialPosition);
	}
	else
	{
		stackAnimation[stack->ID] = AnimationControls::getAnimation(stack->getCreature());
		stackAnimation[stack->ID]->onAnimationReset += std::bind(&onAnimationFinished, stack, stackAnimation[stack->ID]);
		stackAnimation[stack->ID]->pos.h = stackAnimation[stack->ID]->getHeight();
	}
	stackAnimation[stack->ID]->pos.x = coords.x;
	stackAnimation[stack->ID]->pos.y = coords.y;
	stackAnimation[stack->ID]->pos.w = stackAnimation[stack->ID]->getWidth();
	stackAnimation[stack->ID]->setType(ECreatureAnimType::HOLDING);

	if (!instant)
	{
		// immediately make stack transparent, giving correct shifter time to start
		auto shifterFade = ColorFilter::genAlphaShifter(0);
		setStackColorFilter(shifterFade, stack, nullptr, true);

		owner.addToAnimationStage(EAnimationEvents::HIT, [=]()
		{
			addNewAnim(new ColorTransformAnimation(owner, stack, "summonFadeIn", nullptr));
			if (stack->isClone())
				addNewAnim(new ColorTransformAnimation(owner, stack, "cloning", SpellID(SpellID::CLONE).toSpell() ));
		});
	}
}

void BattleStacksController::setActiveStack(const CStack *stack)
{
	if (activeStack) // update UI
		stackAnimation[activeStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		stackAnimation[activeStack->ID]->setBorderColor(AnimationControls::getGoldBorder());

	owner.windowObject->blockUI(activeStack == nullptr);
}

bool BattleStacksController::stackNeedsAmountBox(const CStack * stack) const
{
	BattleHex currentActionTarget;
	if(owner.curInt->curAction)
	{
		auto target = owner.curInt->curAction->getTarget(owner.curInt->cb.get());
		if(!target.empty())
			currentActionTarget = target.at(0).hexValue;
	}

	//do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->getCount() == 1)
		return false;

	if(!stack->alive())
		return false;

	//hide box when target is going to die anyway - do not display "0 creatures"
	if(stack->getCount() == 0)
		return false;

	// if stack has any ongoing animation - hide the box
	for(auto anim : currentAnimations)
	{
		auto stackAnimation = dynamic_cast<BattleStackAnimation*>(anim);
		if(stackAnimation && (stackAnimation->stack->ID == stack->ID))
			return false;
	}

	return true;
}

std::shared_ptr<IImage> BattleStacksController::getStackAmountBox(const CStack * stack)
{
	std::vector<si32> activeSpells = stack->activeSpells();

	if ( activeSpells.empty())
		return amountNormal;

	int effectsPositivness = 0;

	for ( auto const & spellID : activeSpells)
		effectsPositivness += CGI->spellh->objects.at(spellID)->positiveness;

	if (effectsPositivness > 0)
		return amountPositive;

	if (effectsPositivness < 0)
		return amountNegative;

	return amountEffNeutral;
}

void BattleStacksController::showStackAmountBox(Canvas & canvas, const CStack * stack)
{
	//blitting amount background box
	auto amountBG = getStackAmountBox(stack);

	const int sideShift = stack->side == BattleSide::ATTACKER ? 1 : -1;
	const int reverseSideShift = stack->side == BattleSide::ATTACKER ? -1 : 1;
	const BattleHex nextPos = stack->getPosition() + sideShift;
	const bool edge = stack->getPosition() % GameConstants::BFIELD_WIDTH == (stack->side == BattleSide::ATTACKER ? GameConstants::BFIELD_WIDTH - 2 : 1);
	const bool moveInside = !edge && !owner.fieldController->stackCountOutsideHex(nextPos);

	int xAdd = (stack->side == BattleSide::ATTACKER ? 220 : 202) +
			(stack->doubleWide() ? 44 : 0) * sideShift +
			(moveInside ? amountBG->width() + 10 : 0) * reverseSideShift;
	int yAdd = 260 + ((stack->side == BattleSide::ATTACKER || moveInside) ? 0 : -15);

	canvas.draw(amountBG, stackAnimation[stack->ID]->pos.topLeft() + Point(xAdd, yAdd));

	//blitting amount
	Point textPos = stackAnimation[stack->ID]->pos.topLeft() + amountBG->dimensions()/2 + Point(xAdd, yAdd);

	canvas.drawText(textPos, EFonts::FONT_TINY, Colors::WHITE, ETextAlignment::CENTER, TextOperations::formatMetric(stack->getCount(), 4));
}

void BattleStacksController::showStack(Canvas & canvas, const CStack * stack)
{
	ColorFilter fullFilter = ColorFilter::genEmptyShifter();
	for (auto const & filter : stackFilterEffects)
	{
		if (filter.target == stack)
			fullFilter = ColorFilter::genCombined(fullFilter, filter.effect);
	}

	bool stackHasProjectile = owner.projectilesController->hasActiveProjectile(stack, true);

	if (stackHasProjectile)
		stackAnimation[stack->ID]->pause();
	else
		stackAnimation[stack->ID]->play();

	stackAnimation[stack->ID]->nextFrame(canvas, fullFilter, facingRight(stack)); // do actual blit
	stackAnimation[stack->ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);
}

void BattleStacksController::update()
{
	updateHoveredStacks();
	updateBattleAnimations();
}

void BattleStacksController::initializeBattleAnimations()
{
	auto copiedVector = currentAnimations;
	for (auto & elem : copiedVector)
		if (elem && !elem->isInitialized())
			elem->tryInitialize();
}

void BattleStacksController::stepFrameBattleAnimations()
{
	// operate on copy - to prevent potential iterator invalidation due to push_back's
	// FIXME? : remove remaining calls to addNewAnim from BattleAnimation::nextFrame (only Catapult explosion at the time of writing)
	auto copiedVector = currentAnimations;
	for (auto & elem : copiedVector)
		if (elem && elem->isInitialized())
			elem->nextFrame();
}

void BattleStacksController::updateBattleAnimations()
{
	bool hadAnimations = !currentAnimations.empty();
	initializeBattleAnimations();
	stepFrameBattleAnimations();
	vstd::erase(currentAnimations, nullptr);

	if (hadAnimations && currentAnimations.empty())
	{
		owner.executeStagedAnimations();
		if (currentAnimations.empty())
			owner.onAnimationsFinished();
	}

	initializeBattleAnimations();
}

void BattleStacksController::addNewAnim(BattleAnimation *anim)
{
	owner.onAnimationsStarted();
	currentAnimations.push_back(anim);
}

void BattleStacksController::stackRemoved(uint32_t stackID)
{
	if (getActiveStack() && getActiveStack()->ID == stackID)
	{
		BattleAction *action = new BattleAction();
		action->side = owner.defendingHeroInstance ? (owner.curInt->playerID == owner.defendingHeroInstance->tempOwner) : false;
		action->actionType = EActionType::CANCEL;
		action->stackNumber = getActiveStack()->ID;
		owner.givenCommand.setn(action);
		setActiveStack(nullptr);
	}
}

void BattleStacksController::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	owner.addToAnimationStage(EAnimationEvents::HIT, [=](){
		// remove any potentially erased petrification effect
		removeExpiredColorFilters();
	});

	for(auto & attackedInfo : attackedInfos)
	{
		if (!attackedInfo.attacker)
			continue;

		// In H3, attacked stack will not reverse on ranged attack
		if (attackedInfo.indirectAttack)
			continue;

		// Another type of indirect attack - dragon breath
		if (!CStack::isMeleeAttackPossible(attackedInfo.attacker, attackedInfo.defender))
			continue;

		// defender need to face in direction opposited to out attacker
		bool needsReverse = shouldAttackFacingRight(attackedInfo.attacker, attackedInfo.defender) == facingRight(attackedInfo.defender);

		// FIXME: this check is better, however not usable since stacksAreAttacked is called after net pack is applyed - petrification is already removed
		// if (needsReverse && !attackedInfo.defender->isFrozen())
		if (needsReverse && stackAnimation[attackedInfo.defender->ID]->getType() != ECreatureAnimType::FROZEN)
		{
			owner.addToAnimationStage(EAnimationEvents::MOVEMENT, [=]()
			{
				addNewAnim(new ReverseAnimation(owner, attackedInfo.defender, attackedInfo.defender->getPosition()));
			});
		}
	}

	for(auto & attackedInfo : attackedInfos)
	{
		bool useDeathAnim   = attackedInfo.killed;
		bool useDefenceAnim = attackedInfo.defender->defendingAnim && !attackedInfo.indirectAttack && !attackedInfo.killed;

		EAnimationEvents usedEvent = useDefenceAnim ? EAnimationEvents::ATTACK : EAnimationEvents::HIT;

		owner.addToAnimationStage(usedEvent, [=]()
		{
			if (useDeathAnim)
				addNewAnim(new DeathAnimation(owner, attackedInfo.defender, attackedInfo.indirectAttack));
			else if(useDefenceAnim)
				addNewAnim(new DefenceAnimation(owner, attackedInfo.defender));
			else
				addNewAnim(new HittedAnimation(owner, attackedInfo.defender));

			if (attackedInfo.fireShield)
				owner.effectsController->displayEffect(EBattleEffect::FIRE_SHIELD, "FIRESHIE", attackedInfo.attacker->getPosition());

			if (attackedInfo.spellEffect != SpellID::NONE)
			{
				auto spell = attackedInfo.spellEffect.toSpell();
				if (!spell->getCastSound().empty())
					CCS->soundh->playSound(spell->getCastSound());


				owner.displaySpellEffect(spell, attackedInfo.defender->getPosition());
			}
		});
	}

	for (auto & attackedInfo : attackedInfos)
	{
		if (attackedInfo.rebirth)
		{
			owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
				owner.effectsController->displayEffect(EBattleEffect::RESURRECT, "RESURECT", attackedInfo.defender->getPosition());
				addNewAnim(new ResurrectionAnimation(owner, attackedInfo.defender));
			});
		}

		if (attackedInfo.killed && attackedInfo.defender->summoned)
		{
			owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
				addNewAnim(new ColorTransformAnimation(owner, attackedInfo.defender, "summonFadeOut", nullptr));
				stackRemoved(attackedInfo.defender->ID);
			});
		}
	}
	owner.executeStagedAnimations();
	owner.waitForAnimations();
}

void BattleStacksController::stackTeleported(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	assert(destHex.size() > 0);
	owner.checkForAnimations();

	owner.addToAnimationStage(EAnimationEvents::HIT, [=](){
		addNewAnim( new ColorTransformAnimation(owner, stack, "teleportFadeOut", nullptr) );
	});

	owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
		stackAnimation[stack->ID]->pos.moveTo(getStackPositionAtHex(destHex.back(), stack));
		addNewAnim( new ColorTransformAnimation(owner, stack, "teleportFadeIn", nullptr) );
	});

	// animations will be executed by spell
}

void BattleStacksController::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	assert(destHex.size() > 0);
	owner.checkForAnimations();

	if(shouldRotate(stack, stack->getPosition(), destHex[0]))
	{
		owner.addToAnimationStage(EAnimationEvents::ROTATE, [&]()
		{
			addNewAnim(new ReverseAnimation(owner, stack, stack->getPosition()));
		});
	}

	owner.addToAnimationStage(EAnimationEvents::MOVE_START, [&]()
	{
		addNewAnim(new MovementStartAnimation(owner, stack));
	});

	if (!stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1)))
	{
		owner.addToAnimationStage(EAnimationEvents::MOVEMENT, [&]()
		{
			addNewAnim(new MovementAnimation(owner, stack, destHex, distance));
		});
	}

	owner.addToAnimationStage(EAnimationEvents::MOVE_END, [&]()
	{
		addNewAnim(new MovementEndAnimation(owner, stack, destHex.back()));
	});

	owner.executeStagedAnimations();
	owner.waitForAnimations();
}

bool BattleStacksController::shouldAttackFacingRight(const CStack * attacker, const CStack * defender)
{
	bool mustReverse = owner.curInt->cb->isToReverse(
				attacker,
				defender);

	if (attacker->side == BattleSide::ATTACKER)
		return !mustReverse;
	else
		return mustReverse;
}

void BattleStacksController::stackAttacking( const StackAttackInfo & info )
{
	owner.checkForAnimations();

	auto attacker    = info.attacker;
	auto defender    = info.defender;
	auto tile        = info.tile;
	auto spellEffect = info.spellEffect;
	auto multiAttack = !info.secondaryDefender.empty();
	bool needsReverse = false;

	if (info.indirectAttack)
	{
		needsReverse = shouldRotate(attacker, attacker->position, info.tile);
	}
	else
	{
		needsReverse = shouldAttackFacingRight(attacker, defender) != facingRight(attacker);
	}

	if (needsReverse)
	{
		owner.addToAnimationStage(EAnimationEvents::MOVEMENT, [=]()
		{
			addNewAnim(new ReverseAnimation(owner, attacker, attacker->getPosition()));
		});
	}

	if(info.lucky)
	{
		owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(-45));
			owner.effectsController->displayEffect(EBattleEffect::GOOD_LUCK, "GOODLUCK", attacker->getPosition());
		});
	}

	if(info.unlucky)
	{
		owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(-44));
			owner.effectsController->displayEffect(EBattleEffect::BAD_LUCK, "BADLUCK", attacker->getPosition());
		});
	}

	if(info.deathBlow)
	{
		owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(365));
			owner.effectsController->displayEffect(EBattleEffect::DEATH_BLOW, "DEATHBLO", defender->getPosition());
		});

		for(auto elem : info.secondaryDefender)
		{
			owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
				owner.effectsController->displayEffect(EBattleEffect::DEATH_BLOW, elem->getPosition());
			});
		}
	}

	owner.addToAnimationStage(EAnimationEvents::ATTACK, [=]()
	{
		if (info.indirectAttack)
		{
			addNewAnim(new ShootingAnimation(owner, attacker, tile, defender));
		}
		else
		{
			addNewAnim(new MeleeAttackAnimation(owner, attacker, tile, defender, multiAttack));
		}
	});

	if (info.spellEffect != SpellID::NONE)
	{
		owner.addToAnimationStage(EAnimationEvents::HIT, [=]()
		{
			owner.displaySpellHit(spellEffect.toSpell(), tile);
		});
	}

	if (info.lifeDrain)
	{
		owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=]()
		{
			owner.effectsController->displayEffect(EBattleEffect::DRAIN_LIFE, "DRAINLIF", attacker->getPosition());
		});
	}

	//return, animation playback will be handled by stacksAreAttacked
}

bool BattleStacksController::shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex) const
{
	Point begPosition = getStackPositionAtHex(oldPos,stack);
	Point endPosition = getStackPositionAtHex(nextHex, stack);

	if((begPosition.x > endPosition.x) && facingRight(stack))
		return true;
	else if((begPosition.x < endPosition.x) && !facingRight(stack))
		return true;

	return false;
}

void BattleStacksController::endAction(const BattleAction* action)
{
	owner.checkForAnimations();

	//check if we should reverse stacks
	TStacks stacks = owner.curInt->cb->battleGetStacks(CBattleCallback::MINE_AND_ENEMY);

	for (const CStack *s : stacks)
	{
		bool shouldFaceRight  = s && s->side == BattleSide::ATTACKER;

		if (s && facingRight(s) != shouldFaceRight && s->alive() && stackAnimation[s->ID]->isIdle())
		{
			addNewAnim(new ReverseAnimation(owner, s, s->getPosition()));
		}
	}
	owner.executeStagedAnimations();
	owner.waitForAnimations();

	owner.windowObject->blockUI(activeStack == nullptr);
	removeExpiredColorFilters();
}

void BattleStacksController::startAction(const BattleAction* action)
{
	removeExpiredColorFilters();
}

void BattleStacksController::stackActivated(const CStack *stack)
{
	stackToActivate = stack;
	owner.waitForAnimations();
	logAnim->debug("Activating next stack");
	owner.activateStack();
}

void BattleStacksController::activateStack()
{
	if ( !currentAnimations.empty())
		return;

	if ( !stackToActivate)
		return;

	owner.trySetActivePlayer(stackToActivate->owner);

	setActiveStack(stackToActivate);
	stackToActivate = nullptr;

	const CStack * s = getActiveStack();
	if(!s)
		return;
}

void BattleStacksController::setSelectedStack(const CStack *stack)
{
	selectedStack = stack;
}

const CStack* BattleStacksController::getSelectedStack() const
{
	return selectedStack;
}

const CStack* BattleStacksController::getActiveStack() const
{
	return activeStack;
}

bool BattleStacksController::facingRight(const CStack * stack) const
{
	return stackFacingRight.at(stack->ID);
}

Point BattleStacksController::getStackPositionAtHex(BattleHex hexNum, const CStack * stack) const
{
	Point ret(-500, -500); //returned value
	if(stack && stack->initialPosition < 0) //creatures in turrets
		return owner.siegeController->getTurretCreaturePosition(stack->initialPosition);

	static const Point basePos(-190, -139); // position of creature in topleft corner
	static const int imageShiftX = 30; // X offset to base pos for facing right stacks, negative for facing left

	ret.x = basePos.x + 22 * ( (hexNum.getY() + 1)%2 ) + 44 * hexNum.getX();
	ret.y = basePos.y + 42 * hexNum.getY();

	if (stack)
	{
		if(facingRight(stack))
			ret.x += imageShiftX;
		else
			ret.x -= imageShiftX;

		//shifting position for double - hex creatures
		if(stack->doubleWide())
		{
			if(stack->side == BattleSide::ATTACKER)
			{
				if(facingRight(stack))
					ret.x -= 44;
			}
			else
			{
				if(!facingRight(stack))
					ret.x += 44;
			}
		}
	}
	//returning
	return ret;
}

void BattleStacksController::setStackColorFilter(const ColorFilter & effect, const CStack * target, const CSpell * source, bool persistent)
{
	for (auto & filter : stackFilterEffects)
	{
		if (filter.target == target && filter.source == source)
		{
			filter.effect     = effect;
			filter.persistent = persistent;
			return;
		}
	}
	stackFilterEffects.push_back({ effect, target, source, persistent });
}

void BattleStacksController::removeExpiredColorFilters()
{
	vstd::erase_if(stackFilterEffects, [&](const BattleStackFilterEffect & filter)
	{
		if (!filter.persistent)
		{
			if (filter.source && !filter.target->hasBonus(Selector::source(Bonus::SPELL_EFFECT, filter.source->id), Selector::all))
				return true;
			if (filter.effect == ColorFilter::genEmptyShifter())
				return true;
		}
		return false;
	});
}

void BattleStacksController::updateHoveredStacks()
{
	auto newStacks = selectHoveredStacks();

	for (auto const * stack : mouseHoveredStacks)
	{
		if (vstd::contains(newStacks, stack))
			continue;

		if (stack == activeStack)
			stackAnimation[stack->ID]->setBorderColor(AnimationControls::getGoldBorder());
		else
			stackAnimation[stack->ID]->setBorderColor(AnimationControls::getNoBorder());
	}

	for (auto const * stack : newStacks)
	{
		if (vstd::contains(mouseHoveredStacks, stack))
			continue;

		stackAnimation[stack->ID]->setBorderColor(AnimationControls::getBlueBorder());
		if (stackAnimation[stack->ID]->framesInGroup(ECreatureAnimType::MOUSEON) > 0 && stack->alive() && !stack->isFrozen())
			stackAnimation[stack->ID]->playOnce(ECreatureAnimType::MOUSEON);
	}

	mouseHoveredStacks = newStacks;
}

std::vector<const CStack *> BattleStacksController::selectHoveredStacks()
{
	// only allow during our turn - do not try to highlight creatures while they are in the middle of actions
	if (!activeStack)
		return {};

	if(owner.hasAnimations())
		return {};

	auto hoveredQueueUnitId = owner.windowObject->getQueueHoveredUnitId();
	if(hoveredQueueUnitId.is_initialized())
	{
		return { owner.curInt->cb->battleGetStackByID(hoveredQueueUnitId.value(), true) };
	}

	auto hoveredHex = owner.fieldController->getHoveredHex();

	if (!hoveredHex.isValid())
		return {};

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = owner.actionsController->getCurrentCastMode();
	spell = owner.actionsController->getCurrentSpell();
	caster = owner.actionsController->getCurrentSpellcaster();

	if(caster && spell && owner.actionsController->currentActionSpellcasting(hoveredHex) ) //when casting spell
	{
		spells::Target target;
		target.emplace_back(hoveredHex);

		spells::BattleCast event(owner.curInt->cb.get(), caster, mode, spell);
		auto mechanics = spell->battleMechanics(&event);
		return mechanics->getAffectedStacks(target);
	}

	if(hoveredHex.isValid())
	{
		const CStack * const stack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);

		if (stack)
			return {stack};
	}

	return {};
}

const std::vector<uint32_t> BattleStacksController::getHoveredStacksUnitIds() const
{
	auto result = std::vector<uint32_t>();
	for (auto const * stack : mouseHoveredStacks)
	{
		result.push_back(stack->unitId());
	}

	return result;
}
