#include "StdInc.h"
#include "CSpellEffectAnimation.h"

#include "../Graphics.h"
#include "CBattleInterface.h"
#include "../CDefHandler.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"
#include "../../lib/BattleState.h"
#include "../SDL_Extensions.h"
#include "../UIFramework/SRect.h"

//effect animation
bool CSpellEffectAnimation::init()
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
				for (size_t v = 0; v < anim->ourImages.size(); ++v)
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
						for (size_t v = 0; v < be.anim->ourImages.size(); ++v)
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
			SRect &tilePos = owner->bfield[destTile].pos;
			SBattleEffect be;
			be.effectID = ID;
			if(customAnim.size())
				be.anim = CDefHandler::giveDef(customAnim);
			else
				be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);

			if (Vflip)
			{
				for (size_t v = 0; v < be.anim->ourImages.size(); ++v)
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

void CSpellEffectAnimation::nextFrame()
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

void CSpellEffectAnimation::endAnim()
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

	for(size_t b = 0; b < toDel.size(); ++b)
	{
		delete toDel[b]->anim;
		owner->battleEffects.erase(toDel[b]);
	}

	delete this;
}

CSpellEffectAnimation::CSpellEffectAnimation(CBattleInterface * _owner, ui32 _effect, SBattleHex _destTile, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(_effect), destTile(_destTile), customAnim(""), dx(_dx), dy(_dy), Vflip(_Vflip)
{
}

CSpellEffectAnimation::CSpellEffectAnimation(CBattleInterface * _owner, std::string _customAnim, int _x, int _y, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(-1), destTile(0), customAnim(_customAnim), x(_x), y(_y), dx(_dx), dy(_dy), Vflip(_Vflip)
{
}