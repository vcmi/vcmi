#include "StdInc.h"
#include "CClickableHex.h"
#include "CBattleInterface.h"
#include "../../lib/BattleState.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../../lib/CTownHandler.h"
#include "../Graphics.h"
#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../SDL_Extensions.h"
#include "../GUIClasses.h"
#include "CBattleConsole.h"
#include "../UIFramework/CGuiHandler.h"

SPoint CClickableHex::getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * stack, const CBattleInterface * cbi)
{
	SPoint ret(-500, -500); //returned value
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
		ret.y = -139 + 42 * (hexNum/GameConstants::BFIELD_WIDTH); //counting y
		//counting x
		if(attacker)
		{
			ret.x = -160 + 22 * ( ((hexNum/GameConstants::BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % GameConstants::BFIELD_WIDTH);
		}
		else
		{
			ret.x = -219 + 22 * ( ((hexNum/GameConstants::BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % GameConstants::BFIELD_WIDTH);
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
void CClickableHex::activate()
{
	activateHover();
	activateMouseMove();
	activateLClick();
	activateRClick();
}

void CClickableHex::deactivate()
{
	deactivateHover();
	deactivateMouseMove();
	deactivateLClick();
	deactivateRClick();
}

void CClickableHex::hover(bool on)
{
	hovered = on;
	//Hoverable::hover(on);
	if(!on && setAlterText)
	{
		myInterface->console->alterTxt = std::string();
		setAlterText = false;
	}
}

CClickableHex::CClickableHex() : setAlterText(false), myNumber(-1), accessible(true), hovered(false), strictHovered(false), myInterface(NULL)
{
}

void CClickableHex::mouseMoved(const SDL_MouseMotionEvent &sEvent)
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

void CClickableHex::clickLeft(tribool down, bool previousState)
{
	if(!down && hovered && strictHovered) //we've been really clicked!
	{
		myInterface->hexLclicked(myNumber);
	}
}

void CClickableHex::clickRight(tribool down, bool previousState)
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