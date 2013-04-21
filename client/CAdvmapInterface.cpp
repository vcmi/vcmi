#include "StdInc.h"
#include "CAdvmapInterface.h"

#include "../CCallback.h"
#include "CCastleInterface.h"
#include "gui/CCursorHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CKingdomInterface.h"
#include "CMessage.h"
#include "CPlayerInterface.h"
#include "gui/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "../lib/CConfigHandler.h"
#include "CSpellWindow.h"
#include "Graphics.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/mapping/CMap.h"
#include "../lib/JsonNode.h"
#include "mapHandler.h"
#include "CPreGame.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/CSpellHandler.h"
#include "CSoundBase.h"
#include "../lib/CGameState.h"
#include "CMusicHandler.h"
#include "gui/CGuiHandler.h"
#include "gui/CIntObjectClasses.h"
#include "../lib/UnlockGuard.h"

#ifdef _MSC_VER
#pragma warning (disable : 4355)
#endif

/*
 * CAdvMapInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#define ADVOPT (conf.go()->ac)
using namespace boost::logic;
using namespace boost::assign;
using namespace CSDL_Ext;

CAdvMapInt *adventureInt;


CTerrainRect::CTerrainRect()
	:curHoveredTile(-1,-1,-1), currentPath(NULL)
{
	tilesw=(ADVOPT.advmapW+31)/32;
	tilesh=(ADVOPT.advmapH+31)/32;
	pos.x=ADVOPT.advmapX;
	pos.y=ADVOPT.advmapY;
	pos.w=ADVOPT.advmapW;
	pos.h=ADVOPT.advmapH;
	moveX = moveY = 0;
	addUsedEvents(LCLICK | RCLICK | HOVER | MOVE);
}

void CTerrainRect::deactivate()
{
	CIntObject::deactivate();
	curHoveredTile = int3(-1,-1,-1); //we lost info about hovered tile when disabling
}

void CTerrainRect::clickLeft(tribool down, bool previousState)
{
	if ((down==false) || indeterminate(down))
		return;

	int3 mp = whichTileIsIt();
	if (mp.x<0 || mp.y<0 || mp.x >= LOCPLINT->cb->getMapSize().x || mp.y >= LOCPLINT->cb->getMapSize().y)
		return;

	adventureInt->tileLClicked(mp);
}

void CTerrainRect::clickRight(tribool down, bool previousState)
{
	int3 mp = whichTileIsIt();

	if (CGI->mh->map->isInTheMap(mp) && down)
		adventureInt->tileRClicked(mp);
}

void CTerrainRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	int3 tHovered = whichTileIsIt(sEvent.x,sEvent.y);
	int3 pom = adventureInt->verifyPos(tHovered);

	if(tHovered != pom) //tile outside the map
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		return;
	}

	if (pom != curHoveredTile)
		curHoveredTile=pom;
	else
		return;

	adventureInt->tileHovered(curHoveredTile);
}
void CTerrainRect::hover(bool on)
{
	if (!on)
	{
		adventureInt->statusbar.clear();
		CCS->curh->changeGraphic(ECursor::ADVENTURE,0);
	}
	//Hoverable::hover(on);
}
void CTerrainRect::showPath(const SDL_Rect * extRect, SDL_Surface * to)
{
	const static int pns[9][9] = {
				{16, 17, 18,  7, -1, 19,  6,  5, -1},
				{ 8,  9, 18,  7, -1, 19,  6, -1, 20},
				{ 8,  1, 10,  7, -1, 19, -1, 21, 20},
				{24, 17, 18, 15, -1, -1,  6,  5,  4},
				{-1, -1, -1, -1, -1, -1, -1, -1, -1},
				{ 8,  1,  2, -1, -1, 11, 22, 21, 20},
				{24, 17, -1, 23, -1,  3, 14,  5,  4},
				{24, -1,  2, 23, -1,  3, 22, 13,  4},
				{-1,  1,  2, 23, -1,  3, 22, 21, 12}
			}; //table of magic values TODO meaning, change variable name

	for (size_t i=0; i < currentPath->nodes.size()-1; ++i)
	{
		const int3 &curPos = currentPath->nodes[i].coord, &nextPos = currentPath->nodes[i+1].coord;
		if(curPos.z != adventureInt->position.z)
			continue;

		int pn=-1;//number of picture
		if (i==0) //last tile
		{
			int x = 32*(curPos.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(curPos.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x<0 || y<0 || x>pos.w || y>pos.h)
				continue;
			pn=0;
		}
		else
		{
			const int3 &prevPos = currentPath->nodes[i-1].coord;
			std::vector<CGPathNode> & cv = currentPath->nodes;

			/* Vector directions
			 *  0   1   2
			 *    \ | /
			 *  3 - 4 - 5
			 *    / | \
			 *  6   7  8
			 *For example:
			 *  |
			 *  |__\
			 *     /
			 * is id1=7, id2=5 (pns[7][5])
			*/
			bool pathContinuous = curPos.areNeighbours(nextPos) && curPos.areNeighbours(prevPos);
			if(pathContinuous && cv[i].land == cv[i+1].land)
			{
				int id1=(curPos.x-nextPos.x+1)+3*(curPos.y-nextPos.y+1);   //Direction of entering vector
				int id2=(cv[i-1].coord.x-curPos.x+1)+3*(cv[i-1].coord.y-curPos.y+1); //Direction of exiting vector
				pn=pns[id1][id2];
			}
			else //path discontinuity or sea/land transition (eg. when moving through Subterranean Gate or Boat)
			{
				pn = 0;
			}
		}
		if (currentPath->nodes[i].turns)
			pn+=25;
		if (pn>=0)
		{
			CDefEssential * arrows = graphics->heroMoveArrows;
			int x = 32*(curPos.x-adventureInt->position.x)+CGI->mh->offsetX + pos.x,
				y = 32*(curPos.y-adventureInt->position.y)+CGI->mh->offsetY + pos.y;
			if (x< -32 || y< -32 || x>pos.w || y>pos.h)
				continue;
			int hvx = (x+arrows->ourImages[pn].bitmap->w)-(pos.x+pos.w),
				hvy = (y+arrows->ourImages[pn].bitmap->h)-(pos.y+pos.h);

			SDL_Rect prevClip;
			SDL_GetClipRect(to, &prevClip);
			SDL_SetClipRect(to, extRect); //preventing blitting outside of that rect

			if(ADVOPT.smoothMove) //version for smooth hero move, with pos shifts
			{
				if (hvx<0 && hvy<0)
				{
					Rect dstRect = genRect(32, 32, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &dstRect);
				}
				else if(hvx<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else if (hvy<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x + moveX, y + moveY);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
			}
			else //standard version
			{
				if (hvx<0 && hvy<0)
				{
					Rect dstRect = genRect(32, 32, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, NULL, to, &dstRect);
				}
				else if(hvx<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else if (hvy<0)
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h, arrows->ourImages[pn].bitmap->w-hvx, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
				else
				{
					Rect srcRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, 0, 0);
					Rect dstRect = genRect(arrows->ourImages[pn].bitmap->h-hvy, arrows->ourImages[pn].bitmap->w-hvx, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(arrows->ourImages[pn].bitmap, &srcRect, to, &dstRect);
				}
			}
			SDL_SetClipRect(to, &prevClip);

		}
	} //for (int i=0;i<currentPath->nodes.size()-1;i++)
}

void CTerrainRect::show(SDL_Surface * to)
{
	if(ADVOPT.smoothMove)
		CGI->mh->terrainRect
			(adventureInt->position, adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
			 to, &pos, moveX, moveY, false, int3());
	else
		CGI->mh->terrainRect
			(adventureInt->position, adventureInt->anim,
			 &LOCPLINT->cb->getVisibilityMap(), true, adventureInt->heroAnim,
			 to, &pos, 0, 0, false, int3());

	//SDL_BlitSurface(teren,&genRect(pos.h,pos.w,0,0),screen,&genRect(547,594,7,6));
	//SDL_FreeSurface(teren);
	if (currentPath/* && adventureInt->position.z==currentPath->startPos().z*/) //drawing path
	{
		showPath(&pos, to);
	}
}

int3 CTerrainRect::whichTileIsIt(const int & x, const int & y)
{
	int3 ret;
	ret.x = adventureInt->position.x + ((GH.current->motion.x-CGI->mh->offsetX-pos.x)/32);
	ret.y = adventureInt->position.y + ((GH.current->motion.y-CGI->mh->offsetY-pos.y)/32);
	ret.z = adventureInt->position.z;
	return ret;
}

int3 CTerrainRect::whichTileIsIt()
{
	return whichTileIsIt(GH.current->motion.x,GH.current->motion.y);
}

void CResDataBar::clickRight(tribool down, bool previousState)
{
}

CResDataBar::CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist)
{
	bg = BitmapHandler::loadBitmap(defname);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos = genRect(bg->h, bg->w, pos.x+x, pos.y+y);

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + offx + resdist*i;
		txtpos[i].second = pos.y + offy;
	}
	txtpos[7].first = txtpos[6].first + datedist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
	+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
	addUsedEvents(RCLICK);
}

CResDataBar::CResDataBar()
{
	bg = BitmapHandler::loadBitmap(ADVOPT.resdatabarG);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos = genRect(bg->h,bg->w,ADVOPT.resdatabarX,ADVOPT.resdatabarY);

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + ADVOPT.resOffsetX + ADVOPT.resDist*i;
		txtpos[i].second = pos.y + ADVOPT.resOffsetY;
	}
	txtpos[7].first = txtpos[6].first + ADVOPT.resDateDist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
				+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
}

CResDataBar::~CResDataBar()
{
	SDL_FreeSurface(bg);
}
void CResDataBar::draw(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	for (auto i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextLeft(to, text, Colors::WHITE, Point(txtpos[i].first,txtpos[i].second));
	}
	std::vector<std::string> temp;

	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::MONTH)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK)));

	graphics->fonts[FONT_SMALL]->renderTextLeft(to, processStr(datetext,temp), Colors::WHITE, Point(txtpos[7].first,txtpos[7].second));
}

void CResDataBar::show(SDL_Surface * to)
{

}

void CResDataBar::showAll(SDL_Surface * to)
{
	draw(to);
}

CAdvMapInt::CAdvMapInt():
    minimap(Rect(ADVOPT.minimapX, ADVOPT.minimapY, ADVOPT.minimapW, ADVOPT.minimapH)),
statusbar(ADVOPT.statusbarX,ADVOPT.statusbarY,ADVOPT.statusbarG),
kingOverview(CGI->generaltexth->zelp[293].first,CGI->generaltexth->zelp[293].second,
			 boost::bind(&CAdvMapInt::fshowOverview,this),&ADVOPT.kingOverview, SDLK_k),

underground(CGI->generaltexth->zelp[294].first,CGI->generaltexth->zelp[294].second,
			boost::bind(&CAdvMapInt::fswitchLevel,this),&ADVOPT.underground, SDLK_u),

questlog(CGI->generaltexth->zelp[295].first,CGI->generaltexth->zelp[295].second,
		 boost::bind(&CAdvMapInt::fshowQuestlog,this),&ADVOPT.questlog, SDLK_q),

sleepWake(CGI->generaltexth->zelp[296].first,CGI->generaltexth->zelp[296].second,
		  boost::bind(&CAdvMapInt::fsleepWake,this), &ADVOPT.sleepWake, SDLK_w),

moveHero(CGI->generaltexth->zelp[297].first,CGI->generaltexth->zelp[297].second,
		  boost::bind(&CAdvMapInt::fmoveHero,this), &ADVOPT.moveHero, SDLK_m),

spellbook(CGI->generaltexth->zelp[298].first,CGI->generaltexth->zelp[298].second,
		  boost::bind(&CAdvMapInt::fshowSpellbok,this), &ADVOPT.spellbook, SDLK_c),

advOptions(CGI->generaltexth->zelp[299].first,CGI->generaltexth->zelp[299].second,
		  boost::bind(&CAdvMapInt::fadventureOPtions,this), &ADVOPT.advOptions, SDLK_a),

sysOptions(CGI->generaltexth->zelp[300].first,CGI->generaltexth->zelp[300].second,
		  boost::bind(&CAdvMapInt::fsystemOptions,this), &ADVOPT.sysOptions, SDLK_o),

nextHero(CGI->generaltexth->zelp[301].first,CGI->generaltexth->zelp[301].second,
		  boost::bind(&CAdvMapInt::fnextHero,this), &ADVOPT.nextHero, SDLK_h),

endTurn(CGI->generaltexth->zelp[302].first,CGI->generaltexth->zelp[302].second,
		  boost::bind(&CAdvMapInt::fendTurn,this), &ADVOPT.endTurn, SDLK_e),

heroList(ADVOPT.hlistSize, Point(ADVOPT.hlistX, ADVOPT.hlistY), ADVOPT.hlistAU, ADVOPT.hlistAD),
townList(ADVOPT.tlistSize, Point(ADVOPT.tlistX, ADVOPT.tlistY), ADVOPT.tlistAU, ADVOPT.tlistAD),
infoBar(Rect(ADVOPT.infoboxX, ADVOPT.infoboxY, 192, 192) )
{
	duringAITurn = false;
	state = NA;
	spellBeingCasted = NULL;
	pos.x = pos.y = 0;
	pos.w = screen->w;
	pos.h = screen->h;
	selection = NULL;
	townList.onSelect = boost::bind(&CAdvMapInt::selectionChanged,this);
	adventureInt=this;
	bg = BitmapHandler::loadBitmap(ADVOPT.mainGraphic);
	scrollingDir = 0;
	updateScreen  = false;
	anim=0;
	animValHitCount=0; //animation frame
	heroAnim=0;
	heroAnimValHitCount=0; // hero animation frame

	for (int g=0; g<ADVOPT.gemG.size(); ++g)
	{
		gems.push_back(CDefHandler::giveDef(ADVOPT.gemG[g]));
	}


	setPlayer(LOCPLINT->playerID);
	underground.block(!CGI->mh->map->twoLevel);
	addUsedEvents(MOVE);
}

CAdvMapInt::~CAdvMapInt()
{
	SDL_FreeSurface(bg);

	for(int i=0; i<gems.size(); i++)
		delete gems[i];
}

void CAdvMapInt::fshowOverview()
{
	GH.pushInt(new CKingdomInterface);
}
void CAdvMapInt::fswitchLevel()
{
	if(!CGI->mh->map->twoLevel)
		return;
	if (position.z)
	{
		position.z--;
		underground.setIndex(0,true);
		underground.showAll(screenBuf);
	}
	else
	{
		underground.setIndex(1,true);
		position.z++;
		underground.showAll(screenBuf);
	}
	updateScreen = true;
	minimap.setLevel(position.z);
}
void CAdvMapInt::fshowQuestlog()
{
	LOCPLINT->showQuestLog();
}
void CAdvMapInt::fsleepWake()
{
	const CGHeroInstance *h = curHero();
	if (!h)
		return;
	bool newSleep = !isHeroSleeping(h);
	setHeroSleeping(h, newSleep);
	updateSleepWake(h);
	if (newSleep)
	{
		fnextHero();

		//moveHero.block(true);
		//uncomment to enable original HoMM3 behaviour:
		//move button is disabled for hero going to sleep, even though it's enabled when you reselect him
	}
}

void CAdvMapInt::fmoveHero()
{
	const CGHeroInstance *h = curHero();
	if (!h || !terrain.currentPath)
		return;

	LOCPLINT->moveHero(h, *terrain.currentPath);
}

void CAdvMapInt::fshowSpellbok()
{
	if (!curHero()) //checking necessary values
		return;

	centerOn(selection);

	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), curHero(), LOCPLINT, false);
	GH.pushInt(spellWindow);
}

void CAdvMapInt::fadventureOPtions()
{
	GH.pushInt(new CAdventureOptions);
}

void CAdvMapInt::fsystemOptions()
{
	GH.pushInt(new CSystemOptionsWindow());
}

void CAdvMapInt::fnextHero()
{
	auto hero = dynamic_cast<const CGHeroInstance*>(selection);
	int next = getNextHeroIndex(vstd::find_pos(LOCPLINT->wanderingHeroes, hero));
	if (next < 0)
		return;
	select(LOCPLINT->wanderingHeroes[next], true);
}

void CAdvMapInt::fendTurn()
{
	if(!LOCPLINT->makingTurn)
		return;

	if ( settings["adventure"]["heroReminder"].Bool())
	{
		for (int i = 0; i < LOCPLINT->wanderingHeroes.size(); i++)
			if (!isHeroSleeping(LOCPLINT->wanderingHeroes[i]) && (LOCPLINT->wanderingHeroes[i]->movement > 0))
			{
				LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[55], boost::bind(&CAdvMapInt::endingTurn, this), 0, false);
				return;
			}
	}
	endingTurn();
}

void CAdvMapInt::updateSleepWake(const CGHeroInstance *h)
{
	sleepWake.block(!h);
	if (!h)
		return;
	bool state = isHeroSleeping(h);
	sleepWake.setIndex(state ? 1 : 0, true);
	sleepWake.assignedKeys.clear();
	sleepWake.assignedKeys.insert(state ? SDLK_w : SDLK_z);
	sleepWake.update();
}

void CAdvMapInt::updateMoveHero(const CGHeroInstance *h, tribool hasPath)
{
	//default value is for everywhere but CPlayerInterface::moveHero, because paths are not updated from there immediately
	if (hasPath == boost::indeterminate)
		 hasPath = LOCPLINT->paths[h].nodes.size() ? true : false;
	if (!h)
	{
		moveHero.block(true);
		return;
	}
	moveHero.block(!hasPath || (h->movement == 0));
}

int CAdvMapInt::getNextHeroIndex(int startIndex)
{
	if (LOCPLINT->wanderingHeroes.size() == 0)
		return -1;
	if (startIndex < 0)
		startIndex = 0;
	int i = startIndex;
	do
	{
		i++;
		if (i >= LOCPLINT->wanderingHeroes.size())
			i = 0;
	}
	while (((LOCPLINT->wanderingHeroes[i]->movement == 0) || isHeroSleeping(LOCPLINT->wanderingHeroes[i])) && (i != startIndex));

	if ((LOCPLINT->wanderingHeroes[i]->movement != 0) && !isHeroSleeping(LOCPLINT->wanderingHeroes[i]))
		return i;
	else
		return -1;
}

void CAdvMapInt::updateNextHero(const CGHeroInstance *h)
{
	int start = vstd::find_pos(LOCPLINT->wanderingHeroes, h);
	int next = getNextHeroIndex(start);
	if (next < 0)
	{
		nextHero.block(true);
		return;
	}
	const CGHeroInstance *nextH = LOCPLINT->wanderingHeroes[next];
	bool noActiveHeroes = (next == start) && ((nextH->movement == 0) || isHeroSleeping(nextH));
	nextHero.block(noActiveHeroes);
}

void CAdvMapInt::activate()
{
	CIntObject::activate();
	if (!(active & KEYBOARD))
		CIntObject::activate(KEYBOARD);

	screenBuf = screen;
	GH.statusbar = &statusbar;
	if(!duringAITurn)
	{
		kingOverview.activate();
		underground.activate();
		questlog.activate();
		sleepWake.activate();
		moveHero.activate();
		spellbook.activate();
		sysOptions.activate();
		advOptions.activate();
		nextHero.activate();
		endTurn.activate();

		minimap.activate();
		heroList.activate();
		townList.activate();
		terrain.activate();
		infoBar.activate();
		LOCPLINT->cingconsole->activate();

		GH.fakeMouseMove(); //to restore the cursor
	}
}
void CAdvMapInt::deactivate()
{
	CIntObject::deactivate();

	if(!duringAITurn)
	{
		scrollingDir = 0;

		CCS->curh->changeGraphic(ECursor::ADVENTURE,0);
		kingOverview.deactivate();
		underground.deactivate();
		questlog.deactivate();
		sleepWake.deactivate();
		moveHero.deactivate();
		spellbook.deactivate();
		advOptions.deactivate();
		sysOptions.deactivate();
		nextHero.deactivate();
		endTurn.deactivate();
		minimap.deactivate();
		heroList.deactivate();
		townList.deactivate();
		terrain.deactivate();
		infoBar.deactivate();
		LOCPLINT->cingconsole->deactivate();
	}
}
void CAdvMapInt::showAll(SDL_Surface * to)
{
	blitAt(bg,0,0,to);

	if(state != INGAME)
		return;

	kingOverview.showAll(to);
	underground.showAll(to);
	questlog.showAll(to);
	sleepWake.showAll(to);
	moveHero.showAll(to);
	spellbook.showAll(to);
	advOptions.showAll(to);
	sysOptions.showAll(to);
	nextHero.showAll(to);
	endTurn.showAll(to);

	minimap.showAll(to);
	heroList.showAll(to);
	townList.showAll(to);
	updateScreen = true;
	show(to);

	resdatabar.draw(to);

	statusbar.show(to);

	infoBar.showAll(to);
	LOCPLINT->cingconsole->showAll(to);
}

bool CAdvMapInt::isHeroSleeping(const CGHeroInstance *hero)
{
	if (!hero)
		return false;

	return vstd::contains(LOCPLINT->sleepingHeroes, hero);
}

void CAdvMapInt::setHeroSleeping(const CGHeroInstance *hero, bool sleep)
{
	if (sleep)
		LOCPLINT->sleepingHeroes += hero;
	else
		LOCPLINT->sleepingHeroes -= hero;
	updateNextHero(NULL);
}

void CAdvMapInt::show(SDL_Surface * to)
{
	if(state != INGAME)
		return;

	++animValHitCount; //for animations
	if(animValHitCount == 8)
	{
		CGI->mh->updateWater();
		animValHitCount = 0;
		++anim;
		updateScreen = true;
	}
	++heroAnim;

	int scrollSpeed = settings["adventure"]["scrollSpeed"].Float();
	//if advmap needs updating AND (no dialog is shown OR ctrl is pressed)
	if((animValHitCount % (4/scrollSpeed)) == 0
		&&  (
			(GH.topInt() == this)
			|| SDL_GetKeyState(NULL)[SDLK_LCTRL]
			|| SDL_GetKeyState(NULL)[SDLK_RCTRL]
)
	)
	{
		if( (scrollingDir & LEFT)   &&  (position.x>-CGI->mh->frameW) )
			position.x--;

		if( (scrollingDir & RIGHT)  &&  (position.x   <   CGI->mh->map->width - CGI->mh->tilesW + CGI->mh->frameW) )
			position.x++;

		if( (scrollingDir & UP)  &&  (position.y>-CGI->mh->frameH) )
			position.y--;

		if( (scrollingDir & DOWN)  &&  (position.y  <  CGI->mh->map->height - CGI->mh->tilesH + CGI->mh->frameH) )
			position.y++;

		if(scrollingDir)
		{
			updateScreen = true;
			minimap.redraw();
		}
	}
	if(updateScreen)
	{
		terrain.show(to);
		for(int i=0;i<4;i++)
			blitAt(gems[i]->ourImages[LOCPLINT->playerID.getNum()].bitmap,ADVOPT.gemX[i],ADVOPT.gemY[i],to);
		updateScreen=false;
		LOCPLINT->cingconsole->showAll(to);
	}
	infoBar.show(to);
	statusbar.showAll(to);
}

void CAdvMapInt::selectionChanged()
{
	const CGTownInstance *to = LOCPLINT->towns[townList.getSelectedIndex()];
	if (selection != to)
		select(to);
}
void CAdvMapInt::centerOn(int3 on)
{
	bool switchedLevels = on.z != position.z;

	on.x -= CGI->mh->frameW;
	on.y -= CGI->mh->frameH;

	on = LOCPLINT->repairScreenPos(on);

	position = on;
	updateScreen=true;
	underground.setIndex(on.z,true); //change underground switch button image
	underground.redraw();
	if (switchedLevels)
		minimap.setLevel(position.z);
}

void CAdvMapInt::centerOn(const CGObjectInstance *obj)
{
	centerOn(obj->getSightCenter());
}

void CAdvMapInt::keyPressed(const SDL_KeyboardEvent & key)
{
	ui8 Dir = 0;
	int k = key.keysym.sym;
	const CGHeroInstance *h = curHero(); //selected hero
	const CGTownInstance *t = curTown(); //selected town

	switch(k)
	{
	case SDLK_g:
		if(key.state != SDL_PRESSED || GH.topInt()->type & BLOCK_ADV_HOTKEYS)
			return;

		{
			//find first town with tavern
			auto itr = range::find_if(LOCPLINT->towns, boost::bind(&CGTownInstance::hasBuilt, _1, BuildingID::TAVERN));
			if(itr != LOCPLINT->towns.end())
				LOCPLINT->showThievesGuildWindow(*itr);
			else
				LOCPLINT->showInfoDialog("No available town with tavern!");
		}
		return;
	case SDLK_i:
		if(isActive())
			CAdventureOptions::showScenarioInfo();
		return;
	case SDLK_l:
		if(isActive())
			LOCPLINT->proposeLoadingGame();
		return;
	case SDLK_s:
		if(isActive())
			GH.pushInt(new CSavingScreen(CPlayerInterface::howManyPeople > 1));
		return;
	case SDLK_d:
		{
			if(h && isActive() && key.state == SDL_PRESSED)
				LOCPLINT->tryDiggging(h);
			return;
		}
	case SDLK_p:
		if(isActive())
			LOCPLINT->showPuzzleMap();
		return;
	case SDLK_r:
		if(isActive() && LOCPLINT->ctrlPressed())
		{
			LOCPLINT->showYesNoDialog("Are you sure you want to restart game?",
				[]{ LOCPLINT->sendCustomEvent(RESTART_GAME); },
				[]{}, true);
		}
		return;
	case SDLK_SPACE: //space - try to revisit current object with selected hero
		{
			if(!isActive())
				return;
			if(h && key.state == SDL_PRESSED)
			{
				auto unlockPim = vstd::makeUnlockGuard(*LOCPLINT->pim);
				LOCPLINT->cb->moveHero(h,h->pos);
			}
		}
		return;
	case SDLK_RETURN:
		{
			if(!isActive() || !selection || key.state != SDL_PRESSED)
				return;
			if(h)
				LOCPLINT->openHeroWindow(h);
			else if(t)
				LOCPLINT->openTownWindow(t);
			return;
		}
	case SDLK_ESCAPE:
		{
			if(isActive() || GH.topInt() != this || !spellBeingCasted || key.state != SDL_PRESSED)
				return;

			leaveCastingMode();
			return;
		}
	case SDLK_t:
		{
			//act on key down if marketplace windows is not already opened
			if(key.state != SDL_PRESSED || GH.topInt()->type & BLOCK_ADV_HOTKEYS)
				return;

			if(LOCPLINT->ctrlPressed()) //CTRL + T => open marketplace
			{
				//check if we have any marketplace
				const CGTownInstance *townWithMarket = NULL;
				BOOST_FOREACH(const CGTownInstance *t, LOCPLINT->cb->getTownsInfo())
				{
					if(t->hasBuilt(BuildingID::MARKETPLACE))
					{
						townWithMarket = t;
						break;
					}
				}

				if(townWithMarket) //if any town has marketplace, open window
					GH.pushInt(new CMarketplaceWindow(townWithMarket));
				else //if not - complain
					LOCPLINT->showInfoDialog("No available marketplace!");
			}
			else if(isActive()) //no ctrl, advmapint is on the top => switch to town
			{
				townList.selectNext();
			}
			return;
		}
	default:
		{
			static const int3 directions[] = {  int3(-1, +1, 0), int3(0, +1, 0), int3(+1, +1, 0),
												int3(-1, 0, 0),  int3(0, 0, 0),  int3(+1, 0, 0),
												int3(-1, -1, 0), int3(0, -1, 0), int3(+1, -1, 0) };

			//numpad arrow
			if(CGuiHandler::isArrowKey(SDLKey(k)))
				k = CGuiHandler::arrowToNum(SDLKey(k));

			k -= SDLK_KP0 + 1;
			if(k < 0 || k > 8)
				return;

			int3 dir = directions[k];

			if(!isActive() || LOCPLINT->ctrlPressed())//ctrl makes arrow move screen, not hero
			{
				Dir = (dir.x<0 ? LEFT  : 0) |
					  (dir.x>0 ? RIGHT : 0) |
					  (dir.y<0 ? UP    : 0) |
					  (dir.y>0 ? DOWN  : 0) ;
				break;
			}

			if(!h || key.state != SDL_PRESSED)
				break;

			if(k == 4)
			{
				centerOn(h);
				return;
			}

			CGPath &path = LOCPLINT->paths[h];
			terrain.currentPath = &path;
			if(!LOCPLINT->cb->getPath2(h->getPosition(false) + dir, path))
			{
				terrain.currentPath = NULL;
				return;
			}

			if (path.nodes.size() > 2)
				updateMoveHero(h);
			else
			if(!path.nodes[0].turns)
				LOCPLINT->moveHero(h, path);
		}

		return;
	}
	if(Dir && key.state == SDL_PRESSED //arrow is pressed
		&& LOCPLINT->ctrlPressed()
	)
		scrollingDir |= Dir;
	else
		scrollingDir &= ~Dir;
}
void CAdvMapInt::handleRightClick(std::string text, tribool down)
{
	if(down)
	{
		CRClickPopup::createAndPush(text);
	}
}
int3 CAdvMapInt::verifyPos(int3 ver)
{
	if (ver.x<0)
		ver.x=0;
	if (ver.y<0)
		ver.y=0;
	if (ver.z<0)
		ver.z=0;
	if (ver.x>=CGI->mh->sizes.x)
		ver.x=CGI->mh->sizes.x-1;
	if (ver.y>=CGI->mh->sizes.y)
		ver.y=CGI->mh->sizes.y-1;
	if (ver.z>=CGI->mh->sizes.z)
		ver.z=CGI->mh->sizes.z-1;
	return ver;
}

void CAdvMapInt::select(const CArmedInstance *sel, bool centerView /*= true*/)
{
	assert(sel);
	LOCPLINT->cb->setSelection(sel);
	selection = sel;
	if (LOCPLINT->battleInt == NULL && LOCPLINT->makingTurn)
	{
		auto pos = sel->visitablePos();
		auto tile = LOCPLINT->cb->getTile(pos);
		if(tile)
			CCS->musich->playMusicFromSet("terrain", tile->terType, true);
	}
	if(centerView)
		centerOn(sel);

	terrain.currentPath = NULL;
	if(sel->ID==Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance*>(sel);

		infoBar.showTownSelection(town);
		townList.select(town);
		heroList.select(nullptr);

		updateSleepWake(NULL);
		updateMoveHero(NULL);
	}
	else //hero selected
	{
		auto hero = dynamic_cast<const CGHeroInstance*>(sel);

		infoBar.showHeroSelection(hero);
		heroList.select(hero);
		townList.select(nullptr);

		terrain.currentPath = LOCPLINT->getAndVerifyPath(hero);

		updateSleepWake(hero);
		updateMoveHero(hero);
	}
	townList.redraw();
	heroList.redraw();
}

void CAdvMapInt::mouseMoved( const SDL_MouseMotionEvent & sEvent )
{
	//adventure map scrolling with mouse
	if(!SDL_GetKeyState(NULL)[SDLK_LCTRL]  &&  isActive())
	{
		if(sEvent.x<15)
		{
			scrollingDir |= LEFT;
		}
		else
		{
			scrollingDir &= ~LEFT;
		}
		if(sEvent.x>screen->w-15)
		{
			scrollingDir |= RIGHT;
		}
		else
		{
			scrollingDir &= ~RIGHT;
		}
		if(sEvent.y<15)
		{
			scrollingDir |= UP;
		}
		else
		{
			scrollingDir &= ~UP;
		}
		if(sEvent.y>screen->h-15)
		{
			scrollingDir |= DOWN;
		}
		else
		{
			scrollingDir &= ~DOWN;
		}
	}
}

bool CAdvMapInt::isActive()
{
	return active & ~CIntObject::KEYBOARD;
}

void CAdvMapInt::startHotSeatWait(PlayerColor Player)
{
	state = WAITING;
}

void CAdvMapInt::setPlayer(PlayerColor Player)
{
	player = Player;
	graphics->blueToPlayersAdv(bg,player);

	kingOverview.setPlayerColor(player);
	underground.setPlayerColor(player);
	questlog.setPlayerColor(player);
	sleepWake.setPlayerColor(player);
	moveHero.setPlayerColor(player);
	spellbook.setPlayerColor(player);
	sysOptions.setPlayerColor(player);
	advOptions.setPlayerColor(player);
	nextHero.setPlayerColor(player);
	endTurn.setPlayerColor(player);
	graphics->blueToPlayersAdv(resdatabar.bg,player);

	//heroList.updateHList();
	//townList.genList();
}

void CAdvMapInt::startTurn()
{
	state = INGAME;
	if(LOCPLINT->cb->getCurrentPlayer() == LOCPLINT->playerID)
	{
		adjustActiveness(false);
		minimap.setAIRadar(false);
	}
}

void CAdvMapInt::endingTurn()
{
	if(LOCPLINT->cingconsole->active)
		LOCPLINT->cingconsole->deactivate();
	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
}

const CGObjectInstance* CAdvMapInt::getBlockingObject(const int3 &mapPos)
{
	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mapPos);  //blocking objects at tile

	if (bobjs.empty())
		return nullptr;

	if (bobjs.back()->ID == Obj::HERO)
		return bobjs.back();
	else
		return bobjs.front();
}

void CAdvMapInt::tileLClicked(const int3 &mapPos)
{
	if(!LOCPLINT->cb->isVisible(mapPos) || !LOCPLINT->makingTurn)
		return;

	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mapPos);//blocking objects at tile
	const TerrainTile *tile = LOCPLINT->cb->getTile(mapPos);

	const CGObjectInstance *topBlocking = getBlockingObject(mapPos);

	int3 selPos = selection->getSightCenter();
	if(spellBeingCasted && isInScreenRange(selPos, mapPos))
	{
		const TerrainTile *heroTile = LOCPLINT->cb->getTile(selPos);

		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT: //Scuttle Boat
			if(topBlocking && topBlocking->ID == Obj::BOAT)
				leaveCastingMode(true, mapPos);
			break;
		case SpellID::DIMENSION_DOOR:
			if(!tile || tile->isClear(heroTile))
				leaveCastingMode(true, mapPos);
			break;
		}
		return;
	}
	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == Obj::HERO && topBlocking->tempOwner == LOCPLINT->playerID;
	canSelect |= topBlocking && topBlocking->ID == Obj::TOWN && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner);

	if (selection->ID != Obj::HERO) //hero is not selected (presumably town)
	{
		assert(!terrain.currentPath); //path can be active only when hero is selected
		if(selection == topBlocking) //selected town clicked
			LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if ( canSelect )
				select(static_cast<const CArmedInstance*>(topBlocking), false);
		return;
	}
	else if(const CGHeroInstance * currentHero = curHero()) //hero is selected
	{
		const CGPathNode *pn = LOCPLINT->cb->getPathInfo(mapPos);
		if(currentHero == topBlocking) //clicked selected hero
		{
			LOCPLINT->openHeroWindow(currentHero);
			return;
		}
		else if(canSelect && pn->turns == 255 ) //selectable object at inaccessible tile
		{
			select(static_cast<const CArmedInstance*>(topBlocking), false);
			return;
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			if (terrain.currentPath  &&  terrain.currentPath->endPos() == mapPos)//we'll be moving
			{
				LOCPLINT->moveHero(currentHero,*terrain.currentPath);
				return;
			}
			else/* if(mp.z == currentHero->pos.z)*/ //remove old path and find a new one if we clicked on the map level on which hero is present
			{
				CGPath &path = LOCPLINT->paths[currentHero];
				terrain.currentPath = &path;
				bool gotPath = LOCPLINT->cb->getPath2(mapPos, path); //try getting path, erase if failed
				updateMoveHero(currentHero);
				if (!gotPath)
					LOCPLINT->eraseCurrentPathOf(currentHero);
				else
					return;
			}
		}
	} //end of hero is selected "case"
	else
	{
		throw std::runtime_error("Nothing is selected...");
	}

	if(const IShipyard *shipyard = ourInaccessibleShipyard(topBlocking))
	{
		LOCPLINT->showShipyardDialogOrProblemPopup(shipyard);
	}
}

void CAdvMapInt::tileHovered(const int3 &mapPos)
{
	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		statusbar.clear();
		return;
	}
	const CGObjectInstance *objAtTile = getBlockingObject(mapPos);

	//std::vector<std::string> temp = LOCPLINT->cb->getObjDescriptions(mapPos);
	if (objAtTile)
	{
		std::string text = objAtTile->getHoverText();
		boost::replace_all(text,"\n"," ");
		statusbar.print(text);
	}
	else
	{
		std::string hlp;
		CGI->mh->getTerrainDescr(mapPos, hlp, false);
		statusbar.print(hlp);
	}

	const CGPathNode *pnode = LOCPLINT->cb->getPathInfo(mapPos);

	int turns = pnode->turns;
	vstd::amin(turns, 3);

	if(!selection) //may occur just at the start of game (fake move before full intiialization)
		return;

	if(spellBeingCasted)
	{
		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT:
			if(objAtTile && objAtTile->ID == Obj::BOAT)
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 42);
			else
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
			return;
		case SpellID::DIMENSION_DOOR:
			{
				const TerrainTile *t = LOCPLINT->cb->getTile(mapPos, false);
				int3 hpos = selection->getSightCenter();
				if((!t  ||  t->isClear(LOCPLINT->cb->getTile(hpos)))   &&   isInScreenRange(hpos, mapPos))
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 41);
				else
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
				return;
			}
		}
	}

	const bool guardingCreature = CGI->mh->map->isInTheMap(LOCPLINT->cb->guardingCreaturePosition(mapPos));

	if(selection->ID == Obj::TOWN)
	{
		if(objAtTile)
		{
			if(objAtTile->ID == Obj::TOWN && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner) != PlayerRelations::ENEMIES)
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 3);
			else if(objAtTile->ID == Obj::HERO && objAtTile->tempOwner == LOCPLINT->playerID)
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 2);
			else
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		}
		else
			CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
	}
	else if(const CGHeroInstance *h = curHero())
	{
		bool accessible  =  pnode->turns < 255;

		if(objAtTile)
		{
			if(objAtTile->ID == Obj::HERO)
			{
				if(!LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, objAtTile->tempOwner)) //enemy hero
				{
					if(accessible)
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 5 + turns*6);
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
				}
				else //our or ally hero
				{
					if(selection == objAtTile)
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 2);
					else if(accessible)
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 8 + turns*6);
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 2);
				}
			}
			else if(objAtTile->ID == Obj::TOWN)
			{
				if(!LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, objAtTile->tempOwner)) //enemy town
				{
					if(accessible)
					{
						const CGTownInstance* townObj = dynamic_cast<const CGTownInstance*>(objAtTile);

						// Show movement cursor for unguarded enemy towns, otherwise attack cursor.
						if (townObj && !townObj->armedGarrison())
							CCS->curh->changeGraphic(ECursor::ADVENTURE, 9 + turns*6);
						else
							CCS->curh->changeGraphic(ECursor::ADVENTURE, 5 + turns*6);

					}
					else
					{
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
					}
				}
				else //our or ally town
				{
					if(accessible)
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 9 + turns*6);
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 3);
				}
			}
			else if(objAtTile->ID == Obj::BOAT)
			{
				if(accessible)
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 6 + turns*6);
				else
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
			}
			else if (objAtTile->ID == Obj::GARRISON || objAtTile->ID == Obj::GARRISON2)
			{
				if (accessible)
				{
					const CGGarrison* garrObj = dynamic_cast<const CGGarrison*>(objAtTile); //TODO evil evil cast!

					// Show battle cursor for guarded enemy garrisons, otherwise movement cursor.
					if (garrObj  &&  garrObj->stacksCount()
						&& !LOCPLINT->cb->getPlayerRelations( LOCPLINT->playerID, garrObj->tempOwner) )
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 5 + turns*6);
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 9 + turns*6);
				}
				else
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
			}
			else if (guardingCreature && accessible) //(objAtTile->ID == 54) //monster
			{
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 5 + turns*6);
			}
			else
			{
				if(accessible)
				{
					if(pnode->land)
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 9 + turns*6);
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 28 + turns);
				}
				else
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
			}
		}
		else //no objs
		{
			if(accessible/* && pnode->accessible != CGPathNode::FLYABLE*/)
			{
				if (guardingCreature)
				{
					CCS->curh->changeGraphic(ECursor::ADVENTURE, 5 + turns*6);
				} 
				else
				{
					if(pnode->land)
					{
                        if(LOCPLINT->cb->getTile(h->getPosition(false))->terType != ETerrainType::WATER)
							CCS->curh->changeGraphic(ECursor::ADVENTURE, 4 + turns*6);
						else
							CCS->curh->changeGraphic(ECursor::ADVENTURE, 7 + turns*6); //anchor
					}
					else
						CCS->curh->changeGraphic(ECursor::ADVENTURE, 6 + turns*6);
				}
			}
			else
				CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		}
	}

	if(ourInaccessibleShipyard(objAtTile))
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 6);
	}
}

void CAdvMapInt::tileRClicked(const int3 &mapPos)
{
	if(spellBeingCasted)
	{
		leaveCastingMode();
		return;
	}
	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CRClickPopup::createAndPush(VLC->generaltexth->allTexts[61]); //Uncharted Territory
		return;
	}

	const CGObjectInstance * obj = getBlockingObject(mapPos);
	if(!obj)
	{
		// Bare or undiscovered terrain
		const TerrainTile * tile = LOCPLINT->cb->getTile(mapPos);
		if (tile)
		{
			std::string hlp;
			CGI->mh->getTerrainDescr(mapPos, hlp, true);
			CRClickPopup::createAndPush(hlp);
		}
		return;
	}

	CRClickPopup::createAndPush(obj, GH.current->motion, CENTER);
}

void CAdvMapInt::enterCastingMode(const CSpell * sp)
{
	assert(sp->id == SpellID::SCUTTLE_BOAT  ||  sp->id == SpellID::DIMENSION_DOOR);
	spellBeingCasted = sp;

	deactivate();
	terrain.activate();
	GH.fakeMouseMove();
}

void CAdvMapInt::leaveCastingMode(bool cast /*= false*/, int3 dest /*= int3(-1, -1, -1)*/)
{
	assert(spellBeingCasted);
	SpellID id = spellBeingCasted->id;
	spellBeingCasted = NULL;
	terrain.deactivate();
	activate();

	if(cast)
		LOCPLINT->cb->castSpell(curHero(), id, dest);
	else
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

const CGHeroInstance * CAdvMapInt::curHero() const
{
	if(selection && selection->ID == Obj::HERO)
		return static_cast<const CGHeroInstance *>(selection);
	else
		return NULL;
}

const CGTownInstance * CAdvMapInt::curTown() const
{
	if(selection && selection->ID == Obj::TOWN)
		return static_cast<const CGTownInstance *>(selection);
	else
		return NULL;
}

const IShipyard * CAdvMapInt::ourInaccessibleShipyard(const CGObjectInstance *obj) const
{
	const IShipyard *ret = IShipyard::castFrom(obj);

	if(!ret || obj->tempOwner != player || CCS->curh->type || (CCS->curh->frame != 6 && CCS->curh->frame != 0))
		return NULL;

	return ret;
}

void CAdvMapInt::aiTurnStarted()
{
	adjustActiveness(true);
	CCS->musich->playMusicFromSet("enemy-turn", true);
	adventureInt->minimap.setAIRadar(true);
	adventureInt->infoBar.startEnemyTurn(LOCPLINT->cb->getCurrentPlayer());
	adventureInt->infoBar.showAll(screen);//force refresh on inactive object
}

void CAdvMapInt::adjustActiveness(bool aiTurnStart)
{
	bool wasActive = isActive();

	if(wasActive) 
		deactivate();
	adventureInt->duringAITurn = aiTurnStart;
	if(wasActive) 
		activate();
}

CAdventureOptions::CAdventureOptions():
    CWindowObject(PLAYER_COLORED, "ADVOPTS")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	exit = new CAdventureMapButton("","",boost::bind(&CAdventureOptions::close, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	scenInfo = new CAdventureMapButton("","", boost::bind(&CAdventureOptions::close, this), 24, 198, "ADVINFO.DEF",SDLK_i);
	scenInfo->callback += CAdventureOptions::showScenarioInfo;
	//viewWorld = new CAdventureMapButton("","",boost::bind(&CGuiHandler::popIntTotally, &GH, this), 204, 313, "IOK6432.DEF",SDLK_RETURN);

	puzzle = new CAdventureMapButton("","", boost::bind(&CAdventureOptions::close, this), 24, 81, "ADVPUZ.DEF");
	puzzle->callback += boost::bind(&CPlayerInterface::showPuzzleMap, LOCPLINT);

	dig = new CAdventureMapButton("","", boost::bind(&CAdventureOptions::close, this), 24, 139, "ADVDIG.DEF");
	if(const CGHeroInstance *h = adventureInt->curHero())
		dig->callback += boost::bind(&CPlayerInterface::tryDiggging, LOCPLINT, h);
	else
		dig->block(true);
}

void CAdventureOptions::showScenarioInfo()
{
	GH.pushInt(new CScenarioInfo(LOCPLINT->cb->getMapHeader(), LOCPLINT->cb->getStartInfo()));
}
