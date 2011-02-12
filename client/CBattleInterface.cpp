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
#include "CVideoHandler.h"
#include "../lib/CTownHandler.h"
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

//revering animation

bool CReverseAnim::init()
{
	if(owner->creAnims[stack->ID] == NULL || owner->creAnims[stack->ID]->getType() == 5)
	{
		endAnim();

		return false; //there is no such creature
	}

	if(!priority && !isEarliest(false))
		return false;
	
	owner->creAnims[stack->ID]->setType(CCreatureAnim::TURN_R);

	return true;
}

void CReverseAnim::nextFrame()
{
	if(partOfAnim == 1) //first part of animation
	{
		if(owner->creAnims[stack->ID]->onLastFrameInGroup())
		{
			partOfAnim = 2;
		}
	}
	else if(partOfAnim == 2)
	{
		if(!secondPartSetup)
		{
			owner->creDir[stack->ID] = !owner->creDir[stack->ID];

			if(!stack)
			{
				endAnim();
				return;
			}

			Point coords = CBattleHex::getXYUnitAnim(hex, owner->creDir[stack->ID], stack, owner);
			owner->creAnims[stack->ID]->pos.x = coords.x;
			//creAnims[stackID]->pos.y = coords.second;

			if(stack->doubleWide())
			{
				if(stack->attackerOwned)
				{
					if(!owner->creDir[stack->ID])
						owner->creAnims[stack->ID]->pos.x -= 44;
				}
				else
				{
					if(owner->creDir[stack->ID])
						owner->creAnims[stack->ID]->pos.x += 44;
				}
			}

			owner->creAnims[stack->ID]->setType(CCreatureAnim::TURN_L);
			secondPartSetup = true;
		}

		if(owner->creAnims[stack->ID]->onLastFrameInGroup())
		{
			endAnim();
		}
	}
}

void CReverseAnim::endAnim()
{
	CBattleAnimation::endAnim();
	if( stack->alive() )//don't do that if stack is dead
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);

	delete this;
}

CReverseAnim::CReverseAnim(CBattleInterface * _owner, const CStack * stack, THex dest, bool _priority)
: CBattleStackAnimation(_owner, stack), partOfAnim(1), secondPartSetup(false), hex(dest), priority(_priority)
{
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
		owner->creAnims[stack->ID]->setType(CCreatureAnim::DEATH); //death
	}
	else
	{
		// TODO: this block doesn't seems correct if the unit is defending.
		CCS->soundh->playSound(battle_sound(stack->getCreature(), wince));
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HITTED); //getting hit
	}

	return true; //initialized successfuly
}

void CDefenceAnim::nextFrame()
{
	if(!killed && owner->creAnims[stack->ID]->getType() != CCreatureAnim::HITTED)
	{
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HITTED);
	}

	if(!owner->creAnims[stack->ID]->onLastFrameInGroup())
	{
		if( owner->creAnims[stack->ID]->getType() == CCreatureAnim::DEATH && (owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0
			&& !owner->creAnims[stack->ID]->onLastFrameInGroup() )
		{
			owner->creAnims[stack->ID]->incrementFrame();
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

	if(owner->creAnims[stack->ID]->getType() == CCreatureAnim::HITTED)
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);

	//printing info to console

	if(attacker!=NULL)
		owner->printConsoleAttacked(stack, dmg, amountKilled, attacker);

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
	steps = owner->creAnims[stack->ID]->framesInGroup(CCreatureAnim::MOVING)*owner->getAnimSpeedMultiplier()-1;
	if(steps == 0) //this creature seems to have no move animation so we can end it immediately
	{
		endAnim();
		return false;
	}
	whichStep = 0;
	int hexWbase = 44, hexHbase = 42;
	const CStack * movedStack = stack;
	if(!movedStack || owner->creAnims[stack->ID]->getType() == 5)
	{
		endAnim();
		return false;
	}
	//bool twoTiles = movedStack->doubleWide();
	
	Point begPosition = CBattleHex::getXYUnitAnim(curStackPos, movedStack->attackerOwned, movedStack, owner);
	Point endPosition = CBattleHex::getXYUnitAnim(destHex, movedStack->attackerOwned, movedStack, owner);

	int mutPos = THex::mutualPosition(curStackPos, destHex);
	
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

	if(owner->creAnims[stack->ID]->getType() != CCreatureAnim::MOVING)
	{
		owner->creAnims[stack->ID]->setType(CCreatureAnim::MOVING);
	}
	//unit reversed

	if(owner->moveSh <= 0)
		owner->moveSh = CCS->soundh->playSound(battle_sound(movedStack->getCreature(), move), -1);

	//step shift calculation
	posX = owner->creAnims[stack->ID]->pos.x, posY = owner->creAnims[stack->ID]->pos.y; // for precise calculations ;]
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
	owner->creAnims[stack->ID]->pos.x = posX;
	posY += stepY;
	owner->creAnims[stack->ID]->pos.y = posY;

	++whichStep;
	if(whichStep == steps)
	{
		endAnim();
	}
}

void CBattleStackMoved::endAnim()
{
	const CStack * movedStack = stack;

	CBattleAnimation::endAnim();

	if(movedStack)
	{
		bool twoTiles = movedStack->doubleWide();

		if(endMoving)
		{
			owner->addNewAnim(new CBattleMoveEnd(owner, stack, destHex));
		}

		Point coords = CBattleHex::getXYUnitAnim(destHex, owner->creDir[stack->ID], movedStack, owner);
		owner->creAnims[stack->ID]->pos = coords;

		if(!endMoving && twoTiles && bool(movedStack->attackerOwned) && (owner->creDir[stack->ID] != bool(movedStack->attackerOwned) )) //big attacker creature is reversed
			owner->creAnims[stack->ID]->pos.x -= 44;
		else if(!endMoving && twoTiles && (! bool(movedStack->attackerOwned) ) && (owner->creDir[stack->ID] != bool(movedStack->attackerOwned) )) //big defender creature is reversed
			owner->creAnims[stack->ID]->pos.x += 44;
	}

	if(owner->moveSh >= 0)
	{
		CCS->soundh->stopSound(owner->moveSh);
		owner->moveSh = -1;
	}

	delete this;
}

CBattleStackMoved::CBattleStackMoved(CBattleInterface * _owner, const CStack * _stack, THex _destHex, bool _endMoving, int _distance)
: CBattleStackAnimation(_owner, _stack), destHex(_destHex), endMoving(_endMoving), distance(_distance), stepX(0.0f), stepY(0.0f)
{
	curStackPos = stack->position;
}

//move started

bool CBattleMoveStart::init()
{
	if( !isEarliest(false) )
		return false;


	if(!stack || owner->creAnims[stack->ID]->getType() == 5)
	{
		CBattleMoveStart::endAnim();
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));
	owner->creAnims[stack->ID]->setType(CCreatureAnim::MOVE_START);

	return true;
}

void CBattleMoveStart::nextFrame()
{
	if(owner->creAnims[stack->ID]->onLastFrameInGroup())
	{
		endAnim();
	}
	else
	{
 		if((owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0)
 			owner->creAnims[stack->ID]->incrementFrame();
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

	if(!stack || owner->creAnims[stack->ID]->framesInGroup(CCreatureAnim::MOVE_END) == 0 ||
		owner->creAnims[stack->ID]->getType() == CCreatureAnim::DEATH)
	{
		endAnim();

		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	owner->creAnims[stack->ID]->setType(CCreatureAnim::MOVE_END);

	return true;
}

void CBattleMoveEnd::nextFrame()
{
	if(owner->creAnims[stack->ID]->onLastFrameInGroup())
	{
		endAnim();
	}
}

void CBattleMoveEnd::endAnim()
{
	CBattleAnimation::endAnim();

	if(owner->creAnims[stack->ID]->getType() != CCreatureAnim::DEATH)
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HOLDING); //resetting to default

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
	if(owner->creAnims[stack->ID]->getType() != group)
		owner->creAnims[stack->ID]->setType(group);

	if(owner->creAnims[stack->ID]->onFirstFrameInGroup())
	{
		if(shooting)
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), shoot));
		else
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), attack));
	}
	else if(owner->creAnims[stack->ID]->onLastFrameInGroup())
	{
		owner->creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);
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

	if(!attackingStack || owner->creAnims[stack->ID]->getType() == 5)
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

	if(!shooter || owner->creAnims[stack->ID]->getType() == 5)
	{
		endAnim();
		return false;
	}

	//projectile
	float projectileAngle; //in radians; if positive, projectiles goes up
	float straightAngle = 0.2f; //maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	int fromHex = shooter->position;
	projectileAngle = atan2(float(abs(dest - fromHex)/BFIELD_WIDTH), float(abs(dest - fromHex)%BFIELD_WIDTH));
	if(fromHex < dest)
		projectileAngle = -projectileAngle;

	SProjectileInfo spi;
	spi.creID = shooter->getCreature()->idNumber;
	spi.reverse = !shooter->attackerOwned;

	spi.step = 0;
	spi.frameNum = 0;
	if(vstd::contains(CGI->creh->idToProjectileSpin, spi.creID))
		spi.spin = CGI->creh->idToProjectileSpin[spi.creID];
	else
	{
		tlog2 << "Warning - no projectile spin for spi.creID " << spi.creID << std::endl;
		spi.spin = false;
	}

	Point xycoord = CBattleHex::getXYUnitAnim(shooter->position, true, shooter, owner);
	Point destcoord;
	if(attackedStack)
	{
		destcoord = CBattleHex::getXYUnitAnim(dest, false, attackedStack, owner); 
	}
	else //catapult attack
	{
		destcoord.x = -160 + 22 * ( ((dest/BFIELD_WIDTH) + 1)%2 ) + 44 * (dest % BFIELD_WIDTH);
		destcoord.y = -139 + 42 * (dest/BFIELD_WIDTH);
	}
	destcoord.x += 250; destcoord.y += 210; //TODO: find a better place to shoot

	if(projectileAngle > straightAngle) //upper shot
	{
		spi.x = xycoord.x + 200 + shooter->getCreature()->upperRightMissleOffsetX;
		spi.y = xycoord.y + 100 - shooter->getCreature()->upperRightMissleOffsetY;
	}
	else if(projectileAngle < -straightAngle) //lower shot
	{
		spi.x = xycoord.x + 200 + shooter->getCreature()->lowerRightMissleOffsetX;
		spi.y = xycoord.y + 150 - shooter->getCreature()->lowerRightMissleOffsetY;
	}
	else //straight shot
	{
		spi.x = xycoord.x + 200 + shooter->getCreature()->rightMissleOffsetX;
		spi.y = xycoord.y + 125 - shooter->getCreature()->rightMissleOffsetY;
	}
	spi.lastStep = sqrt((float)((destcoord.x - spi.x)*(destcoord.x - spi.x) + (destcoord.y - spi.y) * (destcoord.y - spi.y))) / 40;
	if(spi.lastStep == 0)
		spi.lastStep = 1;
	spi.dx = (destcoord.x - spi.x) / spi.lastStep;
	spi.dy = (destcoord.y - spi.y) / spi.lastStep;
	//set starting frame
	if(spi.spin)
	{
		spi.frameNum = 0;
	}
	else
	{
		spi.frameNum = ((M_PI/2.0f - projectileAngle) / (2.0f *M_PI) + 1/((float)(2*(owner->idToProjectile[spi.creID]->ourImages.size()-1)))) * (owner->idToProjectile[spi.creID]->ourImages.size()-1);
	}
	//set delay
	spi.animStartDelay = CGI->creh->creatures[spi.creID]->attackClimaxFrame;
	owner->projectiles.push_back(spi);

	//attack aniamtion

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
	if(catapult) //catapult attack
	{
		owner->addNewAnim( new CSpellEffectAnim(owner, "SGEXPL.DEF",
			-130 + 22 * ( ((dest/BFIELD_WIDTH) + 1)%2 ) + 44 * (dest % BFIELD_WIDTH) + owner->pos.x,
			-50 + 42 * (dest/BFIELD_WIDTH) + owner->pos.y ));
	}
}

////////////////////////

void CBattleInterface::addNewAnim(CBattleAnimation * anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	animsAreDisplayed.setn(true);
}

CBattleInterface::CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect, CPlayerInterface * att, CPlayerInterface * defen)
	: queue(NULL), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), 
	  activeStack(NULL), stackToActivate(NULL), mouseHoveredStack(-1), previouslyHoveredHex(-1),
	  currentlyHoveredHex(-1), spellDestSelectMode(false), spellToCast(NULL), siegeH(NULL),
	  attackerInt(att), defenderInt(defen), curInt(att), animIDhelper(0), givenCommand(NULL),
	  myTurn(false), resWindow(NULL), moveStarted(false), moveSh(-1), bresult(NULL)
{
	ObjectConstruction h__l__p(this);

	if(!curInt) curInt = LOCPLINT; //may happen when we are defending during network MP game

	animsAreDisplayed.setn(false);
	pos = myRect;
	strongInterest = true;
	givenCommand = new CondSh<BattleAction *>(NULL);
	
	//create stack queue
	bool embedQueue = screen->h < 700;
	queue = new CStackQueue(embedQueue, this);
	if(!embedQueue && curInt->sysOpts.showQueue)
	{
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
	std::vector<const CStack*> stacks = curInt->cb->battleGetStacks(false);
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

			Point moatPos = graphics->wallPositions[siegeH->town->town->typeID][10],
				mlipPos = graphics->wallPositions[siegeH->town->town->typeID][11];

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
	menu = BitmapHandler::loadBitmap("CBAR.BMP");
	graphics->blueToPlayersAdv(menu, hero1->tempOwner);

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
	blitAt(background, pos.x, pos.y);
	blitAt(menu, pos.x, 556 + pos.y);
	CSDL_Ext::update();

	//preparing buttons and console
	bOptions = new AdventureMapButton (CGI->generaltexth->zelp[381].first, CGI->generaltexth->zelp[381].second, boost::bind(&CBattleInterface::bOptionsf,this), 3 + pos.x, 561 + pos.y, "icm003.def", SDLK_o);
	bSurrender = new AdventureMapButton (CGI->generaltexth->zelp[379].first, CGI->generaltexth->zelp[379].second, boost::bind(&CBattleInterface::bSurrenderf,this), 54 + pos.x, 561 + pos.y, "icm001.def", SDLK_s);
	bFlee = new AdventureMapButton (CGI->generaltexth->zelp[380].first, CGI->generaltexth->zelp[380].second, boost::bind(&CBattleInterface::bFleef,this), 105 + pos.x, 561 + pos.y, "icm002.def", SDLK_r);
	bSurrender->block(!curInt->cb->battleCanFlee());
	bFlee->block(!curInt->cb->battleCanFlee());	
	bAutofight  = new AdventureMapButton (CGI->generaltexth->zelp[382].first, CGI->generaltexth->zelp[382].second, boost::bind(&CBattleInterface::bAutofightf,this), 157 + pos.x, 561 + pos.y, "icm004.def", SDLK_a);
	bSpell = new AdventureMapButton (CGI->generaltexth->zelp[385].first, CGI->generaltexth->zelp[385].second, boost::bind(&CBattleInterface::bSpellf,this), 645 + pos.x, 561 + pos.y, "icm005.def", SDLK_c);
	bSpell->block(true);
	bWait = new AdventureMapButton (CGI->generaltexth->zelp[386].first, CGI->generaltexth->zelp[386].second, boost::bind(&CBattleInterface::bWaitf,this), 696 + pos.x, 561 + pos.y, "icm006.def", SDLK_w);
	bDefence = new AdventureMapButton (CGI->generaltexth->zelp[387].first, CGI->generaltexth->zelp[387].second, boost::bind(&CBattleInterface::bDefencef,this), 747 + pos.x, 561 + pos.y, "icm007.def", SDLK_d);
	bDefence->assignedKeys.insert(SDLK_SPACE);
	bConsoleUp = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleUpf,this), 624 + pos.x, 561 + pos.y, "ComSlide.def", SDLK_UP);
	bConsoleDown = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleDownf,this), 624 + pos.x, 580 + pos.y, "ComSlide.def", SDLK_DOWN);
	bConsoleDown->bitmapOffset = 2;
	console = new CBattleConsole();
	console->pos.x = 211 + pos.x;
	console->pos.y = 560 + pos.y;
	console->pos.w = 406;
	console->pos.h = 38;
	if(curInt->cb->battleGetTacticDist())
	{
		btactNext = new AdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bTacticNextStack,this), 213 + pos.x, 560 + pos.y, "icm011.def", SDLK_SPACE);
		btactEnd = new AdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bEndTacticPhase,this), 419 + pos.x, 560 + pos.y, "icm012.def", SDLK_RETURN);
		bDefence->block(true);
		bWait->block(true);
	}

	//loading hero animations
	if(hero1) // attacking hero
	{
		int type = hero1->type->heroType;
		if ( type % 2 )   type--;
		if ( hero1->sex ) type++;
		attackingHero = new CBattleHero(graphics->battleHeroes[type], 0, 0, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : NULL, this);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, -40 + pos.x, pos.y);
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
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, 690 + pos.x, pos.y);
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

	if(curInt->cb->battleGetTacticDist())
	{
		BOOST_FOREACH(const CStack *s, curInt->cb->battleGetStacks())
		{
			if(s->owner == curInt->playerID)
			{
				active = 1;
				stackActivated(s);
				active = 0;
				break;
			}
		}
	}
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
	bConsoleUp->activate();
	bConsoleDown->activate();
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
	bConsoleUp->deactivate();
	bConsoleDown->deactivate();
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

	LOCPLINT->cingconsole->deactivate();
}

void CBattleInterface::show(SDL_Surface * to)
{
	std::vector<const CStack*> stacks = curInt->cb->battleGetStacks(false); //used in a few places
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
				//calculating spell schoold level
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
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &genRect(cellShade->h, cellShade->w, x, y));
					}
				}
			}
			else if(curInt->sysOpts.printMouseShadow) //when not casting spell
			{
				int x = 14 + ((b/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(b%BFIELD_WIDTH) + pos.x;
				int y = 86 + 42 * (b/BFIELD_WIDTH) + pos.y;
				CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &genRect(cellShade->h, cellShade->w, x, y));
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
	BOOST_FOREACH(const CStack *s, stacks)
	{
		if(creAnims.find(s->ID) == creAnims.end()) //eg. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->getType() != 5 && s->position >= 0) //don't show turrets here
			stackAliveByHex[s->position].push_back(s);
	}
	std::vector<const CStack *> stackDeadByHex[BFIELD_SIZE];
	BOOST_FOREACH(const CStack *s, stacks)
	{
		if(creAnims.find(s->ID) == creAnims.end()) //eg. for summoned but not yet handled stacks
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
	for(int b=0; b<BFIELD_SIZE; ++b) //showing alive stacks
	{
		for(size_t v=0; v<stackAliveByHex[b].size(); ++v)
		{
			const CStack *s = stackAliveByHex[b][v];
			
			if(!s->hasBonusOfType(Bonus::FLYING) || creAnims[s->ID]->getType() != 0)
				showAliveStack(s, to);
			else
				flyingStacks.push_back(s);
		}

		//showing obstacles
		std::pair<std::multimap<THex, int>::const_iterator, std::multimap<THex, int>::const_iterator> obstRange =
			hexToObstacle.equal_range(b);

		for(std::multimap<THex, int>::const_iterator it = obstRange.first; it != obstRange.second; ++it)
		{
			CObstacleInstance & curOb = obstacles[it->second];
			std::pair<si16, si16> shift = CGI->heroh->obstacles.find(curOb.ID)->second.posShift;
			int x = ((curOb.pos/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(curOb.pos%BFIELD_WIDTH) + pos.x + shift.first;
			int y = 86 + 42 * (curOb.pos/BFIELD_WIDTH) + pos.y + shift.second;
			std::vector<Cimage> &images = idToObstacle[curOb.ID]->ourImages; //reference to animation of obstacle
			blitAt(images[((animCount+1)/(4/curInt->sysOpts.animSpeed))%images.size()].bitmap, x, y, to);
		}

		//showing wall pieces
		showPieceOfWall(to, b, stacks);
	}
	
	for(int b=0; b<flyingStacks.size(); ++b) //showing flying stacks
		showAliveStack(flyingStacks[b], to);

	//units shown

	//showing hero animations
	if(attackingHero)
		attackingHero->show(to);
	if(defendingHero)
		defendingHero->show(to);

	projectileShowHelper(to);//showing projectiles

	//showing spell effects
	if(battleEffects.size())
	{
		for(std::list<SBattleEffect>::iterator it = battleEffects.begin(); it!=battleEffects.end(); ++it)
		{
			SDL_Surface * bitmapToBlit = it->anim->ourImages[(it->frame)%it->anim->ourImages.size()].bitmap;
			SDL_BlitSurface(bitmapToBlit, NULL, to, &genRect(bitmapToBlit->h, bitmapToBlit->w, it->x, it->y));
		}
	}

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	//showing menu background and console
	blitAt(menu, pos.x, 556 + pos.y, to);
	console->show(to);

	//showing buttons
	bOptions->show(to);
	bSurrender->show(to);
	bFlee->show(to);
	bAutofight->show(to);
	bSpell->show(to);
	bWait->show(to);
	bDefence->show(to);
	bConsoleUp->show(to);
	bConsoleDown->show(to);

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
	if(activeStack!= NULL && !spellDestSelectMode)
	{
		mouseHoveredStack = -1;
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
			CCS->curh->changeGraphic(1, 6);
			if(console->whoSetAlter == 0)
			{
				console->alterTxt = "";
			}
		}
		else
		{
			if(std::find(shadedHexes.begin(),shadedHexes.end(),myNumber) == shadedHexes.end())
			{
				const CStack *shere = curInt->cb->battleGetStackByPos(myNumber);
				const CStack *sactive = activeStack;
				if(shere)
				{
					if(shere->owner == curInt->playerID) //our stack
					{
						if(sactive->hasBonusOfType(Bonus::HEALER))
						{
							//display the possibility to heal this creature
							CCS->curh->changeGraphic(1,17);
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
						mouseHoveredStack = shere->ID;
						if(creAnims[shere->ID]->getType() == CCreatureAnim::HOLDING && creAnims[shere->ID]->framesInGroup(CCreatureAnim::MOUSEON) > 0)
						{
							creAnims[shere->ID]->playOnce(CCreatureAnim::MOUSEON);
						}
						
					}
					else if(curInt->cb->battleCanShoot(activeStack,myNumber)) //we can shoot enemy
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
						sprintf(buf, CGI->generaltexth->allTexts[296].c_str(), shere->count == 1 ? shere->getCreature()->nameSing.c_str() : shere->getCreature()->namePl.c_str(), sactive->shots, estDmg.str().c_str());
						console->alterTxt = buf;
						console->whoSetAlter = 0;
					}
					else if(isTileAttackable(myNumber)) //available enemy (melee attackable)
					{
						CCursorHandler *cursor = CCS->curh;
						const CBattleHex &hoveredHex = bfield[myNumber];

						const double subdividingAngle = 2.0*M_PI/6.0; // Divide a hex into six sectors.
						const double hexMidX = hoveredHex.pos.x + hoveredHex.pos.w/2;
						const double hexMidY = hoveredHex.pos.y + hoveredHex.pos.h/2;
						const double cursorHexAngle = M_PI - atan2(hexMidY - cursor->ypos, cursor->xpos - hexMidX) + subdividingAngle/2;
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
						if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(shadedHexes, myNumber - 1)) 
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

								if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH - 2 + zigzagCorrection))
									attackRow[0] = false;
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH - 1 + zigzagCorrection))
									attackRow[1] = false;
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH + zigzagCorrection))
									attackRow[2] = false;
								if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH + 1 + zigzagCorrection))
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
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH - 1 + zigzagCorrection))
									sectorCursor[1] = -1;
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH + zigzagCorrection))
									sectorCursor[2] = -1;
							}
						}
						// Check to the right.
						if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(shadedHexes, myNumber + 1))
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

								if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH - 2 + zigzagCorrection))
									attackRow[0] = false;
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH - 1 + zigzagCorrection))
									attackRow[1] = false;
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH + zigzagCorrection))
									attackRow[2] = false;
								if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH + 1 + zigzagCorrection))
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
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH + zigzagCorrection))
									sectorCursor[4] = -1;
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH - 1 + zigzagCorrection))
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
						while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1)
							i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
						cursor->changeGraphic(1, sectorCursor[(cursorIndex + i)%sectorCursor.size()]);

						//setting console info
						char buf[500];
						//calculating estimated dmg
						std::pair<ui32, ui32> estimatedDmg = curInt->cb->battleEstimateDamage(sactive, shere);
						std::ostringstream estDmg;
						estDmg << estimatedDmg.first << " - " << estimatedDmg.second;
						//printing
						sprintf(buf, CGI->generaltexth->allTexts[36].c_str(), shere->count == 1 ? shere->getCreature()->nameSing.c_str() : shere->getCreature()->namePl.c_str(), estDmg.str().c_str());
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
				else if( sactive && sactive->hasBonusOfType(Bonus::CATAPULT) && isCatapultAttackable(myNumber) ) //catapulting
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

	SDL_Rect temp_rect = genRect(431, 481, 160, 84);
	CBattleOptionsWindow * optionsWin = new CBattleOptionsWindow(temp_rect, this);
	GH.pushInt(optionsWin);
}

void CBattleInterface::bSurrenderf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;
}

void CBattleInterface::bFleef()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if( curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = boost::bind(&CBattleInterface::reallyFlee,this);
		curInt->showYesNoDialog(CGI->generaltexth->allTexts[28],std::vector<SComponent*>(), ony, 0, false);
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
		sprintf(buffer, CGI->generaltexth->allTexts[340].c_str(), heroName.c_str());

		//printing message
		curInt->showInfoDialog(std::string(buffer), comps);
	}
}

void CBattleInterface::reallyFlee()
{
	giveCommand(4,0,0);
	CCS->curh->changeGraphic(0, 0);
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
	}
	else
	{
		creAnims[stack->ID] = new CCreatureAnimation(stack->getCreature()->animDefName);	
	}
	creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);
	creAnims[stack->ID]->pos = Rect(coords.x, coords.y, creAnims[stack->ID]->fullWidth, creAnims[stack->ID]->fullHeight);
	creDir[stack->ID] = stack->attackerOwned;
}

void CBattleInterface::stackRemoved(const CStack * stack)
{
	int stackID = stack->ID;
	delete creAnims[stackID];
	creAnims.erase(stackID);
	creDir.erase(stackID);
}

void CBattleInterface::stackActivated(const CStack * stack)
{
	//givenCommand = NULL;
	stackToActivate = stack;
	if(pendingAnims.size() == 0)
		activateStack();
}

void CBattleInterface::stackMoved(const CStack * stack, THex destHex, bool endMoving, int distance)
{
	addNewAnim(new CBattleStackMoved(this, stack, destHex, endMoving, distance));
	waitForAnims();
}

void CBattleInterface::stacksAreAttacked(std::vector<SStackAttackedInfo> attackedInfos)
{
	for(int h = 0; h < attackedInfos.size(); ++h)
	{
		addNewAnim(new CDefenceAnim(attackedInfos[h], this));
	}
	waitForAnims();
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
	std::vector<const CStack*> stacks = curInt->cb->battleGetStacks(false);
	BOOST_FOREACH(const CStack *s, stacks)
	{
		//don't show animation when no HP is regenerated
		if (s->firstHPleft == s->MaxHealth())
		{
			continue;
		}

		if( s->hasBonusOfType(Bonus::HP_REGENERATION) && s->alive() )
			displayEffect(74, s->position);

		if( s->hasBonusOfType(Bonus::FULL_HP_REGENERATION, 0) && s->alive() )
			displayEffect(4, s->position);

		if( s->hasBonusOfType(Bonus::FULL_HP_REGENERATION, 1) && s->alive() )
			displayEffect(74, s->position);
	}
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
	case 6:
		assert(curInt->cb->battleGetStackByPos(additional)); //stack to attack must exist
	case 2: case 7: case 9:
		assert(tile < BFIELD_SIZE);
		break;
	}

	myTurn = false;
	activeStack = NULL;
	givenCommand->setn(ba);
}

bool CBattleInterface::isTileAttackable(const THex & number) const
{
	for(size_t b=0; b<shadedHexes.size(); ++b)
	{
		if(THex::mutualPosition(shadedHexes[b], number) != -1 || shadedHexes[b] == number)
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
	const CStack* dest = curInt->cb->battleGetStackByPos(whichOne); //creature at destination tile; -1 if there is no one
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
		if(spellDestSelectMode)
		{
			//checking destination
			bool allowCasting = true;
			bool onlyAlive = spellToCast->additionalInfo != 38 && spellToCast->additionalInfo != 39; //when casting resurrection or animate dead we should be allow to select dead stack
			switch(spellSelMode)
			{
			case 1:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive) || curInt->playerID != curInt->cb->battleGetStackByPos(whichOne, onlyAlive)->owner )
					allowCasting = false;
				break;
			case 2:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive) || curInt->playerID == curInt->cb->battleGetStackByPos(whichOne, onlyAlive)->owner )
					allowCasting = false;
				break;
			case 3:
				if(!curInt->cb->battleGetStackByPos(whichOne, onlyAlive))
					allowCasting = false;
				break;
			case 4:
				if(!blockedByObstacle(whichOne))
					allowCasting = false;
			case 5: //teleport
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
		else //we don't cast any spell
		{
			if(!dest || !dest->alive()) //no creature at that tile
			{
				if(std::find(shadedHexes.begin(),shadedHexes.end(),whichOne)!=shadedHexes.end())// and it's in our range
				{
					CCS->curh->changeGraphic(1, 6); //cursor should be changed
					if(activeStack->doubleWide())
					{
						std::vector<THex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
						int shiftedDest = whichOne + (activeStack->attackerOwned ? 1 : -1);
						if(vstd::contains(acc, whichOne))
							giveCommand(2,whichOne,activeStack->ID);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand(2,shiftedDest,activeStack->ID);
					}
					else
					{
						giveCommand(2,whichOne,activeStack->ID);
					}
				}
				else if(actSt->hasBonusOfType(Bonus::CATAPULT) && isCatapultAttackable(whichOne)) //attacking (catapult)
				{
					giveCommand(9,whichOne,activeStack->ID);
				}
			}
			else if(dest->owner != actSt->owner
				&& curInt->cb->battleCanShoot(activeStack, whichOne) ) //shooting
			{
				CCS->curh->changeGraphic(1, 6); //cursor should be changed
				giveCommand(7,whichOne,activeStack->ID);
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
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(actStack->attackerOwned) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								attackFromHex = destHex-1;
						}
						break;
					}
				case 7: //from bottom left
					{
						int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH-1 : BFIELD_WIDTH );
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(actStack->attackerOwned) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
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
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(actStack->attackerOwned) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								attackFromHex = destHex-1;
						}
						break;
					}
				case 10: //from top right
					{
						bool doubleWide = actStack->doubleWide();
						int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 ) +
							(actStack->attackerOwned && doubleWide ? 1 : 0);
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(actStack->attackerOwned) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
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
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								attackFromHex = destHex-1;
						}
						break;
					}
				case 14: //from top
					{
						int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 );
						if(vstd::contains(shadedHexes, destHex))
							attackFromHex = destHex;
						else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								attackFromHex = destHex+1;
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								attackFromHex = destHex-1;
						}
						break;
					}
				}

				if(attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
				{
					giveCommand(6, attackFromHex, activeStack->ID, whichOne);
					
					CCS->curh->changeGraphic(1, 6); //cursor should be changed
				}

			}
			else if (actSt->hasBonusOfType(Bonus::HEALER) && actSt->owner == dest->owner) //friendly creature we can heal
			{
				giveCommand(12, whichOne, activeStack->ID); //command healing

				CCS->curh->changeGraphic(1, 6); //cursor should be changed
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
	CCS->musich->stopMusic();
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
	case 77: //thunderbolt
		displayEffect(1, sc->tile);
		displayEffect(spell.mainEffectAnim, sc->tile);
		break;
	case 35: //dispel
	case 37: //cure
	case 38: //resurrection
	case 39: //animate dead
	case 78: //dispel helpful spells
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
	if(sc->affectedCres.size() == 1)
	{
		std::string text = CGI->generaltexth->allTexts[195];
		if(sc->castedByHero)
		{
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetFightingHero(sc->side)->name);
		}
		else
		{
			boost::algorithm::replace_first(text, "%s", "Creature"); //TODO: better fix
		}
		boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name);
		boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl );
		console->addText(text);
	}
	else
	{
		std::string text = CGI->generaltexth->allTexts[196];
		if(sc->castedByHero)
		{
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetFightingHero(sc->side)->name);
		}
		else
		{
			boost::algorithm::replace_first(text, "%s", "Creature"); //TODO: better fix
		}
		boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name);
		console->addText(text);
	}
	if(sc->dmgToDisplay != 0)
	{
		std::string dmgInfo = CGI->generaltexth->allTexts[343].substr(1, CGI->generaltexth->allTexts[343].size() - 1);
		boost::algorithm::replace_first(dmgInfo, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay));
		console->addText(dmgInfo);
	}
	waitForAnims();
}

void CBattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	int effID = sse.effect.back().id;
	if(effID != -1) //can be -1 for defensive stance effect
	{
		for(std::vector<ui32>::const_iterator ci = sse.stacks.begin(); ci!=sse.stacks.end(); ++ci)
		{
			displayEffect(CGI->spellh->spells[effID]->mainEffectAnim, curInt->cb->battleGetStackByID(*ci)->position);
		}
	}
	else if (sse.stacks.size() == 1 && sse.effect.size() == 1)
	{
		const Bonus & bns = sse.effect.back();
		if (bns.source == Bonus::OTHER && bns.type == Bonus::PRIMARY_SKILL)
		{
			//defensive stance (I hope)
			const CStack * stack = LOCPLINT->cb->battleGetStackByID(*sse.stacks.begin());
			int txtid = 120;

			if(stack->count != 1)
				txtid++; //move to plural text

			char txt[4000];
			int val = stack->Defense() - (stack->Defense() * 100 )/ (100 + bns.val);
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
	spellSelMode = 0;
	if(spell.attributes.find("CREATURE_TARGET") != std::string::npos) //spell to be cast on one specific creature
	{
		switch(spell.positiveness)
		{
		case -1 :
			spellSelMode = 2;
			break;
		case 0:
			spellSelMode = 3;
			break;
		case 1:
			spellSelMode = 1;
			break;
		}
	}
	if(spell.attributes.find("CREATURE_TARGET_1") != std::string::npos ||
		spell.attributes.find("CREATURE_TARGET_2") != std::string::npos) //spell to be cast on a specific creature but massive on expert
	{
		if(castingHero && castingHero->getSpellSchoolLevel(&spell) < 3)
		{
			switch(spell.positiveness)
			{
			case -1 :
				spellSelMode = 2;
				break;
			case 0:
				spellSelMode = 3;
				break;
			case 1:
				spellSelMode = 1;
				break;
			}
		}
		else
		{
			spellSelMode = -1;
		}
	}
	if(spell.attributes.find("OBSTACLE_TARGET") != std::string::npos) //spell to be cast on an obstacle
	{
		spellSelMode = 4;
	}
	if(spell.range[ castingHero->getSpellSchoolLevel(&spell) ] == "X") //spell has no range
	{
		spellSelMode = -1;
	}

	if(spell.id == 63) //teleport
	{
		spellSelMode = 5;
	}

	if(spell.range[ castingHero->getSpellSchoolLevel(&spell) ].size() > 1) //spell has many-hex range
	{
		spellSelMode = 0;
	}

	if(spellSelMode == -1) //user does not have to select location
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
	curInt->sysOpts.settingsChanged();
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

	int animType = creAnims[ID]->getType();

	int affectingSpeed = curInt->sysOpts.animSpeed;
	if(animType == 1 || animType == 2) //standing stacks should not stand faster :)
		affectingSpeed = 2;
	bool incrementFrame = (animCount%(4/affectingSpeed)==0) && animType!=5 && animType!=20 && animType!=2;

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

	creAnims[ID]->nextFrame(to, creAnims[ID]->pos.x, creAnims[ID]->pos.y, creDir[ID], animCount, incrementFrame, activeStack && ID==activeStack->ID, ID==mouseHoveredStack); //increment always when moving, never if stack died

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
		int xAdd = stack->attackerOwned ? 220 : 202;

		//blitting amoutn background box
		SDL_Surface *amountBG = NULL;
		BonusList spellEffects = stack->getSpellBonuses();
		if(!spellEffects.size())
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
		SDL_BlitSurface(amountBG, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[ID]->pos.x + xAdd, creAnims[ID]->pos.y + 260));
		//blitting amount
		CSDL_Ext::printAtMiddle(
			makeNumberShort(stack->count),
			creAnims[ID]->pos.x + xAdd + 15,
			creAnims[ID]->pos.y + 260 + 5,
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

	static const std::map<int, int> hexToPart = boost::assign::map_list_of(12, 8)(16, 1)(29, 7)(50, 2)(62, 12)(78, 6)(112, 10)(147, 5)(165, 11)(182, 3)(186, 0);
	
	//additionally print bottom wall
	if(hex == 182)
	{
		siegeH->printPartOfWall(to, 4);
	}

	std::map<int, int>::const_iterator it = hexToPart.find(hex);
	if(it != hexToPart.end())
	{
		siegeH->printPartOfWall(to, it->second);

		//print creature in turret
		int posToSeek = -1;
		switch(it->second)
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

void CBattleInterface::redrawBackgroundWithHexes(const CStack * activeStack)
{
	shadedHexes = curInt->cb->battleGetAvailableHexes(activeStack, true);

	//preparating background graphic with hexes and shaded hexes
	blitAt(background, 0, 0, backgroundWithHexes);
	if(curInt->sysOpts.printCellBorders)
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, backgroundWithHexes, NULL);

	if(curInt->sysOpts.printStackRange)
	{
		for(size_t m=0; m<shadedHexes.size(); ++m) //rows
		{
			int i = shadedHexes[m]/BFIELD_WIDTH; //row
			int j = shadedHexes[m]%BFIELD_WIDTH-1; //column
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 * i;
			CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, backgroundWithHexes, &genRect(cellShade->h, cellShade->w, x, y));
		}
	}
}

void CBattleInterface::printConsoleAttacked( const CStack * defender, int dmg, int killed, const CStack * attacker )
{
	char tabh[200];
	int end = sprintf(tabh, CGI->generaltexth->allTexts[attacker->count > 1 ? 377 : 376].c_str(),
		(attacker->count > 1 ? attacker->getCreature()->namePl.c_str() : attacker->getCreature()->nameSing.c_str()),
		dmg);
	if(killed > 0)
	{
		if(killed > 1)
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[379].c_str(), killed, defender->getCreature()->namePl.c_str());
		}
		else //killed == 1
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[378].c_str(), defender->getCreature()->nameSing.c_str());
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
		if(it->animStartDelay>0)
		{
			--(it->animStartDelay);
			continue;
		}
		SDL_Rect dst;
		dst.h = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->h;
		dst.w = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->w;
		dst.x = it->x;
		dst.y = it->y;
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
		//actualizing projectile
		++it->step;
		if(it->step == it->lastStep)
		{
			toBeDeleted.insert(toBeDeleted.end(), it);
		}
		else
		{
			it->x += it->dx;
			it->y += it->dy;
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
	std::set<const CStack *> stacks;
	stacks.insert(LOCPLINT->cb->battleGetStackByID(action->stackNumber));
	stacks.insert(LOCPLINT->cb->battleGetStackByPos(action->destinationTile));

	BOOST_FOREACH(const CStack *s, stacks)
	{
		if(s && creDir[s->ID] != bool(s->attackerOwned) && s->alive())
		{
			addNewAnim(new CReverseAnim(this, s, s->position, false));
		}
	}

	queue->update();
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

	//displaying heal animation
	if (action->actionType == BattleAction::STACK_HEAL)
	{
		displayEffect(50, action->destinationTile);
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
	BattleAction endt = BattleAction::makeEndOFTacticPhase(curInt->cb->battleGetMySide());
	curInt->cb->battleMakeTacticAction(&endt);

}

void CBattleInterface::bTacticNextStack()
{

}

void CBattleHero::show(SDL_Surface *to)
{
	//animation of flag
	if(flip)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&genRect(
				flag->ourImages[flagAnim].bitmap->h,
				flag->ourImages[flagAnim].bitmap->w,
				62 + pos.x,
				39 + pos.y
			)
		);
	}
	else
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&genRect(
				flag->ourImages[flagAnim].bitmap->h,
				flag->ourImages[flagAnim].bitmap->w,
				71 + pos.x,
				39 + pos.y
			)
		);
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

	if(!down && myHero && myOwner->curInt->cb->battleCanCastSpell()) //check conditions
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
	if(stack->position < 0) //creatures in turrets
	{
		const CCreature & turretCreature = *CGI->creh->creatures[ CGI->creh->factionToTurretCreature[cbi->siegeH->town->town->typeID] ];
		int xShift = turretCreature.isDoubleWide() ? 44 : 0;

		switch(stack->position)
		{
		case -2: //keep
			ret = Point(505 + xShift, -66);
			break;
		case -3: //lower turret
			ret = Point(368 + xShift, 304);
			break;
		case -4: //upper turret
			ret = Point(339 + xShift, -192);
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
				ret.x -= 42;
			}
			else
			{
				ret.x += 42;
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
			GH.pushInt(new CCreInfoWindow(*myst));
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
		SDL_BlitSurface(graphics->portraitLarge[owner->attackingHeroInstance->portrait], NULL, background, &genRect(64, 58, 21, 38));
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
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &genRect(64, 58, 21, 38));
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		SDL_BlitSurface(graphics->portraitLarge[owner->defendingHeroInstance->portrait], NULL, background, &genRect(64, 58, 392, 38));
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
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &genRect(64, 58, 392, 38));
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
		int text;
		switch(br.result)
		{
			case 0: text = 304; break;
			case 1: text = 303; break;
			case 2: text = 302; break;
		}

		CCS->musich->playMusic(musicBase::winBattle);
		#ifdef _WIN32
			CCS->videoh->open(VIDEO_WIN);
		#else
			CCS->videoh->open(VIDEO_WIN, true);
		#endif
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
	exit->show(to);
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
	pos = position;
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("comopbck.bmp", true);
	graphics->blueToPlayersAdv(hhlp, owner->curInt->playerID);
	background = SDL_ConvertSurface(hhlp, screen->format, 0);
	SDL_SetColorKey(background, SDL_SRCCOLORKEY, SDL_MapRGB(background->format,0,255,255));
	SDL_FreeSurface(hhlp);


	viewGrid = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintCellBorders, owner, true), boost::bind(&CBattleInterface::setPrintCellBorders, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[427].first)(3,CGI->generaltexth->zelp[427].first), CGI->generaltexth->zelp[427].second, false, "sysopchk.def", NULL, 185, 140, false);
	viewGrid->select(owner->curInt->sysOpts.printCellBorders);
	movementShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintStackRange, owner, true), boost::bind(&CBattleInterface::setPrintStackRange, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[428].first)(3,CGI->generaltexth->zelp[428].first), CGI->generaltexth->zelp[428].second, false, "sysopchk.def", NULL, 185, 173, false);
	movementShadow->select(owner->curInt->sysOpts.printStackRange);
	mouseShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintMouseShadow, owner, true), boost::bind(&CBattleInterface::setPrintMouseShadow, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[429].first)(3,CGI->generaltexth->zelp[429].first), CGI->generaltexth->zelp[429].second, false, "sysopchk.def", NULL, 185, 207, false);
	mouseShadow->select(owner->curInt->sysOpts.printMouseShadow);

	animSpeeds = new CHighlightableButtonsGroup(0);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[422].first),CGI->generaltexth->zelp[422].second, "sysopb9.def",188, 309, 1);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[423].first),CGI->generaltexth->zelp[423].second, "sysob10.def",252, 309, 2);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[424].first),CGI->generaltexth->zelp[424].second, "sysob11.def",315, 309, 4);
	animSpeeds->select(owner->getAnimSpeed(), 1);
	animSpeeds->onChange = boost::bind(&CBattleInterface::setAnimSpeed, owner, _1);

	setToDefault = new AdventureMapButton (CGI->generaltexth->zelp[392].first, CGI->generaltexth->zelp[392].second, boost::bind(&CBattleOptionsWindow::bDefaultf,this), 405, 443, "codefaul.def");
	setToDefault->imgs[0]->fixButtonPos();
	exit = new AdventureMapButton (CGI->generaltexth->zelp[393].first, CGI->generaltexth->zelp[393].second, boost::bind(&CBattleOptionsWindow::bExitf,this), 516, 443, "soretrn.def",SDLK_RETURN);
	exit->imgs[0]->fixButtonPos();

	//printing texts to background
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[392], 242, 32, FONT_BIG, tytulowy, background); //window title
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[393], 122, 214, FONT_MEDIUM, tytulowy, background); //animation speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[394], 122, 293, FONT_MEDIUM, tytulowy, background); //music volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[395], 122, 359, FONT_MEDIUM, tytulowy, background); //effects' volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[396], 353, 66, FONT_MEDIUM, tytulowy, background); //auto - combat options
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[397], 353, 265, FONT_MEDIUM, tytulowy, background); //creature info

		//auto - combat options
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[398], 283, 86, FONT_MEDIUM, zwykly, background); //creatures
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[399], 283, 116, FONT_MEDIUM, zwykly, background); //spells
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[400], 283, 146, FONT_MEDIUM, zwykly, background); //catapult
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[151], 283, 176, FONT_MEDIUM, zwykly, background); //ballista
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[401], 283, 206, FONT_MEDIUM, zwykly, background); //first aid tent

		//creature info
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[402], 283, 285, FONT_MEDIUM, zwykly, background); //all stats
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[403], 283, 315, FONT_MEDIUM, zwykly, background); //spells only
	
		//general options
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[404], 61, 57, FONT_MEDIUM, zwykly, background); //hex grid
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[405], 61, 90, FONT_MEDIUM, zwykly, background); //movement shadow
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[406], 61, 124, FONT_MEDIUM, zwykly, background); //cursor shadow
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[577], 61, 156, FONT_MEDIUM, zwykly, background); //spellbook animation
	//texts printed
}

CBattleOptionsWindow::~CBattleOptionsWindow()
{
	SDL_FreeSurface(background);

	delete setToDefault;
	delete exit;

	delete viewGrid;
	delete movementShadow;
	delete animSpeeds;
	delete mouseShadow;
}

void CBattleOptionsWindow::activate()
{
	setToDefault->activate();
	exit->activate();
	viewGrid->activate();
	movementShadow->activate();
	animSpeeds->activate();
	mouseShadow->activate();
}

void CBattleOptionsWindow::deactivate()
{
	setToDefault->deactivate();
	exit->deactivate();
	viewGrid->deactivate();
	movementShadow->deactivate();
	animSpeeds->deactivate();
	mouseShadow->deactivate();
}

void CBattleOptionsWindow::show(SDL_Surface *to)
{
	if(!to) //"evaluating" to
		to = screen;

	SDL_BlitSurface(background, NULL, to, &pos);

	setToDefault->show(to);
	exit->show(to);
	viewGrid->show(to);
	movementShadow->show(to);
	animSpeeds->show(to);
	mouseShadow->show(to);
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

void CBattleInterface::SiegeHelper::printPartOfWall(SDL_Surface * to, int what)
{
	Point pos = Point(-1, -1);
	switch(what)
	{
	case 1: //background wall
		pos = Point(owner->pos.w + owner->pos.x - walls[1]->w, 55 + owner->pos.y);
		break;
	case 2: //keep
		pos = Point(owner->pos.w + owner->pos.x - walls[2]->w, 154 + owner->pos.y);
		break;
	case 3: //bottom tower
	case 4: //bottom wall
	case 5: //below gate
	case 6: //over gate
	case 7: //upper wall
	case 8: //upper tower
	case 9: //gate
	case 10: //gate arch
	case 11: //bottom static wall
	case 12: //upper static wall
		pos.x = graphics->wallPositions[town->town->typeID][what - 3].x + owner->pos.x;
		pos.y = graphics->wallPositions[town->town->typeID][what - 3].y + owner->pos.y;
		break;
	case 15: //keep creature cover
		pos = Point(owner->pos.w + owner->pos.x - walls[2]->w, 154 + owner->pos.y);
		break;
	case 16: //bottom turret creature cover
		pos.x = graphics->wallPositions[town->town->typeID][0].x + owner->pos.x;
		pos.y = graphics->wallPositions[town->town->typeID][0].y + owner->pos.y;
		break;
	case 17: //upper turret creature cover
		pos.x = graphics->wallPositions[town->town->typeID][5].x + owner->pos.x;
		pos.y = graphics->wallPositions[town->town->typeID][5].y + owner->pos.y;
		break;
	};

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
		CSDL_Ext::blit8bppAlphaTo24bpp(bg, NULL, to, &genRect(bg->h, bg->w, pos.x, pos.y));
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
