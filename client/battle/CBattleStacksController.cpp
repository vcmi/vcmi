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
#include "../gui/CAnimation.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../../lib/battle/BattleHex.h"
#include "../CPlayerInterface.h"
#include "CCreatureAnimation.h"
#include "../../lib/CGameState.h"
#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"
#include "../CMusicHandler.h"
#include "../CGameInfo.h"

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
	owner(owner)
{
	//preparing graphics for displaying amounts of creatures
	amountNormal     = IImage::createFromFile("CMNUMWIN.BMP");
	amountPositive   = IImage::createFromFile("CMNUMWIN.BMP");
	amountNegative   = IImage::createFromFile("CMNUMWIN.BMP");
	amountEffNeutral = IImage::createFromFile("CMNUMWIN.BMP");

	ColorShifterAddMulExcept shifterNormal  ({0,0,0,0}, {150,  48, 237, 255}, {132, 231, 255, 255});
	ColorShifterAddMulExcept shifterPositive({0,0,0,0}, { 45, 255,  45, 255}, {132, 231, 255, 255});
	ColorShifterAddMulExcept shifterNegative({0,0,0,0}, {255,  45,  45, 255}, {132, 231, 255, 255});
	ColorShifterAddMulExcept shifterNeutral ({0,0,0,0}, {255, 255,  45, 255}, {132, 231, 255, 255});

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

void CBattleStacksController::showBattlefieldObjects(SDL_Surface *to, const BattleHex & location )
{
	auto getCurrentPosition = [&](const CStack *stack) -> BattleHex
	{
		for (auto & anim : pendingAnims)
		{
			// certainly ugly workaround but fixes quite annoying bug
			// stack position will be updated only *after* movement is finished
			// before this - stack is always at its initial position. Thus we need to find
			// its current position. Which can be found only in this class
			if (CMovementAnimation *move = dynamic_cast<CMovementAnimation*>(anim.first))
			{
				if (move->stack == stack)
					return move->nextHex;
			}
		}
		return stack->getPosition();
	};

	auto stacks = owner->curInt->cb->battleGetAllStacks(true);

	for (auto & stack : stacks)
	{
		if (creAnims.find(stack->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;

		//if (stack->initialPosition < 0) // turret shooters are handled separately
		//	continue;

		//FIXME: hack to ignore ghost stacks
		if ((creAnims[stack->ID]->getType() == CCreatureAnim::DEAD || creAnims[stack->ID]->getType() == CCreatureAnim::HOLDING) && stack->isGhost())
			continue;//ignore

		if (creAnims[stack->ID]->isDead())
		{
			//if ( location == stack->getPosition() )
			if ( location == BattleHex::HEX_BEFORE_ALL ) //FIXME: any cases when this won't work?
			{};
			continue;
		}

		// standing - blit at current position
		if (!creAnims[stack->ID]->isMoving())
		{
			if ( location == stack->getPosition() )
			{};
			continue;
		}

		// flying creature - just blit them over everyone else
		if (stack->hasBonusOfType(Bonus::FLYING))
		{
			if ( location == BattleHex::HEX_AFTER_ALL)
			{};
			continue;
		}

		// else - unit moving on ground
		{
			if ( location == getCurrentPosition(stack) )
			{};
			continue;
		}
	}
}

void CBattleStacksController::stackReset(const CStack * stack)
{
	auto iter = creAnims.find(stack->ID);

	if(iter == creAnims.end())
	{
		logGlobal->error("Unit %d have no animation", stack->ID);
		return;
	}

	auto animation = iter->second;

	if(stack->alive() && animation->isDead())
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
	creDir[stack->ID] = stack->side == BattleSide::ATTACKER; // must be set before getting stack position

	Point coords = CClickableHex::getXYUnitAnim(stack->getPosition(), stack, owner);

	if(stack->initialPosition < 0) //turret
	{
		assert(owner->siegeController);

		const CCreature *turretCreature = owner->siegeController->getTurretCreature();

		creAnims[stack->ID] = AnimationControls::getAnimation(turretCreature);
		creAnims[stack->ID]->pos.h = 225;

		coords = owner->siegeController->getTurretCreaturePosition(stack->initialPosition);
	}
	else
	{
		creAnims[stack->ID] = AnimationControls::getAnimation(stack->getCreature());
		creAnims[stack->ID]->onAnimationReset += std::bind(&onAnimationFinished, stack, creAnims[stack->ID]);
		creAnims[stack->ID]->pos.h = creAnims[stack->ID]->getHeight();
	}
	creAnims[stack->ID]->pos.x = coords.x;
	creAnims[stack->ID]->pos.y = coords.y;
	creAnims[stack->ID]->pos.w = creAnims[stack->ID]->getWidth();
	creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);

	//loading projectiles for units
	if(stack->isShooter())
	{
		owner->projectilesController->initStackProjectile(stack);
	}
}

void CBattleStacksController::setActiveStack(const CStack *stack)
{
	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getGoldBorder());

	owner->controlPanel->blockUI(activeStack == nullptr);
}

void CBattleStacksController::setHoveredStack(const CStack *stack)
{
	if ( stack == mouseHoveredStack )
		 return;

	if (mouseHoveredStack)
		creAnims[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	// stack must be alive and not active (which uses gold border instead)
	if (stack && stack->alive() && stack != activeStack)
	{
		mouseHoveredStack = stack;

		if (mouseHoveredStack)
		{
			creAnims[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getBlueBorder());
			if (creAnims[mouseHoveredStack->ID]->framesInGroup(CCreatureAnim::MOUSEON) > 0)
				creAnims[mouseHoveredStack->ID]->playOnce(CCreatureAnim::MOUSEON);
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

	if(!stack->alive())
		return false;

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->getCount() == 1) //do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
		return false;

	if(stack->getCount() == 0) //hide box when target is going to die anyway - do not display "0 creatures"
		return false;

	for(auto anim : pendingAnims) //no matter what other conditions below are, hide box when creature is playing hit animation
	{
		auto hitAnimation = dynamic_cast<CDefenceAnimation*>(anim.first);
		if(hitAnimation && (hitAnimation->stack->ID == stack->ID)) //we process only "current creature" as other creatures will be processed reliably on their own iteration
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

void CBattleStacksController::showStackAmountBox(SDL_Surface *to, const CStack * stack)
{
	auto getEffectsPositivness = [&](const std::vector<si32> & activeSpells) -> int
	{
		int pos = 0;
		for (const auto & spellId : activeSpells)
		{
			pos += CGI->spellh->objects.at(spellId)->positiveness;
		}
		return pos;
	};

	auto getAmountBoxBackground = [&](int positivness) -> auto
	{
		if (positivness > 0)
			return amountPositive;
		if (positivness < 0)
			return amountNegative;
		return amountEffNeutral;
	};

	const int sideShift = stack->side == BattleSide::ATTACKER ? 1 : -1;
	const int reverseSideShift = stack->side == BattleSide::ATTACKER ? -1 : 1;
	const BattleHex nextPos = stack->getPosition() + sideShift;
	const bool edge = stack->getPosition() % GameConstants::BFIELD_WIDTH == (stack->side == BattleSide::ATTACKER ? GameConstants::BFIELD_WIDTH - 2 : 1);
	const bool moveInside = !edge && !owner->fieldController->stackCountOutsideHex(nextPos);
	int xAdd = (stack->side == BattleSide::ATTACKER ? 220 : 202) +
			(stack->doubleWide() ? 44 : 0) * sideShift +
			(moveInside ? amountNormal->width() + 10 : 0) * reverseSideShift;
	int yAdd = 260 + ((stack->side == BattleSide::ATTACKER || moveInside) ? 0 : -15);

	//blitting amount background box
	std::vector<si32> activeSpells = stack->activeSpells();

	auto amountBG = activeSpells.empty() ? amountNormal : getAmountBoxBackground(getEffectsPositivness(activeSpells));
	amountBG->draw(to, creAnims[stack->ID]->pos.x + xAdd, creAnims[stack->ID]->pos.y + yAdd);

	//blitting amount
	Point textPos(creAnims[stack->ID]->pos.x + xAdd + amountNormal->width()/2,
			creAnims[stack->ID]->pos.y + yAdd + amountNormal->height()/2);
	graphics->fonts[FONT_TINY]->renderTextCenter(to, makeNumberShort(stack->getCount()), Colors::WHITE, textPos);

}

void CBattleStacksController::showStack(SDL_Surface *to, const CStack * stack)
{
	creAnims[stack->ID]->nextFrame(to, facingRight(stack)); // do actual blit
	creAnims[stack->ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);

	if (stackNeedsAmountBox(stack))
		showStackAmountBox(to, stack);
}

void CBattleStacksController::updateBattleAnimations()
{
	//handle animations
	for (auto & elem : pendingAnims)
	{
		if (!elem.first) //this animation should be deleted
			continue;

		if (!elem.second)
		{
			elem.second = elem.first->init();
		}
		if (elem.second && elem.first)
			elem.first->nextFrame();
	}

	//delete anims
	int preSize = static_cast<int>(pendingAnims.size());
	for (auto it = pendingAnims.begin(); it != pendingAnims.end(); ++it)
	{
		if (it->first == nullptr)
		{
			pendingAnims.erase(it);
			it = pendingAnims.begin();
			break;
		}
	}

	if (preSize > 0 && pendingAnims.empty())
	{
		//anims ended
		owner->controlPanel->blockUI(activeStack == nullptr);

		owner->animsAreDisplayed.setn(false);
	}
}

void CBattleStacksController::addNewAnim(CBattleAnimation *anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	owner->animsAreDisplayed.setn(true);
}

void CBattleStacksController::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stackToActivate = stack;
	owner->waitForAnims();
	if (stackToActivate) //during waiting stack may have gotten activated through show
		activateStack();
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
			action->stackNumber = owner->stacksController->getActiveStack()->ID;
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
			creAnims[attackedInfo.defender->ID]->setType(CCreatureAnim::HOLDING);
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
	Point begPosition = CClickableHex::getXYUnitAnim(oldPos,stack, owner);
	Point endPosition = CClickableHex::getXYUnitAnim(nextHex, stack, owner);

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

		if (s && facingRight(s) != shouldFaceRight && s->alive() && creAnims[s->ID]->isIdle())
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
		if (creAnims[action->stackNumber]->framesInGroup(CCreatureAnim::MOVE_START))
		{
			pendingAnims.push_back(std::make_pair(new CMovementStartAnimation(owner, stack), false));
		}

		if(shouldRotate(stack, stack->getPosition(), actionTarget.at(0).hexValue))
			pendingAnims.push_back(std::make_pair(new CReverseAnimation(owner, stack, stack->getPosition(), true), false));
	}
}

void CBattleStacksController::activateStack()
{
	if ( !pendingAnims.empty())
		return;

	if ( !stackToActivate)
		return;

	owner->trySetActivePlayer(stackToActivate->owner);

	setActiveStack(stackToActivate);
	stackToActivate = nullptr;

	const CStack * s = owner->stacksController->getActiveStack();
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
	return creDir[stack->ID];
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
