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
#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../media/ISoundPlayer.h"
#include "../render/Colors.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"

#include "../../CCallback.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"

static void onAnimationFinished(const CStack *stack, std::weak_ptr<CreatureAnimation> anim)
{
	std::shared_ptr<CreatureAnimation> animation = anim.lock();
	if(!animation)
		return;

	if (!stack->isFrozen() && animation->getType() == ECreatureAnimType::FROZEN)
		animation->setType(ECreatureAnimType::HOLDING);

	if (animation->isIdle())
	{
		const CCreature *creature = stack->unitType();

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
	animIDhelper(0)
{
	//preparing graphics for displaying amounts of creatures
	amountNormal     = GH.renderHandler().loadImage(ImagePath::builtin("CMNUMWIN.BMP"), EImageBlitMode::COLORKEY);
	amountPositive   = GH.renderHandler().loadImage(ImagePath::builtin("CMNUMWIN.BMP"), EImageBlitMode::COLORKEY);
	amountNegative   = GH.renderHandler().loadImage(ImagePath::builtin("CMNUMWIN.BMP"), EImageBlitMode::COLORKEY);
	amountEffNeutral = GH.renderHandler().loadImage(ImagePath::builtin("CMNUMWIN.BMP"), EImageBlitMode::COLORKEY);

	static const auto shifterNormal   = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.6f, 0.2f, 1.0f );
	static const auto shifterPositive = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.2f, 1.0f, 0.2f );
	static const auto shifterNegative = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 0.2f, 0.2f );
	static const auto shifterNeutral  = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 1.0f, 0.2f );

	// do not change border color
	static const int32_t ignoredMask = 1 << 26;

	amountNormal->adjustPalette(shifterNormal, ignoredMask);
	amountPositive->adjustPalette(shifterPositive, ignoredMask);
	amountNegative->adjustPalette(shifterNegative, ignoredMask);
	amountEffNeutral->adjustPalette(shifterNeutral, ignoredMask);

	std::vector<const CStack*> stacks = owner.getBattle()->battleGetAllStacks(true);
	for(const CStack * s : stacks)
	{
		stackAdded(s, true);
	}
}

BattleHex BattleStacksController::getStackCurrentPosition(const CStack * stack) const
{
	if ( !stackAnimation.at(stack->unitId())->isMoving())
		return stack->getPosition();

	if (stack->hasBonusOfType(BonusType::FLYING) && stackAnimation.at(stack->unitId())->getType() == ECreatureAnimType::MOVING )
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
	auto stacks = owner.getBattle()->battleGetAllStacks(false);

	for (auto stack : stacks)
	{
		if (stackAnimation.find(stack->unitId()) == stackAnimation.end()) //e.g. for summoned but not yet handled stacks
			continue;

		//FIXME: hack to ignore ghost stacks
		if ((stackAnimation[stack->unitId()]->getType() == ECreatureAnimType::DEAD || stackAnimation[stack->unitId()]->getType() == ECreatureAnimType::HOLDING) && stack->isGhost())
			continue;

		auto layer = stackAnimation[stack->unitId()]->isDead() ? EBattleFieldLayer::CORPSES : EBattleFieldLayer::STACKS;
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
	auto iter = stackAnimation.find(stack->unitId());

	if(iter == stackAnimation.end())
	{
		logGlobal->error("Unit %d have no animation", stack->unitId());
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
	static const int turretCreatureAnimationHeight = 232;

	stackFacingRight[stack->unitId()] = stack->unitSide() == BattleSide::ATTACKER; // must be set before getting stack position

	Point coords = getStackPositionAtHex(stack->getPosition(), stack);

	if(stack->initialPosition < 0) //turret
	{
		assert(owner.siegeController);

		const CCreature *turretCreature = owner.siegeController->getTurretCreature();

		stackAnimation[stack->unitId()] = AnimationControls::getAnimation(turretCreature);
		stackAnimation[stack->unitId()]->pos.h = turretCreatureAnimationHeight;
		stackAnimation[stack->unitId()]->pos.w = stackAnimation[stack->unitId()]->getWidth();

		// FIXME: workaround for visible animation of Medusa tails (animation disabled in H3)
		if (turretCreature->getId() == CreatureID::MEDUSA )
			stackAnimation[stack->unitId()]->pos.w = 250;

		coords = owner.siegeController->getTurretCreaturePosition(stack->initialPosition);
	}
	else
	{
		stackAnimation[stack->unitId()] = AnimationControls::getAnimation(stack->unitType());
		stackAnimation[stack->unitId()]->onAnimationReset += std::bind(&onAnimationFinished, stack, stackAnimation[stack->unitId()]);
		stackAnimation[stack->unitId()]->pos.h = stackAnimation[stack->unitId()]->getHeight();
		stackAnimation[stack->unitId()]->pos.w = stackAnimation[stack->unitId()]->getWidth();
	}
	stackAnimation[stack->unitId()]->pos.x = coords.x;
	stackAnimation[stack->unitId()]->pos.y = coords.y;
	stackAnimation[stack->unitId()]->setType(ECreatureAnimType::HOLDING);

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
		stackAnimation[activeStack->unitId()]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		stackAnimation[activeStack->unitId()]->setBorderColor(AnimationControls::getGoldBorder());

	owner.windowObject->blockUI(activeStack == nullptr);

	if (activeStack)
		stackAmountBoxHidden.clear();
}

bool BattleStacksController::stackNeedsAmountBox(const CStack * stack) const
{
	//do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
	if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON) && stack->getCount() == 1)
		return false;

	if(!stack->alive())
		return false;

	//hide box when target is going to die anyway - do not display "0 creatures"
	if(stack->getCount() == 0)
		return false;

	// if stack has any ongoing animation - hide the box
	if (stackAmountBoxHidden.count(stack->unitId()))
		return false;

	return true;
}

std::shared_ptr<IImage> BattleStacksController::getStackAmountBox(const CStack * stack)
{
	std::vector<SpellID> activeSpells = stack->activeSpells();

	if ( activeSpells.empty())
		return amountNormal;

	int effectsPositivness = 0;

	for(const auto & spellID : activeSpells)
	{
		auto positiveness = CGI->spells()->getByIndex(spellID)->getPositiveness();
		if(!boost::logic::indeterminate(positiveness))
		{
			if(positiveness)
				effectsPositivness++;
			else
				effectsPositivness--;
		}
	}

	if (effectsPositivness > 0)
		return amountPositive;

	if (effectsPositivness < 0)
		return amountNegative;

	return amountEffNeutral;
}

void BattleStacksController::showStackAmountBox(Canvas & canvas, const CStack * stack)
{
	auto amountBG = getStackAmountBox(stack);

	bool doubleWide = stack->doubleWide();
	bool turnedRight = facingRight(stack);
	bool attacker = stack->unitSide() == BattleSide::ATTACKER;

	BattleHex stackPos = stack->getPosition();

	// double-wide unit turned around - use opposite hex for stack label
	if (doubleWide && turnedRight != attacker)
		stackPos = stack->occupiedHex();

	BattleHex frontPos = turnedRight ?
		stackPos.cloneInDirection(BattleHex::RIGHT) :
		stackPos.cloneInDirection(BattleHex::LEFT);

	bool moveInside = !owner.fieldController->stackCountOutsideHex(frontPos);

	Point boxPosition;

	if (moveInside)
	{
		boxPosition = owner.fieldController->hexPositionLocal(stackPos).center() + Point(-15, 1);
	}
	else
	{
		if (turnedRight)
			boxPosition = owner.fieldController->hexPositionLocal(frontPos).center() + Point (-22, 1);
		else
			boxPosition = owner.fieldController->hexPositionLocal(frontPos).center() + Point(-8, -14);
	}

	Point textPosition = Point(amountBG->dimensions().x/2 + boxPosition.x, boxPosition.y + graphics->fonts[EFonts::FONT_TINY]->getLineHeight() - 6);

	canvas.draw(amountBG, boxPosition);
	canvas.drawText(textPosition, EFonts::FONT_TINY, Colors::WHITE, ETextAlignment::TOPCENTER, TextOperations::formatMetric(stack->getCount(), 4));
}

void BattleStacksController::showStack(Canvas & canvas, const CStack * stack)
{
	ColorFilter fullFilter = ColorFilter::genEmptyShifter();
	for(const auto & filter : stackFilterEffects)
	{
		if (filter.target == stack)
			fullFilter = ColorFilter::genCombined(fullFilter, filter.effect);
	}

	stackAnimation[stack->unitId()]->nextFrame(canvas, fullFilter, facingRight(stack)); // do actual blit
}

void BattleStacksController::tick(uint32_t msPassed)
{
	updateHoveredStacks();
	updateBattleAnimations(msPassed);
}

void BattleStacksController::initializeBattleAnimations()
{
	auto copiedVector = currentAnimations;
	for (auto & elem : copiedVector)
		if (elem && !elem->isInitialized())
			elem->tryInitialize();
}

void BattleStacksController::tickFrameBattleAnimations(uint32_t msPassed)
{
	for (auto stack : owner.getBattle()->battleGetAllStacks(true))
	{
		if (stackAnimation.find(stack->unitId()) == stackAnimation.end()) //e.g. for summoned but not yet handled stacks
			continue;

		stackAnimation[stack->unitId()]->incrementFrame(msPassed / 1000.f);
	}

	// operate on copy - to prevent potential iterator invalidation due to push_back's
	// FIXME? : remove remaining calls to addNewAnim from BattleAnimation::nextFrame (only Catapult explosion at the time of writing)

	auto copiedVector = currentAnimations;
	for (auto & elem : copiedVector)
		if (elem && elem->isInitialized())
			elem->tick(msPassed);
}

void BattleStacksController::updateBattleAnimations(uint32_t msPassed)
{
	bool hadAnimations = !currentAnimations.empty();
	initializeBattleAnimations();
	tickFrameBattleAnimations(msPassed);
	vstd::erase(currentAnimations, nullptr);

	if (currentAnimations.empty())
		owner.executeStagedAnimations();

	if (hadAnimations && currentAnimations.empty())
		owner.onAnimationsFinished();

	initializeBattleAnimations();
}

void BattleStacksController::addNewAnim(BattleAnimation *anim)
{
	if (currentAnimations.empty())
		stackAmountBoxHidden.clear();

	owner.onAnimationsStarted();
	currentAnimations.push_back(anim);

	auto stackAnimation = dynamic_cast<BattleStackAnimation*>(anim);
	if(stackAnimation)
		stackAmountBoxHidden.insert(stackAnimation->stack->unitId());
}

void BattleStacksController::stackRemoved(uint32_t stackID)
{
	if (getActiveStack() && getActiveStack()->unitId() == stackID)
		setActiveStack(nullptr);
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

		// FIXME: this check is better, however not usable since stacksAreAttacked is called after net pack is applied - petrification is already removed
		// if (needsReverse && !attackedInfo.defender->isFrozen())
		if (needsReverse && stackAnimation[attackedInfo.defender->unitId()]->getType() != ECreatureAnimType::FROZEN)
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
				owner.effectsController->displayEffect(EBattleEffect::FIRE_SHIELD, AudioPath::builtin("FIRESHIE"), attackedInfo.attacker->getPosition());

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
				owner.effectsController->displayEffect(EBattleEffect::RESURRECT, AudioPath::builtin("RESURECT"), attackedInfo.defender->getPosition());
				addNewAnim(new ResurrectionAnimation(owner, attackedInfo.defender));
			});
		}

		if (attackedInfo.killed && attackedInfo.defender->summoned)
		{
			owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
				addNewAnim(new ColorTransformAnimation(owner, attackedInfo.defender, "summonFadeOut", nullptr));
				stackRemoved(attackedInfo.defender->unitId());
			});
		}
	}
	owner.executeStagedAnimations();
	owner.waitForAnimations();
}

void BattleStacksController::stackTeleported(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	assert(destHex.size() > 0);
	//owner.checkForAnimations(); // NOTE: at this point spellcast animations were added, but not executed

	owner.addToAnimationStage(EAnimationEvents::HIT, [=](){
		addNewAnim( new ColorTransformAnimation(owner, stack, "teleportFadeOut", nullptr) );
	});

	owner.addToAnimationStage(EAnimationEvents::AFTER_HIT, [=](){
		stackAnimation[stack->unitId()]->pos.moveTo(getStackPositionAtHex(destHex.back(), stack));
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

	if (!stack->hasBonus(Selector::typeSubtype(BonusType::FLYING, BonusCustomSubtype::movementTeleporting)))
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
	bool mustReverse = owner.getBattle()->isToReverse(attacker, defender);

	if (attacker->unitSide() == BattleSide::ATTACKER)
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
			owner.effectsController->displayEffect(EBattleEffect::GOOD_LUCK, AudioPath::builtin("GOODLUCK"), attacker->getPosition());
		});
	}

	if(info.unlucky)
	{
		owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(-44));
			owner.effectsController->displayEffect(EBattleEffect::BAD_LUCK, AudioPath::builtin("BADLUCK"), attacker->getPosition());
		});
	}

	if(info.deathBlow)
	{
		owner.addToAnimationStage(EAnimationEvents::BEFORE_HIT, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(365));
			owner.effectsController->displayEffect(EBattleEffect::DEATH_BLOW, AudioPath::builtin("DEATHBLO"), defender->getPosition());
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
			owner.effectsController->displayEffect(EBattleEffect::DRAIN_LIFE, AudioPath::builtin("DRAINLIF"), attacker->getPosition());
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

void BattleStacksController::endAction(const BattleAction & action)
{
	owner.checkForAnimations();

	//check if we should reverse stacks
	TStacks stacks = owner.getBattle()->battleGetStacks(CPlayerBattleCallback::MINE_AND_ENEMY);

	for (const CStack *s : stacks)
	{
		bool shouldFaceRight  = s && s->unitSide() == BattleSide::ATTACKER;

		if (s && facingRight(s) != shouldFaceRight && s->alive() && stackAnimation[s->unitId()]->isIdle())
		{
			addNewAnim(new ReverseAnimation(owner, s, s->getPosition()));
		}
	}
	owner.executeStagedAnimations();
	owner.waitForAnimations();

	stackAmountBoxHidden.clear();

	owner.windowObject->blockUI(activeStack == nullptr);
	removeExpiredColorFilters();
}

void BattleStacksController::startAction(const BattleAction & action)
{
	// if timer run out and we did not act in time - deactivate current stack
	setActiveStack(nullptr);
	removeExpiredColorFilters();
}

void BattleStacksController::stackActivated(const CStack *stack)
{
	stackToActivate = stack;
	owner.waitForAnimations();
	logAnim->debug("Activating next stack");
	owner.activateStack();
}

void BattleStacksController::deactivateStack()
{
	if (!activeStack) {
		return;
	}
	stackToActivate = activeStack;
	setActiveStack(nullptr);
}

void BattleStacksController::activateStack()
{
	if ( !currentAnimations.empty())
		return;

	if ( !stackToActivate)
		return;

	owner.trySetActivePlayer(stackToActivate->unitOwner());

	setActiveStack(stackToActivate);
	stackToActivate = nullptr;

	const CStack * s = getActiveStack();
	if(!s)
		return;
}

const CStack* BattleStacksController::getActiveStack() const
{
	return activeStack;
}

bool BattleStacksController::facingRight(const CStack * stack) const
{
	return stackFacingRight.at(stack->unitId());
}

Point BattleStacksController::getStackPositionAtHex(BattleHex hexNum, const CStack * stack) const
{
	Point ret(-500, -500); //returned value
	if(stack && stack->initialPosition < 0) //creatures in turrets
		return owner.siegeController->getTurretCreaturePosition(stack->initialPosition);

	static const Point basePos(-189, -139); // position of creature in topleft corner
	static const int imageShiftX = 29; // X offset to base pos for facing right stacks, negative for facing left

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
			if(stack->unitSide() == BattleSide::ATTACKER)
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
			if (filter.source && !filter.target->hasBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(filter.source->id)), Selector::all))
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

	if(newStacks.size() == 0)
		owner.windowObject->updateStackInfoWindow(nullptr);

	for(const auto * stack : mouseHoveredStacks)
	{
		if (vstd::contains(newStacks, stack))
			continue;

		if (stack == activeStack)
			stackAnimation[stack->unitId()]->setBorderColor(AnimationControls::getGoldBorder());
		else
			stackAnimation[stack->unitId()]->setBorderColor(AnimationControls::getNoBorder());
	}

	for(const auto * stack : newStacks)
	{
		if (vstd::contains(mouseHoveredStacks, stack))
			continue;

		owner.windowObject->updateStackInfoWindow(newStacks.size() == 1 && vstd::find_pos(newStacks, stack) == 0 ? stack : nullptr);
		stackAnimation[stack->unitId()]->setBorderColor(AnimationControls::getBlueBorder());
		if (stackAnimation[stack->unitId()]->framesInGroup(ECreatureAnimType::MOUSEON) > 0 && stack->alive() && !stack->isFrozen())
			stackAnimation[stack->unitId()]->playOnce(ECreatureAnimType::MOUSEON);
	}

	if(mouseHoveredStacks != newStacks)
		GH.windows().totalRedraw(); //fix for frozen stack info window and blue border in action bar

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
	if(hoveredQueueUnitId.has_value())
	{
		return { owner.getBattle()->battleGetStackByID(hoveredQueueUnitId.value(), true) };
	}

	auto hoveredHex = owner.fieldController->getHoveredHex();

	if (!hoveredHex.isValid())
		return {};

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = owner.actionsController->getCurrentCastMode();
	spell = owner.actionsController->getCurrentSpell(hoveredHex);
	caster = owner.actionsController->getCurrentSpellcaster();

	if(caster && spell && owner.actionsController->currentActionSpellcasting(hoveredHex) ) //when casting spell
	{
		spells::Target target;
		target.emplace_back(hoveredHex);

		spells::BattleCast event(owner.getBattle().get(), caster, mode, spell);
		auto mechanics = spell->battleMechanics(&event);
		return mechanics->getAffectedStacks(target);
	}

	if(hoveredHex.isValid())
	{
		const CStack * const stack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);

		if (stack)
			return {stack};
	}

	return {};
}

const std::vector<uint32_t> BattleStacksController::getHoveredStacksUnitIds() const
{
	auto result = std::vector<uint32_t>();
	for(const auto * stack : mouseHoveredStacks)
	{
		result.push_back(stack->unitId());
	}

	return result;
}
