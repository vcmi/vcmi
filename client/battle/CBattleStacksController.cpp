/*
 * CBattleStacksController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleStacksController.h"

#include "CBattleSiegeController.h"
#include "CBattleInterfaceClasses.h"
#include "CBattleInterface.h"
#include "CBattleAnimations.h"
#include "CBattleFieldController.h"
#include "CBattleEffectsController.h"
#include "CBattleProjectileController.h"
#include "CBattleControlPanel.h"

#include "CCreatureAnimation.h"

#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CCanvas.h"

#include "../../CCallback.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/CGameState.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"

static void onAnimationFinished(const CStack *stack, std::weak_ptr<CCreatureAnimation> anim)
{
	std::shared_ptr<CCreatureAnimation> animation = anim.lock();
	if(!animation)
		return;

	if (animation->isIdle())
	{
		const CCreature *creature = stack->getCreature();

		if (animation->framesInGroup(CCreatureAnim::MOUSEON) > 0)
		{
			if (CRandomGenerator::getDefault().nextDouble(99.0) < creature->animation.timeBetweenFidgets *10)
				animation->playOnce(CCreatureAnim::MOUSEON);
			else
				animation->setType(CCreatureAnim::HOLDING);
		}
		else
		{
			animation->setType(CCreatureAnim::HOLDING);
		}
	}
	// always reset callback
	animation->onAnimationReset += std::bind(&onAnimationFinished, stack, anim);
}

CBattleStacksController::CBattleStacksController(CBattleInterface * owner):
	owner(owner),
	activeStack(nullptr),
	mouseHoveredStack(nullptr),
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

	ColorShifterAddMulExcept shifterNormal  ({0,0,0,0}, {150,  50, 255, 255}, {255, 231, 132, 255});
	ColorShifterAddMulExcept shifterPositive({0,0,0,0}, { 50, 255,  50, 255}, {255, 231, 132, 255});
	ColorShifterAddMulExcept shifterNegative({0,0,0,0}, {255,  50,  50, 255}, {255, 231, 132, 255});
	ColorShifterAddMulExcept shifterNeutral ({0,0,0,0}, {255, 255,  50, 255}, {255, 231, 132, 255});

	amountNormal->adjustPalette(&shifterNormal);
	amountPositive->adjustPalette(&shifterPositive);
	amountNegative->adjustPalette(&shifterNegative);
	amountEffNeutral->adjustPalette(&shifterNeutral);

	std::vector<const CStack*> stacks = owner->curInt->cb->battleGetAllStacks(true);
	for(const CStack * s : stacks)
	{
		stackAdded(s);
	}
}

void CBattleStacksController::showBattlefieldObjects(std::shared_ptr<CCanvas> canvas, const BattleHex & location )
{
	auto getCurrentPosition = [&](const CStack *stack) -> BattleHex
	{
		for (auto & anim : currentAnimations)
		{
			// certainly ugly workaround but fixes quite annoying bug
			// stack position will be updated only *after* movement is finished
			// before this - stack is always at its initial position. Thus we need to find
			// its current position. Which can be found only in this class
			if (CStackMoveAnimation *move = dynamic_cast<CStackMoveAnimation*>(anim))
			{
				if (move->stack == stack)
					return move->currentHex;
			}

		}
		return stack->getPosition();
	};

	auto stacks = owner->curInt->cb->battleGetAllStacks(true);

	for (auto & stack : stacks)
	{
		if (stackAnimation.find(stack->ID) == stackAnimation.end()) //e.g. for summoned but not yet handled stacks
			continue;

		//if (stack->initialPosition < 0) // turret shooters are handled separately
		//	continue;

		//FIXME: hack to ignore ghost stacks
		if ((stackAnimation[stack->ID]->getType() == CCreatureAnim::DEAD || stackAnimation[stack->ID]->getType() == CCreatureAnim::HOLDING) && stack->isGhost())
			continue;//ignore

		if (stackAnimation[stack->ID]->isDead())
		{
			//if ( location == stack->getPosition() )
			if ( location == BattleHex::HEX_BEFORE_ALL ) //FIXME: any cases when using BEFORE_ALL won't work?
				showStack(canvas, stack);
			continue;
		}

		// standing - blit at current position
		if (!stackAnimation[stack->ID]->isMoving())
		{
			if ( location == stack->getPosition() )
				showStack(canvas, stack);
			continue;
		}

		// flying creature - just blit them over everyone else
		if (stack->hasBonusOfType(Bonus::FLYING))
		{
			if ( location == BattleHex::HEX_AFTER_ALL)
				showStack(canvas, stack);
			continue;
		}

		// else - unit moving on ground
		{
			if ( location == getCurrentPosition(stack) )
				showStack(canvas, stack);
			continue;
		}
	}
}

void CBattleStacksController::stackReset(const CStack * stack)
{
	auto iter = stackAnimation.find(stack->ID);

	if(iter == stackAnimation.end())
	{
		logGlobal->error("Unit %d have no animation", stack->ID);
		return;
	}

	auto animation = iter->second;

	if(stack->alive() && animation->isDeadOrDying())
		animation->setType(CCreatureAnim::HOLDING);

	if (stack->isClone())
	{
		auto shifter = ColorShifterAddMul::deepBlue();
		animation->shiftColor(&shifter);
	}

	//TODO: handle more cases
}

void CBattleStacksController::stackAdded(const CStack * stack)
{
	stackFacingRight[stack->ID] = stack->side == BattleSide::ATTACKER; // must be set before getting stack position

	Point coords = getStackPositionAtHex(stack->getPosition(), stack);

	if(stack->initialPosition < 0) //turret
	{
		assert(owner->siegeController);

		const CCreature *turretCreature = owner->siegeController->getTurretCreature();

		stackAnimation[stack->ID] = AnimationControls::getAnimation(turretCreature);
		stackAnimation[stack->ID]->pos.h = 225;

		coords = owner->siegeController->getTurretCreaturePosition(stack->initialPosition);
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
	stackAnimation[stack->ID]->setType(CCreatureAnim::HOLDING);
}

void CBattleStacksController::setActiveStack(const CStack *stack)
{
	if (activeStack) // update UI
		stackAnimation[activeStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		stackAnimation[activeStack->ID]->setBorderColor(AnimationControls::getGoldBorder());

	owner->controlPanel->blockUI(activeStack == nullptr);
}

void CBattleStacksController::setHoveredStack(const CStack *stack)
{
	if ( stack == mouseHoveredStack )
		 return;

	if (mouseHoveredStack)
		stackAnimation[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	// stack must be alive and not active (which uses gold border instead)
	if (stack && stack->alive() && stack != activeStack)
	{
		mouseHoveredStack = stack;

		if (mouseHoveredStack)
		{
			stackAnimation[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getBlueBorder());
			if (stackAnimation[mouseHoveredStack->ID]->framesInGroup(CCreatureAnim::MOUSEON) > 0)
				stackAnimation[mouseHoveredStack->ID]->playOnce(CCreatureAnim::MOUSEON);
		}
	}
	else
		mouseHoveredStack = nullptr;
}

bool CBattleStacksController::stackNeedsAmountBox(const CStack * stack)
{
	BattleHex currentActionTarget;
	if(owner->curInt->curAction)
	{
		auto target = owner->curInt->curAction->getTarget(owner->curInt->cb.get());
		if(!target.empty())
			currentActionTarget = target.at(0).hexValue;
	}

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->getCount() == 1) //do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
		return false;

	if (!owner->battleActionsStarted) // do not perform any further checks since they are related to actions that will only occur after intro music
		return true;

	if(!stack->alive())
		return false;

	if(stack->getCount() == 0) //hide box when target is going to die anyway - do not display "0 creatures"
		return false;

	for(auto anim : currentAnimations) //no matter what other conditions below are, hide box when creature is playing hit animation
	{
		auto hitAnimation = dynamic_cast<CDefenceAnimation*>(anim);
		if(hitAnimation && (hitAnimation->stack->ID == stack->ID))
			return false;
	}

	if(owner->curInt->curAction)
	{
		if(owner->curInt->curAction->stackNumber == stack->ID) //stack is currently taking action (is not a target of another creature's action etc)
		{
			if(owner->curInt->curAction->actionType == EActionType::WALK || owner->curInt->curAction->actionType == EActionType::SHOOT) //hide when stack walks or shoots
				return false;

			else if(owner->curInt->curAction->actionType == EActionType::WALK_AND_ATTACK && currentActionTarget != stack->getPosition()) //when attacking, hide until walk phase finished
				return false;
		}

		if(owner->curInt->curAction->actionType == EActionType::SHOOT && currentActionTarget == stack->getPosition()) //hide if we are ranged attack target
			return false;
	}
	return true;
}

std::shared_ptr<IImage> CBattleStacksController::getStackAmountBox(const CStack * stack)
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

void CBattleStacksController::showStackAmountBox(std::shared_ptr<CCanvas> canvas, const CStack * stack)
{
	//blitting amount background box
	auto amountBG = getStackAmountBox(stack);

	const int sideShift = stack->side == BattleSide::ATTACKER ? 1 : -1;
	const int reverseSideShift = stack->side == BattleSide::ATTACKER ? -1 : 1;
	const BattleHex nextPos = stack->getPosition() + sideShift;
	const bool edge = stack->getPosition() % GameConstants::BFIELD_WIDTH == (stack->side == BattleSide::ATTACKER ? GameConstants::BFIELD_WIDTH - 2 : 1);
	const bool moveInside = !edge && !owner->fieldController->stackCountOutsideHex(nextPos);

	int xAdd = (stack->side == BattleSide::ATTACKER ? 220 : 202) +
			(stack->doubleWide() ? 44 : 0) * sideShift +
			(moveInside ? amountBG->width() + 10 : 0) * reverseSideShift;
	int yAdd = 260 + ((stack->side == BattleSide::ATTACKER || moveInside) ? 0 : -15);

	canvas->draw(amountBG, stackAnimation[stack->ID]->pos.topLeft() + Point(xAdd, yAdd));

	//blitting amount
	Point textPos = stackAnimation[stack->ID]->pos.topLeft() + amountBG->dimensions()/2 + Point(xAdd, yAdd);

	canvas->drawText(textPos, EFonts::FONT_TINY, Colors::WHITE, ETextAlignment::CENTER, makeNumberShort(stack->getCount()));
}

void CBattleStacksController::showStack(std::shared_ptr<CCanvas> canvas, const CStack * stack)
{
	stackAnimation[stack->ID]->nextFrame(canvas, facingRight(stack)); // do actual blit
	stackAnimation[stack->ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);

	if (stackNeedsAmountBox(stack))
		showStackAmountBox(canvas, stack);
}

void CBattleStacksController::updateBattleAnimations()
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
	for (auto it = currentAnimations.begin(); it != currentAnimations.end();)
	{
		if (*it == nullptr)
			it = currentAnimations.erase(it);
		else
			++it;
	}

	if (hadAnimations && currentAnimations.empty())
	{
		//anims ended
		owner->controlPanel->blockUI(activeStack == nullptr);
		owner->animsAreDisplayed.setn(false);
	}
}

void CBattleStacksController::addNewAnim(CBattleAnimation *anim)
{
	currentAnimations.push_back(anim);
	owner->animsAreDisplayed.setn(true);
}

void CBattleStacksController::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stackToActivate = stack;
	owner->waitForAnims();
	if (stackToActivate) //during waiting stack may have gotten activated through show
		owner->activateStack();
}

void CBattleStacksController::stackRemoved(uint32_t stackID)
{
	if (getActiveStack() != nullptr)
	{
		if (getActiveStack()->ID == stackID)
		{
			BattleAction *action = new BattleAction();
			action->side = owner->defendingHeroInstance ? (owner->curInt->playerID == owner->defendingHeroInstance->tempOwner) : false;
			action->actionType = EActionType::CANCEL;
			action->stackNumber = getActiveStack()->ID;
			owner->givenCommand.setn(action);
			setActiveStack(nullptr);
		}
	}
	//todo: ensure that ghost stack animation has fadeout effect
}

void CBattleStacksController::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	for(auto & attackedInfo : attackedInfos)
	{
		//if (!attackedInfo.cloneKilled) //FIXME: play dead animation for cloned creature before it vanishes
			addNewAnim(new CDefenceAnimation(attackedInfo, owner));

		if(attackedInfo.rebirth)
		{
			owner->effectsController->displayEffect(EBattleEffect::RESURRECT, soundBase::RESURECT, attackedInfo.defender->getPosition()); //TODO: play reverse death animation
		}
	}
	owner->waitForAnims();

	for (auto & attackedInfo : attackedInfos)
	{
		if (attackedInfo.rebirth)
			stackAnimation[attackedInfo.defender->ID]->setType(CCreatureAnim::HOLDING);
		if (attackedInfo.cloneKilled)
			stackRemoved(attackedInfo.defender->ID);
	}
}

void CBattleStacksController::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	addNewAnim(new CMovementAnimation(owner, stack, destHex, distance));
	owner->waitForAnims();
}

void CBattleStacksController::stackAttacking( const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting )
{
	if (shooting)
	{
		addNewAnim(new CShootingAnimation(owner, attacker, dest, attacked));
	}
	else
	{
		addNewAnim(new CMeleeAttackAnimation(owner, attacker, dest, attacked));
	}
	//waitForAnims();
}

bool CBattleStacksController::shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex)
{
	Point begPosition = getStackPositionAtHex(oldPos,stack);
	Point endPosition = getStackPositionAtHex(nextHex, stack);

	if((begPosition.x > endPosition.x) && facingRight(stack))
		return true;
	else if((begPosition.x < endPosition.x) && !facingRight(stack))
		return true;

	return false;
}


void CBattleStacksController::endAction(const BattleAction* action)
{
	//check if we should reverse stacks
	//for some strange reason, it's not enough
	TStacks stacks = owner->curInt->cb->battleGetStacks(CBattleCallback::MINE_AND_ENEMY);

	for (const CStack *s : stacks)
	{
		bool shouldFaceRight  = s && s->side == BattleSide::ATTACKER;

		if (s && facingRight(s) != shouldFaceRight && s->alive() && stackAnimation[s->ID]->isIdle())
		{
			addNewAnim(new CReverseAnimation(owner, s, s->getPosition(), false));
		}
	}
}

void CBattleStacksController::startAction(const BattleAction* action)
{
	const CStack *stack = owner->curInt->cb->battleGetStackByID(action->stackNumber);
	setHoveredStack(nullptr);

	auto actionTarget = action->getTarget(owner->curInt->cb.get());

	if(action->actionType == EActionType::WALK
		|| (action->actionType == EActionType::WALK_AND_ATTACK && actionTarget.at(0).hexValue != stack->getPosition()))
	{
		assert(stack);
		owner->moveStarted = true;
		if (stackAnimation[action->stackNumber]->framesInGroup(CCreatureAnim::MOVE_START))
			addNewAnim(new CMovementStartAnimation(owner, stack));

		//if(shouldRotate(stack, stack->getPosition(), actionTarget.at(0).hexValue))
		//	addNewAnim(new CReverseAnimation(owner, stack, stack->getPosition(), true));
	}
}

void CBattleStacksController::activateStack()
{
	if ( !currentAnimations.empty())
		return;

	if ( !stackToActivate)
		return;

	owner->trySetActivePlayer(stackToActivate->owner);

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
			creatureSpellToCast = owner->curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
		//TODO: what if creature can cast BOTH random genie spell and aimed spell?
		//TODO: faerie dragon type spell should be selected by server
	}
	else
	{
		stackCanCastSpell = false;
		creatureSpellToCast = -1;
	}
}

void CBattleStacksController::setSelectedStack(const CStack *stack)
{
	selectedStack = stack;
}

const CStack* CBattleStacksController::getSelectedStack()
{
	return selectedStack;
}

const CStack* CBattleStacksController::getActiveStack()
{
	return activeStack;
}

bool CBattleStacksController::facingRight(const CStack * stack)
{
	return stackFacingRight[stack->ID];
}

bool CBattleStacksController::activeStackSpellcaster()
{
	return stackCanCastSpell;
}

SpellID CBattleStacksController::activeStackSpellToCast()
{
	if (!stackCanCastSpell)
		return SpellID::NONE;
	return SpellID(creatureSpellToCast);
}

Point CBattleStacksController::getStackPositionAtHex(BattleHex hexNum, const CStack * stack)
{
	Point ret(-500, -500); //returned value
	if(stack && stack->initialPosition < 0) //creatures in turrets
		return owner->siegeController->getTurretCreaturePosition(stack->initialPosition);

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
	return ret + owner->pos.topLeft();

}
