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
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Canvas.h"
#include "../../lib/spells/ISpellMechanics.h"

#include "../../CCallback.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/CGameState.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"

static void onAnimationFinished(const CStack *stack, std::weak_ptr<CreatureAnimation> anim)
{
	std::shared_ptr<CreatureAnimation> animation = anim.lock();
	if(!animation)
		return;

	if (!stack->isFrozen() && animation->getType() == ECreatureAnimType::FROZEN)
		animation->setType(ECreatureAnimType::HOLDING);

	if (animation->isIdle())
	{
		if (stack->isFrozen())
			animation->setType(ECreatureAnimType::FROZEN);

		const CCreature *creature = stack->getCreature();

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
	stackCanCastSpell(false),
	creatureSpellToCast(uint32_t(-1)),
	animIDhelper(0)
{
	//preparing graphics for displaying amounts of creatures
	amountNormal     = IImage::createFromFile("CMNUMWIN.BMP");
	amountPositive   = IImage::createFromFile("CMNUMWIN.BMP");
	amountNegative   = IImage::createFromFile("CMNUMWIN.BMP");
	amountEffNeutral = IImage::createFromFile("CMNUMWIN.BMP");

	static const auto shifterNormal   = ColorFilter::genRangeShifter( 0,0,0, 0.6, 0.2, 1.0 );
	static const auto shifterPositive = ColorFilter::genRangeShifter( 0,0,0, 0.2, 1.0, 0.2 );
	static const auto shifterNegative = ColorFilter::genRangeShifter( 0,0,0, 1.0, 0.2, 0.2 );
	static const auto shifterNeutral  = ColorFilter::genRangeShifter( 0,0,0, 1.0, 1.0, 0.2 );

	amountNormal->adjustPalette(shifterNormal);
	amountPositive->adjustPalette(shifterPositive);
	amountNegative->adjustPalette(shifterNegative);
	amountEffNeutral->adjustPalette(shifterNeutral);

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
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

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
		owner.executeOnAnimationCondition(EAnimationEvents::HIT, true, [=]()
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

		owner.executeOnAnimationCondition(EAnimationEvents::HIT, true, [=]()
		{
			addNewAnim(ColorTransformAnimation::summonAnimation(owner, stack));
			if (stack->isClone())
				addNewAnim(ColorTransformAnimation::cloneAnimation(owner, stack, SpellID(SpellID::CLONE).toSpell()));
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

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->getCount() == 1) //do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
		return false;

	if(!stack->alive())
		return false;

	if(stack->getCount() == 0) //hide box when target is going to die anyway - do not display "0 creatures"
		return false;

	for(auto anim : currentAnimations) //no matter what other conditions below are, hide box when creature is playing hit animation
	{
		auto hitAnimation = dynamic_cast<DefenceAnimation*>(anim);
		if(hitAnimation && (hitAnimation->stack->ID == stack->ID))
			return false;
	}

	if(owner.curInt->curAction)
	{
		if(owner.curInt->curAction->stackNumber == stack->ID) //stack is currently taking action (is not a target of another creature's action etc)
		{
			if(owner.curInt->curAction->actionType == EActionType::WALK || owner.curInt->curAction->actionType == EActionType::SHOOT) //hide when stack walks or shoots
				return false;

			else if(owner.curInt->curAction->actionType == EActionType::WALK_AND_ATTACK && currentActionTarget != stack->getPosition()) //when attacking, hide until walk phase finished
				return false;
		}

		if(owner.curInt->curAction->actionType == EActionType::SHOOT && currentActionTarget == stack->getPosition()) //hide if we are ranged attack target
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

	canvas.drawText(textPos, EFonts::FONT_TINY, Colors::WHITE, ETextAlignment::CENTER, makeNumberShort(stack->getCount()));
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

void BattleStacksController::updateBattleAnimations()
{
	for (auto & elem : currentAnimations)
	{
		if (!elem)
			continue;

		if (elem->isInitialized())
			elem->nextFrame();
		else
			elem->tryInitialize();
	}

	bool hadAnimations = !currentAnimations.empty();
	vstd::erase(currentAnimations, nullptr);

	if (hadAnimations && currentAnimations.empty())
	{
		//anims ended
		owner.setAnimationCondition(EAnimationEvents::ACTION, false);
	}
}

void BattleStacksController::addNewAnim(BattleAnimation *anim)
{
	currentAnimations.push_back(anim);
	owner.setAnimationCondition(EAnimationEvents::ACTION, true);
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
	owner.executeOnAnimationCondition(EAnimationEvents::HIT, true, [=](){
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

		if (needsReverse && !attackedInfo.defender->isFrozen())
		{
			owner.executeOnAnimationCondition(EAnimationEvents::MOVEMENT, true, [=]()
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

		owner.executeOnAnimationCondition(usedEvent, true, [=]()
		{
			if (useDeathAnim)
				addNewAnim(new DeathAnimation(owner, attackedInfo.defender, attackedInfo.indirectAttack));
			else if(useDefenceAnim)
				addNewAnim(new DefenceAnimation(owner, attackedInfo.defender));
			else
				addNewAnim(new HittedAnimation(owner, attackedInfo.defender));

			if (attackedInfo.fireShield)
				owner.effectsController->displayEffect(EBattleEffect::FIRE_SHIELD, soundBase::FIRESHIE, attackedInfo.attacker->getPosition());

			if (attackedInfo.spellEffect != SpellID::NONE)
				owner.displaySpellEffect(attackedInfo.spellEffect, attackedInfo.defender->getPosition());
		});
	}

	for (auto & attackedInfo : attackedInfos)
	{
		if (attackedInfo.rebirth)
		{
			owner.executeOnAnimationCondition(EAnimationEvents::AFTER_HIT, true, [=](){
				owner.effectsController->displayEffect(EBattleEffect::RESURRECT, soundBase::RESURECT, attackedInfo.defender->getPosition());
				addNewAnim(new ResurrectionAnimation(owner, attackedInfo.defender));
			});
		}

		if (attackedInfo.killed && attackedInfo.defender->summoned)
		{
			owner.executeOnAnimationCondition(EAnimationEvents::AFTER_HIT, true, [=](){
				addNewAnim(ColorTransformAnimation::fadeOutAnimation(owner, attackedInfo.defender));
				stackRemoved(attackedInfo.defender->ID);
			});
		}
	}
	executeAttackAnimations();
}

void BattleStacksController::stackTeleported(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	assert(destHex.size() > 0);
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

	owner.executeOnAnimationCondition(EAnimationEvents::HIT, true, [=](){
		addNewAnim( ColorTransformAnimation::teleportOutAnimation(owner, stack) );
	});

	owner.executeOnAnimationCondition(EAnimationEvents::AFTER_HIT, true, [=](){
		stackAnimation[stack->ID]->pos.moveTo(getStackPositionAtHex(destHex.back(), stack));
		addNewAnim( ColorTransformAnimation::teleportInAnimation(owner, stack) );
	});

	// animations will be executed by spell
}

void BattleStacksController::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	assert(destHex.size() > 0);
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

	if(shouldRotate(stack, stack->getPosition(), destHex[0]))
	{
		addNewAnim(new ReverseAnimation(owner, stack, stack->getPosition()));
		owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	}

	addNewAnim(new MovementStartAnimation(owner, stack));
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);

	// if creature can teleport, e.g Devils - skip movement animation
	if (!stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1)) )
	{
		addNewAnim(new MovementAnimation(owner, stack, destHex, distance));
		owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	}

	addNewAnim(new MovementEndAnimation(owner, stack, destHex.back()));
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
}

bool BattleStacksController::shouldAttackFacingRight(const CStack * attacker, const CStack * defender)
{
	bool mustReverse = owner.curInt->cb->isToReverse(
				attacker->getPosition(),
				attacker,
				defender);

	if (attacker->side == BattleSide::ATTACKER)
		return !mustReverse;
	else
		return mustReverse;
}

void BattleStacksController::stackAttacking( const StackAttackInfo & info )
{
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

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
		owner.executeOnAnimationCondition(EAnimationEvents::MOVEMENT, true, [=]()
		{
			addNewAnim(new ReverseAnimation(owner, attacker, attacker->getPosition()));
		});
	}

	if(info.lucky)
	{
		owner.executeOnAnimationCondition(EAnimationEvents::BEFORE_HIT, true, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(-45));
			owner.effectsController->displayEffect(EBattleEffect::GOOD_LUCK, soundBase::GOODLUCK, attacker->getPosition());
		});
	}

	if(info.unlucky)
	{
		owner.executeOnAnimationCondition(EAnimationEvents::BEFORE_HIT, true, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(-44));
			owner.effectsController->displayEffect(EBattleEffect::BAD_LUCK, soundBase::BADLUCK, attacker->getPosition());
		});
	}

	if(info.deathBlow)
	{
		owner.executeOnAnimationCondition(EAnimationEvents::BEFORE_HIT, true, [=]() {
			owner.appendBattleLog(info.attacker->formatGeneralMessage(365));
			owner.effectsController->displayEffect(EBattleEffect::DEATH_BLOW, soundBase::deathBlow, defender->getPosition());
		});

		for(auto elem : info.secondaryDefender)
		{
			owner.executeOnAnimationCondition(EAnimationEvents::BEFORE_HIT, true, [=]() {
				owner.effectsController->displayEffect(EBattleEffect::DEATH_BLOW, elem->getPosition());
			});
		}
	}

	owner.executeOnAnimationCondition(EAnimationEvents::ATTACK, true, [=]()
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
		owner.executeOnAnimationCondition(EAnimationEvents::HIT, true, [=]()
		{
			owner.displaySpellHit(spellEffect, tile);
		});
	}

	if (info.lifeDrain)
	{
		owner.executeOnAnimationCondition(EAnimationEvents::AFTER_HIT, true, [=]()
		{
			owner.effectsController->displayEffect(EBattleEffect::DRAIN_LIFE, soundBase::DRAINLIF, attacker->getPosition());
		});
	}

	//return, animation playback will be handled by stacksAreAttacked
}

void BattleStacksController::executeAttackAnimations()
{
	owner.setAnimationCondition(EAnimationEvents::MOVEMENT, true);
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.setAnimationCondition(EAnimationEvents::MOVEMENT, false);

	owner.setAnimationCondition(EAnimationEvents::BEFORE_HIT, true);
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.setAnimationCondition(EAnimationEvents::BEFORE_HIT, false);

	owner.setAnimationCondition(EAnimationEvents::ATTACK, true);
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.setAnimationCondition(EAnimationEvents::ATTACK, false);

	// Note that HIT event can also be emitted by attack animation
	owner.setAnimationCondition(EAnimationEvents::HIT, true);
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.setAnimationCondition(EAnimationEvents::HIT, false);

	owner.setAnimationCondition(EAnimationEvents::AFTER_HIT, true);
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.setAnimationCondition(EAnimationEvents::AFTER_HIT, false);

	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
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
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);

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
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);

	//Ensure that all animation flags were reset
	assert(owner.getAnimationCondition(EAnimationEvents::OPENING) == false);
	assert(owner.getAnimationCondition(EAnimationEvents::ACTION) == false);
	assert(owner.getAnimationCondition(EAnimationEvents::MOVEMENT) == false);
	assert(owner.getAnimationCondition(EAnimationEvents::ATTACK) == false);
	assert(owner.getAnimationCondition(EAnimationEvents::HIT) == false);
	assert(owner.getAnimationCondition(EAnimationEvents::PROJECTILES) == false);

	owner.windowObject->blockUI(activeStack == nullptr);
	removeExpiredColorFilters();
}

void BattleStacksController::startAction(const BattleAction* action)
{
	removeExpiredColorFilters();
}

void BattleStacksController::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stackToActivate = stack;
	owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);
	owner.activateStack();
}

void BattleStacksController::activateStack() //TODO: check it all before game state is changed due to abilities
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

	//set casting flag to true if creature can use it to not check it every time
	const auto spellcaster = s->getBonusLocalFirst(Selector::type()(Bonus::SPELLCASTER));
	const auto randomSpellcaster = s->getBonusLocalFirst(Selector::type()(Bonus::RANDOM_SPELLCASTER));
	if(s->canCast() && (spellcaster || randomSpellcaster))
	{
		stackCanCastSpell = true;
		if(randomSpellcaster)
			creatureSpellToCast = -1; //spell will be set later on cast
		else
			creatureSpellToCast = owner.curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
		//TODO: what if creature can cast BOTH random genie spell and aimed spell?
		//TODO: faerie dragon type spell should be selected by server
	}
	else
	{
		stackCanCastSpell = false;
		creatureSpellToCast = -1;
	}
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

bool BattleStacksController::activeStackSpellcaster()
{
	return stackCanCastSpell;
}

SpellID BattleStacksController::activeStackSpellToCast()
{
	if (!stackCanCastSpell)
		return SpellID::NONE;
	return SpellID(creatureSpellToCast);
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
	return ret + owner.fieldController->pos.topLeft();
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
		if (filter.persistent)
			return false;
		if (filter.effect == ColorFilter::genEmptyShifter())
			return false;
		if (filter.source && filter.target->hasBonus(Selector::source(Bonus::SPELL_EFFECT, filter.source->id)))
			return false;
		return true;
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
		if (stackAnimation[stack->ID]->framesInGroup(ECreatureAnimType::MOUSEON) > 0)
			stackAnimation[stack->ID]->playOnce(ECreatureAnimType::MOUSEON);

	}

	mouseHoveredStacks = newStacks;
}

std::vector<const CStack *> BattleStacksController::selectHoveredStacks()
{
	// only allow during our turn - do not try to highlight creatures while they are in the middle of actions
	if (!activeStack)
		return {};

	auto hoveredHex = owner.fieldController->getHoveredHex();

	if (!hoveredHex.isValid())
		return {};

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = spells::Mode::HERO;

	if(owner.actionsController->spellcastingModeActive())//hero casts spell
	{
		spell = owner.actionsController->selectedSpell().toSpell();
		caster = owner.getActiveHero();
	}
	else if(owner.stacksController->activeStackSpellToCast() != SpellID::NONE)//stack casts spell
	{
		spell = SpellID(owner.stacksController->activeStackSpellToCast()).toSpell();
		caster = owner.stacksController->getActiveStack();
		mode = spells::Mode::CREATURE_ACTIVE;
	}

	if(caster && spell) //when casting spell
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
