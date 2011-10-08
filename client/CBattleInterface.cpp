#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "../lib/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "CAnimation.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CHeroHandler.h"
#include "CDefHandler.h"
#include "../lib/CSpellHandler.h"
#include "CMusicHandler.h"
#include "CMessage.h"
#include "CCursorHandler.h"
#include "../CCallback.h"
#include "../lib/BattleState.h"
#include "../lib/CGeneralTextHandler.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "CSpellWindow.h"
#include "CConfigHandler.h"
#include <queue>
#include <sstream>
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "CPlayerInterface.h"
#include "CCreatureWindow.h"
#include "CVideoHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/map.h"
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#ifndef __GNUC__
const double M_PI = 3.14159265358979323846;
#else
#define _USE_MATH_DEFINES
#include <cmath>
#endif
#include <boost/format.hpp>

const time_t CBattleInterface::HOVER_ANIM_DELTA = 1;

/*
 * CBattleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface * screen;
extern SDL_Color zwykly;

CondSh<bool> CBattleInterface::animsAreDisplayed;

struct CMP_stack2
{
	inline bool operator ()(const CStack& a, const CStack& b)
	{
		return (a.Speed())>(b.Speed());
	}
} cmpst2 ;

static void transformPalette(SDL_Surface * surf, float rCor, float gCor, float bCor)
{
	SDL_Color * colorsToChange = surf->format->palette->colors;
	for(int g=0; g<surf->format->palette->ncolors; ++g)
	{
		if((colorsToChange+g)->b != 132 &&
			(colorsToChange+g)->g != 231 &&
			(colorsToChange+g)->r != 255) //it's not yellow border
		{
			(colorsToChange+g)->r = (float)((colorsToChange+g)->r) * rCor;
			(colorsToChange+g)->g = (float)((colorsToChange+g)->g) * gCor;
			(colorsToChange+g)->b = (float)((colorsToChange+g)->b) * bCor;
		}
	}
}
////////////////////////Battle helpers

//general anim

void CBattleAnimation::endAnim()
{
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		if(it->first == this)
		{
			it->first = NULL;
		}
	}

}

bool CBattleAnimation::isEarliest(bool perStackConcurrency)
{
	int lowestMoveID = owner->animIDhelper + 5;
	CBattleStackAnimation * thAnim = dynamic_cast<CBattleStackAnimation *>(this);
	CSpellEffectAnim * thSen = dynamic_cast<CSpellEffectAnim *>(this);

	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CBattleStackAnimation * stAnim = dynamic_cast<CBattleStackAnimation *>(it->first);
		CSpellEffectAnim * sen = dynamic_cast<CSpellEffectAnim *>(it->first);
		if(perStackConcurrency && stAnim && thAnim && stAnim->stack->ID != thAnim->stack->ID)
			continue;

		if(sen && thSen && sen != thSen && perStackConcurrency)
			continue;

		CReverseAnim * revAnim = dynamic_cast<CReverseAnim *>(stAnim);

		if(revAnim && thAnim && stAnim && stAnim->stack->ID == thAnim->stack->ID && revAnim->priority)
			return false;

		if(it->first)
			amin(lowestMoveID, it->first->ID);
	}
	return ID == lowestMoveID || lowestMoveID == (owner->animIDhelper + 5);
}

CBattleAnimation::CBattleAnimation(CBattleInterface * _owner)
: owner(_owner), ID(_owner->animIDhelper++)
{}

//Dummy animation


bool CDummyAnim::init()
{
	return true;
}

void CDummyAnim::nextFrame()
{
	counter++;
	if(counter > howMany)
		endAnim();
}

void CDummyAnim::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CDummyAnim::CDummyAnim(CBattleInterface * _owner, int howManyFrames) : CBattleAnimation(_owner), counter(0), howMany(howManyFrames)
{
}


//effect animation
bool CSpellEffectAnim::init()
{
	if(!isEarliest(true))
		return false;

	if(effect == 12) //armageddon
	{
		if(effect == -1 || graphics->battleACToDef[effect].size() != 0)
		{
			CDefHandler * anim;
			if(customAnim.size())
				anim = CDefHandler::giveDef(customAnim);
			else
				anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);

			if (Vflip)
			{
				for (int v=0; v<anim->ourImages.size(); ++v)
				{
					CSDL_Ext::VflipSurf(anim->ourImages[v].bitmap);
				}
			}

			for(int i=0; i * anim->width < owner->pos.w ; ++i)
			{
				for(int j=0; j * anim->height < owner->pos.h ; ++j)
				{
					SBattleEffect be;
					be.effectID = ID;
					be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);
					if (Vflip)
					{
						for (int v=0; v<be.anim->ourImages.size(); ++v)
						{
							CSDL_Ext::VflipSurf(be.anim->ourImages[v].bitmap);
						}
					}
					be.frame = 0;
					be.maxFrame = be.anim->ourImages.size();
					be.x = i * anim->width + owner->pos.x;
					be.y = j * anim->height + owner->pos.y;

					owner->battleEffects.push_back(be);
				}
			}
		}
		else //there is nothing to play
		{
			endAnim();
			return false;
		}
	}
	else // Effects targeted at a specific creature/hex.
	{
		if(effect == -1 || graphics->battleACToDef[effect].size() != 0)
		{
			const CStack* destStack = owner->curInt->cb->battleGetStackByPos(destTile, false);
			Rect &tilePos = owner->bfield[destTile].pos;
			SBattleEffect be;
			be.effectID = ID;
			if(customAnim.size())
				be.anim = CDefHandler::giveDef(customAnim);
			else
				be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);

			if (Vflip)
			{
				for (int v=0; v<be.anim->ourImages.size(); ++v)
				{
					CSDL_Ext::VflipSurf(be.anim->ourImages[v].bitmap);
				}
			}

			be.frame = 0;
			be.maxFrame = be.anim->ourImages.size();
			if(effect == 1)
				be.maxFrame = 3;

			switch (effect)
			{
			case -1:
				be.x = x;
				be.y = y;
				break;
			case 0: // Prayer and Lightning Bolt.
			case 1:
				// Position effect with it's bottom center touching the bottom center of affected tile(s).
				be.x = tilePos.x + tilePos.w/2 - be.anim->width/2;
				be.y = tilePos.y + tilePos.h - be.anim->height;
				break;

			default:
				// Position effect with it's center touching the top center of affected tile(s).
				be.x = tilePos.x + tilePos.w/2 - be.anim->width/2;
				be.y = tilePos.y - be.anim->height/2;
				break;
			}

			// Correction for 2-hex creatures.
			if (destStack != NULL && destStack->doubleWide())
				be.x += (destStack->attackerOwned ? -1 : 1)*tilePos.w/2;

			owner->battleEffects.push_back(be);
		}
		else //there is nothing to play
		{
			endAnim();
			return false;
		}
	}
	//battleEffects 
	return true;
}

void CSpellEffectAnim::nextFrame()
{
	//notice: there may be more than one effect in owner->battleEffects correcponding to this animation (ie. armageddon)
	for(std::list<SBattleEffect>::iterator it = owner->battleEffects.begin(); it != owner->battleEffects.end(); ++it)
	{
		if(it->effectID == ID)
		{
			++(it->frame);

			if(it->frame == it->maxFrame)
			{
				endAnim();
				break;
			}
			else
			{
				it->x += dx;
				it->y += dy;
			}
		}
	}
}

void CSpellEffectAnim::endAnim()
{
	CBattleAnimation::endAnim();

	std::vector<std::list<SBattleEffect>::iterator> toDel;

	for(std::list<SBattleEffect>::iterator it = owner->battleEffects.begin(); it != owner->battleEffects.end(); ++it)
	{
		if(it->effectID == ID)
		{
			toDel.push_back(it);
		}
	}

	for(int b=0; b<toDel.size(); ++b)
	{
		delete toDel[b]->anim;
		owner->battleEffects.erase(toDel[b]);
	}

	delete this;
}

CSpellEffectAnim::CSpellEffectAnim(CBattleInterface * _owner, ui32 _effect, THex _destTile, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(_effect), destTile(_destTile), customAnim(""), dx(_dx), dy(_dy), Vflip(_Vflip)
{
}

CSpellEffectAnim::CSpellEffectAnim(CBattleInterface * _owner, std::string _customAnim, int _x, int _y, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(-1), destTile(0), customAnim(_customAnim), x(_x), y(_y), dx(_dx), dy(_dy), Vflip(_Vflip)
{
}

//stack's aniamtion

CBattleStackAnimation::CBattleStackAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleAnimation(_owner), stack(_stack)
{
}

bool CBattleStackAnimation::isToReverseHlp(THex hexFrom, THex hexTo, bool curDir)
{
	int fromMod = hexFrom % BFIELD_WIDTH;
	int fromDiv = hexFrom / BFIELD_WIDTH;
	int toMod = hexTo % BFIELD_WIDTH;

	if(curDir && fromMod < toMod)
		return false;
	else if(curDir && fromMod > toMod)
		return true;
	else if(curDir && fromMod == toMod)
	{
		return fromDiv % 2 == 0;
	}
	else if(!curDir && fromMod < toMod)
		return true;
	else if(!curDir && fromMod > toMod)
		return false;
	else if(!curDir && fromMod == toMod)
	{
		return fromDiv % 2 == 1;
	}
	tlog1 << "Catastrope in CBattleStackAnimation::isToReverse!" << std::endl;
	return false; //should never happen
}

bool CBattleStackAnimation::isToReverse(THex hexFrom, THex hexTo, bool curDir, bool toDoubleWide, bool toDir)
{
	if(hexTo < 0) //turret
		return false;

	if(toDoubleWide)
	{
		return isToReverseHlp(hexFrom, hexTo, curDir) &&
			(toDir ? isToReverseHlp(hexFrom, hexTo-1, curDir) : isToReverseHlp(hexFrom, hexTo+1, curDir) );
	}
	else
	{
		return isToReverseHlp(hexFrom, hexTo, curDir);
	}
}

CCreatureAnimation * CBattleStackAnimation::myAnim()
{
	return owner->creAnims[stack->ID];
}

//revering animation

bool CReverseAnim::init()
{
	if(myAnim() == NULL || myAnim()->getType() == 5)
	{
		endAnim();

		return false; //there is no such creature
	}

	if(!priority && !isEarliest(false))
		return false;
	
	if(myAnim()->framesInGroup(CCreatureAnim::TURN_R))
		myAnim()->setType(CCreatureAnim::TURN_R);
	else
		setupSecondPart();

		
	return true;
}

void CReverseAnim::nextFrame()
{
	if(partOfAnim == 1) //first part of animation
	{
		if(myAnim()->onLastFrameInGroup())
		{
			partOfAnim = 2;
		}
	}
	else if(partOfAnim == 2)
	{
		if(!secondPartSetup)
		{
			setupSecondPart();
		}
		if(myAnim()->onLastFrameInGroup())
		{
			endAnim();
		}
	}
}

void CReverseAnim::endAnim()
{
	CBattleAnimation::endAnim();
	if( stack->alive() )//don't do that if stack is dead
		myAnim()->setType(CCreatureAnim::HOLDING);

	delete this;
}

CReverseAnim::CReverseAnim(CBattleInterface * _owner, const CStack * stack, THex dest, bool _priority)
: CBattleStackAnimation(_owner, stack), partOfAnim(1), secondPartSetup(false), hex(dest), priority(_priority)
{
}

void CReverseAnim::setupSecondPart()
{
	owner->creDir[stack->ID] = !owner->creDir[stack->ID];

	if(!stack)
	{
		endAnim();
		return;
	}

	Point coords = CBattleHex::getXYUnitAnim(hex, owner->creDir[stack->ID], stack, owner);
	myAnim()->pos.x = coords.x;
	//creAnims[stackID]->pos.y = coords.second;

	if(stack->doubleWide())
	{
		if(stack->attackerOwned)
		{
			if(!owner->creDir[stack->ID])
				myAnim()->pos.x -= 44;
		}
		else
		{
			if(owner->creDir[stack->ID])
				myAnim()->pos.x += 44;
		}
	}

	secondPartSetup = true;

	if(myAnim()->framesInGroup(CCreatureAnim::TURN_L))
		myAnim()->setType(CCreatureAnim::TURN_L);
	else
		endAnim();
}

//defence anim

bool CDefenceAnim::init()
{
	//checking initial conditions

	//if(owner->creAnims[stackID]->getType() != 2)
	//{
	//	return false;
	//}

	if(attacker == NULL && owner->battleEffects.size() > 0)
		return false;

	int lowestMoveID = owner->animIDhelper + 5;
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CDefenceAnim * defAnim = dynamic_cast<CDefenceAnim *>(it->first);
		if(defAnim && defAnim->stack->ID != stack->ID)
			continue;

		CBattleAttack * attAnim = dynamic_cast<CBattleAttack *>(it->first);
		if(attAnim && attAnim->stack->ID != stack->ID)
			continue;

		if(attacker != NULL)
		{
			int attackerAnimType = owner->creAnims[attacker->ID]->getType();
			if( attackerAnimType == 11 && attackerAnimType == 12 && attackerAnimType == 13 && owner->creAnims[attacker->ID]->getFrame() < attacker->getCreature()->attackClimaxFrame )
				return false;
		}

		CReverseAnim * animAsRev = dynamic_cast<CReverseAnim *>(it->first);

		if(animAsRev && animAsRev->priority)
			return false;

		if(it->first)
			amin(lowestMoveID, it->first->ID);
	}
	if(ID > lowestMoveID)
		return false;

	

	//reverse unit if necessary
	if(attacker && isToReverse(stack->position, attacker->position, owner->creDir[stack->ID], attacker->doubleWide(), owner->creDir[attacker->ID]))
	{
		owner->addNewAnim(new CReverseAnim(owner, stack, stack->position, true));
		return false;
	}
	//unit reversed

	if(byShooting) //delay hit animation
	{		
		for(std::list<SProjectileInfo>::const_iterator it = owner->projectiles.begin(); it != owner->projectiles.end(); ++it)
		{
			if(it->creID == attacker->getCreature()->idNumber)
			{
				return false;
			}
		}
	}

	//initializing
	if(killed)
	{
		CCS->soundh->playSound(battle_sound(stack->getCreature(), killed));
		myAnim()->setType(CCreatureAnim::DEATH); //death
	}
	else
	{
		// TODO: this block doesn't seems correct if the unit is defending.
		CCS->soundh->playSound(battle_sound(stack->getCreature(), wince));
		myAnim()->setType(CCreatureAnim::HITTED); //getting hit
	}

	return true; //initialized successfuly
}

void CDefenceAnim::nextFrame()
{
	if(!killed && myAnim()->getType() != CCreatureAnim::HITTED)
	{
		myAnim()->setType(CCreatureAnim::HITTED);
	}

	if(!myAnim()->onLastFrameInGroup())
	{
		if( myAnim()->getType() == CCreatureAnim::DEATH && (owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0
			&& !myAnim()->onLastFrameInGroup() )
		{
			myAnim()->incrementFrame();
		}
	}
	else
	{
		endAnim();
	}
	
}

void CDefenceAnim::endAnim()
{
	//restoring animType

	if(myAnim()->getType() == CCreatureAnim::HITTED)
		myAnim()->setType(CCreatureAnim::HOLDING);

	//printing info to console

	//if(attacker!=NULL)
	//	owner->printConsoleAttacked(stack, dmg, amountKilled, attacker);

	//const CStack * attacker = owner->curInt->cb->battleGetStackByID(IDby, false);
	//const CStack * attacked = owner->curInt->cb->battleGetStackByID(stackID, false);

	CBattleAnimation::endAnim();

	delete this;
}

CDefenceAnim::CDefenceAnim(SStackAttackedInfo _attackedInfo, CBattleInterface * _owner)
: CBattleStackAnimation(_owner, _attackedInfo.defender), dmg(_attackedInfo.dmg),
	amountKilled(_attackedInfo.amountKilled), attacker(_attackedInfo.attacker), byShooting(_attackedInfo.byShooting),
	killed(_attackedInfo.killed)
{
}

////move anim

bool CBattleStackMoved::init()
{
	if( !isEarliest(false) )
		return false;

	//a few useful variables
	steps = myAnim()->framesInGroup(CCreatureAnim::MOVING)*owner->getAnimSpeedMultiplier()-1;
	if(steps == 0) //this creature seems to have no move animation so we can end it immediately
	{
		endAnim();
		return false;
	}
	whichStep = 0;
	int hexWbase = 44, hexHbase = 42;
	const CStack * movedStack = stack;
	if(!movedStack || myAnim()->getType() == 5)
	{
		endAnim();
		return false;
	}
	//bool twoTiles = movedStack->doubleWide();
	
	Point begPosition = CBattleHex::getXYUnitAnim(curStackPos, movedStack->attackerOwned, movedStack, owner);
	Point endPosition = CBattleHex::getXYUnitAnim(nextHex, movedStack->attackerOwned, movedStack, owner);

	int mutPos = THex::mutualPosition(curStackPos, nextHex);
	
	//reverse unit if necessary
	if((begPosition.x > endPosition.x) && owner->creDir[stack->ID] == true)
	{
		owner->addNewAnim(new CReverseAnim(owner, stack, curStackPos, true));
		return false;
	}
	else if ((begPosition.x < endPosition.x) && owner->creDir[stack->ID] == false)
	{
		owner->addNewAnim(new CReverseAnim(owner, stack, curStackPos, true));
		return false;
	}

	if(myAnim()->getType() != CCreatureAnim::MOVING)
	{
		myAnim()->setType(CCreatureAnim::MOVING);
	}
	//unit reversed

//	if(owner->moveSh <= 0)
//		owner->moveSh = CCS->soundh->playSound(battle_sound(movedStack->getCreature(), move), -1);

	//step shift calculation
	posX = myAnim()->pos.x, posY = myAnim()->pos.y; // for precise calculations ;]
	if(mutPos == -1 && movedStack->hasBonusOfType(Bonus::FLYING)) 
	{
		steps *= distance;
		steps /= 2; //to make animation faster

		stepX = (endPosition.x - (float)begPosition.x)/steps;
		stepY = (endPosition.y - (float)begPosition.y)/steps;
	}
	else
	{
		switch(mutPos)
		{
		case 0:
			stepX = (-1.0)*((float)hexWbase)/(2.0f*steps);
			stepY = (-1.0)*((float)hexHbase)/((float)steps);
			break;
		case 1:
			stepX = ((float)hexWbase)/(2.0f*steps);
			stepY = (-1.0)*((float)hexHbase)/((float)steps);
			break;
		case 2:
			stepX = ((float)hexWbase)/((float)steps);
			stepY = 0.0;
			break;
		case 3:
			stepX = ((float)hexWbase)/(2.0f*steps);
			stepY = ((float)hexHbase)/((float)steps);
			break;
		case 4:
			stepX = (-1.0)*((float)hexWbase)/(2.0f*steps);
			stepY = ((float)hexHbase)/((float)steps);
			break;
		case 5:
			stepX = (-1.0)*((float)hexWbase)/((float)steps);
			stepY = 0.0;
			break;
		}
	}
	//step shifts calculated

	return true;
}

void CBattleStackMoved::nextFrame()
{
	//moving instructions
	posX += stepX;
	myAnim()->pos.x = posX;
	posY += stepY;
	myAnim()->pos.y = posY;

	// Increments step count and check if we are finished with current animation
	++whichStep;
	if(whichStep == steps)
	{
		// Sets the position of the creature animation sprites
		Point coords = CBattleHex::getXYUnitAnim(nextHex, owner->creDir[stack->ID], stack, owner);
		myAnim()->pos = coords;
		
		// true if creature haven't reached the final destination hex
		if ((nextPos + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			nextPos++;
			curStackPos = nextHex;
			nextHex = destTiles[nextPos];
			
			// update position of double wide creatures
			bool twoTiles = stack->doubleWide();
			if(twoTiles && bool(stack->attackerOwned) && (owner->creDir[stack->ID] != bool(stack->attackerOwned) )) //big attacker creature is reversed
				myAnim()->pos.x -= 44;
			else if(twoTiles && (! bool(stack->attackerOwned) ) && (owner->creDir[stack->ID] != bool(stack->attackerOwned) )) //big defender creature is reversed
				myAnim()->pos.x += 44;
			
			// re-init animation
			for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
			{
				if (it->first == this)
				{
					it->second = false;
					break;
				}
			}
		}
		else
			endAnim();
	}
}

void CBattleStackMoved::endAnim()
{
	const CStack * movedStack = stack;

	CBattleAnimation::endAnim();

	if(movedStack)
		owner->addNewAnim(new CBattleMoveEnd(owner, stack, nextHex));


	if(owner->moveSh >= 0)
	{
		CCS->soundh->stopSound(owner->moveSh);
		owner->moveSh = -1;
	}

	delete this;
}

CBattleStackMoved::CBattleStackMoved(CBattleInterface * _owner, const CStack * _stack, std::vector<THex> _destTiles, int _distance)
: CBattleStackAnimation(_owner, _stack), destTiles(_destTiles), nextPos(0), distance(_distance), stepX(0.0f), stepY(0.0f)
{
	curStackPos = stack->position;
	nextHex = destTiles.front();
}

//move started

bool CBattleMoveStart::init()
{
	if( !isEarliest(false) )
		return false;


	if(!stack || myAnim()->getType() == 5)
	{
		CBattleMoveStart::endAnim();
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));
	myAnim()->setType(CCreatureAnim::MOVE_START);

	return true;
}

void CBattleMoveStart::nextFrame()
{
	if(myAnim()->onLastFrameInGroup())
	{
		endAnim();
	}
	else
	{
 		if((owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0)
 			myAnim()->incrementFrame();
	}
}

void CBattleMoveStart::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CBattleMoveStart::CBattleMoveStart(CBattleInterface * _owner, const CStack * _stack)
: CBattleStackAnimation(_owner, _stack)
{
}

//move finished

bool CBattleMoveEnd::init()
{
	if( !isEarliest(true) )
		return false;

	if(!stack || myAnim()->framesInGroup(CCreatureAnim::MOVE_END) == 0 ||
		myAnim()->getType() == CCreatureAnim::DEATH)
	{
		endAnim();

		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	myAnim()->setType(CCreatureAnim::MOVE_END);

	return true;
}

void CBattleMoveEnd::nextFrame()
{
	if(myAnim()->onLastFrameInGroup())
	{
		endAnim();
	}
}

void CBattleMoveEnd::endAnim()
{
	CBattleAnimation::endAnim();

	if(myAnim()->getType() != CCreatureAnim::DEATH)
		myAnim()->setType(CCreatureAnim::HOLDING); //resetting to default

	CCS->curh->show();
	delete this;
}

CBattleMoveEnd::CBattleMoveEnd(CBattleInterface * _owner, const CStack * _stack, THex destTile)
: CBattleStackAnimation(_owner, _stack), destinationTile(destTile)
{
}

//general attack anim

void CBattleAttack::nextFrame()
{
	if(myAnim()->getType() != group)
		myAnim()->setType(group);

	if(myAnim()->onFirstFrameInGroup())
	{
		if(shooting)
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), shoot));
		else
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), attack));
	}
	else if(myAnim()->onLastFrameInGroup())
	{
		myAnim()->setType(CCreatureAnim::HOLDING);
		endAnim();
		return; //execution of endAnim deletes this !!!
	}
}

bool CBattleAttack::checkInitialConditions()
{
	return isEarliest(false);
}

CBattleAttack::CBattleAttack(CBattleInterface * _owner, const CStack * attacker, THex _dest, const CStack * defender)
: CBattleStackAnimation(_owner, attacker), dest(_dest), attackedStack(defender), attackingStack(attacker)
{

	assert(attackingStack && "attackingStack is NULL in CBattleAttack::CBattleAttack !\n");
	if(attackingStack->getCreature()->idNumber != 145) //catapult is allowed to attack not-creature
	{
		assert(attackedStack && "attackedStack is NULL in CBattleAttack::CBattleAttack !\n");
	}
	else //catapult can attack walls only
	{
		assert(owner->curInt->cb->battleGetWallUnderHex(_dest) >= 0);
	}
	attackingStackPosBeforeReturn = attackingStack->position;
}

////melee attack

bool CMeleeAttack::init()
{
	if( !CBattleAttack::checkInitialConditions() )
		return false;

	//if(owner->creAnims[stackID]->getType()!=2)
	//{
	//	return false;
	//}

	if(!attackingStack || myAnim()->getType() == 5)
	{
		endAnim();
		
		return false;
	}

	bool toReverse = isToReverse(attackingStackPosBeforeReturn, dest, owner->creDir[stack->ID], attackedStack->doubleWide(), owner->creDir[attackedStack->ID]);

	if(toReverse)
	{

		owner->addNewAnim(new CReverseAnim(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}
	//reversed

	shooting = false;

	static const CCreatureAnim::EAnimType mutPosToGroup[] = {CCreatureAnim::ATTACK_UP, CCreatureAnim::ATTACK_UP,
		CCreatureAnim::ATTACK_FRONT, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_FRONT};

	int revShiftattacker = (attackingStack->attackerOwned ? -1 : 1);

	int mutPos = THex::mutualPosition(attackingStackPosBeforeReturn, dest);
	if(mutPos == -1 && attackingStack->doubleWide())
	{
		mutPos = THex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->position);
	}
	if (mutPos == -1 && attackedStack->doubleWide())
	{
		mutPos = THex::mutualPosition(attackingStackPosBeforeReturn, attackedStack->occupiedHex());
	}
	if (mutPos == -1 && attackedStack->doubleWide() && attackingStack->doubleWide())
	{
		mutPos = THex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->occupiedHex());
	}


	switch(mutPos) //attack direction
	{
	case 0: case 1: case 2: case 3: case 4: case 5:
		group = mutPosToGroup[mutPos];
		break;
	default:
		tlog1<<"Critical Error! Wrong dest in stackAttacking! dest: "<<dest<<" attacking stack pos: "<<attackingStackPosBeforeReturn<<" mutual pos: "<<mutPos<<std::endl;
		group = CCreatureAnim::ATTACK_FRONT;
		break;
	}

	return true;
}

void CMeleeAttack::nextFrame()
{
	/*for(std::list<std::pair<CBattleAnimation *, bool> >::const_iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CBattleMoveStart * anim = dynamic_cast<CBattleMoveStart *>(it->first);
		CReverseAnim * anim2 = dynamic_cast<CReverseAnim *>(it->first);
		if( (anim && anim->stackID == stackID) || (anim2 && anim2->stackID == stackID ) )
			return;
	}*/

	CBattleAttack::nextFrame();
}

void CMeleeAttack::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CMeleeAttack::CMeleeAttack(CBattleInterface * _owner, const CStack * attacker, THex _dest, const CStack * _attacked)
: CBattleAttack(_owner, attacker, _dest, _attacked)
{
}

//shooting anim

bool CShootingAnim::init()
{
	if( !CBattleAttack::checkInitialConditions() )
		return false;

	const CStack * shooter = attackingStack;

	if(!shooter || myAnim()->getType() == 5)
	{
		endAnim();
		return false;
	}

	// Create the projectile animation
	
	float projectileAngle; //in radians; if positive, projectiles goes up
	float straightAngle = 0.2f; //maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	int fromHex = shooter->position;
	projectileAngle = atan2(float(abs(dest - fromHex) / BFIELD_WIDTH), float(abs(dest - fromHex) % BFIELD_WIDTH));
	if(fromHex < dest)
		projectileAngle = -projectileAngle;
	
	// Get further info about the shooter e.g. relative pos of projectile to unit.
	// If the creature id is 149 then it's a arrow tower which has no additional info so get the
	// actual arrow tower shooter instead.
	const CCreature *shooterInfo = shooter->getCreature();
	if (shooterInfo->idNumber == 149)
	{
		int creID = CGI->creh->factionToTurretCreature[owner->siegeH->town->town->typeID];
		shooterInfo = CGI->creh->creatures[creID];
	}
	
	SProjectileInfo spi;
	spi.creID = shooter->getCreature()->idNumber;
	spi.stackID = shooter->ID;
	spi.reverse = !shooter->attackerOwned;

	spi.step = 0;
	spi.frameNum = 0;
	if(vstd::contains(CGI->creh->idToProjectileSpin, shooterInfo->idNumber))
		spi.spin = CGI->creh->idToProjectileSpin[shooterInfo->idNumber];
	else
	{
		tlog2 << "Warning - no projectile spin for spi.creID " << shooterInfo->idNumber << std::endl;
		spi.spin = false;
	}

	Point xycoord = CBattleHex::getXYUnitAnim(shooter->position, true, shooter, owner);
	Point destcoord;
	
	
	// The "master" point where all projectile positions relate to.
	static const Point projectileOrigin(181, 252);

	if (attackedStack)
	{
		destcoord = CBattleHex::getXYUnitAnim(dest, false, attackedStack, owner); 
		destcoord.x += 250; destcoord.y += 210; //TODO: find a better place to shoot

		// Calculate projectile start position. Offsets are read out of the CRANIM.TXT.
		if (projectileAngle > straightAngle)
		{
			//upper shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->upperRightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->upperRightMissleOffsetY;
		}
		else if (projectileAngle < -straightAngle) 
		{
			//lower shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->lowerRightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->lowerRightMissleOffsetY;
		}
		else 
		{
			//straight shot
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->rightMissleOffsetX;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->rightMissleOffsetY;
		}
	
		double animSpeed = 23.0 * owner->getAnimSpeed(); // flight speed of projectile
		spi.lastStep = sqrt((float)((destcoord.x - spi.x)*(destcoord.x - spi.x) + (destcoord.y - spi.y) * (destcoord.y - spi.y))) / animSpeed;
		if(spi.lastStep == 0)
			spi.lastStep = 1;
		spi.dx = (destcoord.x - spi.x) / spi.lastStep;
		spi.dy = (destcoord.y - spi.y) / spi.lastStep;
		spi.catapultInfo = 0;
	}
	else 
	{
		// Catapult attack
		// These are the values for equations of this kind: f(x) = ax^2 + bx + c
		static const std::vector<CatapultProjectileInfo*> trajectoryCurves = boost::assign::list_of<CatapultProjectileInfo*>(new CatapultProjectileInfo(4.309, -3.198, 569.2, -296, 182))
			(new CatapultProjectileInfo(4.710, -3.11, 558.68, -258, 175))(new CatapultProjectileInfo(5.056, -3.003, 546.9, -236, 174))
			(new CatapultProjectileInfo(4.760, -2.74, 526.47, -216, 215))(new CatapultProjectileInfo(4.288, -2.496, 508.98, -223, 274))
			(new CatapultProjectileInfo(3.683, -3.018, 558.39, -324, 176))(new CatapultProjectileInfo(2.884, -2.607, 528.95, -366, 312))
			(new CatapultProjectileInfo(3.783, -2.364, 501.35, -227, 318));

		static std::map<int, int> hexToCurve = boost::assign::map_list_of<int, int>(29, 0)(62, 1)(95, 2)(130, 3)(182, 4)(12, 5)(50, 6)(183, 7);

		std::map<int, int>::iterator it = hexToCurve.find(dest.hex);

		if (it == hexToCurve.end())
		{
			tlog1 << "For the hex position " << dest.hex << " is no curve defined.";
			endAnim();
			return false;
		}
		else
		{
			int curveID = it->second;
			spi.catapultInfo = trajectoryCurves[curveID];
			double animSpeed = 3.318 * owner->getAnimSpeed();
			spi.lastStep = (spi.catapultInfo->toX - spi.catapultInfo->fromX) / animSpeed;
			spi.dx = animSpeed;
			spi.dy = 0;
			spi.x = xycoord.x + projectileOrigin.x + shooterInfo->rightMissleOffsetX + 17.;
			spi.y = xycoord.y + projectileOrigin.y + shooterInfo->rightMissleOffsetY + 10.;

			// Add explosion anim
			int xEnd = spi.x + spi.lastStep * spi.dx;
			int yEnd = spi.catapultInfo->calculateY(xEnd);
			owner->addNewAnim( new CSpellEffectAnim(owner, "SGEXPL.DEF", xEnd - 126, yEnd - 105));
		}
	}
	
	// Set starting frame
	if(spi.spin)
	{
		spi.frameNum = 0;
	}
	else
	{
		spi.frameNum = ((M_PI/2.0f - projectileAngle) / (2.0f *M_PI) + 1/((float)(2*(owner->idToProjectile[spi.creID]->ourImages.size()-1)))) * (owner->idToProjectile[spi.creID]->ourImages.size()-1);
	}

	// Set projectile animation start delay which is specified in frames
	spi.animStartDelay = shooterInfo->attackClimaxFrame;
	owner->projectiles.push_back(spi);

	//attack animation

	shooting = true;

	if(projectileAngle > straightAngle) //upper shot
		group = CCreatureAnim::SHOOT_UP;
	else if(projectileAngle < -straightAngle) //lower shot
		group = CCreatureAnim::SHOOT_DOWN;
	else //straight shot
		group = CCreatureAnim::SHOOT_FRONT;

	return true;
}

void CShootingAnim::nextFrame()
{
	for(std::list<std::pair<CBattleAnimation *, bool> >::const_iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CBattleMoveStart * anim = dynamic_cast<CBattleMoveStart *>(it->first);
		CReverseAnim * anim2 = dynamic_cast<CReverseAnim *>(it->first);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
			return;
	}

	CBattleAttack::nextFrame();
}

void CShootingAnim::endAnim()
{
	CBattleAnimation::endAnim();
	delete this;
}

CShootingAnim::CShootingAnim(CBattleInterface * _owner, const CStack * attacker, THex _dest, const CStack * _attacked, bool _catapult, int _catapultDmg)
: CBattleAttack(_owner, attacker, _dest, _attacked), catapultDamage(_catapultDmg), catapult(_catapult)
{
	
}

////////////////////////

void CBattleInterface::addNewAnim(CBattleAnimation * anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	animsAreDisplayed.setn(true);
}

CBattleInterface::CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect, CPlayerInterface * att, CPlayerInterface * defen)
	: queue(NULL), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), 
	  activeStack(NULL), stackToActivate(NULL), mouseHoveredStack(-1), lastMouseHoveredStackAnimationTime(-1), previouslyHoveredHex(-1), spellSelMode(NO_LOCATION),
	  currentlyHoveredHex(-1), attackingHex(-1), tacticianInterface(NULL),  stackCanCastSpell(false), spellDestSelectMode(false), spellToCast(NULL),
	  siegeH(NULL), attackerInt(att), defenderInt(defen), curInt(att), animIDhelper(0), givenCommand(NULL),
	  myTurn(false), resWindow(NULL), moveStarted(false), moveSh(-1), bresult(NULL)
      
{
	ObjectConstruction h__l__p(this);

	if(!curInt) curInt = LOCPLINT; //may happen when we are defending during network MP game

	animsAreDisplayed.setn(false);
	pos = myRect;
	strongInterest = true;
	givenCommand = new CondSh<BattleAction *>(NULL);

	if(attackerInt && attackerInt->cb->battleGetTacticDist()) //hotseat -> check tactics for both players (defender may be local human)
		tacticianInterface = attackerInt;
	else if(defenderInt && defenderInt->cb->battleGetTacticDist()) 
		tacticianInterface = defenderInt;

	tacticsMode = tacticianInterface;  //if we found interface of player with tactics, then enter tactics mode

	//create stack queue
	bool embedQueue = screen->h < 700;
	queue = new CStackQueue(embedQueue, this);
	if(!embedQueue)
	{
		if(curInt->sysOpts.showQueue)
			pos.y += queue->pos.h / 2; //center whole window

		queue->moveTo(Point(pos.x, pos.y - queue->pos.h));
// 		queue->pos.x = pos.x;
// 		queue->pos.y = pos.y - queue->pos.h;
//  		pos.h += queue->pos.h;
//  		center();
	}
	queue->update();

	//preparing siege info
	const CGTownInstance * town = curInt->cb->battleGetDefendedTown();
	if(town && town->hasFort())
	{
		siegeH = new SiegeHelper(town, this);
	}

	curInt->battleInt = this;

	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	std::vector<const CStack*> stacks = curInt->cb->battleGetAllStacks();
	BOOST_FOREACH(const CStack *s, stacks)
	{
		newStack(s);
	}

	//preparing menu background and terrain
	if(siegeH)
	{
		background = BitmapHandler::loadBitmap( siegeH->getSiegeName(0), false );
		ui8 siegeLevel = curInt->cb->battleGetSiegeLevel();
		if(siegeLevel >= 2) //citadel or castle
		{
			//print moat/mlip
			SDL_Surface * moat = BitmapHandler::loadBitmap( siegeH->getSiegeName(13) ),
				* mlip = BitmapHandler::loadBitmap( siegeH->getSiegeName(14) );

			Point moatPos = graphics->wallPositions[siegeH->town->town->typeID][12],
				mlipPos = graphics->wallPositions[siegeH->town->town->typeID][13];

			if(moat) //eg. tower has no moat
				blitAt(moat, moatPos.x,moatPos.y, background);
			if(mlip) //eg. tower has no mlip
				blitAt(mlip, mlipPos.x, mlipPos.y, background);

			SDL_FreeSurface(moat);
			SDL_FreeSurface(mlip);
		}
	}
	else
	{
		std::vector< std::string > & backref = graphics->battleBacks[ curInt->cb->battleGetBattlefieldType() ];
		background = BitmapHandler::loadBitmap(backref[ rand() % backref.size()], false );
	}
	
	//preparing menu background
	//graphics->blueToPlayersAdv(menu, hero1->tempOwner);

	//preparing graphics for displaying amounts of creatures
	amountNormal = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNormal);
	transformPalette(amountNormal, 0.59f, 0.19f, 0.93f);
	
	amountPositive = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountPositive);
	transformPalette(amountPositive, 0.18f, 1.00f, 0.18f);

	amountNegative = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNegative);
	transformPalette(amountNegative, 1.00f, 0.18f, 0.18f);

	amountEffNeutral = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountEffNeutral);
	transformPalette(amountEffNeutral, 1.00f, 1.00f, 0.18f);

	////blitting menu background and terrain
// 	blitAt(background, pos.x, pos.y);
// 	blitAt(menu, pos.x, 556 + pos.y);

	//preparing buttons and console
	bOptions = new AdventureMapButton (CGI->generaltexth->zelp[381].first, CGI->generaltexth->zelp[381].second, boost::bind(&CBattleInterface::bOptionsf,this), 3 + pos.x, 561 + pos.y, "icm003.def", SDLK_o);
	bSurrender = new AdventureMapButton (CGI->generaltexth->zelp[379].first, CGI->generaltexth->zelp[379].second, boost::bind(&CBattleInterface::bSurrenderf,this), 54 + pos.x, 561 + pos.y, "icm001.def", SDLK_s);
	bFlee = new AdventureMapButton (CGI->generaltexth->zelp[380].first, CGI->generaltexth->zelp[380].second, boost::bind(&CBattleInterface::bFleef,this), 105 + pos.x, 561 + pos.y, "icm002.def", SDLK_r);
	bFlee->block(!curInt->cb->battleCanFlee());	
	bSurrender->block(curInt->cb->battleGetSurrenderCost() < 0);
	bAutofight  = new AdventureMapButton (CGI->generaltexth->zelp[382].first, CGI->generaltexth->zelp[382].second, boost::bind(&CBattleInterface::bAutofightf,this), 157 + pos.x, 561 + pos.y, "icm004.def", SDLK_a);
	bSpell = new AdventureMapButton (CGI->generaltexth->zelp[385].first, CGI->generaltexth->zelp[385].second, boost::bind(&CBattleInterface::bSpellf,this), 645 + pos.x, 561 + pos.y, "icm005.def", SDLK_c);
	bSpell->block(true);
	bWait = new AdventureMapButton (CGI->generaltexth->zelp[386].first, CGI->generaltexth->zelp[386].second, boost::bind(&CBattleInterface::bWaitf,this), 696 + pos.x, 561 + pos.y, "icm006.def", SDLK_w);
	bDefence = new AdventureMapButton (CGI->generaltexth->zelp[387].first, CGI->generaltexth->zelp[387].second, boost::bind(&CBattleInterface::bDefencef,this), 747 + pos.x, 561 + pos.y, "icm007.def", SDLK_d);
	bDefence->assignedKeys.insert(SDLK_SPACE);
	bConsoleUp = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleUpf,this), 624 + pos.x, 561 + pos.y, "ComSlide.def", SDLK_UP);
	bConsoleDown = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleDownf,this), 624 + pos.x, 580 + pos.y, "ComSlide.def", SDLK_DOWN);
	bConsoleDown->setOffset(2);
	console = new CBattleConsole();
	console->pos.x = 211 + pos.x;
	console->pos.y = 560 + pos.y;
	console->pos.w = 406;
	console->pos.h = 38;
	if(tacticsMode)
	{
		btactNext = new AdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bTacticNextStack,this), 213 + pos.x, 560 + pos.y, "icm011.def", SDLK_SPACE);
		btactEnd = new AdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bEndTacticPhase,this), 419 + pos.x, 560 + pos.y, "icm012.def", SDLK_RETURN);
		bDefence->block(true);
		bWait->block(true);
		menu = BitmapHandler::loadBitmap("COPLACBR.BMP");
	}
	else
	{
		menu = BitmapHandler::loadBitmap("CBAR.BMP");
		btactEnd = btactNext = NULL;
	}
	graphics->blueToPlayersAdv(menu, curInt->playerID);

	//loading hero animations
	if(hero1) // attacking hero
	{
		int type = hero1->type->heroType;
		if ( type % 2 )   type--;
		if ( hero1->sex ) type++;
		attackingHero = new CBattleHero(graphics->battleHeroes[type], 0, 0, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : NULL, this);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, pos.x - 43, pos.y - 19);
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		int type = hero2->type->heroType;
		if ( type % 2 )   type--;
		if ( hero2->sex ) type++;
		defendingHero = new CBattleHero(graphics->battleHeroes[type ], 0, 0, true, hero2->tempOwner, hero2->tempOwner == curInt->playerID ? hero2 : NULL, this);
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, pos.x + 693, pos.y - 19);
	}
	else
	{
		defendingHero = NULL;
	}

	//preparing cells and hexes
	cellBorder = BitmapHandler::loadBitmap("CCELLGRD.BMP");
	CSDL_Ext::alphaTransform(cellBorder);
	cellShade = BitmapHandler::loadBitmap("CCELLSHD.BMP");
	CSDL_Ext::alphaTransform(cellShade);
	for(int h=0; h<BFIELD_SIZE; ++h)
	{
		bfield[h].myNumber = h;

		int x = 14 + ((h/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(h%BFIELD_WIDTH);
		int y = 86 + 42 * (h/BFIELD_WIDTH);
		bfield[h].pos = genRect(cellShade->h, cellShade->w, x + pos.x, y + pos.y);
		bfield[h].accessible = true;
		bfield[h].myInterface = this;
	}
	//locking occupied positions on batlefield
	BOOST_FOREACH(const CStack *s, stacks)  //stacks gained at top of this function
		if(s->position >= 0) //turrets have position < 0
			bfield[s->position].accessible = false;

	//loading projectiles for units
	BOOST_FOREACH(const CStack *s, stacks)
	{
		int creID = (s->getCreature()->idNumber == 149) ? CGI->creh->factionToTurretCreature[siegeH->town->town->typeID] : s->getCreature()->idNumber; //id of creature whose shots should be loaded
		if(s->getCreature()->isShooting() && vstd::contains(CGI->creh->idToProjectile, creID))
		{
			CDefHandler *&projectile = idToProjectile[s->getCreature()->idNumber];
			projectile = CDefHandler::giveDef(CGI->creh->idToProjectile[creID]);

			if(projectile->ourImages.size() > 2) //add symmetric images
			{
				for(int k = projectile->ourImages.size()-2; k > 1; --k)
				{
					Cimage ci;
					ci.bitmap = CSDL_Ext::rotate01(projectile->ourImages[k].bitmap);
					ci.groupNumber = 0;
					ci.imName = std::string();
					projectile->ourImages.push_back(ci);
				}
			}
			for(int s=0; s<projectile->ourImages.size(); ++s) //alpha transforming
			{
				CSDL_Ext::alphaTransform(projectile->ourImages[s].bitmap);
			}
		}
	}


	//preparing graphic with cell borders
	cellBorders = CSDL_Ext::newSurface(background->w, background->h, cellBorder);
	//copying palette
	for(int g=0; g<cellBorder->format->palette->ncolors; ++g) //we assume that cellBorders->format->palette->ncolors == 256
	{
		cellBorders->format->palette->colors[g] = cellBorder->format->palette->colors[g];
	}
	//palette copied
	for(int i=0; i<BFIELD_HEIGHT; ++i) //rows
	{
		for(int j=0; j<BFIELD_WIDTH-2; ++j) //columns
		{
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 * i;
			for(int cellX = 0; cellX < cellBorder->w; ++cellX)
			{
				for(int cellY = 0; cellY < cellBorder->h; ++cellY)
				{
					if(y+cellY < cellBorders->h && x+cellX < cellBorders->w)
						* ((Uint8*)cellBorders->pixels + (y+cellY) * cellBorders->pitch + (x+cellX)) |= * ((Uint8*)cellBorder->pixels + cellY * cellBorder->pitch + cellX);
				}
			}
		}
	}

	backgroundWithHexes = CSDL_Ext::newSurface(background->w, background->h, screen);

	//preparing obstacle defs
	std::vector<CObstacleInstance> obst = curInt->cb->battleGetAllObstacles();
	for(int t=0; t<obst.size(); ++t)
	{
		idToObstacle[obst[t].ID] = CDefHandler::giveDef(CGI->heroh->obstacles.find(obst[t].ID)->second.defName);
		for(int n=0; n<idToObstacle[obst[t].ID]->ourImages.size(); ++n)
		{
			SDL_SetColorKey(idToObstacle[obst[t].ID]->ourImages[n].bitmap, SDL_SRCCOLORKEY, SDL_MapRGB(idToObstacle[obst[t].ID]->ourImages[n].bitmap->format,0,255,255));
		}
	}

	for (int i = 0; i < ARRAY_COUNT(bfield); i++)
	{
		children.push_back(&bfield[i]);
	}

	if(tacticsMode)
	{
		active = 1;
		bTacticNextStack();
		active = 0;
	}

	CCS->musich->stopMusic();

	int channel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	CCS->soundh->setCallback(channel, boost::bind(&CMusicHandler::playMusicFromSet, CCS->musich, CCS->musich->battleMusics, -1));
    memset(stackCountOutsideHexes, 1, BFIELD_SIZE * sizeof(bool)); //initialize array with trues	
}

CBattleInterface::~CBattleInterface()
{
	if (active) //dirty fix for #485
	{
		deactivate();
	}
	SDL_FreeSurface(background);
	SDL_FreeSurface(menu);
	SDL_FreeSurface(amountNormal);
	SDL_FreeSurface(amountNegative);
	SDL_FreeSurface(amountPositive);
	SDL_FreeSurface(amountEffNeutral);
	SDL_FreeSurface(cellBorders);
	SDL_FreeSurface(backgroundWithHexes);
	delete bOptions;
	delete bSurrender;
	delete bFlee;
	delete bAutofight;
	delete bSpell;
	delete bWait;
	delete bDefence;
	delete bConsoleUp;
	delete bConsoleDown;
	delete console;
	delete givenCommand;

	delete attackingHero;
	delete defendingHero;
	delete queue;

	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	for(std::map< int, CCreatureAnimation * >::iterator g=creAnims.begin(); g!=creAnims.end(); ++g)
		delete g->second;

	for(std::map< int, CDefHandler * >::iterator g=idToProjectile.begin(); g!=idToProjectile.end(); ++g)
		delete g->second;

	for(std::map< int, CDefHandler * >::iterator g=idToObstacle.begin(); g!=idToObstacle.end(); ++g)
		delete g->second;

	delete siegeH;
	curInt->battleInt = NULL;

	//TODO: play AI tracks if battle was during AI turn
	//if (!curInt->makingTurn) 
	//CCS->musich->playMusicFromSet(CCS->musich->aiMusics, -1);
	
	if(adventureInt && adventureInt->selection)
	{
		int terrain = LOCPLINT->cb->getTile(adventureInt->selection->visitablePos())->tertype;
		CCS->musich->playMusic(CCS->musich->terrainMusics[terrain], -1);
	}
}

void CBattleInterface::setPrintCellBorders(bool set)
{
	curInt->sysOpts.printCellBorders = set;
	curInt->sysOpts.settingsChanged();
	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintStackRange(bool set)
{
	curInt->sysOpts.printStackRange = set;
	curInt->sysOpts.settingsChanged();
	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintMouseShadow(bool set)
{
	curInt->sysOpts.printMouseShadow = set;
	curInt->sysOpts.settingsChanged();
}

void CBattleInterface::activate()
{
	activateKeys();
	activateMouseMove();
	activateRClick();
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();
	for(int b=0; b<BFIELD_SIZE; ++b)
	{
		bfield[b].activate();
	}
	if(attackingHero)
		attackingHero->activate();
	if(defendingHero)
		defendingHero->activate();
	if(curInt->sysOpts.showQueue)
		queue->activate();

	if(tacticsMode)
	{
		btactNext->activate();
		btactEnd->activate();
	}
	else
	{
		bConsoleUp->activate();
		bConsoleDown->activate();
	}

	LOCPLINT->cingconsole->activate();
}

void CBattleInterface::deactivate()
{
	deactivateKeys();
	deactivateMouseMove();
	deactivateRClick();
	bOptions->deactivate();
	bSurrender->deactivate();
	bFlee->deactivate();
	bAutofight->deactivate();
	bSpell->deactivate();
	bWait->deactivate();
	bDefence->deactivate();
	for(int b=0; b<BFIELD_SIZE; ++b)
	{
		bfield[b].deactivate();
	}
	if(attackingHero)
		attackingHero->deactivate();
	if(defendingHero)
		defendingHero->deactivate();
	if(curInt->sysOpts.showQueue)
		queue->deactivate();

	if(tacticsMode)
	{
		btactNext->deactivate();
		btactEnd->deactivate();
	}
	else
	{
		bConsoleUp->deactivate();
		bConsoleDown->deactivate();
	}

	LOCPLINT->cingconsole->deactivate();
}

void CBattleInterface::show(SDL_Surface * to)
{
	std::vector<const CStack*> stacks = curInt->cb->battleGetAllStacks(); //used in a few places
	++animCount;
	if(!to) //"evaluating" to
		to = screen;
	
	SDL_Rect buf;
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	//printing background and hexes
	if(activeStack != NULL && creAnims[activeStack->ID]->getType() != 0) //show everything with range
	{
		blitAt(backgroundWithHexes, pos.x, pos.y, to);
	}
	else
	{
		//showing background
		blitAt(background, pos.x, pos.y, to);
		if(curInt->sysOpts.printCellBorders)
		{
			CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, to, &pos);
		}
	}
	//printing hovered cell
	for(int b=0; b<BFIELD_SIZE; ++b)
	{
		if(bfield[b].strictHovered && bfield[b].hovered)
		{
			if(previouslyHoveredHex == -1) previouslyHoveredHex = b; //something to start with
			if(currentlyHoveredHex == -1) currentlyHoveredHex = b; //something to start with
			if(currentlyHoveredHex != b) //repair hover info
			{
				previouslyHoveredHex = currentlyHoveredHex;
				currentlyHoveredHex = b;
			}
			//print shade
			if(spellToCast) //when casting spell
			{
				//calculating spell school level
				const CSpell & spToCast =  *CGI->spellh->spells[spellToCast->additionalInfo];
				ui8 schoolLevel = 0;
				if( activeStack->attackerOwned )
				{
					if(attackingHeroInstance)
						schoolLevel = attackingHeroInstance->getSpellSchoolLevel(&spToCast);
				}
				else
				{
					if(defendingHeroInstance)
						schoolLevel = defendingHeroInstance->getSpellSchoolLevel(&spToCast);
				}
				//obtaining range and printing it
				std::set<ui16> shaded = spToCast.rangeInHexes(b, schoolLevel);
				for(std::set<ui16>::iterator it = shaded.begin(); it != shaded.end(); ++it) //for spells with range greater then one hex
				{
					if(curInt->sysOpts.printMouseShadow && (*it % BFIELD_WIDTH != 0) && (*it % BFIELD_WIDTH != 16))
					{
						int x = 14 + ((*it/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(*it%BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (*it/BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &temp_rect);
					}
				}
			}
			else if(curInt->sysOpts.printMouseShadow) //when not casting spell
			{//TODO: do not check it every frame
				if (activeStack) //highlight all attackable hexes
				{ 
					std::set<THex> set = curInt->cb->battleGetAttackedHexes(activeStack, currentlyHoveredHex, attackingHex);
					BOOST_FOREACH(THex hex, set)
					{
						int x = 14 + ((hex/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(hex%BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (hex/BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &temp_rect);
					}
				}
				//always highlight pointed hex
				int x = 14 + ((b/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(b%BFIELD_WIDTH) + pos.x;
				int y = 86 + 42 * (b/BFIELD_WIDTH) + pos.y;
				SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
				CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &temp_rect);
			}
		}
	}

	
	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	//prevents blitting outside this window
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	//preparing obstacles to be shown
	std::vector<CObstacleInstance> obstacles = curInt->cb->battleGetAllObstacles();
	std::multimap<THex, int> hexToObstacle;
	for(int b=0; b<obstacles.size(); ++b)
	{
		THex position = CGI->heroh->obstacles.find(obstacles[b].ID)->second.getMaxBlocked(obstacles[b].pos);
		hexToObstacle.insert(std::make_pair(position, b));
	}
	
	////showing units //a lot of work...
	std::vector<const CStack *> stackAliveByHex[BFIELD_SIZE];
	//double loop because dead stacks should be printed first
	for (int i = 0; i < stacks.size(); i++)
	{
		const CStack *s = stacks[i];
		if(creAnims.find(s->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->getType() != 5 && s->position >= 0) //don't show turrets here
			stackAliveByHex[s->position].push_back(s);
	}
	std::vector<const CStack *> stackDeadByHex[BFIELD_SIZE];
	for (int i = 0; i < stacks.size(); i++)
	{
		const CStack *s = stacks[i];
		if(creAnims.find(s->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->getType() == 5)
			stackDeadByHex[s->position].push_back(s);
	}

	//handle animations
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = pendingAnims.begin(); it != pendingAnims.end(); ++it)
	{
		if(!it->first) //this animation should be deleted
			continue;

		if(!it->second)
		{
			it->second = it->first->init();
		}
		if(it->second && it->first)
			it->first->nextFrame();
	}

	//delete anims
	int preSize = pendingAnims.size();
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = pendingAnims.begin(); it != pendingAnims.end(); ++it)
	{
		if(it->first == NULL)
		{
			pendingAnims.erase(it);
			it = pendingAnims.begin();
			break;
		}
	}

	if(preSize > 0 && pendingAnims.size() == 0)
	{
		//action finished, restore the interface
		if(!active)
			activate();

		//activation of next stack
		if(pendingAnims.size() == 0 && stackToActivate != NULL)
		{
			activateStack();
		}
		//anims ended
		animsAreDisplayed.setn(false);
	}

	for(int b=0; b<BFIELD_SIZE; ++b) //showing dead stacks
	{
		for(size_t v=0; v<stackDeadByHex[b].size(); ++v)
		{
			creAnims[stackDeadByHex[b][v]->ID]->nextFrame(to, creAnims[stackDeadByHex[b][v]->ID]->pos.x, creAnims[stackDeadByHex[b][v]->ID]->pos.y, creDir[stackDeadByHex[b][v]->ID], animCount, false); //increment always when moving, never if stack died
		}
	}
	std::vector<const CStack *> flyingStacks; //flying stacks should be displayed later, over other stacks and obstacles
	if (!siegeH)
	{
		for(int b = 0; b < BFIELD_SIZE; ++b) //showing alive stacks
		{
			showAliveStacks(stackAliveByHex, b, &flyingStacks, to);
			showObstacles(&hexToObstacle, obstacles, b, to);
			showPieceOfWall(to, b, stacks);
		}
	}
	// Siege drawing
	else
	{
		for (int i = 0; i < 4; i++)
		{
			// xMin, xMax => go from hex x pos to hex x pos
			// yMin, yMax => go from hex y pos to hex y pos
			// xMove => 0: left side, 1: right side
			// xMoveDir => 0: decrement, 1: increment, alters every second hex line either xMin or xMax depending on xMove
			int xMin, xMax, yMin, yMax, xMove, xMoveDir = 0;
			
			switch (i)
			{
				// display units shown at the upper left side
			case 0:	
					xMin = 0;
					yMin = 0;
					xMax = 11;
					yMax = 4;
					xMove = 1;
					break;
				// display wall/units shown at the upper wall area/right upper side
			case 1:
					xMin = 12;
					yMin = 0;
					xMax = 16;
					yMax = 4;
					xMove = 0;
					break;
				// display units shown at the lower wall area/right lower side
			case 2:
					xMin = 10;
					yMin = 5;
					xMax = 16;
					yMax = 10;
					xMove = 0;
					xMoveDir = 1;
					break;
				// display units shown at the left lower side
			case 3:
					xMin = 0;
					yMin = 5;
					xMax = 9;
					yMax = 10;
					xMove = 1;
					xMoveDir = 1;
					break;
			}

			int runNum = 0;
			for (int j = yMin; j <= yMax; j++)
			{
				if (runNum > 0)
				{
					if (xMin == xMax)
						xMax = xMin = ((runNum % 2) == 0) ? (xMin + (xMoveDir == 0 ? -1 : 1)) : xMin; 
					else if (xMove == 1)
						xMax = ((runNum % 2) == 0) ? (xMax + (xMoveDir == 0 ? -1 : 1)) : xMax; 
					else if (xMove == 0)
						xMin = ((runNum % 2) == 0) ? (xMin + (xMoveDir == 0 ? -1 : 1)) : xMin; 
				}

				for (int k = xMin; k <= xMax; k++)
				{
					int hex = j * 17 + k;
					showAliveStacks(stackAliveByHex, hex, &flyingStacks, to);
					showObstacles(&hexToObstacle, obstacles, hex, to);
					showPieceOfWall(to, hex, stacks);
				}

				++runNum;
			}
		}
	}
	
	for(int b=0; b<flyingStacks.size(); ++b) //showing flying stacks
		showAliveStack(flyingStacks[b], to);

	//units shown

	// Show projectiles
	projectileShowHelper(to);

	//showing spell effects
	if(battleEffects.size())
	{
		for(std::list<SBattleEffect>::iterator it = battleEffects.begin(); it!=battleEffects.end(); ++it)
		{
			SDL_Surface * bitmapToBlit = it->anim->ourImages[(it->frame)%it->anim->ourImages.size()].bitmap;
			SDL_Rect temp_rect = genRect(bitmapToBlit->h, bitmapToBlit->w, it->x, it->y);
			SDL_BlitSurface(bitmapToBlit, NULL, to, &temp_rect);
		}
	}

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	//showing menu background and console
	blitAt(menu, pos.x, 556 + pos.y, to);
	
	if(tacticsMode)
	{
		btactNext->showAll(to);
		btactEnd->showAll(to);
	}
	else
	{
		console->showAll(to);
		bConsoleUp->showAll(to);
		bConsoleDown->showAll(to);
	}

	//showing buttons
	bOptions->showAll(to);
	bSurrender->showAll(to);
	bFlee->showAll(to);
	bAutofight->showAll(to);
	bSpell->showAll(to);
	bWait->showAll(to);
	bDefence->showAll(to);

	//showing window with result of battle
	if(resWindow)
	{
		resWindow->show(to);
	}

	//showing in-game console
	LOCPLINT->cingconsole->show(to);

	Rect posWithQueue = Rect(pos.x, pos.y, 800, 600);

	if(curInt->sysOpts.showQueue)
	{
		if(!queue->embedded)
		{
			posWithQueue.y -= queue->pos.h;
			posWithQueue.h += queue->pos.h;
		}

		//showing queue
		if(!bresult)
			queue->showAll(to);
		else
			queue->blitBg(to); //blit only background, stacks are deleted
	}

	//printing border around interface
	if(screen->w != 800 || screen->h !=600)
	{
		CMessage::drawBorder(curInt->playerID,to,posWithQueue.w + 28, posWithQueue.h + 28, posWithQueue.x-14, posWithQueue.y-15);
	}
}

void CBattleInterface::showAliveStacks(std::vector<const CStack *> *aliveStacks, int hex, std::vector<const CStack *> *flyingStacks, SDL_Surface *to)
{
	//showing hero animations
	if (hex == 0)
		if(attackingHero)
			attackingHero->show(to);

	if (hex == 16)
		if(defendingHero)
			defendingHero->show(to);
	
	for(int v = 0; v < aliveStacks[hex].size(); ++v)
	{
		const CStack *s = aliveStacks[hex][v];

		if(!s->hasBonusOfType(Bonus::FLYING) || creAnims[s->ID]->getType() != 0)
			showAliveStack(s, to);
		else
			flyingStacks->push_back(s);
	}
}

void CBattleInterface::showObstacles(std::multimap<THex, int> *hexToObstacle, std::vector<CObstacleInstance> &obstacles, int hex, SDL_Surface *to)
{
	std::pair<std::multimap<THex, int>::const_iterator, std::multimap<THex, int>::const_iterator> obstRange =
		hexToObstacle->equal_range(hex);

	for(std::multimap<THex, int>::const_iterator it = obstRange.first; it != obstRange.second; ++it)
	{
		CObstacleInstance & curOb = obstacles[it->second];
		std::pair<si16, si16> shift = CGI->heroh->obstacles.find(curOb.ID)->second.posShift;
		int x = ((curOb.pos/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(curOb.pos%BFIELD_WIDTH) + pos.x + shift.first;
		int y = 86 + 42 * (curOb.pos/BFIELD_WIDTH) + pos.y + shift.second;
		std::vector<Cimage> &images = idToObstacle[curOb.ID]->ourImages; //reference to animation of obstacle
		blitAt(images[((animCount+1)/(4/curInt->sysOpts.animSpeed))%images.size()].bitmap, x, y, to);
	}
}

void CBattleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_q && key.state == SDL_PRESSED)
	{
		if(curInt->sysOpts.showQueue) //hide queue
			hideQueue();
		else
			showQueue();

		curInt->sysOpts.settingsChanged();
	}
	else if(key.keysym.sym == SDLK_ESCAPE && spellDestSelectMode)
	{
		endCastingSpell();
	}
}
void CBattleInterface::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	if(activeStack && !spellDestSelectMode)
	{
        int lastMouseHoveredStack = mouseHoveredStack;
		bool stackCastsSpell;
		mouseHoveredStack = -1;
		int myNumber = -1; //number of hovered tile
		for(int g = 0; g < BFIELD_SIZE; ++g)
		{
			if(bfield[g].hovered && bfield[g].strictHovered)
			{
				myNumber = g;
				break;
			}
		}
		if(myNumber == -1)
		{
			CCS->curh->changeGraphic(1, 6);
			if(console->whoSetAlter == 0)
			{
				console->alterTxt = "";
			}
		}
		else //battlefield hex
		{
            if(!vstd::contains(occupyableHexes, myNumber) || activeStack->coversPos(myNumber))
			{
				const CStack *shere = curInt->cb->battleGetStackByPos(myNumber, false);
				const CStack *sactive = activeStack;
				if(shere)
				{
					bool ourStack = shere->owner == curInt->playerID;
					//determine if creature spell is going to be cast
					stackCastsSpell = false;
					if (stackCanCastSpell && spellSelMode > STACK_SPELL_CANCELLED) //player did not decide to cancel this spell
					{
						if ((int)creatureSpellToCast > -1) //use randomized spell (Faerie Dragon), or only avaliable spell (Archangel)
						{
							const CSpell * spell =  CGI->spellh->spells[creatureSpellToCast];
							if (curInt->cb->battleCanCastThisSpell(spell, THex(myNumber)) == SpellCasting::OK)
							{
								if (spell->positiveness > -1 && ourStack || spell->positiveness < 1 && !ourStack)
								{
									CCS->curh->changeGraphic(3, 0);
									stackCastsSpell = true;
								}
							}
						}
						else if (ourStack) //must have only random positive spell (genie)
						{
							if (shere != sactive) //can't cast on itself
							{
								int spellID = curInt->cb->battleGetRandomStackSpell(shere, CBattleInfoCallback::RANDOM_GENIE);
								if (spellID > -1) //can cast any spell on target stack
								{
									CCS->curh->changeGraphic(3, 0);
									stackCastsSpell = true;
								}
							}
						}
					}

					if(ourStack) //our stack
					{
						if (shere->alive())
						{
							if (!stackCastsSpell) //use other abilities or display info
							{
								if(sactive->hasBonusOfType(Bonus::HEALER))
								{
									//display the possibility to heal this creature
									CCS->curh->changeGraphic(1, 17);
								}
								else
								{
									//info about creature
									CCS->curh->changeGraphic(1,5);
								}
								//setting console text
								char buf[500];
								sprintf(buf, CGI->generaltexth->allTexts[297].c_str(), shere->count == 1 ? shere->getCreature()->nameSing.c_str() : shere->getCreature()->namePl.c_str());
								console->alterTxt = buf;
								console->whoSetAlter = 0;
								const time_t curTime = time(NULL);
								if (shere->ID != lastMouseHoveredStack &&
								   curTime > lastMouseHoveredStackAnimationTime + HOVER_ANIM_DELTA &&
								   creAnims[shere->ID]->getType() == CCreatureAnim::HOLDING &&
								   creAnims[shere->ID]->framesInGroup(CCreatureAnim::MOUSEON) > 0)
								{
									creAnims[shere->ID]->playOnce(CCreatureAnim::MOUSEON);
									lastMouseHoveredStackAnimationTime = curTime;
								}
							}
						} //end of alive
						else if (sactive->hasBonusOfType(Bonus::DAEMON_SUMMONING) && sactive->casts)
						{
							CCS->curh->changeGraphic(3, 0);
						}
						mouseHoveredStack = shere->ID; //for dead also?
					}
					//end of our stack
					else if (!stackCastsSpell) //if not, then try attack
					{
						if (curInt->cb->battleCanShoot(activeStack,myNumber)) //we can shoot enemy
						{
							if(curInt->cb->battleHasDistancePenalty(activeStack, myNumber) ||
								curInt->cb->battleHasWallPenalty(activeStack, myNumber))
							{
								CCS->curh->changeGraphic(1,15);
							}
							else
							{
								CCS->curh->changeGraphic(1,3);
							}
							//setting console text
							char buf[500];
							//calculating estimated dmg
							std::pair<ui32, ui32> estimatedDmg = curInt->cb->battleEstimateDamage(sactive, shere);
							std::ostringstream estDmg;
							estDmg << estimatedDmg.first << " - " << estimatedDmg.second;
							//printing
							sprintf(buf, CGI->generaltexth->allTexts[296].c_str(), shere->count == 1 ? shere->getCreature()->nameSing.c_str() : shere->getCreature()->namePl.c_str(),
								sactive->shots, estDmg.str().c_str());
							console->alterTxt = buf;
							console->whoSetAlter = 0;
						}
						else if (isTileAttackable(myNumber)) //available enemy (melee attackable)
						{
							//handle direction of cursor and attackable tile
							setBattleCursor(myNumber);

							//setting console info
							char buf[500];
							//calculating estimated dmg
							std::pair<ui32, ui32> estimatedDmg = curInt->cb->battleEstimateDamage(sactive, shere);
							std::ostringstream estDmg;
							estDmg << estimatedDmg.first << " - " << estimatedDmg.second;
							//printing
							sprintf(buf, CGI->generaltexth->allTexts[36].c_str(), shere->count == 1 ? shere->getCreature()->nameSing.c_str() : shere->getCreature()->namePl.c_str(),
								estDmg.str().c_str());
							console->alterTxt = buf;
							console->whoSetAlter = 0;
						}
						else //unavailable enemy
						{
							CCS->curh->changeGraphic(1,0);
							console->alterTxt = "";
							console->whoSetAlter = 0;
						}
					}
				} //end of stack
				//TODO: allow aiming for creature spells
				else if (sactive && sactive->hasBonusOfType(Bonus::CATAPULT) && isCatapultAttackable(myNumber)) //catapulting
				{
					CCS->curh->changeGraphic(1,16);
					console->alterTxt = "";
					console->whoSetAlter = 0;
				}
				else //empty unavailable tile
				{
					CCS->curh->changeGraphic(1,0);
					console->alterTxt = "";
					console->whoSetAlter = 0;
				}
			}
			else //available tile
			{
				//setting console text and cursor
				const CStack *sactive = activeStack;
				if(sactive) //there can be a moment when stack is dead ut next is not yet activated
				{
					char buf[500];
					if(sactive->hasBonusOfType(Bonus::FLYING))
					{
						CCS->curh->changeGraphic(1,2);
						sprintf(buf, CGI->generaltexth->allTexts[295].c_str(), sactive->count == 1 ? sactive->getCreature()->nameSing.c_str() : sactive->getCreature()->namePl.c_str());
					}
					else
					{
						CCS->curh->changeGraphic(1,1);
						sprintf(buf, CGI->generaltexth->allTexts[294].c_str(), sactive->count == 1 ? sactive->getCreature()->nameSing.c_str() : sactive->getCreature()->namePl.c_str());
					}

					console->alterTxt = buf;
					console->whoSetAlter = 0;
				}
			}
		}
	}
	else if(spellDestSelectMode)
	{
		int myNumber = -1; //number of hovered tile
		for(int g=0; g<BFIELD_SIZE; ++g)
		{
			if(bfield[g].hovered && bfield[g].strictHovered)
			{
				myNumber = g;
				break;
			}
		}
		if(myNumber == -1)
		{
			CCS->curh->changeGraphic(1, 0);
			//setting console text
			console->alterTxt = CGI->generaltexth->allTexts[23];
			console->whoSetAlter = 0;
		}
		else
		{
			//get dead stack if we cast resurrection or animate dead
			const CStack * stackUnder = curInt->cb->battleGetStackByPos(myNumber, spellToCast->additionalInfo != 38 && spellToCast->additionalInfo != 39);

			if(stackUnder && spellToCast->additionalInfo == 39 && !stackUnder->hasBonusOfType(Bonus::UNDEAD)) //animate dead can be cast only on undead creatures
				stackUnder = NULL;

			bool whichCase; //for cases 1, 2 and 3
			switch(spellSelMode)
			{
			case 1:
				whichCase = stackUnder && curInt->playerID == stackUnder->owner;
				break;
			case 2:
				whichCase = stackUnder && curInt->playerID != stackUnder->owner;
				break;
			case 3:
				whichCase = stackUnder;
				break;
			}

			switch(spellSelMode)
			{
			case 0:
				CCS->curh->changeGraphic(3, 0);
				//setting console text
				char buf[500];
				sprintf(buf, CGI->generaltexth->allTexts[26].c_str(), CGI->spellh->spells[spellToCast->additionalInfo]->name.c_str());
				console->alterTxt = buf;
				console->whoSetAlter = 0;
				break;
			case 1: case 2: case 3:
				if( whichCase )
				{
					CCS->curh->changeGraphic(3, 0);
					//setting console text
					char buf[500];
					std::string creName = stackUnder->count > 1 ? stackUnder->getCreature()->namePl : stackUnder->getCreature()->nameSing;
						sprintf(buf, CGI->generaltexth->allTexts[27].c_str(), CGI->spellh->spells[spellToCast->additionalInfo]->name.c_str(), creName.c_str());
					console->alterTxt = buf;
					console->whoSetAlter = 0;
					break;
				}
				else
				{
					CCS->curh->changeGraphic(1, 0);
					//setting console text
					console->alterTxt = CGI->generaltexth->allTexts[23];
					console->whoSetAlter = 0;
				}
				break;
			case 4: //TODO: implement this case
				if( blockedByObstacle(myNumber) )
				{
					CCS->curh->changeGraphic(3, 0);
				}
				else
				{
					CCS->curh->changeGraphic(1, 0);
				}
				break;
			}
		}
	}
}

void CBattleInterface::setBattleCursor(const int myNumber)
{
	const CBattleHex & hoveredHex = bfield[myNumber];
	CCursorHandler *cursor = CCS->curh;

	const double subdividingAngle = 2.0*M_PI/6.0; // Divide a hex into six sectors.
	const double hexMidX = hoveredHex.pos.x + hoveredHex.pos.w/2;
	const double hexMidY = hoveredHex.pos.y + hoveredHex.pos.h/2;
	const double cursorHexAngle = M_PI - atan2(hexMidY - cursor->ypos, cursor->xpos - hexMidX) + subdividingAngle/2; //TODO: refactor this nightmare
	const double sector = fmod(cursorHexAngle/subdividingAngle, 6.0);
	const int zigzagCorrection = !((myNumber/BFIELD_WIDTH)%2); // Off-by-one correction needed to deal with the odd battlefield rows.

	std::vector<int> sectorCursor; // From left to bottom left.
	sectorCursor.push_back(8);
	sectorCursor.push_back(9);
	sectorCursor.push_back(10);
	sectorCursor.push_back(11);
	sectorCursor.push_back(12);
	sectorCursor.push_back(7);

	const bool doubleWide = activeStack->doubleWide();
	bool aboveAttackable = true, belowAttackable = true;

	// Exclude directions which cannot be attacked from.
	// Check to the left.
	if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - 1)) 
	{
		sectorCursor[0] = -1;
	}
	// Check top left, top right as well as above for 2-hex creatures.
	if (myNumber/BFIELD_WIDTH == 0) 
	{
			sectorCursor[1] = -1;
			sectorCursor[2] = -1;
			aboveAttackable = false;
	} 
	else 
	{
		if (doubleWide) 
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[1] = -1;
			if (!(attackRow[1] && attackRow[2]))
				aboveAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[2] = -1;
		}
		else
		{
			if (!vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[1] = -1;
			if (!vstd::contains(occupyableHexes, myNumber - BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[2] = -1;
		}
	}
	// Check to the right.
	if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + 1))
	{
		sectorCursor[3] = -1;
	}
	// Check bottom right, bottom left as well as below for 2-hex creatures.
	if (myNumber/BFIELD_WIDTH == BFIELD_HEIGHT - 1)
	{
		sectorCursor[4] = -1;
		sectorCursor[5] = -1;
		belowAttackable = false;
	} 
	else 
	{
		if (doubleWide)
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[5] = -1;
			if (!(attackRow[1] && attackRow[2]))
				belowAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[4] = -1;
		} 
		else 
		{
			if (!vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[4] = -1;
			if (!vstd::contains(occupyableHexes, myNumber + BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[5] = -1;
		}
	}

	// Determine index from sector.
	int cursorIndex;
	if (doubleWide) 
	{
		sectorCursor.insert(sectorCursor.begin() + 5, belowAttackable ? 13 : -1);
		sectorCursor.insert(sectorCursor.begin() + 2, aboveAttackable ? 14 : -1);

		if (sector < 1.5)
			cursorIndex = sector;
		else if (sector >= 1.5 && sector < 2.5)
			cursorIndex = 2;
		else if (sector >= 2.5 && sector < 4.5)
			cursorIndex = (int) sector + 1;
		else if (sector >= 4.5 && sector < 5.5)
			cursorIndex = 6;
		else
			cursorIndex = (int) sector + 2;
	} 
	else 
	{
		cursorIndex = sector;
	}

	// Find the closest direction attackable, starting with the right one.
	// FIXME: Is this really how the original H3 client does it?
	int i = 0;
	while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1) //Why hast thou forsaken me?
		i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
	int index = (cursorIndex + i)%sectorCursor.size(); //hopefully we get elements from sectorCursor
	cursor->changeGraphic(1, sectorCursor[index]);
	switch (index)
	{
		case 0:
			attackingHex = myNumber - 1; //left
			break;
		case 1:
			attackingHex = myNumber - BFIELD_WIDTH - 1 + zigzagCorrection; //top left
			break;
		case 2:
			attackingHex = myNumber - BFIELD_WIDTH + zigzagCorrection; //top right
			break;
		case 3:
			break;
			attackingHex = myNumber + 1; //right
		case 4:
			break;
			attackingHex = myNumber + BFIELD_WIDTH + zigzagCorrection; //bottom right
		case 5:
			attackingHex = myNumber + BFIELD_WIDTH - 1 + zigzagCorrection; //bottom left
			break;
	}
	THex hex(attackingHex);
	if (!hex.isValid())
		attackingHex = -1;
}

void CBattleInterface::clickRight(tribool down, bool previousState)
{
	if(!down && spellDestSelectMode)
	{
		endCastingSpell();
	}
}

void CBattleInterface::bOptionsf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	CCS->curh->changeGraphic(0,0);

	Rect tempRect = genRect(431, 481, 160, 84);
	tempRect += pos.topLeft();
	CBattleOptionsWindow * optionsWin = new CBattleOptionsWindow(tempRect, this);
	GH.pushInt(optionsWin);
}

void CBattleInterface::bSurrenderf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	int cost = curInt->cb->battleGetSurrenderCost();
	if(cost >= 0)
	{
		const CGHeroInstance *opponent = curInt->cb->battleGetFightingHero(1);
		std::string enemyHeroName = opponent ? opponent->name : "#ENEMY#"; //TODO: should surrendering without enemy hero be enabled? 
		std::string surrenderMessage = boost::str(boost::format(CGI->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		curInt->showYesNoDialog(surrenderMessage, std::vector<SComponent*>(), boost::bind(&CBattleInterface::reallySurrender,this), 0, false);
	}
}

void CBattleInterface::bFleef()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if( curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = boost::bind(&CBattleInterface::reallyFlee,this);
		curInt->showYesNoDialog(CGI->generaltexth->allTexts[28],std::vector<SComponent*>(), ony, 0, false); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<SComponent*> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if(attackingHeroInstance)
			if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor())
				heroName = attackingHeroInstance->name;
		if(defendingHeroInstance)
			if(defendingHeroInstance->tempOwner == curInt->cb->getMyColor())
				heroName = defendingHeroInstance->name;
		//calculating text
		char buffer[1000];
		sprintf(buffer, CGI->generaltexth->allTexts[340].c_str(), heroName.c_str()); //The Shackles of War are present.  %s can not retreat!

		//printing message
		curInt->showInfoDialog(std::string(buffer), comps);
	}
}

void CBattleInterface::reallyFlee()
{
	giveCommand(BattleAction::RETREAT,0,0);
	CCS->curh->changeGraphic(0, 0);
}

void CBattleInterface::reallySurrender()
{
	if(curInt->cb->getResourceAmount(Res::GOLD) < curInt->cb->battleGetSurrenderCost())
	{
		curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		giveCommand(BattleAction::SURRENDER,0,0);
		CCS->curh->changeGraphic(0, 0);
	}
}

void CBattleInterface::bAutofightf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;
}

void CBattleInterface::bSpellf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	CCS->curh->changeGraphic(0,0);

	const CGHeroInstance * chi = NULL;
	if(attackingHeroInstance->tempOwner == curInt->playerID)
		chi = attackingHeroInstance;
	else
		chi = defendingHeroInstance;
	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), chi, curInt);
	GH.pushInt(spellWindow);
}

void CBattleInterface::bWaitf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if(activeStack != NULL)
		giveCommand(8,0,activeStack->ID);
}

void CBattleInterface::bDefencef()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if(activeStack != NULL)
		giveCommand(3,0,activeStack->ID);
}

void CBattleInterface::bConsoleUpf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	console->scrollUp();
}

void CBattleInterface::bConsoleDownf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	console->scrollDown();
}

void CBattleInterface::newStack(const CStack * stack)
{
	Point coords = CBattleHex::getXYUnitAnim(stack->position, stack->owner == attackingHeroInstance->tempOwner, stack, this);;

	if(stack->position < 0) //turret
	{
		const CCreature & turretCreature = *CGI->creh->creatures[ CGI->creh->factionToTurretCreature[siegeH->town->town->typeID] ];
		creAnims[stack->ID] = new CCreatureAnimation(turretCreature.animDefName);	

		// Turret positions are read out of the /config/wall_pos.txt
		int posID = 0;
		switch (stack->position)
		{
		case -2: // keep creature
			posID = 18;
			break;
		case -3: // bottom creature
			posID = 19;
			break;
		case -4: // upper creature
			posID = 20;
			break;
		}

		if (posID != 0)
		{
			coords.x = graphics->wallPositions[siegeH->town->town->typeID][posID - 1].x + this->pos.x;
			coords.y = graphics->wallPositions[siegeH->town->town->typeID][posID - 1].y + this->pos.y;
		}
	}
	else
	{
		creAnims[stack->ID] = new CCreatureAnimation(stack->getCreature()->animDefName);	
	}
	creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);
	creAnims[stack->ID]->pos = Rect(coords.x, coords.y, creAnims[stack->ID]->fullWidth, creAnims[stack->ID]->fullHeight);
	creDir[stack->ID] = stack->attackerOwned;
}

void CBattleInterface::stackRemoved(int stackID)
{
	delete creAnims[stackID];
	creAnims.erase(stackID);
	creDir.erase(stackID);
}

void CBattleInterface::stackActivated(const CStack * stack) //TODO: check it all before game state is changed due to abilities
{
	//don't show animation when no HP is regenerated
	if (stack->firstHPleft != stack->MaxHealth())
	{
		if( stack->hasBonusOfType(Bonus::HP_REGENERATION) || stack->hasBonusOfType(Bonus::FULL_HP_REGENERATION, 1))
		{
			displayEffect(74, stack->position);
			CCS->soundh->playSound(soundBase::REGENER);
		}
		if( stack->hasBonusOfType(Bonus::FULL_HP_REGENERATION, 0))
		{
			displayEffect(74, stack->position);
			CCS->soundh->playSound(soundBase::REGENER);
		}
	}

	if(stack->hasBonusOfType(Bonus::MANA_DRAIN))
	{
		CGHeroInstance * enemy = NULL; //probably could be smarter and not duplicated
		if (defendingHero)
			if (defendingHero->myHero->tempOwner != stack->owner)
				enemy = const_cast<CGHeroInstance *>(defendingHero->myHero);
		if (attackingHero)
			if (attackingHero->myHero->tempOwner != stack->owner)
				enemy = const_cast<CGHeroInstance *>(attackingHero->myHero);
		if (enemy)
		{
			ui32 manaDrained = stack->valOfBonuses(Bonus::MANA_DRAIN);
			amin (manaDrained, enemy->mana);
			if (manaDrained)
			{
				displayEffect(77, stack->position);
				CCS->soundh->playSound(soundBase::MANADRAI);
			}
		}
	}
	if(stack->hasBonusOfType(Bonus::POISON))
	{
		displayEffect(67, stack->position);
		CCS->soundh->playSound(soundBase::POISON);
	}

	//givenCommand = NULL;
	stackToActivate = stack;
	if(pendingAnims.size() == 0)
		activateStack();
}

void CBattleInterface::stackMoved(const CStack * stack, std::vector<THex> destHex, int distance)
{
	addNewAnim(new CBattleStackMoved(this, stack, destHex, distance));
	waitForAnims();
}

void CBattleInterface::stacksAreAttacked(std::vector<SStackAttackedInfo> attackedInfos)
{
	for(int h = 0; h < attackedInfos.size(); ++h)
	{
		addNewAnim(new CDefenceAnim(attackedInfos[h], this));
		if (attackedInfos[h].rebirth)
		{
			displayEffect(50, attackedInfos[h].defender->position); //TODO: play reverse death animation
			CCS->soundh->playSound(soundBase::RESURECT);
		}
	}
	waitForAnims();
	int targets = 0, killed = 0, damage = 0;
	for(int h = 0; h < attackedInfos.size(); ++h)
	{
		++targets;
		killed += attackedInfos[h].killed;
		damage += attackedInfos[h].dmg;
	}
	if (targets > 1)
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, true); //creatures perish
	else
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, false);

	for(int h = 0; h < attackedInfos.size(); ++h)
	{
		if (attackedInfos[h].rebirth)
			creAnims[attackedInfos[h].defender->ID]->setType(CCreatureAnim::HOLDING);
	}
}

void CBattleInterface::stackAttacking( const CStack * attacker, THex dest, const CStack * attacked, bool shooting )
{
	if (shooting)
	{
		addNewAnim(new CShootingAnim(this, attacker, dest, attacked));
	}
	else
	{
		addNewAnim(new CMeleeAttack(this, attacker, dest, attacked));
	}
	waitForAnims();
}

void CBattleInterface::newRoundFirst( int round )
{
	//handle regeneration
	std::vector<const CStack*> stacks = curInt->cb->battleGetStacks(); //gets only alive stacks
//	BOOST_FOREACH(const CStack *s, stacks)
//	{
//	}
	waitForAnims();
}

void CBattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);

	//unlock spellbook
	//bSpell->block(!curInt->cb->battleCanCastSpell());
	//don't unlock spellbook - this should be done when we have axctive creature

	
}

void CBattleInterface::giveCommand(ui8 action, THex tile, ui32 stack, si32 additional)
{
	if(!curInt->cb->battleGetStackByID(stack) && action != 1 && action != 4 && action != 5)
	{
		return;
	}
	BattleAction * ba = new BattleAction(); //is deleted in CPlayerInterface::activeStack()
	ba->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
	ba->actionType = action;
	ba->destinationTile = tile;
	ba->stackNumber = stack;
	ba->additionalInfo = additional;

	//some basic validations
	switch(action)
	{
		case BattleAction::WALK_AND_ATTACK:
			assert(curInt->cb->battleGetStackByPos(additional)); //stack to attack must exist
		case BattleAction::WALK:
		case BattleAction::SHOOT:
		case BattleAction::CATAPULT:
			assert(tile < BFIELD_SIZE);
			break;
	}

	if(!tacticsMode)
	{
		myTurn = false;
		activeStack = NULL;
		givenCommand->setn(ba);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(ba);
		delNull(ba);
		bTacticNextStack();
	}
}

bool CBattleInterface::isTileAttackable(const THex & number) const
{
	for(size_t b=0; b<occupyableHexes.size(); ++b)
	{
		if(THex::mutualPosition(occupyableHexes[b], number) != -1 || occupyableHexes[b] == number)
			return true;
	}
	return false;
}

bool CBattleInterface::blockedByObstacle(THex hex) const
{
	std::vector<CObstacleInstance> obstacles = curInt->cb->battleGetAllObstacles();
	std::set<THex> coveredHexes;
	for(int b = 0; b < obstacles.size(); ++b)
	{
		std::vector<THex> blocked = CGI->heroh->obstacles.find(obstacles[b].ID)->second.getBlocked(obstacles[b].pos);
		for(int w = 0; w < blocked.size(); ++w)
			coveredHexes.insert(blocked[w]);
	}
	return vstd::contains(coveredHexes, hex);
}

bool CBattleInterface::isCatapultAttackable(THex hex) const
{
	if(!siegeH)
		return false;

	int wallUnder = curInt->cb->battleGetWallUnderHex(hex);
	if(wallUnder == -1)
		return false;

	return curInt->cb->battleGetWallState(wallUnder) < 3;
}

const CGHeroInstance * CBattleInterface::getActiveHero()
{
	const CStack * attacker = activeStack;
	if (!attacker)
	{
		return NULL;
	}

	if (attacker->attackerOwned)
	{
		return attackingHeroInstance;
	}
	
	return defendingHeroInstance;
}

void CBattleInterface::hexLclicked(int whichOne)
{
	const CStack * actSt = activeStack;
	const CStack* dest = curInt->cb->battleGetStackByPos(whichOne, false); //creature at destination tile; -1 if there is no one
	if(!actSt)
	{
		tlog3 << "Hex l-clicked when no active stack!\n";
		return;
	}

	if( ((whichOne%BFIELD_WIDTH)!=0 && (whichOne%BFIELD_WIDTH)!=(BFIELD_WIDTH-1)) //if player is trying to attack enemey unit or move creature stack
		|| (actSt->hasBonusOfType(Bonus::CATAPULT) && !spellDestSelectMode || dest ) //enemy's first aid tent can stand there and we want to shoot it
		)
	{
		if(!myTurn)
			return; //we are not permit to do anything
		if(spellDestSelectMode) //TODO: choose target for area creature spell
		{
			//checking destination
			bool allowCasting = true;
			bool onlyAlive = vstd::contains(CGI->spellh->risingSpells, spellToCast->additionalInfo); //when casting resurrection or animate dead we should be allow to select dead stack
			//TODO: more general handling of dead targets
			switch(spellSelMode)
			{
			case FRIENDLY_CREATURE:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive) || curInt->playerID != dest->owner )
					allowCasting = false;
				break;
			case HOSTILE_CREATURE:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive) || curInt->playerID == dest->owner )
					allowCasting = false;
				break;
			case ANY_CREATURE:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive))
					allowCasting = false;
				break;
			case OBSTACLE:
				if(!blockedByObstacle(whichOne))
					allowCasting = false;
			case TELEPORT: //teleport
				const CSpell *s = CGI->spellh->spells[spellToCast->additionalInfo];
				ui8 skill = getActiveHero()->getSpellSchoolLevel(s); //skill level
				if (!curInt->cb->battleCanTeleportTo(activeStack, whichOne, skill))
				{
					allowCasting = false;
				}
				break;
			}
			//destination checked
			if(allowCasting)
			{
				spellToCast->destinationTile = whichOne;
				curInt->cb->battleMakeAction(spellToCast);
				endCastingSpell();
			}
		}
		else //we don't aim for spell target
		{
			bool walkableTile = false;
			if (dest)
			{
				if (dest->alive())
				{
					if(dest->owner != actSt->owner && curInt->cb->battleCanShoot(activeStack, whichOne)) //shooting
					{
						CCS->curh->changeGraphic(1, 6); //cursor should be changed
						giveCommand (BattleAction::SHOOT, whichOne, activeStack->ID);
					}
					else if(dest->owner != actSt->owner) //attacking
					{
						const CStack * actStack = activeStack;
						int attackFromHex = -1; //hex from which we will attack chosen stack
						switch(CCS->curh->number)
						{
						case 12: //from bottom right
							{
								bool doubleWide = actStack->doubleWide();
								int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH+1 ) +
									(actStack->attackerOwned && doubleWide ? 1 : 0);
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(actStack->attackerOwned) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						case 7: //from bottom left
							{
								int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH-1 : BFIELD_WIDTH );
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(actStack->attackerOwned) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						case 8: //from left
							{
								if(actStack->doubleWide() && !actStack->attackerOwned)
								{
									std::vector<THex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
									if(vstd::contains(acc, whichOne))
										attackFromHex = whichOne - 1;
									else
										attackFromHex = whichOne - 2;
								}
								else
								{
									attackFromHex = whichOne - 1;
								}
								break;
							}
						case 9: //from top left
							{
								int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH+1 : BFIELD_WIDTH );
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(actStack->attackerOwned) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						case 10: //from top right
							{
								bool doubleWide = actStack->doubleWide();
								int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 ) +
									(actStack->attackerOwned && doubleWide ? 1 : 0);
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(actStack->attackerOwned) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						case 11: //from right
							{
								if(actStack->doubleWide() && actStack->attackerOwned)
								{
									std::vector<THex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
									if(vstd::contains(acc, whichOne))
										attackFromHex = whichOne + 1;
									else
										attackFromHex = whichOne + 2;
								}
								else
								{
									attackFromHex = whichOne + 1;
								}
								break;
							}
						case 13: //from bottom
							{
								int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH+1 );
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						case 14: //from top
							{
								int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 );
								if(vstd::contains(occupyableHexes, destHex))
									attackFromHex = destHex;
								else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
								{
									if(vstd::contains(occupyableHexes, destHex+1))
										attackFromHex = destHex+1;
								}
								else //if we are defender
								{
									if(vstd::contains(occupyableHexes, destHex-1))
										attackFromHex = destHex-1;
								}
								break;
							}
						}

						if(attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
						{
							giveCommand(BattleAction::WALK_AND_ATTACK, attackFromHex, activeStack->ID, whichOne);
					
							CCS->curh->changeGraphic(1, 6); //cursor should be changed
						}

					}
					else if (actSt->hasBonusOfType(Bonus::HEALER) && actSt->owner == dest->owner) //friendly creature we can heal
					{ //TODO: spellDestSelectMode > -2 if we don't want to heal but perform some other (?) action
						giveCommand(BattleAction::STACK_HEAL, whichOne, activeStack->ID); //command healing

						CCS->curh->changeGraphic(1, 6); //cursor should be changed
					}

				} //stack is not alive
				else if (actSt->hasBonusOfType(Bonus::DAEMON_SUMMONING) && actSt->casts &&
						actSt->owner == dest->owner && spellSelMode > -2)//friendly body we can (and want) rise
				{
					giveCommand(BattleAction::DAEMON_SUMMONING, whichOne, activeStack->ID);

					CCS->curh->changeGraphic(1, 6); //cursor should be changed
				}
				else //not a subject of resurrection
					walkableTile = true;
			}
			else
			{
				walkableTile = true;
			}

			if (walkableTile) // we can try to move to this tile
			{
				if(std::find(occupyableHexes.begin(), occupyableHexes.end(), whichOne) != occupyableHexes.end())// and it's in our range
				{
					CCS->curh->changeGraphic(1, 6); //cursor should be changed
					if(activeStack->doubleWide())
					{
						std::vector<THex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
						int shiftedDest = whichOne + (activeStack->attackerOwned ? 1 : -1);
						if(vstd::contains(acc, whichOne))
							giveCommand (BattleAction::WALK ,whichOne, activeStack->ID);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand (BattleAction::WALK, shiftedDest, activeStack->ID);
					}
					else
					{
						giveCommand(BattleAction::WALK, whichOne, activeStack->ID);
					}
				}
				else if(actSt->hasBonusOfType(Bonus::CATAPULT) && isCatapultAttackable(whichOne)) //attacking (catapult)
				{
					giveCommand(BattleAction::CATAPULT, whichOne, activeStack->ID);
				}
			}
		}
	}
}

void CBattleInterface::stackIsCatapulting(const CatapultAttack & ca)
{
	for(std::set< std::pair< std::pair< ui8, si16 >, ui8> >::const_iterator it = ca.attackedParts.begin(); it != ca.attackedParts.end(); ++it)
	{
		const CStack * stack = curInt->cb->battleGetStackByID(ca.attacker);
		addNewAnim(new CShootingAnim(this, stack, it->first.second, NULL, true, it->second));

		SDL_FreeSurface(siegeH->walls[it->first.first + 2]);
		siegeH->walls[it->first.first + 2] = BitmapHandler::loadBitmap(
			siegeH->getSiegeName(it->first.first + 2, curInt->cb->battleGetWallState(it->first.first)) );
	}
	waitForAnims();
}

void CBattleInterface::battleFinished(const BattleResult& br)
{
	bresult = &br;
	LOCPLINT->pim->unlock();
	animsAreDisplayed.waitUntil(false);
	LOCPLINT->pim->lock();
	displayBattleFinished();
}

void CBattleInterface::displayBattleFinished()
{
	CCS->curh->changeGraphic(0,0);
	
	SDL_Rect temp_rect = genRect(561, 470, (screen->w - 800)/2 + 165, (screen->h - 600)/2 + 19);
	resWindow = new CBattleResultWindow(*bresult, temp_rect, this);
	GH.pushInt(resWindow);
}

void CBattleInterface::spellCast( const BattleSpellCast * sc )
{
	const CSpell &spell = *CGI->spellh->spells[sc->id];

	//spell opening battle is cast when no stack is active
	if(sc->castedByHero && ( activeStack == NULL || sc->side == !activeStack->attackerOwned) )
		bSpell->block(true);

	std::vector< std::string > anims; //for magic arrow and ice bolt

	if (vstd::contains(CCS->soundh->spellSounds, &spell))
		CCS->soundh->playSound(CCS->soundh->spellSounds[&spell]);

	switch(sc->id)
	{
	case 15: //magic arrow
		{
			//initialization of anims
			anims.push_back("C20SPX0.DEF"); anims.push_back("C20SPX1.DEF"); anims.push_back("C20SPX2.DEF"); anims.push_back("C20SPX3.DEF"); anims.push_back("C20SPX4.DEF");
		}
	case 16: //ice bolt
		{
			if(anims.size() == 0) //initialization of anims
			{
				anims.push_back("C08SPW0.DEF"); anims.push_back("C08SPW1.DEF"); anims.push_back("C08SPW2.DEF"); anims.push_back("C08SPW3.DEF"); anims.push_back("C08SPW4.DEF");
			}
		} //end of ice bolt only part
		{ //common ice bolt and magic arrow part
			//initial variables
			std::string animToDisplay;
			Point srccoord = (sc->side ? Point(770, 60) : Point(30, 60)) + pos;
			Point destcoord = CBattleHex::getXYUnitAnim(sc->tile, !sc->side, curInt->cb->battleGetStackByPos(sc->tile), this); //position attacked by arrow
			destcoord.x += 250; destcoord.y += 240;

			//animation angle
			float angle = atan2(float(destcoord.x - srccoord.x), float(destcoord.y - srccoord.y));
			bool Vflip = false;
			if (angle < 0)
			{
				Vflip = true;
				angle = -angle;
			}

			//choosing animation by angle
			if(angle > 1.50)
				animToDisplay = anims[0];
			else if(angle > 1.20)
				animToDisplay = anims[1];
			else if(angle > 0.90)
				animToDisplay = anims[2];
			else if(angle > 0.60)
				animToDisplay = anims[3];
			else
				animToDisplay = anims[4];

			//displaying animation
			CDefEssential * animDef = CDefHandler::giveDefEss(animToDisplay);
			
			int steps = sqrt((float)((destcoord.x - srccoord.x)*(destcoord.x - srccoord.x) + (destcoord.y - srccoord.y) * (destcoord.y - srccoord.y))) / 40;
			if(steps <= 0)
				steps = 1;

			int dx = (destcoord.x - srccoord.x - animDef->ourImages[0].bitmap->w)/steps, dy = (destcoord.y - srccoord.y - animDef->ourImages[0].bitmap->h)/steps;

			delete animDef;
			addNewAnim(new CSpellEffectAnim(this, animToDisplay, srccoord.x, srccoord.y, dx, dy, Vflip));

			break; //for 15 and 16 cases
		}
	case 17: //lightning bolt
	case 57: //Titan's Thunder
	case 77: //thunderbolt
		displayEffect(1, sc->tile);
		displayEffect(spell.mainEffectAnim, sc->tile);
		break;
	case 35: //dispel
	case 37: //cure
	case 38: //resurrection
	case 39: //animate dead
		for(std::set<ui32>::const_iterator it = sc->affectedCres.begin(); it != sc->affectedCres.end(); ++it)
		{
			displayEffect(spell.mainEffectAnim, curInt->cb->battleGetStackByID(*it, false)->position);
		}
		break;
	case 66: case 67: case 68: case 69: //summon elemental
		addNewAnim(new CDummyAnim(this, 2));
		break;
	} //switch(sc->id)

	//support for resistance
	for(int j=0; j<sc->resisted.size(); ++j)
	{
		int tile = curInt->cb->battleGetStackByID(sc->resisted[j])->position;
		displayEffect(78, tile);
	}

	//displaying message in console
	bool customSpell = false;
	bool plural = false; //add singular / plural form of creature text if this is true
	int textID = 0;
	if(sc->affectedCres.size() == 1)
	{
		std::string text = CGI->generaltexth->allTexts[195];
		if(sc->castedByHero)
		{
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetFightingHero(sc->side)->name);
			boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name); //spell name
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl ); //target
		}
		else
		{
			switch(sc->id)
			{
				case 70: //Stone Gaze
					customSpell = true;
					plural = true;
					textID = 558;
					break;
				case 71: //Poison
					customSpell = true;
					plural = true;
					textID = 561;
					break;
				case 72: //Bind
					customSpell = true;
					text = CGI->generaltexth->allTexts[560];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl );
					break;	//Roots and vines bind the %s to the ground!
				case 73: //Disease
					customSpell = true;
					plural = true;
					textID = 553;
					break;
				case 74: //Paralyze
					customSpell = true;
					plural = true;
					textID = 563;
					break;
				case 75: // Aging
				{
					customSpell = true;
					if (curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->count > 1)
					{
						text = CGI->generaltexth->allTexts[552];
						boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					}
					else
					{
						text = CGI->generaltexth->allTexts[551];
						boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->nameSing);
					}
					//The %s shrivel with age, and lose %d hit points."	
					TBonusListPtr bl = curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getBonuses(Selector::type(Bonus::STACK_HEALTH));
					bl->remove_if(Selector::source(Bonus::SPELL_EFFECT, 75));
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bl->totalValue()/2));
				}
					break;
				case 77: //Thunderbolt
					text = CGI->generaltexth->allTexts[367];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					console->addText(text);
					text = CGI->generaltexth->allTexts[343].substr(1, CGI->generaltexth->allTexts[343].size() - 1); //Does %d points of damage.
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay)); //no more text afterwards
					console->addText(text);
					customSpell = true;
					text = ""; //yeah, it's a terrible mess
					break;
				case 78: //Dispell helpful spells
					text = CGI->generaltexth->allTexts[555];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					customSpell = true;
					break;
				case 79: // Death Stare
					customSpell = true;
					if (sc->dmgToDisplay)
					{
						if (sc->dmgToDisplay > 1)
						{
							text = CGI->generaltexth->allTexts[119]; //%d %s die under the terrible gaze of the %s.
							boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay));
							boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl );
						}
						else
						{
							text = CGI->generaltexth->allTexts[118]; //One %s dies under the terrible gaze of the %s.
							boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->nameSing);
						}
						boost::algorithm::replace_first(text, "%s", CGI->creh->creatures[sc->attackerType]->namePl); //casting stack
					}
					else
						text = "";
					break;
				default:
					text = CGI->generaltexth->allTexts[565]; //The %s casts %s	
					boost::algorithm::replace_first(text, "%s", CGI->creh->creatures[sc->attackerType]->namePl); //casting stack
			}
			if (plural)
			{
				if (curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->count > 1)
				{
					text = CGI->generaltexth->allTexts[textID + 1];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
				}
				else
				{
					text = CGI->generaltexth->allTexts[textID];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->nameSing);
				}
			}
		}
		if (!customSpell && !sc->dmgToDisplay)
			boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name); //simple spell name
		if (text.size())
			console->addText(text);
	}
	else
	{
		std::string text = CGI->generaltexth->allTexts[196];
		if(sc->castedByHero)
		{
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetFightingHero(sc->side)->name);
		}
		else if(sc->attackerType < CGI->creh->creatures.size())
		{
			boost::algorithm::replace_first(text, "%s", CGI->creh->creatures[sc->attackerType]->namePl); //creature caster
		}
		else
		{
			//TODO artifacts that cast spell; scripts some day
			boost::algorithm::replace_first(text, "Something", CGI->creh->creatures[sc->attackerType]->namePl); //creature caster
		}
		boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name);
		console->addText(text);
	}
	if(sc->dmgToDisplay && !customSpell)
	{
		std::string dmgInfo = CGI->generaltexth->allTexts[376];
		boost::algorithm::replace_first(dmgInfo, "%s", CGI->spellh->spells[sc->id]->name); //simple spell name
		boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay));
		console->addText(dmgInfo); //todo: casualties (?)
	}
	waitForAnims();
}

void CBattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	int effID = sse.effect.back().sid;
	if(effID != -1) //can be -1 for defensive stance effect
	{
		for(std::vector<ui32>::const_iterator ci = sse.stacks.begin(); ci!=sse.stacks.end(); ++ci)
		{
			displayEffect(CGI->spellh->spells[effID]->mainEffectAnim, curInt->cb->battleGetStackByID(*ci)->position);
		}
	}
	else if (sse.stacks.size() == 1 && sse.effect.size() == 2)
	{
		const Bonus & bns = sse.effect.front();
		if (bns.source == Bonus::OTHER && bns.type == Bonus::PRIMARY_SKILL)
		{
			//defensive stance
			const CStack * stack = LOCPLINT->cb->battleGetStackByID(*sse.stacks.begin());
			int txtid = 120;

			if(stack->count != 1)
				txtid++; //move to plural text

			char txt[4000];
			BonusList defenseBonuses = *(stack->getBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE)));
			defenseBonuses.remove_if(Selector::durationType(Bonus::STACK_GETS_TURN)); //remove bonuses gained from defensive stance
			int val = stack->Defense() - defenseBonuses.totalValue();
			sprintf(txt, CGI->generaltexth->allTexts[txtid].c_str(),  (stack->count != 1) ? stack->getCreature()->namePl.c_str() : stack->getCreature()->nameSing.c_str(), val);
			console->addText(txt);
		}
		
	}


	if (activeStack != NULL) //it can be -1 when a creature casts effect
	{
		redrawBackgroundWithHexes(activeStack);
	}
}

void CBattleInterface::castThisSpell(int spellID)
{
	BattleAction * ba = new BattleAction;
	ba->actionType = BattleAction::HERO_SPELL;
	ba->additionalInfo = spellID; //spell number
	ba->destinationTile = -1;
	ba->stackNumber = (attackingHeroInstance->tempOwner == curInt->playerID) ? -1 : -2;
	ba->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
	spellToCast = ba;
	spellDestSelectMode = true;

	//choosing possible tragets
	const CGHeroInstance * castingHero = (attackingHeroInstance->tempOwner == curInt->playerID) ? attackingHeroInstance : defendingHeroInstance;
	const CSpell & spell = *CGI->spellh->spells[spellID];
	spellSelMode = ANY_LOCATION;
	if(spell.getTargetType() == CSpell::CREATURE)
	{
		switch(spell.positiveness)
		{
		case -1 :
			spellSelMode = HOSTILE_CREATURE;
			break;
		case 0:
			spellSelMode = ANY_CREATURE;
			break;
		case 1:
			spellSelMode = FRIENDLY_CREATURE;
			break;
		}
	}
	if(spell.getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(castingHero && castingHero->getSpellSchoolLevel(&spell) < 3)
		{
			switch(spell.positiveness)
			{
			case -1 :
				spellSelMode = HOSTILE_CREATURE;
				break;
			case 0:
				spellSelMode = ANY_CREATURE;
				break;
			case 1:
				spellSelMode = FRIENDLY_CREATURE;
				break;
			}
		}
		else
		{
			spellSelMode = NO_LOCATION;
		}
	}
	if(spell.getTargetType() == CSpell::OBSTACLE)
	{
		spellSelMode = OBSTACLE;
	}
	if(spell.range[ castingHero->getSpellSchoolLevel(&spell) ] == "X") //spell has no range
	{
		spellSelMode = NO_LOCATION;
	}

	if(spell.id == 63) //teleport
	{
		spellSelMode = TELEPORT;
	}

	if(spell.range[ castingHero->getSpellSchoolLevel(&spell) ].size() > 1) //spell has many-hex range
	{
		spellSelMode = ANY_LOCATION;
	}

	if(spellSelMode == NO_LOCATION) //user does not have to select location
	{
		spellToCast->destinationTile = -1;
		curInt->cb->battleMakeAction(spellToCast);
		endCastingSpell();
	}
	else
	{
		CCS->curh->changeGraphic(3, 0); 
	}
}

void CBattleInterface::displayEffect(ui32 effect, int destTile)
{
	addNewAnim(new CSpellEffectAnim(this, effect, destTile));
}

void CBattleInterface::setAnimSpeed(int set)
{
	curInt->sysOpts.animSpeed = set;
	curInt->sysOpts.settingsChanged();
}

int CBattleInterface::getAnimSpeed() const
{
	return curInt->sysOpts.animSpeed;
}

void CBattleInterface::activateStack()
{
	activeStack = stackToActivate;
	stackToActivate = NULL;
	const CStack *s = activeStack;

	myTurn = true;
	if(attackerInt && defenderInt) //hotseat -> need to pick which interface "takes over" as active
		curInt = attackerInt->playerID == s->owner ? attackerInt : defenderInt;

	queue->update();
	redrawBackgroundWithHexes(activeStack);
	bWait->block(vstd::contains(s->state, WAITING)); //block waiting button if stack has been already waiting

	//block cast spell button if hero doesn't have a spellbook
	bSpell->block(!curInt->cb->battleCanCastSpell());
	bSurrender->block((curInt == attackerInt ? defendingHeroInstance : attackingHeroInstance) == NULL);
	bFlee->block(!curInt->cb->battleCanFlee());
	bSurrender->block(curInt->cb->battleGetSurrenderCost() < 0);

	//set casting flag to true if creature can use it to not check it every time
	if (s->casts && s->hasBonus(Selector::type(Bonus::SPELLCASTER) || Selector::type(Bonus::RANDOM_SPELLCASTER)))
	{
		stackCanCastSpell = true;
		creatureSpellToCast = curInt->cb->battleGetRandomStackSpell(s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
	}
	else
	{
		stackCanCastSpell = false;
		creatureSpellToCast = -1;
	}

	GH.fakeMouseMove();

	if(!pendingAnims.size() && !active)
		activate();
}

float CBattleInterface::getAnimSpeedMultiplier() const
{
	switch(curInt->sysOpts.animSpeed)
	{
	case 1:
		return 3.5f;
	case 2:
		return 2.2f;
	case 4:
		return 1.0f;
	default:
		return 0.0f;
	}
}

void CBattleInterface::endCastingSpell()
{
	assert(spellDestSelectMode);

	delete spellToCast;
	spellToCast = NULL;
	spellDestSelectMode = false;
	CCS->curh->changeGraphic(1, 6);
}

void CBattleInterface::showAliveStack(const CStack *stack, SDL_Surface * to)
{
	int ID = stack->ID;
	if(creAnims.find(ID) == creAnims.end()) //eg. for summoned but not yet handled stacks
		return;
	const CCreature *creature = stack->getCreature();
	SDL_Rect unitRect = {creAnims[ID]->pos.x, creAnims[ID]->pos.y, creAnims[ID]->fullWidth, creAnims[ID]->fullHeight};
	
	int animType = creAnims[ID]->getType();

	int affectingSpeed = curInt->sysOpts.animSpeed;
	if(animType == 1 || animType == 2) //standing stacks should not stand faster :)
		affectingSpeed = 2;
	bool incrementFrame = (animCount%(4/affectingSpeed)==0) && animType!=5 && animType!=20 && animType!=2;

	if (creature->idNumber == 149)
	{
		// a turret creature has a limited height, so cut it at a certain position; turret creature has no standing anim
		unitRect.h = graphics->wallPositions[siegeH->town->town->typeID][20].y;
	}
	else
	{
		// standing animation
		if(animType == 2)
		{
			if(standingFrame.find(ID)!=standingFrame.end())
			{
				incrementFrame = (animCount%(8/affectingSpeed)==0);
				if(incrementFrame)
				{
					++standingFrame[ID];
					if(standingFrame[ID] == creAnims[ID]->framesInGroup(CCreatureAnim::HOLDING))
					{
						standingFrame.erase(standingFrame.find(ID));
					}
				}
			}
			else
			{
				if((rand()%50) == 0)
				{
					standingFrame.insert(std::make_pair(ID, 0));
				}
			}
		}
	}

	// As long as the projectile of the shooter-stack is flying incrementFrame should be false
	bool shootingFinished = true;
	for (std::list<SProjectileInfo>::iterator it = projectiles.begin(); it != projectiles.end(); ++it)
	{
		if (it->stackID == ID)
		{
			shootingFinished = false;
			if (it->animStartDelay == 0)
				incrementFrame = false;
		}
	}
	
	// Increment always when moving, never if stack died
	creAnims[ID]->nextFrame(to, unitRect.x, unitRect.y, creDir[ID], animCount, incrementFrame, activeStack && ID==activeStack->ID, ID==mouseHoveredStack, &unitRect);

	//printing amount
	if(stack->count > 0 //don't print if stack is not alive
		&& (!curInt->curAction
			|| (curInt->curAction->stackNumber != ID //don't print if stack is currently taking an action
				&& (curInt->curAction->actionType != BattleAction::WALK_AND_ATTACK  ||  stack->position != curInt->curAction->additionalInfo) //nor if it's an object of attack
				&& (curInt->curAction->destinationTile != stack->position) //nor if it's on destination tile for current action
				)
			)
			&& !stack->hasBonusOfType(Bonus::SIEGE_WEAPON) //and not a war machine...
	)
	{
        const THex nextPos = stack->position + (stack->attackerOwned ? 1 : -1);
        const bool edge = stack->position % BFIELD_WIDTH == (stack->attackerOwned ? BFIELD_WIDTH - 2 : 1);
        const bool moveInside = !edge && !stackCountOutsideHexes[nextPos];
		int xAdd = (stack->attackerOwned ? 220 : 202) +
                   (stack->doubleWide() ? 44 : 0) * (stack->attackerOwned ? +1 : -1) +
                   (moveInside ? amountNormal->w + 10 : 0) * (stack->attackerOwned ? -1 : +1);
        int yAdd = 260 + ((stack->attackerOwned || moveInside) ? 0 : -15);
		//blitting amount background box
		SDL_Surface *amountBG = NULL;
		TBonusListPtr spellEffects = stack->getSpellBonuses();
		if(!spellEffects->size())
		{
			amountBG = amountNormal;
		}
		else
		{
			int pos=0; //determining total positiveness of effects
			std::vector<si32> spellIds = stack->activeSpells();
			for(std::vector<si32>::const_iterator it = spellIds.begin(); it != spellIds.end(); it++)
			{
				pos += CGI->spellh->spells[ *it ]->positiveness;
			}
			if(pos > 0)
			{
				amountBG = amountPositive;
			}
			else if(pos < 0)
			{
				amountBG = amountNegative;
			}
			else
			{
				amountBG = amountEffNeutral;
			}
		}
		SDL_Rect temp_rect = genRect(amountNormal->h, amountNormal->w, creAnims[ID]->pos.x + xAdd, creAnims[ID]->pos.y + yAdd);
		SDL_BlitSurface(amountBG, NULL, to, &temp_rect);
		//blitting amount
		CSDL_Ext::printAtMiddle(
			makeNumberShort(stack->count),
			creAnims[ID]->pos.x + xAdd + 15,
			creAnims[ID]->pos.y + yAdd + 5,
			FONT_TINY,
			zwykly,
			to
		);
	}
}

void CBattleInterface::showPieceOfWall(SDL_Surface * to, int hex, const std::vector<const CStack*> & stacks)
{
	if(!siegeH)
		return;

	using namespace boost::assign;
	static const std::map<int, std::list<int> > hexToPart = map_list_of<int, std::list<int> >(12, list_of<int>(8)(1)(7))(45, list_of<int>(12)(6))
		/*gate (78, list_of<int>(9))*/(101, list_of<int>(10))(118, list_of<int>(2))(165, list_of<int>(11))(186, list_of<int>(3));

	std::map<int, std::list<int> >::const_iterator it = hexToPart.find(hex);
	if(it != hexToPart.end())
	{
		BOOST_FOREACH(int wallNum, it->second)
		{
			siegeH->printPartOfWall(to, wallNum);

			//print creature in turret
			int posToSeek = -1;
			switch(wallNum)
			{
			case 3: //bottom turret
				posToSeek = -3;
				break;
			case 8: //upper turret
				posToSeek = -4;
				break;
			case 2: //keep
				posToSeek = -2;
				break;
			}

			if(posToSeek != -1)
			{
				const CStack *turret = NULL;

				BOOST_FOREACH(const CStack *s, stacks)
				{
					if(s->position == posToSeek)
					{
						turret = s;
						break;
					}
				}

				if(turret)
				{
					showAliveStack(turret, to);
					//blitting creature cover
					switch(posToSeek)
					{
					case -3: //bottom turret
						siegeH->printPartOfWall(to, 16);
						break;
					case -4: //upper turret
						siegeH->printPartOfWall(to, 17);
						break;
					case -2: //keep
						siegeH->printPartOfWall(to, 15);
						break;
					}
				}

			}
		}
	}

	// Damaged wall below gate have to be drawn earlier than a non-damaged wall below gate.
	if ((hex == 112 && curInt->cb->battleGetWallState(3) == 3) || (hex == 147 && curInt->cb->battleGetWallState(3) != 3))
		siegeH->printPartOfWall(to, 5);
	// Damaged bottom wall have to be drawn earlier than a non-damaged bottom wall.
	if ((hex == 165 && curInt->cb->battleGetWallState(4) == 3) || (hex == 185 && curInt->cb->battleGetWallState(4) != 3))
		siegeH->printPartOfWall(to, 4);

}

void CBattleInterface::redrawBackgroundWithHexes(const CStack * activeStack)
{
	attackableHexes.clear();
	if (activeStack)
		occupyableHexes = curInt->cb->battleGetAvailableHexes(activeStack, true, &attackableHexes);
    curInt->cb->battleGetStackCountOutsideHexes(stackCountOutsideHexes);
	//preparating background graphic with hexes and shaded hexes
	blitAt(background, 0, 0, backgroundWithHexes);
	if(curInt->sysOpts.printCellBorders)
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, backgroundWithHexes, NULL);

	if(curInt->sysOpts.printStackRange)
	{
		std::vector<THex> hexesToShade = occupyableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		BOOST_FOREACH(THex hex, hexesToShade)
		{
			int i = hex.getY(); //row
			int j = hex.getX()-1; //column
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 * i;
			SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
			CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, backgroundWithHexes, &temp_rect);
		}
	}
}

void CBattleInterface::printConsoleAttacked( const CStack * defender, int dmg, int killed, const CStack * attacker, bool multiple )
{
	char tabh[200];
	int end = 0;
	if (attacker) //ignore if stacks were killed by spell
	{
		end = sprintf(tabh, CGI->generaltexth->allTexts[attacker->count > 1 ? 377 : 376].c_str(),
		(attacker->count > 1 ? attacker->getCreature()->namePl.c_str() : attacker->getCreature()->nameSing.c_str()), dmg);
	}
	if(killed > 0)
	{
		if(killed > 1)
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[379].c_str(), killed,
				multiple ? CGI->generaltexth->allTexts[43].c_str() : defender->getCreature()->namePl.c_str()); // creatures perish
		}
		else //killed == 1
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[378].c_str(), 
				multiple ? CGI->generaltexth->allTexts[42].c_str() : defender->getCreature()->nameSing.c_str()); // creature perishes
		}
	}

	console->addText(std::string(tabh));
}

void CBattleInterface::projectileShowHelper(SDL_Surface * to)
{
	if(to == NULL)
		to = screen;
	std::list< std::list<SProjectileInfo>::iterator > toBeDeleted;
	for(std::list<SProjectileInfo>::iterator it=projectiles.begin(); it!=projectiles.end(); ++it)
	{
		// Creature have to be in a shooting anim and the anim start delay must be over.
		// Otherwise abort to start moving the projectile.
		if (it->animStartDelay > 0)
		{
			if(it->animStartDelay == creAnims[it->stackID]->getAnimationFrame() + 1
					&& creAnims[it->stackID]->getType() >= 14 && creAnims[it->stackID]->getType() <= 16)
				it->animStartDelay = 0;
			else
				continue;
		}

		SDL_Rect dst;
		dst.h = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->h;
		dst.w = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->w;
		dst.x = it->x;
		dst.y = it->y;
		
		// The equation below calculates the center pos of the canon, but we need the top left pos
		// of it for drawing
		if (it->catapultInfo)
		{
			dst.x -= 17.;
			dst.y -= 10.;
		}

		if(it->reverse)
		{
			SDL_Surface * rev = CSDL_Ext::rotate01(idToProjectile[it->creID]->ourImages[it->frameNum].bitmap);
			CSDL_Ext::blit8bppAlphaTo24bpp(rev, NULL, to, &dst);
			SDL_FreeSurface(rev);
		}
		else
		{
			CSDL_Ext::blit8bppAlphaTo24bpp(idToProjectile[it->creID]->ourImages[it->frameNum].bitmap, NULL, to, &dst);
		}

		// Update projectile
		++it->step;
		if(it->step == it->lastStep)
		{
			toBeDeleted.insert(toBeDeleted.end(), it);
		}
		else
		{
			if (it->catapultInfo)
			{
				// Parabolic shot of the trajectory, as follows: f(x) = ax^2 + bx + c
				it->x += it->dx;
				it->y = it->catapultInfo->calculateY(it->x - this->pos.x) + this->pos.y;
			}
			else
			{
				// Normal projectile, just add the calculated "deltas" to the x and y positions.
				it->x += it->dx;
				it->y += it->dy;
			}

			if(it->spin)
			{
				++(it->frameNum);
				it->frameNum %= idToProjectile[it->creID]->ourImages.size();
			}
		}
	}
	for(std::list< std::list<SProjectileInfo>::iterator >::iterator it = toBeDeleted.begin(); it!= toBeDeleted.end(); ++it)
	{
		projectiles.erase(*it);
	}
}

void CBattleInterface::endAction(const BattleAction* action)
{	
	//if((action->actionType==2 || (action->actionType==6 && action->destinationTile!=cb->battleGetPos(action->stackNumber)))) //activating interface when move is finished
// 	{
// 		activate();
// 	}
	if(action->actionType == BattleAction::HERO_SPELL)
	{
		if(action->side)
			defendingHero->setPhase(0);
		else
			attackingHero->setPhase(0);
	}
	if(action->actionType == BattleAction::WALK && creAnims[action->stackNumber]->getType() != 2) //walk or walk & attack
	{
		const CStack * stack = curInt->cb->battleGetStackByID(action->stackNumber);
		pendingAnims.push_back(std::make_pair(new CBattleMoveEnd(this, stack, action->destinationTile), false));
	}
	if(action->actionType == BattleAction::CATAPULT) //catapult
	{
	}

	//check if we should reverse stacks
	//for some strange reason, it's not enough
// 	std::set<const CStack *> stacks;
// 	stacks.insert(LOCPLINT->cb->battleGetStackByID(action->stackNumber));
// 	stacks.insert(LOCPLINT->cb->battleGetStackByPos(action->destinationTile));
	TStacks stacks = curInt->cb->battleGetStacks(CBattleCallback::MINE_AND_ENEMY);

	BOOST_FOREACH(const CStack *s, stacks)
	{
		if(s && creDir[s->ID] != bool(s->attackerOwned) && s->alive())
		{
			addNewAnim(new CReverseAnim(this, s, s->position, false));
		}
	}

	queue->update();

	if(tacticsMode) //we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
		redrawBackgroundWithHexes(activeStack);
}

void CBattleInterface::hideQueue()
{
	curInt->sysOpts.showQueue = false;

	queue->deactivate();

	if(!queue->embedded)
	{
		moveBy(Point(0, -queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void CBattleInterface::showQueue()
{
	curInt->sysOpts.showQueue = true;

	queue->activate();

	if(!queue->embedded)
	{
		moveBy(Point(0, +queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void CBattleInterface::startAction(const BattleAction* action)
{
	if(action->actionType == BattleAction::END_TACTIC_PHASE)
	{
		SDL_FreeSurface(menu);
		menu = BitmapHandler::loadBitmap("CBAR.bmp");

		graphics->blueToPlayersAdv(menu, curInt->playerID);
		bDefence->block(false);
		bWait->block(false);
		if(active)
		{
			if(btactEnd && btactNext) //if the other side had tactics, there are no buttons
			{
				btactEnd->deactivate();
				btactNext->deactivate();
				bConsoleDown->activate();
				bConsoleUp->activate();
			}
		}
		redraw();

		return;
	}

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(stack)
	{
		queue->update();
	}
	else
	{
		assert(action->actionType == BattleAction::HERO_SPELL); //only cast spell is valid action without acting stack number
	}

	if(action->actionType == BattleAction::WALK 
		|| (action->actionType == BattleAction::WALK_AND_ATTACK && action->destinationTile != stack->position))
	{
		moveStarted = true;
		if(creAnims[action->stackNumber]->framesInGroup(CCreatureAnim::MOVE_START))
		{
			const CStack * stack = curInt->cb->battleGetStackByID(action->stackNumber);
			pendingAnims.push_back(std::make_pair(new CBattleMoveStart(this, stack), false));
		}
	}

	if(active)
		deactivate();

	char txt[400];

	if(action->actionType == BattleAction::HERO_SPELL) //when hero casts spell
	{
		if(action->side)
			defendingHero->setPhase(4);
		else
			attackingHero->setPhase(4);
		return;
	}
	if(!stack)
	{
		tlog1<<"Something wrong with stackNumber in actionStarted. Stack number: "<<action->stackNumber<<std::endl;
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case BattleAction::WAIT:
		txtid = 136;
		break;
	case BattleAction::BAD_MORALE:
		txtid = -34; //negative -> no separate singular/plural form
		displayEffect(30,stack->position);
		break;
	}

	if(txtid > 0  &&  stack->count != 1)
		txtid++; //move to plural text
	else if(txtid < 0)
		txtid = -txtid;

	if(txtid)
	{
		sprintf(txt, CGI->generaltexth->allTexts[txtid].c_str(),  (stack->count != 1) ? stack->getCreature()->namePl.c_str() : stack->getCreature()->nameSing.c_str(), 0);
		console->addText(txt);
	}

	//displaying special abilities
	switch (action->actionType)
	{
		case BattleAction::STACK_HEAL:
			displayEffect(74, action->destinationTile);
			CCS->soundh->playSound(soundBase::REGENER);
			break;
	}
}

void CBattleInterface::waitForAnims()
{
	LOCPLINT->pim->unlock();
	animsAreDisplayed.waitWhileTrue();
	LOCPLINT->pim->lock();
}

void CBattleInterface::bEndTacticPhase()
{
	btactEnd->block(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed();
}

void CBattleInterface::bTacticNextStack()
{
	TStacks stacksOfMine = tacticianInterface->cb->battleGetStacks(CBattleCallback::ONLY_MINE);
	stacksOfMine.erase(std::remove_if(stacksOfMine.begin(), stacksOfMine.end(), &immobile), stacksOfMine.end());
	TStacks::iterator it = vstd::find(stacksOfMine, activeStack);
	if(it != stacksOfMine.end() && ++it != stacksOfMine.end())
		stackActivated(*it);
	else
		stackActivated(stacksOfMine.front());
}

void CBattleHero::show(SDL_Surface *to)
{
	//animation of flag
	if(flip)
	{
		SDL_Rect temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 61,
			pos.y + 39);
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&temp_rect);
	}
	else
	{
		SDL_Rect temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 72,
			pos.y + 39);
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&temp_rect);
	}
	++flagAnimCount;
	if(flagAnimCount%4==0)
	{
		++flagAnim;
		flagAnim %= flag->ourImages.size();
	}
	//animation of hero
	int tick=-1;
	for(int i=0; i<dh->ourImages.size(); ++i)
	{
		if(dh->ourImages[i].groupNumber==phase)
			++tick;
		if(tick==image)
		{
			SDL_Rect posb = pos;
			CSDL_Ext::blit8bppAlphaTo24bpp(dh->ourImages[i].bitmap, NULL, to, &posb);
			if(phase != 4 || nextPhase != -1 || image < 4)
			{
				if(flagAnimCount%2==0)
				{
					++image;
				}
				if(dh->ourImages[(i+1)%dh->ourImages.size()].groupNumber!=phase) //back to appropriate frame
				{
					image = 0;
				}
			}
			if(phase == 4 && nextPhase != -1 && image == 7)
			{
				phase = nextPhase;
				nextPhase = -1;
				image = 0;
			}
			break;
		}
	}
}

void CBattleHero::activate()
{
	activateLClick();
}
void CBattleHero::deactivate()
{
	deactivateLClick();
}

void CBattleHero::setPhase(int newPhase)
{
	if(phase != 4)
	{
		phase = newPhase;
		image = 0;
	}
	else
	{
		nextPhase = newPhase;
	}
}

void CBattleHero::clickLeft(tribool down, bool previousState)
{
	if(myOwner->spellDestSelectMode) //we are casting a spell
		return;

	if(!down && myHero && myOwner->myTurn && myOwner->curInt->cb->battleCanCastSpell()) //check conditions
	{
		for(int it=0; it<BFIELD_SIZE; ++it) //do nothing when any hex is hovered - hero's animation overlaps battlefield
		{
			if(myOwner->bfield[it].hovered && myOwner->bfield[it].strictHovered)
				return;
		}
		CCS->curh->changeGraphic(0,0);

		CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), myHero, myOwner->curInt);
		GH.pushInt(spellWindow);
	}
}

CBattleHero::CBattleHero(const std::string & defName, int phaseG, int imageG, bool flipG, unsigned char player, const CGHeroInstance * hero, const CBattleInterface * owner): flip(flipG), myHero(hero), myOwner(owner), phase(phaseG), nextPhase(-1), image(imageG), flagAnim(0), flagAnimCount(0)
{
	dh = CDefHandler::giveDef( defName );
	for(int i=0; i<dh->ourImages.size(); ++i) //transforming images
	{
		if(flip)
		{
			SDL_Surface * hlp = CSDL_Ext::rotate01(dh->ourImages[i].bitmap);
			SDL_FreeSurface(dh->ourImages[i].bitmap);
			dh->ourImages[i].bitmap = hlp;
		}
		CSDL_Ext::alphaTransform(dh->ourImages[i].bitmap);
	}

	if(flip)
		flag = CDefHandler::giveDef("CMFLAGR.DEF");
	else
		flag = CDefHandler::giveDef("CMFLAGL.DEF");

	//coloring flag and adding transparency
	for(int i=0; i<flag->ourImages.size(); ++i)
	{
		CSDL_Ext::alphaTransform(flag->ourImages[i].bitmap);
		graphics->blueToPlayersAdv(flag->ourImages[i].bitmap, player);
	}
}

CBattleHero::~CBattleHero()
{
	delete dh;
	delete flag;
}

Point CBattleHex::getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * stack, const CBattleInterface * cbi)
{
	Point ret(-500, -500); //returned value
	if(stack && stack->position < 0) //creatures in turrets
	{
		switch(stack->position)
		{
		case -2: //keep
			ret = graphics->wallPositions[cbi->siegeH->town->town->typeID][17];
			break;
		case -3: //lower turret
			ret = graphics->wallPositions[cbi->siegeH->town->town->typeID][18];
			break;
		case -4: //upper turret
			ret = graphics->wallPositions[cbi->siegeH->town->town->typeID][19];
			break;	
		}
	}
	else
	{
		ret.y = -139 + 42 * (hexNum/BFIELD_WIDTH); //counting y
		//counting x
		if(attacker)
		{
			ret.x = -160 + 22 * ( ((hexNum/BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % BFIELD_WIDTH);
		}
		else
		{
			ret.x = -219 + 22 * ( ((hexNum/BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % BFIELD_WIDTH);
		}
		//shifting position for double - hex creatures
		if(stack && stack->doubleWide())
		{
			if(attacker)
			{
				ret.x -= 44;
			}
			else
			{
				ret.x += 45;
			}
		}
	}
	//returning
	return ret +CPlayerInterface::battleInt->pos;
}
void CBattleHex::activate()
{
	activateHover();
	activateMouseMove();
	activateLClick();
	activateRClick();
}

void CBattleHex::deactivate()
{
	deactivateHover();
	deactivateMouseMove();
	deactivateLClick();
	deactivateRClick();
}

void CBattleHex::hover(bool on)
{
	hovered = on;
	//Hoverable::hover(on);
	if(!on && setAlterText)
	{
		myInterface->console->alterTxt = std::string();
		setAlterText = false;
	}
}

CBattleHex::CBattleHex() : setAlterText(false), myNumber(-1), accessible(true), hovered(false), strictHovered(false), myInterface(NULL)
{
}

void CBattleHex::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	if(myInterface->cellShade)
	{
		if(CSDL_Ext::SDL_GetPixel(myInterface->cellShade, sEvent.x-pos.x, sEvent.y-pos.y) == 0) //hovered pixel is outside hex
		{
			strictHovered = false;
		}
		else //hovered pixel is inside hex
		{
			strictHovered = true;
		}
	}

	if(hovered && strictHovered) //print attacked creature to console
	{
		const CStack * attackedStack = myInterface->curInt->cb->battleGetStackByPos(myNumber);
		if(myInterface->console->alterTxt.size() == 0 &&attackedStack != NULL &&
			attackedStack->owner != myInterface->curInt->playerID &&
			attackedStack->alive())
		{
			char tabh[160];
			const std::string & attackedName = attackedStack->count == 1 ? attackedStack->getCreature()->nameSing : attackedStack->getCreature()->namePl;
			sprintf(tabh, CGI->generaltexth->allTexts[220].c_str(), attackedName.c_str());
			myInterface->console->alterTxt = std::string(tabh);
			setAlterText = true;
		}
	}
	else if(setAlterText)
	{
		myInterface->console->alterTxt = std::string();
		setAlterText = false;
	}
}

void CBattleHex::clickLeft(tribool down, bool previousState)
{
	if(!down && hovered && strictHovered) //we've been really clicked!
	{
		myInterface->hexLclicked(myNumber);
	}
}

void CBattleHex::clickRight(tribool down, bool previousState)
{
	const CStack * myst = myInterface->curInt->cb->battleGetStackByPos(myNumber); //stack info
	if(hovered && strictHovered && myst!=NULL)
	{
		
		if(!myst->alive()) return;
		if(down)
		{
			GH.pushInt(createCreWindow(myst));
		}
	}
}

CBattleConsole::CBattleConsole() : lastShown(-1), alterTxt(""), whoSetAlter(0)
{
}

CBattleConsole::~CBattleConsole()
{
	texts.clear();
}

void CBattleConsole::show(SDL_Surface * to)
{
	if(ingcAlter.size())
	{
		CSDL_Ext::printAtMiddleWB(ingcAlter, pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, zwykly, to);
	}
	else if(alterTxt.size())
	{
		CSDL_Ext::printAtMiddleWB(alterTxt, pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, zwykly, to);
	}
	else if(texts.size())
	{
		if(texts.size()==1)
		{
			CSDL_Ext::printAtMiddleWB(texts[0], pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, zwykly, to);
		}
		else
		{
			CSDL_Ext::printAtMiddleWB(texts[lastShown-1], pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, zwykly, to);
			CSDL_Ext::printAtMiddleWB(texts[lastShown], pos.x + pos.w/2, pos.y + 27, FONT_SMALL, 80, zwykly, to);
		}
	}
}

bool CBattleConsole::addText(const std::string & text)
{
	if(text.size()>70)
		return false; //text too long!
	int firstInToken = 0;
	for(int i=0; i<text.size(); ++i) //tokenize
	{
		if(text[i] == 10)
		{
			texts.push_back( text.substr(firstInToken, i-firstInToken) );
			firstInToken = i+1;
		}
	}

	texts.push_back( text.substr(firstInToken, text.size()) );
	lastShown = texts.size()-1;
	return true;
}

void CBattleConsole::eraseText(unsigned int pos)
{
	if(pos < texts.size())
	{
		texts.erase(texts.begin() + pos);
		if(lastShown == texts.size())
			--lastShown;
	}
}

void CBattleConsole::changeTextAt(const std::string & text, unsigned int pos)
{
	if(pos >= texts.size()) //no such pos
		return;
	texts[pos] = text;
}

void CBattleConsole::scrollUp(unsigned int by)
{
	if(lastShown > by)
		lastShown -= by;
}

void CBattleConsole::scrollDown(unsigned int by)
{
	if(lastShown + by < texts.size())
		lastShown += by;
}

CBattleResultWindow::CBattleResultWindow(const BattleResult &br, const SDL_Rect & pos, CBattleInterface * _owner)
: owner(_owner)
{
	this->pos = pos;
	background = BitmapHandler::loadBitmap("CPRESULT.BMP", true);
	graphics->blueToPlayersAdv(background, owner->curInt->playerID);
	SDL_Surface * pom = SDL_ConvertSurface(background, screen->format, screen->flags);
	SDL_FreeSurface(background);
	background = pom;
	exit = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleResultWindow::bExitf,this), 384 + pos.x, 505 + pos.y, "iok6432.def", SDLK_RETURN);
	exit->borderColor = Colors::MetallicGold;
	exit->borderEnabled = true;

	if(br.winner==0) //attacker won
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 59, 124, FONT_SMALL, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 408, 124, FONT_SMALL, zwykly, background);
	}
	else //if(br.winner==1)
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 59, 124, FONT_SMALL, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 412, 124, FONT_SMALL, zwykly, background);
	}

	
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[407], 232, 302, FONT_BIG, tytulowy, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[408], 232, 332, FONT_BIG, zwykly, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[409], 237, 428, FONT_BIG, zwykly, background);

	std::string attackerName, defenderName;

	if(owner->attackingHeroInstance) //a hero attacked
	{
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->attackingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting attackerName
		attackerName = owner->attackingHeroInstance->name;
	}
	else //a monster attacked
	{
		int bestMonsterID = -1;
		int bestPower = 0;
		for(TSlots::const_iterator it = owner->army1->Slots().begin(); it!=owner->army1->Slots().end(); ++it)
		{
			if( it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->defendingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting defenderName
		defenderName = owner->defendingHeroInstance->name;
	}
	else //a monster defended
	{
		int bestMonsterID = -1;
		int bestPower = 0;
		for(TSlots::const_iterator it = owner->army2->Slots().begin(); it!=owner->army2->Slots().end(); ++it)
		{
			if( it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting defenderName
		defenderName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}

	//printing attacker and defender's names
	CSDL_Ext::printAt(attackerName, 89, 37, FONT_SMALL, zwykly, background);
	CSDL_Ext::printTo(defenderName, 381, 53, FONT_SMALL, zwykly, background);
	//printing casualities
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[523], 235, 360 + 97*step, FONT_SMALL, zwykly, background);
		}
		else
		{
			int xPos = 235 - (br.casualties[step].size()*32 + (br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + step*97;
			for(std::map<ui32,si32>::const_iterator it=br.casualties[step].begin(); it!=br.casualties[step].end(); ++it)
			{
				blitAt(graphics->smallImgs[it->first], xPos, yPos, background);
				std::ostringstream amount;
				amount<<it->second;
				CSDL_Ext::printAtMiddle(amount.str(), xPos+16, yPos + 42, FONT_SMALL, zwykly, background);
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = (owner->curInt->playerID == owner->attackingHeroInstance->tempOwner);
	if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
	{
		int text=-1;
		switch(br.result)
		{
			case 0: text = 304; break;
			case 1: text = 303; break;
			case 2: text = 302; break;
		}

		CCS->musich->playMusic(musicBase::winBattle);
		CCS->videoh->open(VIDEO_WIN);
		std::string str = CGI->generaltexth->allTexts[text];
		
		const CGHeroInstance * ourHero = weAreAttacker? owner->attackingHeroInstance : owner->defendingHeroInstance;
		if (ourHero)
		{
			str += CGI->generaltexth->allTexts[305];
			boost::algorithm::replace_first(str,"%s",ourHero->name);
			boost::algorithm::replace_first(str,"%d",boost::lexical_cast<std::string>(br.exp[weAreAttacker?0:1]));
		}
		CSDL_Ext::printAtMiddleWB(str, 235, 235, FONT_SMALL, 55, zwykly, background);
	}
	else // we lose
	{
		switch(br.result)
		{
		case 0: //normal victory
		{
			CCS->musich->playMusic(musicBase::loseCombat);
			CCS->videoh->open(VIDEO_LOSE_BATTLE_START);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[311], 235, 235, FONT_SMALL, zwykly, background);
			break;
		}
		case 1: //flee
		{
			CCS->musich->playMusic(musicBase::retreatBattle);
			CCS->videoh->open(VIDEO_RETREAT_START);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[310], 235, 235, FONT_SMALL, zwykly, background);
			break;
		}
		case 2: //surrender
		{
			CCS->musich->playMusic(musicBase::surrenderBattle);
			CCS->videoh->open(VIDEO_SURRENDER);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[309], 235, 220, FONT_SMALL, zwykly, background);
			break;
		}
		}
	}
}

CBattleResultWindow::~CBattleResultWindow()
{
	SDL_FreeSurface(background);
}

void CBattleResultWindow::activate()
{
	owner->curInt->showingDialog->set(true);
	exit->activate();
}

void CBattleResultWindow::deactivate()
{
	exit->deactivate();
}

void CBattleResultWindow::show(SDL_Surface *to)
{
	//evaluating to
	if(!to)
		to = screen;

	CCS->videoh->update(107, 70, background, false, true);

	SDL_BlitSurface(background, NULL, to, &pos);
	exit->showAll(to);
}

void CBattleResultWindow::bExitf()
{
	if(LOCPLINT->cb->getStartInfo()->mode == StartInfo::DUEL)
	{
		std::exit(0);
	}

	CPlayerInterface * intTmp = owner->curInt;
	GH.popInts(2); //first - we; second - battle interface
	intTmp->showingDialog->setn(false);
	CCS->videoh->close();
}

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner): myInt(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos = position;
	background = new CPicture("comopbck.bmp");
	background->colorize(owner->curInt->playerID);

	viewGrid = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintCellBorders, owner, true), boost::bind(&CBattleInterface::setPrintCellBorders, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[427].first)(3,CGI->generaltexth->zelp[427].first), CGI->generaltexth->zelp[427].second, false, "sysopchk.def", NULL, 25, 56, false);
	viewGrid->select(owner->curInt->sysOpts.printCellBorders);
	movementShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintStackRange, owner, true), boost::bind(&CBattleInterface::setPrintStackRange, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[428].first)(3,CGI->generaltexth->zelp[428].first), CGI->generaltexth->zelp[428].second, false, "sysopchk.def", NULL, 25, 89, false);
	movementShadow->select(owner->curInt->sysOpts.printStackRange);
	mouseShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintMouseShadow, owner, true), boost::bind(&CBattleInterface::setPrintMouseShadow, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[429].first)(3,CGI->generaltexth->zelp[429].first), CGI->generaltexth->zelp[429].second, false, "sysopchk.def", NULL, 25, 122, false);
	mouseShadow->select(owner->curInt->sysOpts.printMouseShadow);

	animSpeeds = new CHighlightableButtonsGroup(0);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[422].first),CGI->generaltexth->zelp[422].second, "sysopb9.def", 28, 225, 1);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[423].first),CGI->generaltexth->zelp[423].second, "sysob10.def", 92, 225, 2);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[424].first),CGI->generaltexth->zelp[424].second, "sysob11.def",156, 225, 4);
	animSpeeds->select(owner->getAnimSpeed(), 1);
	animSpeeds->onChange = boost::bind(&CBattleInterface::setAnimSpeed, owner, _1);

	setToDefault = new AdventureMapButton (CGI->generaltexth->zelp[393], boost::bind(&CBattleOptionsWindow::bDefaultf,this), 246, 359, "codefaul.def");
	setToDefault->swappedImages = true;
	setToDefault->update();
	exit = new AdventureMapButton (CGI->generaltexth->zelp[392], boost::bind(&CBattleOptionsWindow::bExitf,this), 357, 359, "soretrn.def",SDLK_RETURN);
	exit->swappedImages = true;
	exit->update();

	//creating labels
	labels.push_back(new CLabel(242,  32, FONT_BIG,    CENTER, tytulowy, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(new CLabel(122, 214, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(new CLabel(122, 293, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(new CLabel(122, 359, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(new CLabel(353,  66, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(new CLabel(353, 265, FONT_MEDIUM, CENTER, tytulowy, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(new CLabel(283,  86, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(new CLabel(283, 116, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(new CLabel(283, 146, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(new CLabel(283, 176, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(new CLabel(283, 206, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(new CLabel(283, 285, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(new CLabel(283, 315, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[403]));//spells only
	
	//general options
	labels.push_back(new CLabel(61,  57, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[404]));
	labels.push_back(new CLabel(61,  90, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[405]));
	labels.push_back(new CLabel(61, 123, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[406]));
	labels.push_back(new CLabel(61, 156, FONT_MEDIUM, TOPLEFT, zwykly, CGI->generaltexth->allTexts[407]));
}

void CBattleOptionsWindow::bDefaultf()
{
}

void CBattleOptionsWindow::bExitf()
{
	GH.popIntTotally(this);
}

std::string CBattleInterface::SiegeHelper::townTypeInfixes[F_NUMBER] = {"CS", "RM", "TW", "IN", "NC", "DN", "ST", "FR", "EL"};

CBattleInterface::SiegeHelper::SiegeHelper(const CGTownInstance *siegeTown, const CBattleInterface * _owner)
  : owner(_owner), town(siegeTown)
{
	for(int g=0; g<ARRAY_COUNT(walls); ++g)
	{
		walls[g] = BitmapHandler::loadBitmap( getSiegeName(g) );
	}
}

CBattleInterface::SiegeHelper::~SiegeHelper()
{
	for(int g=0; g<ARRAY_COUNT(walls); ++g)
	{
		SDL_FreeSurface(walls[g]);
	}
}

std::string CBattleInterface::SiegeHelper::getSiegeName(ui16 what, ui16 additInfo) const
{
	if(what == 2 || what == 3 || what == 8)
	{
		if(additInfo == 3) additInfo = 2;
	}
	char buf[100];
	SDL_itoa(additInfo, buf, 10);
	std::string addit(buf);
	switch(what)
	{
	case 0: //background
		return "SG" + townTypeInfixes[town->town->typeID] + "BACK.BMP";
	case 1: //background wall
		{
			switch(town->town->typeID)
			{
			case 5: case 4: case 1: case 6:
				return "SG" + townTypeInfixes[town->town->typeID] + "TPW1.BMP";
			case 0: case 2: case 3: case 7: case 8:
				return "SG" + townTypeInfixes[town->town->typeID] + "TPWL.BMP";
			default:
				return "";
			}
		}
	case 2: //keep
		return "SG" + townTypeInfixes[town->town->typeID] + "MAN" + addit + ".BMP";
	case 3: //bottom tower
		return "SG" + townTypeInfixes[town->town->typeID] + "TW1" + addit + ".BMP";
	case 4: //bottom wall
		return "SG" + townTypeInfixes[town->town->typeID] + "WA1" + addit + ".BMP";
	case 5: //below gate
		return "SG" + townTypeInfixes[town->town->typeID] + "WA3" + addit + ".BMP";
	case 6: //over gate
		return "SG" + townTypeInfixes[town->town->typeID] + "WA4" + addit + ".BMP";
	case 7: //upper wall
		return "SG" + townTypeInfixes[town->town->typeID] + "WA6" + addit + ".BMP";
	case 8: //upper tower
		return "SG" + townTypeInfixes[town->town->typeID] + "TW2" + addit + ".BMP";
	case 9: //gate
		return "SG" + townTypeInfixes[town->town->typeID] + "DRW" + addit + ".BMP";
	case 10: //gate arch
		return "SG" + townTypeInfixes[town->town->typeID] + "ARCH.BMP";
	case 11: //bottom static wall
		return "SG" + townTypeInfixes[town->town->typeID] + "WA2.BMP";
	case 12: //upper static wall
		return "SG" + townTypeInfixes[town->town->typeID] + "WA5.BMP";
	case 13: //moat
		return "SG" + townTypeInfixes[town->town->typeID] + "MOAT.BMP";
	case 14: //mlip
		return "SG" + townTypeInfixes[town->town->typeID] + "MLIP.BMP";
	case 15: //keep creature cover
		return "SG" + townTypeInfixes[town->town->typeID] + "MANC.BMP";
	case 16: //bottom turret creature cover
		return "SG" + townTypeInfixes[town->town->typeID] + "TW1C.BMP";
	case 17: //upper turret creature cover
		return "SG" + townTypeInfixes[town->town->typeID] + "TW2C.BMP";
	default:
		return "";
	}
}

/// What: 1. background wall, 2. keep, 3. bottom tower, 4. bottom wall, 5. wall below gate, 
/// 6. wall over gate, 7. upper wall, 8. upper tower, 9. gate, 10. gate arch, 11. bottom static wall, 12. upper static wall, 13. moat, 14. mlip, 
/// 15. keep turret cover, 16. lower turret cover, 17. upper turret cover
/// Positions are loaded from the config file: /config/wall_pos.txt
void CBattleInterface::SiegeHelper::printPartOfWall(SDL_Surface * to, int what)
{
	Point pos = Point(-1, -1);
	
	if (what >= 1 && what <= 17)
	{
		pos.x = graphics->wallPositions[town->town->typeID][what - 1].x + owner->pos.x;
		pos.y = graphics->wallPositions[town->town->typeID][what - 1].y + owner->pos.y;
	}

	if(pos.x != -1)
	{
		blitAt(walls[what], pos.x, pos.y, to);
	}
}

void CStackQueue::update()
{
	stacksSorted.clear();
	owner->curInt->cb->getStackQueue(stacksSorted, QUEUE_SIZE);
	for (int i = 0; i < QUEUE_SIZE ; i++)
	{
		stackBoxes[i]->setStack(stacksSorted[i]);
	}
}

CStackQueue::CStackQueue(bool Embedded, CBattleInterface * _owner)
:embedded(Embedded), owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if(embedded)
	{
		box = NULL;
		bg = NULL;
		pos.w = QUEUE_SIZE * 37;
		pos.h = 32; //height of small creature img
		pos.x = screen->w/2 - pos.w/2;
		pos.y = (screen->h - 600)/2 + 10;
	}
	else
	{
		box = BitmapHandler::loadBitmap("CHRROP.pcx");
		bg = BitmapHandler::loadBitmap("DIBOXPI.pcx");
		pos.w = 600;
		pos.h = bg->h;
	}

	stackBoxes.resize(QUEUE_SIZE);
	for (int i = 0; i < QUEUE_SIZE; i++)
	{
		stackBoxes[i] = new StackBox(box);
		stackBoxes[i]->pos.x += 6 + (embedded ? 37 : 79)*i;
	}
}

CStackQueue::~CStackQueue()
{
	SDL_FreeSurface(box);
}

void CStackQueue::showAll( SDL_Surface *to )
{
	blitBg(to);

	CIntObject::showAll(to);
}

void CStackQueue::blitBg( SDL_Surface * to )
{
	if(bg)
	{
		for (int w = 0; w < pos.w; w += bg->w)
		{
			blitAtLoc(bg, w, 0, to);		
		}
	}
}

void CStackQueue::StackBox::showAll( SDL_Surface *to )
{
	assert(my);
	if(bg)
	{
		graphics->blueToPlayersAdv(bg, my->owner);
		//SDL_UpdateRect(bg, 0, 0, 0, 0);
		SDL_Rect temp_rect = genRect(bg->h, bg->w, pos.x, pos.y);
		CSDL_Ext::blit8bppAlphaTo24bpp(bg, NULL, to, &temp_rect);
		//blitAt(bg, pos, to);
		blitAt(graphics->bigImgs[my->getCreature()->idNumber], pos.x +9, pos.y + 1, to);
		printAtMiddleLoc(makeNumberShort(my->count), pos.w/2, pos.h - 12, FONT_MEDIUM, zwykly, to);
	}
	else
	{
		blitAt(graphics->smallImgs[-2], pos, to);
		blitAt(graphics->smallImgs[my->getCreature()->idNumber], pos, to);
		const SDL_Color &ownerColor = (my->owner == 255 ? *graphics->neutralColor : graphics->playerColors[my->owner]);
		CSDL_Ext::drawBorder(to, pos, int3(ownerColor.r, ownerColor.g, ownerColor.b));
		printAtMiddleLoc(makeNumberShort(my->count), pos.w/2, pos.h - 8, FONT_TINY, zwykly, to);
	}
}

void CStackQueue::StackBox::setStack( const CStack *nStack )
{
	my = nStack;
}

CStackQueue::StackBox::StackBox(SDL_Surface *BG)
 :my(NULL), bg(BG)
{
	if(bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
	else
	{
		pos.w = pos.h = 32;
	}

	pos.y += 2;
}

CStackQueue::StackBox::~StackBox()
{
}

void CStackQueue::StackBox::hover( bool on )
{

}

double CatapultProjectileInfo::calculateY(double x)
{
	return (facA * pow(10., -3.)) * pow(x, 2.0) + facB * x + facC;
}
