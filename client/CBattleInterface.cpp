#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "../hch/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CSpellHandler.h"
#include "CMessage.h"
#include "CCursorHandler.h"
#include "../CCallback.h"
#include "../lib/CGameState.h"
#include "../hch/CGeneralTextHandler.h"
#include "CCreatureAnimation.h"
#include "Graphics.h"
#include "CSpellWindow.h"
#include "CConfigHandler.h"
#include <queue>
#include <sstream>
#include "../lib/CondSh.h"
#include "../lib/NetPacks.h"
#include "CPlayerInterface.h"
#include "../hch/CVideoHandler.h"
#include "../hch/CTownHandler.h"
#include <boost/assign/list_of.hpp>
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
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM, *GEOR16;
extern SDL_Color zwykly;

BattleSettings CBattleInterface::settings;

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

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect)
	: attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), activeStack(-1), 
	  mouseHoveredStack(-1), previouslyHoveredHex(-1), currentlyHoveredHex(-1), spellDestSelectMode(false),
	  spellToCast(NULL), attackingInfo(NULL), givenCommand(NULL), myTurn(false), resWindow(NULL), 
	  showStackQueue(false), moveStarted(false), moveSh(-1), siegeH(NULL)
{
	pos = myRect;
	strongInterest = true;
	givenCommand = new CondSh<BattleAction *>(NULL);
	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks();
	for(std::map<int, CStack>::iterator b=stacks.begin(); b!=stacks.end(); ++b)
	{
		newStack(b->second.ID);
	}

	
	//preparing siege info
	const CGTownInstance * town = LOCPLINT->cb->battleGetDefendedTown();
	if(town)
	{
		siegeH = new SiegeHelper(town, this);
	}

	//preparing menu background and terrain
	if(siegeH)
	{
		background = BitmapHandler::loadBitmap( siegeH->getSiegeName(0) );
	}
	else
	{
		std::vector< std::string > & backref = graphics->battleBacks[ LOCPLINT->cb->battleGetBattlefieldType() ];
		background = BitmapHandler::loadBitmap(backref[ rand() % backref.size()] );
	}
	
	//preparign menu background
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
	bAutofight  = new AdventureMapButton (CGI->generaltexth->zelp[382].first, CGI->generaltexth->zelp[382].second, boost::bind(&CBattleInterface::bAutofightf,this), 157 + pos.x, 561 + pos.y, "icm004.def", SDLK_a);
	bSpell = new AdventureMapButton (CGI->generaltexth->zelp[385].first, CGI->generaltexth->zelp[385].second, boost::bind(&CBattleInterface::bSpellf,this), 645 + pos.x, 561 + pos.y, "icm005.def", SDLK_c);
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

	//loading hero animations
	if(hero1) // attacking hero
	{
		attackingHero = new CBattleHero(graphics->battleHeroes[hero1->type->heroType], 0, 0, false, hero1->tempOwner, hero1->tempOwner == LOCPLINT->playerID ? hero1 : NULL, this);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, -40 + pos.x, pos.y);
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		defendingHero = new CBattleHero(graphics->battleHeroes[hero2->type->heroType], 0, 0, true, hero2->tempOwner, hero2->tempOwner == LOCPLINT->playerID ? hero2 : NULL, this);
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
		bfield[h].accesible = true;
		bfield[h].myInterface = this;
	}
	//locking occupied positions on batlefield
	for(std::map<int, CStack>::iterator it = stacks.begin(); it!=stacks.end(); ++it) //stacks gained at top of this function
	{
		bfield[it->second.position].accesible = false;
	}

	//loading projectiles for units
	for(std::map<int, CStack>::iterator g = stacks.begin(); g != stacks.end(); ++g)
	{
		if(g->second.creature->isShooting() && CGI->creh->idToProjectile[g->second.creature->idNumber] != std::string())
		{
			idToProjectile[g->second.creature->idNumber] = CDefHandler::giveDef(CGI->creh->idToProjectile[g->second.creature->idNumber]);

			if(idToProjectile[g->second.creature->idNumber]->ourImages.size() > 2) //add symmetric images
			{
				for(int k = idToProjectile[g->second.creature->idNumber]->ourImages.size()-2; k > 1; --k)
				{
					Cimage ci;
					ci.bitmap = CSDL_Ext::rotate01(idToProjectile[g->second.creature->idNumber]->ourImages[k].bitmap);
					ci.groupNumber = 0;
					ci.imName = std::string();
					idToProjectile[g->second.creature->idNumber]->ourImages.push_back(ci);
				}
			}
			for(int s=0; s<idToProjectile[g->second.creature->idNumber]->ourImages.size(); ++s) //alpha transforming
			{
				CSDL_Ext::alphaTransform(idToProjectile[g->second.creature->idNumber]->ourImages[s].bitmap);
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
	std::vector<CObstacleInstance> obst = LOCPLINT->cb->battleGetAllObstacles();
	for(int t=0; t<obst.size(); ++t)
	{
		idToObstacle[obst[t].ID] = CDefHandler::giveDef(CGI->heroh->obstacles[obst[t].ID].defName);
		for(int n=0; n<idToObstacle[obst[t].ID]->ourImages.size(); ++n)
		{
			SDL_SetColorKey(idToObstacle[obst[t].ID]->ourImages[n].bitmap, SDL_SRCCOLORKEY, SDL_MapRGB(idToObstacle[obst[t].ID]->ourImages[n].bitmap->format,0,255,255));
		}
	}
}

CBattleInterface::~CBattleInterface()
{
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

	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	for(std::map< int, CCreatureAnimation * >::iterator g=creAnims.begin(); g!=creAnims.end(); ++g)
		delete g->second;

	for(std::map< int, CDefHandler * >::iterator g=idToProjectile.begin(); g!=idToProjectile.end(); ++g)
		delete g->second;

	for(std::map< int, CDefHandler * >::iterator g=idToObstacle.begin(); g!=idToObstacle.end(); ++g)
		delete g->second;

	delete siegeH;
	LOCPLINT->battleInt = NULL;
}

void CBattleInterface::setPrintCellBorders(bool set)
{
	settings.printCellBorders = set;
	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintStackRange(bool set)
{
	settings.printStackRange = set;
	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintMouseShadow(bool set)
{
	settings.printMouseShadow = set;
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

	LOCPLINT->cingconsole->deactivate();
}

void CBattleInterface::show(SDL_Surface * to)
{
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks(); //used in a few places
	++animCount;
	if(!to) //"evaluating" to
		to = screen;
	
	SDL_Rect buf;
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);
	//printing background and hexes
	if(activeStack != -1 && creAnims[activeStack]->getType() != 0) //show everything with range
	{
		blitAt(backgroundWithHexes, pos.x, pos.y, to);
	}
	else
	{
		//showing background
		blitAt(background, pos.x, pos.y, to);
		if(settings.printCellBorders)
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
				const CSpell & spToCast =  CGI->spellh->spells[spellToCast->additionalInfo];
				ui8 schoolLevel = 0;
				if( LOCPLINT->cb->battleGetStackByID(activeStack)->attackerOwned )
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
					if(settings.printMouseShadow && (*it % BFIELD_WIDTH != 0) && (*it % BFIELD_WIDTH != 16))
					{
						int x = 14 + ((*it/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(*it%BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (*it/BFIELD_WIDTH) + pos.y;
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &genRect(cellShade->h, cellShade->w, x, y));
					}
				}
			}
			else if(settings.printMouseShadow) //when not casting spell
			{
				int x = 14 + ((b/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(b%BFIELD_WIDTH) + pos.x;
				int y = 86 + 42 * (b/BFIELD_WIDTH) + pos.y;
				CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &genRect(cellShade->h, cellShade->w, x, y));
			}
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

	//prevents blitting outside this window
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	//showing obstacles
	std::vector<CObstacleInstance> obstacles = LOCPLINT->cb->battleGetAllObstacles();
	for(int b=0; b<obstacles.size(); ++b)
	{
		int x = ((obstacles[b].pos/BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(obstacles[b].pos%BFIELD_WIDTH) + pos.x;
		int y = 86 + 42 * (obstacles[b].pos/BFIELD_WIDTH) + pos.y;
		std::vector<Cimage> &images = idToObstacle[obstacles[b].ID]->ourImages; //reference to animation of obstacle
		blitAt(images[((animCount+1)/(4/settings.animSpeed))%images.size()].bitmap, x, y, to);
	}

	//showing siege background
	if(siegeH)
	{
		siegeH->printSiegeBackground(to);
	}

	//showing hero animations
	if(attackingHero)
		attackingHero->show(to);
	if(defendingHero)
		defendingHero->show(to);

	////showing units //a lot of work...
	std::vector<int> stackAliveByHex[BFIELD_SIZE];
	//double loop because dead stacks should be printed first
	for(std::map<int, CStack>::iterator j=stacks.begin(); j!=stacks.end(); ++j)
	{
		if(j->second.alive())
			stackAliveByHex[j->second.position].push_back(j->second.ID);
	}
	std::vector<int> stackDeadByHex[BFIELD_SIZE];
	for(std::map<int, CStack>::iterator j=stacks.begin(); j!=stacks.end(); ++j)
	{
		if(!j->second.alive())
			stackDeadByHex[j->second.position].push_back(j->second.ID);
	}

	attackingShowHelper(); // handle attack animation

	for(int b=0; b<BFIELD_SIZE; ++b) //showing dead stacks
	{
		for(size_t v=0; v<stackDeadByHex[b].size(); ++v)
		{
			creAnims[stackDeadByHex[b][v]]->nextFrame(to, creAnims[stackDeadByHex[b][v]]->pos.x + pos.x, creAnims[stackDeadByHex[b][v]]->pos.y + pos.y, creDir[stackDeadByHex[b][v]], animCount, false); //increment always when moving, never if stack died
		}
	}
	for(int b=0; b<BFIELD_SIZE; ++b) //showing alive stacks
	{
		for(size_t v=0; v<stackAliveByHex[b].size(); ++v)
		{
			int curStackID = stackAliveByHex[b][v];
			
			showAliveStack(stackAliveByHex[b][v], stacks, to);
		}

		showPieceOfWall(to, b);
	}
	//units shown
	projectileShowHelper(to);//showing projectiles

	//showing spell effects
	if(battleEffects.size())
	{
		std::vector< std::list<SBattleEffect>::iterator > toErase;
		for(std::list<SBattleEffect>::iterator it = battleEffects.begin(); it!=battleEffects.end(); ++it)
		{
			SDL_Surface * bitmapToBlit = it->anim->ourImages[(it->frame)%it->anim->ourImages.size()].bitmap;
			SDL_BlitSurface(bitmapToBlit, NULL, to, &genRect(bitmapToBlit->h, bitmapToBlit->w, pos.x + it->x, pos.y + it->y));
			++(it->frame);

			if(it->frame == it->maxFrame)
			{
				toErase.push_back(it);
			}
		}
		for(size_t b=0; b<toErase.size(); ++b)
		{
			delete toErase[b]->anim;
			battleEffects.erase(toErase[b]);
		}
	}
	

	//showing queue of stacks
	if(showStackQueue)
	{
		int xPos = screen->w/2 - ( stacks.size() * 37 )/2;
		int yPos = (screen->h - 600)/2 + 10;

		std::vector<CStack> stacksSorted;
		stacksSorted = LOCPLINT->cb->battleGetStackQueue();
		int startFrom = -1;
		for(size_t n=0; n<stacksSorted.size(); ++n)
		{
			if(stacksSorted[n].ID == activeStack)
			{
				startFrom = n;
				break;
			}
		}
		if(startFrom != -1)
		{
			for(size_t b=startFrom; b<stacksSorted.size()+startFrom; ++b)
			{
				SDL_BlitSurface(graphics->smallImgs[-2], NULL, to, &genRect(32, 32, xPos, yPos));
				//printing colored border
				for(int xFrom = xPos-1; xFrom<xPos+33; ++xFrom)
				{
					for(int yFrom = yPos-1; yFrom<yPos+33; ++yFrom)
					{
						if(xFrom == xPos-1 || xFrom == xPos+32 || yFrom == yPos-1 || yFrom == yPos+32)
						{
							SDL_Color pc;
							if(stacksSorted[b % stacksSorted.size()].owner != 255)
							{
								pc = graphics->playerColors[stacksSorted[b % stacksSorted.size()].owner];
							}
							else
							{
								pc = *graphics->neutralColor;
							}
							CSDL_Ext::SDL_PutPixelWithoutRefresh(to, xFrom, yFrom, pc.r, pc.g, pc.b);
						}
					}
				}
				//colored border printed
				SDL_BlitSurface(graphics->smallImgs[stacksSorted[b % stacksSorted.size()].creature->idNumber], NULL, to, &genRect(32, 32, xPos, yPos));
				xPos += 37;
			}
		}
	}

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	//showing window with result of battle
	if(resWindow)
	{
		resWindow->show(to);
	}

	//showing in-gmae console
	LOCPLINT->cingconsole->show(to);

	//printing border around interface
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
}
void CBattleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_q)
	{
		showStackQueue = key.state==SDL_PRESSED;
	}
	else if(key.keysym.sym == SDLK_ESCAPE && spellDestSelectMode)
	{
		endCastingSpell();
	}
}
void CBattleInterface::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	if(activeStack>=0 && !spellDestSelectMode)
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
			CGI->curh->changeGraphic(1, 6);
			if(console->whoSetAlter == 0)
			{
				console->alterTxt = "";
			}
		}
		else
		{
			if(std::find(shadedHexes.begin(),shadedHexes.end(),myNumber) == shadedHexes.end())
			{
				const CStack *shere = LOCPLINT->cb->battleGetStackByPos(myNumber);
				const CStack *sactive = LOCPLINT->cb->battleGetStackByID(activeStack);
				if(shere)
				{
					if(shere->owner == LOCPLINT->playerID) //our stack
					{
						CGI->curh->changeGraphic(1,5);
						//setting console text
						char buf[500];
						sprintf(buf, CGI->generaltexth->allTexts[297].c_str(), shere->amount == 1 ? shere->creature->nameSing.c_str() : shere->creature->namePl.c_str());
						console->alterTxt = buf;
						console->whoSetAlter = 0;
						mouseHoveredStack = shere->ID;
						if(creAnims[shere->ID]->getType() == 2 && creAnims[shere->ID]->framesInGroup(1) > 0)
						{
							creAnims[shere->ID]->playOnce(1);
						}
					}
					else if(LOCPLINT->cb->battleCanShoot(activeStack,myNumber)) //we can shoot enemy
					{
						CGI->curh->changeGraphic(1,3);
						//setting console text
						char buf[500];
						sprintf(buf, CGI->generaltexth->allTexts[296].c_str(), shere->amount == 1 ? shere->creature->nameSing.c_str() : shere->creature->namePl.c_str(), sactive->shots, "?");
						console->alterTxt = buf;
						console->whoSetAlter = 0;
					}
					else if(isTileAttackable(myNumber)) //available enemy (melee attackable)
					{
						CCursorHandler *cursor = CGI->curh;
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

						const bool doubleWide = LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE);
						bool aboveAttackable = true, belowAttackable = true;

						// Exclude directions which cannot be attacked from.
						// Check to the left.
						if (myNumber%BFIELD_WIDTH <= 1 || !vstd::contains(shadedHexes, myNumber - 1)) {
							sectorCursor[0] = -1;
						}
						// Check top left, top right as well as above for 2-hex creatures.
						if (myNumber/BFIELD_WIDTH == 0) {
								sectorCursor[1] = -1;
								sectorCursor[2] = -1;
								aboveAttackable = false;
						} else {
							if (doubleWide) {
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
							} else {
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH - 1 + zigzagCorrection))
									sectorCursor[1] = -1;
								if (!vstd::contains(shadedHexes, myNumber - BFIELD_WIDTH + zigzagCorrection))
									sectorCursor[2] = -1;
							}
						}
						// Check to the right.
						if (myNumber%BFIELD_WIDTH >= BFIELD_WIDTH - 2 || !vstd::contains(shadedHexes, myNumber + 1)) {
							sectorCursor[3] = -1;
						}
						// Check bottom right, bottom left as well as below for 2-hex creatures.
						if (myNumber/BFIELD_WIDTH == BFIELD_HEIGHT - 1) {
							sectorCursor[4] = -1;
							sectorCursor[5] = -1;
							belowAttackable = false;
						} else {
							if (doubleWide) {
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
							} else {
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH + zigzagCorrection))
									sectorCursor[4] = -1;
								if (!vstd::contains(shadedHexes, myNumber + BFIELD_WIDTH - 1 + zigzagCorrection))
									sectorCursor[5] = -1;
							}
						}

						// Determine index from sector.
						int cursorIndex;
						if (doubleWide) {
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
						} else {
							cursorIndex = sector;
						}

						// Find the closest direction attackable, starting with the right one.
						// FIXME: Is this really how the original H3 client does it?
						int i = 0;
						while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1)
							i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
						cursor->changeGraphic(1, sectorCursor[(cursorIndex + i)%sectorCursor.size()]);
					}
					else //unavailable enemy
					{
						CGI->curh->changeGraphic(1,0);
						console->alterTxt = "";
						console->whoSetAlter = 0;
					}
				}
				else //empty unavailable tile
				{
					CGI->curh->changeGraphic(1,0);
					console->alterTxt = "";
					console->whoSetAlter = 0;
				}
			}
			else //available tile
			{
				const CStack *sactive = LOCPLINT->cb->battleGetStackByID(activeStack);
				if(LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::FLYING))
				{
					CGI->curh->changeGraphic(1,2);
					//setting console text
					char buf[500];
					sprintf(buf, CGI->generaltexth->allTexts[295].c_str(), sactive->amount == 1 ? sactive->creature->nameSing.c_str() : sactive->creature->namePl.c_str());
					console->alterTxt = buf;
					console->whoSetAlter = 0;
				}
				else
				{
					CGI->curh->changeGraphic(1,1);
					//setting console text
					char buf[500];
					sprintf(buf, CGI->generaltexth->allTexts[294].c_str(), sactive->amount == 1 ? sactive->creature->nameSing.c_str() : sactive->creature->namePl.c_str());
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
			CGI->curh->changeGraphic(1, 0);
			//setting console text
			console->alterTxt = CGI->generaltexth->allTexts[23];
			console->whoSetAlter = 0;
		}
		else
		{
			//get dead stack if we cast resurrection or animate dead
			const CStack * stackUnder = LOCPLINT->cb->battleGetStackByPos(myNumber, spellToCast->additionalInfo != 38 && spellToCast->additionalInfo != 39);

			if(stackUnder && spellToCast->additionalInfo == 39 && !stackUnder->hasFeatureOfType(StackFeature::UNDEAD)) //animate dead can be cast only on undead creatures
				stackUnder = NULL;

			bool whichCase; //for cases 1, 2 and 3
			switch(spellSelMode)
			{
			case 1:
				whichCase = stackUnder && LOCPLINT->playerID == stackUnder->owner;
				break;
			case 2:
				whichCase = stackUnder && LOCPLINT->playerID != stackUnder->owner;
				break;
			case 3:
				whichCase = stackUnder;
				break;
			}

			switch(spellSelMode)
			{
			case 0:
				CGI->curh->changeGraphic(3, 0);
				//setting console text
				char buf[500];
				sprintf(buf, CGI->generaltexth->allTexts[26].c_str(), CGI->spellh->spells[spellToCast->additionalInfo].name.c_str());
				console->alterTxt = buf;
				console->whoSetAlter = 0;
				break;
			case 1: case 2: case 3:
				if( whichCase )
				{
					CGI->curh->changeGraphic(3, 0);
					//setting console text
					char buf[500];
					std::string creName = stackUnder->amount > 1 ? stackUnder->creature->namePl : stackUnder->creature->nameSing;
						sprintf(buf, CGI->generaltexth->allTexts[27].c_str(), CGI->spellh->spells[spellToCast->additionalInfo].name.c_str(), creName.c_str());
					console->alterTxt = buf;
					console->whoSetAlter = 0;
					break;
				}
				else
				{
					CGI->curh->changeGraphic(1, 0);
					//setting console text
					console->alterTxt = CGI->generaltexth->allTexts[23];
					console->whoSetAlter = 0;
				}
				break;
			case 4: //TODO: implement this case
				if( blockedByObstacle(myNumber) )
				{
					CGI->curh->changeGraphic(3, 0);
				}
				else
				{
					CGI->curh->changeGraphic(1, 0);
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

bool CBattleInterface::reverseCreature(int number, int hex, bool wideTrick)
{
	if(creAnims[number]==NULL)
		return false; //there is no such creature
	creAnims[number]->setType(8);
	//int firstFrame = creAnims[number]->getFrame();
	//for(int g=0; creAnims[number]->getFrame() != creAnims[number]->framesInGroup(8) + firstFrame - 1; ++g)
	while(!creAnims[number]->onLastFrameInGroup())
	{
		show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	creDir[number] = !creDir[number];

	const CStack * curs = LOCPLINT->cb->battleGetStackByID(number);
	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(hex, creDir[number], curs);
	creAnims[number]->pos.x = coords.first;
	//creAnims[number]->pos.y = coords.second;

	if(wideTrick && curs->hasFeatureOfType(StackFeature::DOUBLE_WIDE))
	{
		if(curs->attackerOwned)
		{
			if(!creDir[number])
				creAnims[number]->pos.x -= 44;
		}
		else
		{
			if(creDir[number])
				creAnims[number]->pos.x += 44;
		}
	}

	creAnims[number]->setType(7);
	//firstFrame = creAnims[number]->getFrame();
	//for(int g=0; creAnims[number]->getFrame() != creAnims[number]->framesInGroup(7) + firstFrame - 1; ++g)
	while(!creAnims[number]->onLastFrameInGroup())
	{
		show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	creAnims[number]->setType(2);

	return true;
}

void CBattleInterface::handleStartMoving(int number)
{
	for(int i=0; i<creAnims[number]->framesInGroup(20)*getAnimSpeedMultiplier()-1; ++i)
	{
		show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
		if((animCount+1)%(4/settings.animSpeed)==0)
			creAnims[number]->incrementFrame();
	}
}

void CBattleInterface::bOptionsf()
{
	CGI->curh->changeGraphic(0,0);

	SDL_Rect temp_rect = genRect(431, 481, 160, 84);
	CBattleOptionsWindow * optionsWin = new CBattleOptionsWindow(temp_rect, this);
	GH.pushInt(optionsWin);
}

void CBattleInterface::bSurrenderf()
{
}

void CBattleInterface::bFleef()
{
	if( LOCPLINT->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = boost::bind(&CBattleInterface::reallyFlee,this);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[28],std::vector<SComponent*>(), ony, 0, false);
	}
	else
	{
		std::vector<SComponent*> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if(attackingHeroInstance)
			if(attackingHeroInstance->tempOwner == LOCPLINT->cb->getMyColor())
				heroName = attackingHeroInstance->name;
		if(defendingHeroInstance)
			if(defendingHeroInstance->tempOwner == LOCPLINT->cb->getMyColor())
				heroName = defendingHeroInstance->name;
		//calculating text
		char buffer[1000];
		sprintf(buffer, CGI->generaltexth->allTexts[340].c_str(), heroName.c_str());

		//printing message
		LOCPLINT->showInfoDialog(std::string(buffer), comps);
	}
}

void CBattleInterface::reallyFlee()
{
	giveCommand(4,0,0);
	CGI->curh->changeGraphic(0, 0);
}

void CBattleInterface::bAutofightf()
{
}

void CBattleInterface::bSpellf()
{
	CGI->curh->changeGraphic(0,0);

	const CGHeroInstance * chi = NULL;
	if(attackingHeroInstance->tempOwner == LOCPLINT->playerID)
		chi = attackingHeroInstance;
	else
		chi = defendingHeroInstance;
	CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), chi);
	GH.pushInt(spellWindow);
}

void CBattleInterface::bWaitf()
{
	if(activeStack != -1)
		giveCommand(8,0,activeStack);
}

void CBattleInterface::bDefencef()
{
	if(activeStack != -1)
 		giveCommand(3,0,activeStack);
}

void CBattleInterface::bConsoleUpf()
{
	console->scrollUp();
}

void CBattleInterface::bConsoleDownf()
{
	console->scrollDown();
}

void CBattleInterface::newStack(int stackID)
{
	const CStack * newStack = LOCPLINT->cb->battleGetStackByID(stackID);

	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(newStack->position, newStack->owner == attackingHeroInstance->tempOwner, newStack);
	creAnims[stackID] = (new CCreatureAnimation(newStack->creature->animDefName));
	creAnims[stackID]->setType(2);
	creAnims[stackID]->pos = genRect(creAnims[newStack->ID]->fullHeight, creAnims[newStack->ID]->fullWidth, coords.first, coords.second);
	creDir[stackID] = newStack->owner == attackingHeroInstance->tempOwner;
}

void CBattleInterface::stackRemoved(int stackID)
{
	delete creAnims[stackID];
	creAnims.erase(stackID);
}

void CBattleInterface::stackActivated(int number)
{
	//givenCommand = NULL;
	activeStack = number;
	myTurn = true;
	redrawBackgroundWithHexes(number);
	bWait->block(vstd::contains(LOCPLINT->cb->battleGetStackByID(number)->state,WAITING)); //block waiting button if stack has been already waiting

	//block cast spell button if hero doesn't have a spellbook
	if(attackingHeroInstance && attackingHeroInstance->tempOwner==LOCPLINT->cb->battleGetStackByID(number)->owner)
	{
		if(!attackingHeroInstance->getArt(17)) //don't unlock if already locked
			bSpell->block(!attackingHeroInstance->getArt(17));
	}
	else if(defendingHeroInstance && defendingHeroInstance->tempOwner==LOCPLINT->cb->battleGetStackByID(number)->owner)
	{
		if(!defendingHeroInstance->getArt(17)) //don't unlock if already locked
			bSpell->block(!defendingHeroInstance->getArt(17));
	}
}

void CBattleInterface::stackMoved(int number, int destHex, bool endMoving, int distance)
{
	bool startMoving = creAnims[number]->getType()==20;
	//a few useful variables
	int curStackPos = LOCPLINT->cb->battleGetPos(number);
	int steps = creAnims[number]->framesInGroup(0)*getAnimSpeedMultiplier()-1;
	int hexWbase = 44, hexHbase = 42;
	const CStack * movedStack = LOCPLINT->cb->battleGetStackByID(number);
	bool twoTiles = movedStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE);

	
	std::pair<int, int> begPosition = CBattleHex::getXYUnitAnim(curStackPos, movedStack->attackerOwned, movedStack);
	std::pair<int, int> endPosition = CBattleHex::getXYUnitAnim(destHex, movedStack->attackerOwned, movedStack);

	if(startMoving) //animation of starting move; some units don't have this animation (ie. halberdier)
	{
		if (movedStack->creature->sounds.startMoving)
			CGI->soundh->playSound(movedStack->creature->sounds.startMoving);
		handleStartMoving(number);
	}
	if(moveStarted)
	{
		moveSh = CGI->soundh->playSound(movedStack->creature->sounds.move, -1);
		CGI->curh->hide();
		creAnims[number]->setType(0);
		moveStarted = false;
	}

	int mutPos = BattleInfo::mutualPosition(curStackPos, destHex);
	float stepX=0.0, stepY=0.0; //how far stack is moved in one frame; calculated later

	//reverse unit if necessary
	if((begPosition.first > endPosition.first) && creDir[number] == true)
	{
		reverseCreature(number, curStackPos, twoTiles);
	}
	else if ((begPosition.first < endPosition.first) && creDir[number] == false)
	{
		reverseCreature(number, curStackPos, twoTiles);
	}
	if(creAnims[number]->getType() != 0)
	{
		creAnims[number]->setType(0);
	}
	//unit reversed

	//step shift calculation
	float posX = creAnims[number]->pos.x, posY = creAnims[number]->pos.y; // for precise calculations ;]
	if(mutPos == -1 && movedStack->hasFeatureOfType(StackFeature::FLYING)) 
	{
		steps *= distance;

		stepX = (endPosition.first - (float)begPosition.first)/steps;
		stepY = (endPosition.second - (float)begPosition.second)/steps;
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

	//switch(mutPos) //reverse unit if necessary
	//{
	//case 0:	case 4:	case 5:
	//	if(creDir[number] == true)
	//		reverseCreature(number, curStackPos, twoTiles);
	//	break;
	//case 1:	case 2: case 3:
	//	if(creDir[number] == false)
	//		reverseCreature(number, curStackPos, twoTiles);
	//	break;
	//}

	//moving instructions
	for(int i=0; i<steps; ++i)
	{
		posX += stepX;
		creAnims[number]->pos.x = posX;
		posY += stepY;
		creAnims[number]->pos.y = posY;
		
		show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	//unit moved
	if(endMoving)
	{
		handleEndOfMove(number, destHex);
	}

	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(destHex, creDir[number], movedStack);
	creAnims[number]->pos.x = coords.first;
	if(!endMoving && twoTiles && (movedStack->owner == attackingHeroInstance->tempOwner) && (creDir[number] != (movedStack->owner == attackingHeroInstance->tempOwner))) //big attacker creature is reversed
		creAnims[number]->pos.x -= 44;
	else if(!endMoving && twoTiles && (movedStack->owner != attackingHeroInstance->tempOwner) && (creDir[number] != (movedStack->owner == attackingHeroInstance->tempOwner))) //big defender creature is reversed
		creAnims[number]->pos.x += 44;
	creAnims[number]->pos.y = coords.second;
}

void CBattleInterface::stacksAreAttacked(std::vector<CBattleInterface::SStackAttackedInfo> attackedInfos)
{
	//restoring default state of battleWindow by calling show func
	while(true)
	{
		show(screen);
		CSDL_Ext::update();
		SDL_framerateDelay(LOCPLINT->mainFPSmng);

		//checking break conditions
		bool break_loop = true;
		for(size_t g=0; g<attackedInfos.size(); ++g)
		{
			if(creAnims[attackedInfos[g].ID]->getType() != 2)
			{
				break_loop = false;
			}
			if(attackingInfo && attackingInfo->IDby == attackedInfos[g].IDby)
			{
				break_loop = false;
			}
		}
		if(break_loop) break;
	}
	if(attackedInfos.size() == 1 && attackedInfos[0].byShooting) //delay hit animation
	{
		CStack attacker = *LOCPLINT->cb->battleGetStackByID(attackedInfos[0].IDby);
		while(true)
		{
			bool found = false;
			for(std::list<SProjectileInfo>::const_iterator it = projectiles.begin(); it!=projectiles.end(); ++it)
			{
				if(it->creID == attacker.creature->idNumber)
				{
					found = true;
					break;
				}
			}
			if(!found)
				break;
			else
			{
				show(screen);
				CSDL_Ext::update(screen);
				SDL_framerateDelay(LOCPLINT->mainFPSmng);
			}
		}
	}
	//initializing
	int maxLen = 0;
	for(size_t g=0; g<attackedInfos.size(); ++g)
	{
		const CStack * attacked = LOCPLINT->cb->battleGetStackByID(attackedInfos[g].ID, false);
			
		if(attackedInfos[g].killed)
		{
			CGI->soundh->playSound(attacked->creature->sounds.killed);
			creAnims[attackedInfos[g].ID]->setType(5); //death
		}
		else
		{
			// TODO: this block doesn't seems correct if the unit is defending.
			CGI->soundh->playSound(attacked->creature->sounds.wince);
			creAnims[attackedInfos[g].ID]->setType(3); //getting hit
		}
	}
	//main showing loop
	bool continueLoop = true;
	while(continueLoop)
	{
		show(screen);
		CSDL_Ext::update(screen);
		SDL_Delay(5);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
		for(size_t g=0; g<attackedInfos.size(); ++g)
		{
			if((animCount+1)%(4/settings.animSpeed)==0 && !creAnims[attackedInfos[g].ID]->onLastFrameInGroup())
			{
				creAnims[attackedInfos[g].ID]->incrementFrame();
			}
			if(creAnims[attackedInfos[g].ID]->onLastFrameInGroup() && creAnims[attackedInfos[g].ID]->getType() == 3)
				creAnims[attackedInfos[g].ID]->setType(2);
		}
		bool isAnotherOne = false; //if true, there is a stack whose hit/death anim must be continued
		for(size_t g=0; g<attackedInfos.size(); ++g)
		{
			if(!creAnims[attackedInfos[g].ID]->onLastFrameInGroup())
			{
				isAnotherOne = true;
				break;
			}
		}
		if(!isAnotherOne)
			continueLoop = false;
	}
	//restoring animType
	for(size_t g=0; g<attackedInfos.size(); ++g)
	{
		if(creAnims[attackedInfos[g].ID]->getType() == 3)
			creAnims[attackedInfos[g].ID]->setType(2);
	}

	//printing info to console
	for(size_t g=0; g<attackedInfos.size(); ++g)
	{
		if(attackedInfos[g].IDby!=-1)
			printConsoleAttacked(attackedInfos[g].ID, attackedInfos[g].dmg, attackedInfos[g].amountKilled, attackedInfos[g].IDby);
	}
}

void CBattleInterface::stackAttacking(int ID, int dest)
{
	if(attackingInfo == NULL && creAnims[ID]->getType() == 20)
	{
		handleStartMoving(ID);
		creAnims[ID]->setType(2);
	}
	while(attackingInfo != NULL || creAnims[ID]->getType()!=2)
	{
		show(screen);
		CSDL_Ext::update(screen);
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	CStack aStack = *LOCPLINT->cb->battleGetStackByID(ID); //attacking stack
	int reversedShift = 0; //shift of attacking stack's position due to reversing
	if(aStack.attackerOwned)
	{
		if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
		{
			switch(BattleInfo::mutualPosition(aStack.position, dest)) //attack direction
			{
				case 0:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 5:
					reverseCreature(ID, aStack.position, true);
					break;
				case -1:
					if(BattleInfo::mutualPosition(aStack.position + (aStack.attackerOwned ? -1 : 1), dest) >= 0) //if reversing stack will make its position adjacent to dest
					{
						reverseCreature(ID, aStack.position, true);
						reversedShift = (aStack.attackerOwned ? -1 : 1);
					}
					break;
			}
		}
		else //else for if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
		{
			switch(BattleInfo::mutualPosition(aStack.position, dest)) //attack direction
			{
				case 0:
					reverseCreature(ID, aStack.position, true);
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					reverseCreature(ID, aStack.position, true);
					break;
				case 5:
					reverseCreature(ID, aStack.position, true);
					break;
			}
		}
	}
	else //if(aStack.attackerOwned)
	{
		if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
		{
			switch(BattleInfo::mutualPosition(aStack.position, dest)) //attack direction
			{
				case 0:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 1:
					break;
				case 2:
					reverseCreature(ID, aStack.position, true);
					break;
				case 3:
					break;
				case 4:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 5:
					//reverseCreature(ID, aStack.position, true);
					break;
				case -1:
					if(BattleInfo::mutualPosition(aStack.position + (aStack.attackerOwned ? -1 : 1), dest) >= 0) //if reversing stack will make its position adjacent to dest
					{
						reverseCreature(ID, aStack.position, true);
						reversedShift = (aStack.attackerOwned ? -1 : 1);
					}
					break;
			}
		}
		else //else for if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
		{
			switch(BattleInfo::mutualPosition(aStack.position, dest)) //attack direction
			{
				case 0:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 1:
					reverseCreature(ID, aStack.position, true);
					break;
				case 2:
					reverseCreature(ID, aStack.position, true);
					break;
				case 3:
					reverseCreature(ID, aStack.position, true);
					break;
				case 4:
					//reverseCreature(ID, aStack.position, true);
					break;
				case 5:
					//reverseCreature(ID, aStack.position, true);
					break;
			}
		}
	}

	attackingInfo = new CAttHelper;
	attackingInfo->dest = dest;
	attackingInfo->frame = 0;
	attackingInfo->hitCount = 0;
	attackingInfo->ID = ID;
	attackingInfo->IDby = LOCPLINT->cb->battleGetStackByPos(dest, false)->ID;
	attackingInfo->reversing = false;
	attackingInfo->posShiftDueToDist = reversedShift;
	attackingInfo->shooting = false;
	attackingInfo->sh = -1;

	switch(BattleInfo::mutualPosition(aStack.position + reversedShift, dest)) //attack direction
	{
		case 0:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(11);
			break;
		case 1:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(11);
			break;
		case 2:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(12);
			break;
		case 3:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(13);
			break;
		case 4:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(13);
			break;
		case 5:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(12);
			break;
		default:
			tlog1<<"Critical Error! Wrong dest in stackAttacking! dest: "<<dest<<" attacking stack pos: "<<aStack.position<<" reversed shift: "<<reversedShift<<std::endl;
	}
}

void CBattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);

	//unlock spellbook
	bSpell->block(!LOCPLINT->cb->battleCanCastSpell());

	//handle regeneration
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks();
	for(std::map<int, CStack>::const_iterator it = stacks.begin(); it != stacks.end(); ++it)
	{
		if( it->second.hasFeatureOfType(StackFeature::HP_REGENERATION) )
			displayEffect(74, it->second.position);

		if( it->second.hasFeatureOfType(StackFeature::FULL_HP_REGENERATION, 0) )
			displayEffect(4, it->second.position);

		if( it->second.hasFeatureOfType(StackFeature::FULL_HP_REGENERATION, 1) )
			displayEffect(74, it->second.position);
	}
}

void CBattleInterface::giveCommand(ui8 action, ui16 tile, ui32 stack, si32 additional)
{
	BattleAction * ba = new BattleAction(); //is deleted in CPlayerInterface::activeStack()
	ba->side = defendingHeroInstance ? (LOCPLINT->playerID == defendingHeroInstance->tempOwner) : false;
	ba->actionType = action;
	ba->destinationTile = tile;
	ba->stackNumber = stack;
	ba->additionalInfo = additional;
	givenCommand->setn(ba);
	myTurn = false;
	activeStack = -1;
}

bool CBattleInterface::isTileAttackable(const int & number) const
{
	for(size_t b=0; b<shadedHexes.size(); ++b)
	{
		if(BattleInfo::mutualPosition(shadedHexes[b], number) != -1 || shadedHexes[b] == number)
			return true;
	}
	return false;
}

bool CBattleInterface::blockedByObstacle(int hex) const
{
	std::vector<CObstacleInstance> obstacles = LOCPLINT->cb->battleGetAllObstacles();
	std::set<int> coveredHexes;
	for(int b = 0; b < obstacles.size(); ++b)
	{
		std::vector<int> blocked = CGI->heroh->obstacles[obstacles[b].ID].getBlocked(obstacles[b].pos);
		for(int w = 0; w < blocked.size(); ++w)
			coveredHexes.insert(blocked[w]);
	}
	return vstd::contains(coveredHexes, hex);
}

void CBattleInterface::handleEndOfMove(int stackNumber, int destinationTile)
{
	const CStack * movedStack = LOCPLINT->cb->battleGetStackByID(stackNumber);
	if(!movedStack)
		return;
	if(creAnims[stackNumber]->framesInGroup(21)!=0) // some units don't have this animation (ie. halberdier)
	{
		if (movedStack->creature->sounds.endMoving)
		{
			CGI->soundh->playSound(movedStack->creature->sounds.endMoving);
		}

		creAnims[stackNumber]->setType(21);

		//for(int i=0; i<creAnims[number]->framesInGroup(21)*getAnimSpeedMultiplier()-1; ++i)
		while(!creAnims[stackNumber]->onLastFrameInGroup())
		{
			show(screen);
			CSDL_Ext::update(screen);
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
		}
	}
	creAnims[stackNumber]->setType(2); //resetting to default
	CGI->curh->show();
	CGI->soundh->stopSound(moveSh);

	if(creDir[stackNumber] != (movedStack->owner == attackingHeroInstance->tempOwner))
		reverseCreature(stackNumber, destinationTile, movedStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE));	
}

void CBattleInterface::hexLclicked(int whichOne)
{
	if((whichOne%BFIELD_WIDTH)!=0 && (whichOne%BFIELD_WIDTH)!=(BFIELD_WIDTH-1)) //if player is trying to attack enemey unit or move creature stack
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
				if(!LOCPLINT->cb->battleGetStackByPos(whichOne, onlyAlive) || LOCPLINT->playerID != LOCPLINT->cb->battleGetStackByPos(whichOne, onlyAlive)->owner )
					allowCasting = false;
				break;
			case 2:
				if(!LOCPLINT->cb->battleGetStackByPos(whichOne, onlyAlive) || LOCPLINT->playerID == LOCPLINT->cb->battleGetStackByPos(whichOne, onlyAlive)->owner )
					allowCasting = false;
				break;
			case 3:
				if(!LOCPLINT->cb->battleGetStackByPos(whichOne, onlyAlive))
					allowCasting = false;
				break;
			case 4:
				if(!blockedByObstacle(whichOne))
					allowCasting = false;
				break;
			}
			//destination checked
			if(allowCasting)
			{
				spellToCast->destinationTile = whichOne;
				LOCPLINT->cb->battleMakeAction(spellToCast);
				endCastingSpell();
			}
		}
		else //we don't cast any spell
		{
			const CStack* dest = LOCPLINT->cb->battleGetStackByPos(whichOne); //creature at destination tile; -1 if there is no one
			if(!dest || !dest->alive()) //no creature at that tile
			{
				if(std::find(shadedHexes.begin(),shadedHexes.end(),whichOne)!=shadedHexes.end())// and it's in our range
				{
					CGI->curh->changeGraphic(1, 6); //cursor should be changed
					if(LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE))
					{
						std::vector<int> acc = LOCPLINT->cb->battleGetAvailableHexes(activeStack, false);
						int shiftedDest = whichOne + (LOCPLINT->cb->battleGetStackByID(activeStack)->attackerOwned ? 1 : -1);
						if(vstd::contains(acc, whichOne))
							giveCommand(2,whichOne,activeStack);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand(2,shiftedDest,activeStack);
					}
					else
					{
						giveCommand(2,whichOne,activeStack);
					}
				}
			}
			else if(dest->owner != attackingHeroInstance->tempOwner
				&& LOCPLINT->cb->battleCanShoot(activeStack, whichOne) ) //shooting
			{
				CGI->curh->changeGraphic(1, 6); //cursor should be changed
				giveCommand(7,whichOne,activeStack);
			}
			else if(dest->owner != attackingHeroInstance->tempOwner) //attacking
			{
				const CStack * actStack = LOCPLINT->cb->battleGetStackByID(activeStack);
				int attackFromHex = -1; //hex from which we will attack chosen stack
				switch(CGI->curh->number)
				{
				case 12: //from bottom right
					{
						bool doubleWide = LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE);
						int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH+1 ) + doubleWide;
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
					if(LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && !LOCPLINT->cb->battleGetStackByID(activeStack)->attackerOwned)
					{
						std::vector<int> acc = LOCPLINT->cb->battleGetAvailableHexes(activeStack, false);
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
						bool doubleWide = LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE);
						int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 ) + doubleWide;
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
					if(LOCPLINT->cb->battleGetStackByID(activeStack)->hasFeatureOfType(StackFeature::DOUBLE_WIDE) && LOCPLINT->cb->battleGetStackByID(activeStack)->attackerOwned)
					{
						std::vector<int> acc = LOCPLINT->cb->battleGetAvailableHexes(activeStack, false);
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
				case 13: //from bottom
					{
						int destHex = whichOne + ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH+1 );
						if(vstd::contains(shadedHexes, destHex))
							giveCommand(6,destHex,activeStack,whichOne);
						else if(attackingHeroInstance->tempOwner == LOCPLINT->cb->getMyColor()) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								giveCommand(6,destHex+1,activeStack,whichOne);
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								giveCommand(6,destHex-1,activeStack,whichOne);
						}
						break;
					}
				case 14: //from top
					{
						int destHex = whichOne - ( (whichOne/BFIELD_WIDTH)%2 ? BFIELD_WIDTH : BFIELD_WIDTH-1 );
						if(vstd::contains(shadedHexes, destHex))
							giveCommand(6,destHex,activeStack,whichOne);
						else if(attackingHeroInstance->tempOwner == LOCPLINT->cb->getMyColor()) //if we are attacker
						{
							if(vstd::contains(shadedHexes, destHex+1))
								giveCommand(6,destHex+1,activeStack,whichOne);
						}
						else //if we are defender
						{
							if(vstd::contains(shadedHexes, destHex-1))
								giveCommand(6,destHex-1,activeStack,whichOne);
						}
						break;
					}
				}

				giveCommand(6, attackFromHex, activeStack, whichOne);

				CGI->curh->changeGraphic(1, 6); //cursor should be changed
			}
		}
	}
}

void CBattleInterface::stackIsShooting(int ID, int dest)
{
	if(attackingInfo != NULL)
	{
		return; //something went wrong
	}
	//projectile
	float projectileAngle; //in radians; if positive, projectiles goes up
	float straightAngle = 0.2f; //maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	int fromHex = LOCPLINT->cb->battleGetPos(ID);
	projectileAngle = atan2(float(abs(dest - fromHex)/BFIELD_WIDTH), float(abs(dest - fromHex)%BFIELD_WIDTH));
	if(fromHex < dest)
		projectileAngle = -projectileAngle;

	SProjectileInfo spi;
	spi.creID = LOCPLINT->cb->battleGetStackByID(ID)->creature->idNumber;
	spi.reverse = !LOCPLINT->cb->battleGetStackByID(ID)->attackerOwned;

	spi.step = 0;
	spi.frameNum = 0;
	spi.spin = CGI->creh->idToProjectileSpin[spi.creID];

	std::pair<int, int> xycoord = CBattleHex::getXYUnitAnim(LOCPLINT->cb->battleGetPos(ID), true, LOCPLINT->cb->battleGetStackByID(ID));
	std::pair<int, int> destcoord = CBattleHex::getXYUnitAnim(dest, false, LOCPLINT->cb->battleGetStackByID(ID)); 
	destcoord.first += 250; destcoord.second += 210; //TODO: find a better place to shoot

	if(projectileAngle > straightAngle) //upper shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).upperRightMissleOffsetX;
		spi.y = xycoord.second + 100 - LOCPLINT->cb->battleGetCreature(ID).upperRightMissleOffsetY;
	}
	else if(projectileAngle < -straightAngle) //lower shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).lowerRightMissleOffsetX;
		spi.y = xycoord.second + 150 - LOCPLINT->cb->battleGetCreature(ID).lowerRightMissleOffsetY;
	}
	else //straight shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).rightMissleOffsetX;
		spi.y = xycoord.second + 125 - LOCPLINT->cb->battleGetCreature(ID).rightMissleOffsetY;
	}
	spi.lastStep = sqrt((float)((destcoord.first - spi.x)*(destcoord.first - spi.x) + (destcoord.second - spi.y) * (destcoord.second - spi.y))) / 40;
	if(spi.lastStep == 0)
		spi.lastStep = 1;
	spi.dx = (destcoord.first - spi.x) / spi.lastStep;
	spi.dy = (destcoord.second - spi.y) / spi.lastStep;
	//set starting frame
	if(spi.spin)
	{
		spi.frameNum = 0;
	}
	else
	{
		spi.frameNum = ((M_PI/2.0f - projectileAngle) / (2.0f *M_PI) + 1/((float)(2*(idToProjectile[spi.creID]->ourImages.size()-1)))) * (idToProjectile[spi.creID]->ourImages.size()-1);
	}
	//set delay
	spi.animStartDelay = CGI->creh->creatures[spi.creID].attackClimaxFrame;
	projectiles.push_back(spi);

	//attack aniamtion
	attackingInfo = new CAttHelper;
	attackingInfo->dest = dest;
	attackingInfo->frame = 0;
	attackingInfo->hitCount = 0;
	attackingInfo->ID = ID;
	attackingInfo->reversing = false;
	attackingInfo->posShiftDueToDist = 0;
	attackingInfo->shooting = true;
	attackingInfo->sh = -1;
	if(projectileAngle > straightAngle) //upper shot
		attackingInfo->shootingGroup = 14;
	else if(projectileAngle < -straightAngle) //lower shot
		attackingInfo->shootingGroup = 16;
	else //straight shot
		attackingInfo->shootingGroup = 15;
	attackingInfo->maxframe = creAnims[ID]->framesInGroup(attackingInfo->shootingGroup);
}

void CBattleInterface::battleFinished(const BattleResult& br)
{
	CGI->curh->changeGraphic(0,0);
	
	SDL_Rect temp_rect = genRect(561, 470, (screen->w - 800)/2 + 165, (screen->h - 600)/2 + 19);
	CGI->musich->stopMusic();
	resWindow = new CBattleResultWindow(br, temp_rect, this);
	GH.pushInt(resWindow);
}

void CBattleInterface::spellCast(SpellCast * sc)
{
	CSpell &spell = CGI->spellh->spells[sc->id];

	if(sc->side == !LOCPLINT->cb->battleGetStackByID(activeStack)->attackerOwned)
		bSpell->block(true);

	std::vector< std::string > anims; //for magic arrow and ice bolt

	if (spell.soundID != soundBase::invalid)
		CGI->soundh->playSound(spell.soundID);

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
			std::pair<int, int> srccoord = sc->side ? std::make_pair(770, 60) : std::make_pair(30, 60);
			std::pair<int, int> destcoord = CBattleHex::getXYUnitAnim(sc->tile, !sc->side, LOCPLINT->cb->battleGetStackByPos(sc->tile)); //position attacked by arrow
			destcoord.first += 250; destcoord.second += 240;

			//animation angle
			float angle = atan2(float(destcoord.first - srccoord.first), float(destcoord.second - srccoord.second));

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
			int steps = sqrt((float)((destcoord.first - srccoord.first)*(destcoord.first - srccoord.first) + (destcoord.second - srccoord.second) * (destcoord.second - srccoord.second))) / 40;
			if(steps <= 0)
				steps = 1;

			CDefHandler * animDef = CDefHandler::giveDef(animToDisplay);

			int dx = (destcoord.first - srccoord.first - animDef->ourImages[0].bitmap->w)/steps, dy = (destcoord.second - srccoord.second - animDef->ourImages[0].bitmap->h)/steps;

			SDL_Rect buf;
			SDL_GetClipRect(screen, &buf);
			SDL_SetClipRect(screen, &pos); //setting rect we can blit to
			for(int g=0; g<steps; ++g)
			{
				show(screen);
				SDL_Rect & srcr = animDef->ourImages[g%animDef->ourImages.size()].bitmap->clip_rect;
				SDL_Rect dstr = genRect(srcr.h, srcr.w, srccoord.first + g*dx, srccoord.second + g*dy);
				SDL_BlitSurface(animDef->ourImages[g%animDef->ourImages.size()].bitmap, &srcr, screen, &dstr);
				CSDL_Ext::update(screen);
				SDL_framerateDelay(LOCPLINT->mainFPSmng);
			}
			SDL_SetClipRect(screen, &buf); //restoring previous clip rect
			break; //for 15 and 16 cases
		}
	case 17: //lightning bolt
		displayEffect(1, sc->tile);
		displayEffect(spell.mainEffectAnim, sc->tile);
		break;
	case 35: //dispel
	case 37: //cure
	case 38: //resurrection
	case 39: //animate dead
		for(std::set<ui32>::const_iterator it = sc->affectedCres.begin(); it != sc->affectedCres.end(); ++it)
		{
			displayEffect(spell.mainEffectAnim, LOCPLINT->cb->battleGetStackByID(*it, false)->position);
		}
		break;
	} //switch(sc->id)

	//support for resistance
	for(int j=0; j<sc->resisted.size(); ++j)
	{
		int tile = LOCPLINT->cb->battleGetStackByID(sc->resisted[j])->position;
		displayEffect(78, tile);
	}
}

void CBattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	for(std::set<ui32>::const_iterator ci = sse.stacks.begin(); ci!=sse.stacks.end(); ++ci)
	{
		displayEffect(CGI->spellh->spells[sse.effect.id].mainEffectAnim, LOCPLINT->cb->battleGetStackByID(*ci)->position);
	}
	redrawBackgroundWithHexes(activeStack);
}

void CBattleInterface::castThisSpell(int spellID)
{
	BattleAction * ba = new BattleAction;
	ba->actionType = 1;
	ba->additionalInfo = spellID; //spell number
	ba->destinationTile = -1;
	ba->stackNumber = (attackingHeroInstance->tempOwner == LOCPLINT->playerID) ? -1 : -2;
	ba->side = defendingHeroInstance ? (LOCPLINT->playerID == defendingHeroInstance->tempOwner) : false;
	spellToCast = ba;
	spellDestSelectMode = true;

	//choosing possible tragets
	const CGHeroInstance * castingHero = (attackingHeroInstance->tempOwner == LOCPLINT->playerID) ? attackingHeroInstance : attackingHeroInstance;
	spellSelMode = 0;
	if(CGI->spellh->spells[spellID].attributes.find("CREATURE_TARGET") != std::string::npos) //spell to be cast on one specific creature
	{
		switch(CGI->spellh->spells[spellID].positiveness)
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
	if(CGI->spellh->spells[spellID].attributes.find("CREATURE_TARGET_1") != std::string::npos ||
		CGI->spellh->spells[spellID].attributes.find("CREATURE_TARGET_2") != std::string::npos) //spell to be cast on a specific creature but massive on expert
	{
		if(castingHero && castingHero->getSpellSecLevel(spellID) < 3)
		{
			switch(CGI->spellh->spells[spellID].positiveness)
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
	if(CGI->spellh->spells[spellID].attributes.find("OBSTACLE_TARGET") != std::string::npos) //spell to be cast on an obstacle
	{
		spellSelMode = 4;
	}
	if(spellSelMode == -1) //user does not have to select location
	{
		spellToCast->destinationTile = -1;
		LOCPLINT->cb->battleMakeAction(spellToCast);
		delete spellToCast;
		spellToCast = NULL;
		spellDestSelectMode = false;
		CGI->curh->changeGraphic(1, 6);
	}
	else
	{
		CGI->curh->changeGraphic(3, 0); 
	}
}

void CBattleInterface::displayEffect(ui32 effect, int destTile)
{
	if(effect == 12) //armageddon
	{
		if(graphics->battleACToDef[effect].size() != 0)
		{
			CDefHandler * anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);
			for(int i=0; i * anim->width < pos.w ; ++i)
			{
				for(int j=0; j * anim->height < pos.h ; ++j)
				{
					SBattleEffect be;
					be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);
					be.frame = 0;
					be.maxFrame = be.anim->ourImages.size();
					be.x = i * anim->width;
					be.y = j * anim->height;

					battleEffects.push_back(be);
				}
			}
		}
	}
	else
	{
		if(graphics->battleACToDef[effect].size() != 0)
		{
			SBattleEffect be;
			be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);
			be.frame = 0;
			be.maxFrame = be.anim->ourImages.size();
			be.x = 22 * ( ((destTile/BFIELD_WIDTH) + 1)%2 ) + 44 * (destTile % BFIELD_WIDTH) + 45;
			be.y = 105 + 42 * (destTile/BFIELD_WIDTH);

			if(effect != 1 && effect != 0)
			{
				be.x -= be.anim->ourImages[0].bitmap->w/2;
				be.y -= be.anim->ourImages[0].bitmap->h/2;
			}
			else if(effect == 1)
			{
				be.x -= be.anim->ourImages[0].bitmap->w;
				be.y -= be.anim->ourImages[0].bitmap->h;
			}
			else if (effect == 0)
			{
				be.x -= be.anim->ourImages[0].bitmap->w/2;
				be.y -= be.anim->ourImages[0].bitmap->h;
			}

			battleEffects.push_back(be);
		}
	}
	//battleEffects 
}

void CBattleInterface::setAnimSpeed(int set)
{
	settings.animSpeed = set;
}

int CBattleInterface::getAnimSpeed() const
{
	return settings.animSpeed;
}

float CBattleInterface::getAnimSpeedMultiplier() const
{
	switch(settings.animSpeed)
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
	CGI->curh->changeGraphic(1, 6);
}

void CBattleInterface::attackingShowHelper()
{
	if(attackingInfo && !attackingInfo->reversing)
	{
		if(attackingInfo->frame == 0)
		{
			const CStack * aStack = LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(attackingInfo->shooting)
			{
				// TODO: I see that we enter this function twice with
				// attackingInfo->frame==0, so all the inits are done
				// twice. The following is just a workaround until
				// that is fixed. Once done, we can get rid of
				// attackingInfo->sh
				if (attackingInfo->sh == -1)
					attackingInfo->sh = CGI->soundh->playSound(aStack->creature->sounds.shoot);
				creAnims[attackingInfo->ID]->setType(attackingInfo->shootingGroup);
			}
			else
			{
				// TODO: see comment above
				if (attackingInfo->sh == -1)
					attackingInfo->sh = CGI->soundh->playSound(aStack->creature->sounds.attack);

				static std::map<int, int> dirToType = boost::assign::map_list_of (0, 11)(1, 11)(2, 12)(3, 13)(4, 13)(5, 12);
				int type; //dependent on attack direction
				if(aStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					type = dirToType[ BattleInfo::mutualPosition(aStack->position + attackingInfo->posShiftDueToDist, attackingInfo->dest) ]; //attack direction
				}
				else //else for if(aStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					type = BattleInfo::mutualPosition(aStack->position, attackingInfo->dest);
				}
				creAnims[attackingInfo->ID]->setType(type);
			}
		}
		else if(attackingInfo->frame == (attackingInfo->maxframe - 1))
		{
			attackingInfo->reversing = true;

			const CStack* aStackp = LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(aStackp == NULL)
				return;
			CStack aStack = *aStackp;
			if(aStack.attackerOwned)
			{
				if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 1:
							break;
						case 2:
							break;
						case 3:
							break;
						case 4:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 5:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case -1:
							if(attackingInfo->posShiftDueToDist) //if reversing stack will make its position adjacent to dest
							{
								reverseCreature(attackingInfo->ID, aStack.position, true);
							}
							break;
					}
				}
				else //else for if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 1:
							break;
						case 2:
							break;
						case 3:
							break;
						case 4:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 5:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
					}
				}
			}
			else //if(aStack.attackerOwned)
			{
				if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 1:
							break;
						case 2:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 3:
							break;
						case 4:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 5:
							//reverseCreature(ID, aStack.position, true);
							break;
						case -1:
							if(attackingInfo->posShiftDueToDist) //if reversing stack will make its position adjacent to dest
							{
								reverseCreature(attackingInfo->ID, aStack.position, true);
							}
							break;
					}
				}
				else //else for if(aStack.hasFeatureOfType(StackFeature::DOUBLE_WIDE))
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 1:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 2:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 3:
							reverseCreature(attackingInfo->ID, aStack.position, true);
							break;
						case 4:
							//reverseCreature(ID, aStack.position, true);
							break;
						case 5:
							//reverseCreature(ID, aStack.position, true);
							break;
					}
				}
			}
			attackingInfo->reversing = false;
			creAnims[attackingInfo->ID]->setType(2);
			delete attackingInfo;
			attackingInfo = NULL;
		}
		if(attackingInfo)
		{
			attackingInfo->hitCount++;
			if(attackingInfo->hitCount%(4/settings.animSpeed) == 0)
				attackingInfo->frame++;
		}
	}
}

void CBattleInterface::showAliveStack(int ID, const std::map<int, CStack> & stacks, SDL_Surface * to)
{
	if(creAnims.find(ID) == creAnims.end()) //eg. for summoned but not yet handled stacks
		return;

	const CStack &curStack = stacks.find(ID)->second;
	int animType = creAnims[ID]->getType();

	int affectingSpeed = settings.animSpeed;
	if(animType == 1 || animType == 2) //standing stacks should not stand faster :)
		affectingSpeed = 2;
	bool incrementFrame = (animCount%(4/affectingSpeed)==0) && animType!=5 && animType!=20 && animType!=3 && animType!=2;

	if(animType == 2)
	{
		if(standingFrame.find(ID)!=standingFrame.end())
		{
			incrementFrame = (animCount%(8/affectingSpeed)==0);
			if(incrementFrame)
			{
				++standingFrame[ID];
				if(standingFrame[ID] == creAnims[ID]->framesInGroup(2))
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

	creAnims[ID]->nextFrame(to, creAnims[ID]->pos.x + pos.x, creAnims[ID]->pos.y + pos.y, creDir[ID], animCount, incrementFrame, ID==activeStack, ID==mouseHoveredStack); //increment always when moving, never if stack died

	//printing amount
	if(curStack.amount > 0 //don't print if stack is not alive
		&& (!LOCPLINT->curAction
			|| (LOCPLINT->curAction->stackNumber != ID //don't print if stack is currently taking an action
				&& (LOCPLINT->curAction->actionType != 6  ||  curStack.position != LOCPLINT->curAction->additionalInfo) //nor if it's an object of attack
				&& (LOCPLINT->curAction->destinationTile != curStack.position) //nor if it's on destination tile for current action
				)
			)
			&& !curStack.hasFeatureOfType(StackFeature::SIEGE_WEAPON) //and not a war machine...
	)
	{
		int xAdd = curStack.attackerOwned ? 220 : 202;

		//blitting amoutn background box
		SDL_Surface *amountBG = NULL;
		if(curStack.effects.size() == 0)
		{
			amountBG = amountNormal;
		}
		else
		{
			int pos=0; //determining total positiveness of effects
			for(int c=0; c<curStack.effects.size(); ++c)
			{
				pos += CGI->spellh->spells[ curStack.effects[c].id ].positiveness;
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
		SDL_BlitSurface(amountBG, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[ID]->pos.x + xAdd + pos.x, creAnims[ID]->pos.y + 260 + pos.y));
		//blitting amount
		CSDL_Ext::printAtMiddleWB(
			makeNumberShort(curStack.amount),
			creAnims[ID]->pos.x + xAdd + 14 + pos.x,
			creAnims[ID]->pos.y + 260 + 4 + pos.y,
			GEOR13,
			20,
			zwykly,
			to
        );
	}
}

void CBattleInterface::showPieceOfWall(SDL_Surface * to, int hex)
{
	if(!siegeH)
		return;

	static std::map<int, int> hexToPart = boost::assign::map_list_of(12, 8)(29, 7)(62, 12)(78, 6)(112, 10)(147, 5)(165, 11)(182, 4);

	std::map<int, int>::const_iterator it = hexToPart.find(hex);
	if(it != hexToPart.end())
	{
		siegeH->printPartOfWall(to, it->second);
	}

	//additionally print lower tower
	if(hex == 182)
	{
		siegeH->printPartOfWall(to, 3);
	}
}

void CBattleInterface::redrawBackgroundWithHexes(int activeStack)
{
	shadedHexes = LOCPLINT->cb->battleGetAvailableHexes(activeStack, true);

	//preparating background graphic with hexes and shaded hexes
	blitAt(background, 0, 0, backgroundWithHexes);
	if(settings.printCellBorders)
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, backgroundWithHexes, NULL);

	if(settings.printStackRange)
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

void CBattleInterface::printConsoleAttacked(int ID, int dmg, int killed, int IDby)
{
	char tabh[200];
	const CStack * attacker = LOCPLINT->cb->battleGetStackByID(IDby, false);
	const CStack * defender = LOCPLINT->cb->battleGetStackByID(ID, false);
	int end = sprintf(tabh, CGI->generaltexth->allTexts[attacker->amount > 1 ? 377 : 376].c_str(),
		(attacker->amount > 1 ? attacker->creature->namePl.c_str() : attacker->creature->nameSing.c_str()),
		dmg);
	if(killed > 0)
	{
		if(killed > 1)
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[379].c_str(), killed, defender->creature->namePl.c_str());
		}
		else //killed == 1
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[378].c_str(), defender->creature->nameSing.c_str());
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
	if(!down && myHero && LOCPLINT->cb->battleCanCastSpell()) //check conditions
	{
		for(int it=0; it<BFIELD_SIZE; ++it) //do nothing when any hex is hovered - hero's animation overlaps battlefield
		{
			if(myOwner->bfield[it].hovered && myOwner->bfield[it].strictHovered)
				return;
		}
		CGI->curh->changeGraphic(0,0);

		CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (conf.cc.resx - 620)/2, (conf.cc.resy - 595)/2), myHero);
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
	dh->alphaTransformed = true;

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

std::pair<int, int> CBattleHex::getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * stack)
{
	std::pair<int, int> ret = std::make_pair(-500, -500); //returned value
	ret.second = -139 + 42 * (hexNum/BFIELD_WIDTH); //counting y
	//counting x
	if(attacker)
	{
		ret.first = -160 + 22 * ( ((hexNum/BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % BFIELD_WIDTH);
	}
	else
	{
		ret.first = -219 + 22 * ( ((hexNum/BFIELD_WIDTH) + 1)%2 ) + 44 * (hexNum % BFIELD_WIDTH);
	}
	//shifting position for double - hex creatures
	if(stack && stack->hasFeatureOfType(StackFeature::DOUBLE_WIDE))
	{
		if(attacker)
		{
			ret.first -= 42;
		}
		else
		{
			ret.first += 42;
		}
	}
	//returning
	return ret;
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

CBattleHex::CBattleHex() : setAlterText(false), myNumber(-1), accesible(true), hovered(false), strictHovered(false), myInterface(NULL)
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
		if(myInterface->console->alterTxt.size() == 0 && LOCPLINT->cb->battleGetStack(myNumber) != -1 &&
			LOCPLINT->cb->battleGetStackByPos(myNumber)->owner != LOCPLINT->playerID &&
			LOCPLINT->cb->battleGetStackByPos(myNumber)->alive())
		{
			char tabh[160];
			CStack attackedStack = *LOCPLINT->cb->battleGetStackByPos(myNumber);
			const std::string & attackedName = attackedStack.amount == 1 ? attackedStack.creature->nameSing : attackedStack.creature->namePl;
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
	int stID = LOCPLINT->cb->battleGetStack(myNumber); //id of stack being on this tile
	if(hovered && strictHovered && stID!=-1)
	{
		const CStack & myst = *LOCPLINT->cb->battleGetStackByID(stID); //stack info
		if(!myst.alive()) return;
		StackState *pom = NULL;
		if(down)
		{
			pom = new StackState();
			const CGHeroInstance *h = myst.owner == myInterface->attackingHeroInstance->tempOwner ? myInterface->attackingHeroInstance : myInterface->defendingHeroInstance;

			pom->attackBonus = myst.Attack() - myst.creature->attack;
			pom->defenseBonus = myst.Defense() - myst.creature->defence;
			pom->luck = myst.Luck();
			pom->morale = myst.Morale();
			pom->speedBonus = myst.Speed() - myst.creature->speed;
			pom->healthBonus = myst.MaxHealth() - myst.creature->hitPoints;
			if(myst.hasFeatureOfType(StackFeature::SIEGE_WEAPON))
				pom->dmgMultiplier = h->getPrimSkillLevel(0) + 1;
			else
				pom->dmgMultiplier = 1;

			pom->shotsLeft = myst.shots;
			for(int vb=0; vb<myst.effects.size(); ++vb)
			{
				pom->effects.insert(myst.effects[vb].id);
			}
			pom->currentHealth = myst.firstHPleft;
			GH.pushInt(new CCreInfoWindow(myst.creature->idNumber, 0, myst.amount, pom, 0, 0, NULL));
		}
		delete pom;
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
		CSDL_Ext::printAtMiddleWB(ingcAlter, pos.x + pos.w/2, pos.y + 10, GEOR13, 80, zwykly, to);
	}
	else if(alterTxt.size())
	{
		CSDL_Ext::printAtMiddleWB(alterTxt, pos.x + pos.w/2, pos.y + 10, GEOR13, 80, zwykly, to);
	}
	else if(texts.size())
	{
		if(texts.size()==1)
		{
			CSDL_Ext::printAtMiddleWB(texts[0], pos.x + pos.w/2, pos.y + 10, GEOR13, 80, zwykly, to);
		}
		else
		{
			CSDL_Ext::printAtMiddleWB(texts[lastShown-1], pos.x + pos.w/2, pos.y + 10, GEOR13, 80, zwykly, to);
			CSDL_Ext::printAtMiddleWB(texts[lastShown], pos.x + pos.w/2, pos.y + 26, GEOR13, 80, zwykly, to);
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

CBattleResultWindow::CBattleResultWindow(const BattleResult &br, const SDL_Rect & pos, const CBattleInterface * owner)
{
	this->pos = pos;
	background = BitmapHandler::loadBitmap("CPRESULT.BMP", true);
	graphics->blueToPlayersAdv(background, LOCPLINT->playerID);
	SDL_Surface * pom = SDL_ConvertSurface(background, screen->format, screen->flags);
	SDL_FreeSurface(background);
	background = pom;
	exit = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleResultWindow::bExitf,this), 384 + pos.x, 505 + pos.y, "iok6432.def", SDLK_RETURN);

	if(br.winner==0) //attacker won
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 60, 122, GEOR13, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 410, 122, GEOR13, zwykly, background);
	}
	else //if(br.winner==1)
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 60, 122, GEOR13, zwykly, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 410, 122, GEOR13, zwykly, background);
	}

	
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[407], 235, 299, GEOR16, tytulowy, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[408], 235, 329, GEOR16, zwykly, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[409], 235, 426, GEOR16, zwykly, background);

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
		for(std::map<si32,std::pair<ui32,si32> >::const_iterator it = owner->army1->slots.begin(); it!=owner->army1->slots.end(); ++it)
		{
			if( CGI->creh->creatures[it->first].AIValue > bestPower)
			{
				bestPower = CGI->creh->creatures[it->first].AIValue;
				bestMonsterID = it->first;
			}
		}
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &genRect(64, 58, 21, 38));
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID].namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		SDL_BlitSurface(graphics->portraitLarge[owner->defendingHeroInstance->portrait], NULL, background, &genRect(64, 58, 391, 38));
		//setting defenderName
		defenderName = owner->defendingHeroInstance->name;
	}
	else //a monster defended
	{
		int bestMonsterID = -1;
		int bestPower = 0;
		for(std::map<si32,std::pair<ui32,si32> >::const_iterator it = owner->army2->slots.begin(); it!=owner->army2->slots.end(); ++it)
		{
			if( CGI->creh->creatures[it->second.first].AIValue > bestPower)
			{
				bestPower = CGI->creh->creatures[it->second.first].AIValue;
				bestMonsterID = it->second.first;
			}
		}
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &genRect(64, 58, 391, 38));
		//setting defenderName
		defenderName =  CGI->creh->creatures[bestMonsterID].namePl;
	}

	//printing attacker and defender's names
	CSDL_Ext::printAtMiddle(attackerName, 156, 44, GEOR16, zwykly, background);
	CSDL_Ext::printAtMiddle(defenderName, 314, 44, GEOR16, zwykly, background);
	//printing casualities
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[523], 235, 360 + 97*step, GEOR16, zwykly, background);
		}
		else
		{
			int xPos = 235 - (br.casualties[step].size()*32 + (br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + step*97;
			for(std::set<std::pair<ui32,si32> >::const_iterator it=br.casualties[step].begin(); it!=br.casualties[step].end(); ++it)
			{
				blitAt(graphics->smallImgs[it->first], xPos, yPos, background);
				std::ostringstream amount;
				amount<<it->second;
				CSDL_Ext::printAtMiddle(amount.str(), xPos+16, yPos + 42, GEOR13, zwykly, background);
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = (LOCPLINT->playerID == owner->attackingHeroInstance->tempOwner);
	switch(br.result)
	{
	case 0: //normal victory
		if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
		{
			CGI->musich->playMusic(musicBase::winBattle);
#ifdef _WIN32
			CGI->videoh->open(VIDEO_WIN);
#else
			CGI->videoh->open(VIDEO_WIN, true);
#endif
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[304], 235, 235, GEOR13, zwykly, background);
		}
		else
		{
			CGI->musich->playMusic(musicBase::loseCombat);
			CGI->videoh->open(VIDEO_LOSE_BATTLE_START);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[311], 235, 235, GEOR13, zwykly, background);
		}
		break;
	case 1: //flee
		if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
		{
			CGI->musich->playMusic(musicBase::winBattle);
#ifdef _WIN32
			CGI->videoh->open(VIDEO_WIN);
#else
			CGI->videoh->open(VIDEO_WIN, true);
#endif
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[303], 235, 235, GEOR13, zwykly, background);
		}
		else
		{
			CGI->musich->playMusic(musicBase::retreatBattle);
			CGI->videoh->open(VIDEO_RETREAT_START);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[310], 235, 235, GEOR13, zwykly, background);
		}
		break;
	case 2: //surrender
		if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
		{
			CGI->musich->playMusic(musicBase::winBattle);
#ifdef _WIN32
			CGI->videoh->open(VIDEO_WIN);
#else
			CGI->videoh->open(VIDEO_WIN, true);
#endif
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[302], 235, 235, GEOR13, zwykly, background);
		}
		else
		{
			CGI->musich->playMusic(musicBase::surrenderBattle);
			CGI->videoh->open(VIDEO_SURRENDER);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[309], 235, 235, GEOR13, zwykly, background);
		}
		break;
	}
}

CBattleResultWindow::~CBattleResultWindow()
{
	SDL_FreeSurface(background);
}

void CBattleResultWindow::activate()
{
	LOCPLINT->showingDialog->set(true);
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

	CGI->videoh->update(107, 70, background, false, true);

	SDL_BlitSurface(background, NULL, to, &pos);
	exit->show(to);
}

void CBattleResultWindow::bExitf()
{
	GH.popInts(2); //first - we; second - battle interface
	LOCPLINT->showingDialog->setn(false);
	CGI->videoh->close();
}

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner): myInt(owner)
{
	pos = position;
	background = BitmapHandler::loadBitmap("comopbck.bmp", true);
	graphics->blueToPlayersAdv(background, LOCPLINT->playerID);

	viewGrid = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintCellBorders, owner, true), boost::bind(&CBattleInterface::setPrintCellBorders, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[427].first)(3,CGI->generaltexth->zelp[427].first), CGI->generaltexth->zelp[427].second, false, "sysopchk.def", NULL, 185, 140, false);
	viewGrid->select(owner->settings.printCellBorders);
	movementShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintStackRange, owner, true), boost::bind(&CBattleInterface::setPrintStackRange, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[428].first)(3,CGI->generaltexth->zelp[428].first), CGI->generaltexth->zelp[428].second, false, "sysopchk.def", NULL, 185, 173, false);
	movementShadow->select(owner->settings.printStackRange);
	mouseShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintMouseShadow, owner, true), boost::bind(&CBattleInterface::setPrintMouseShadow, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[429].first)(3,CGI->generaltexth->zelp[429].first), CGI->generaltexth->zelp[429].second, false, "sysopchk.def", NULL, 185, 207, false);
	mouseShadow->select(owner->settings.printMouseShadow);

	animSpeeds = new CHighlightableButtonsGroup(0);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[422].first),CGI->generaltexth->zelp[422].second, "sysopb9.def",188, 309, 1);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[423].first),CGI->generaltexth->zelp[423].second, "sysob10.def",252, 309, 2);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[424].first),CGI->generaltexth->zelp[424].second, "sysob11.def",315, 309, 4);
	animSpeeds->select(owner->getAnimSpeed(), 1);
	animSpeeds->onChange = boost::bind(&CBattleInterface::setAnimSpeed, owner, _1);

	setToDefault = new AdventureMapButton (CGI->generaltexth->zelp[392].first, CGI->generaltexth->zelp[392].second, boost::bind(&CBattleOptionsWindow::bDefaultf,this), 405, 443, "codefaul.def");
	std::swap(setToDefault->imgs[0][0], setToDefault->imgs[0][1]);
	exit = new AdventureMapButton (CGI->generaltexth->zelp[393].first, CGI->generaltexth->zelp[393].second, boost::bind(&CBattleOptionsWindow::bExitf,this), 516, 443, "soretrn.def",SDLK_RETURN);
	std::swap(exit->imgs[0][0], exit->imgs[0][1]);

	//printing texts to background
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[392], 240, 32, GEOR16, tytulowy, background); //window title
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[393], 122, 211, GEOR16, tytulowy, background); //animation speed
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[394], 122, 292, GEOR16, tytulowy, background); //music volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[395], 122, 358, GEOR16, tytulowy, background); //effects' volume
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[396], 353, 64, GEOR16, tytulowy, background); //auto - combat options
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[397], 353, 264, GEOR16, tytulowy, background); //creature info

		//auto - combat options
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[398], 283, 87, GEOR16, zwykly, background); //creatures
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[399], 283, 117, GEOR16, zwykly, background); //spells
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[400], 283, 147, GEOR16, zwykly, background); //catapult
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[151], 283, 177, GEOR16, zwykly, background); //ballista
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[401], 283, 207, GEOR16, zwykly, background); //first aid tent

		//creature info
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[402], 283, 286, GEOR16, zwykly, background); //all stats
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[403], 283, 316, GEOR16, zwykly, background); //spells only
	
		//general options
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[404], 61, 58, GEOR16, zwykly, background); //hex grid
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[405], 61, 91, GEOR16, zwykly, background); //movement shadow
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[406], 61, 124, GEOR16, zwykly, background); //cursor shadow
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[577], 61, 157, GEOR16, zwykly, background); //spellbook animation
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
: town(siegeTown), owner(_owner)
{
	backWall = BitmapHandler::loadBitmap( getSiegeName(1) );

	for(int g=0; g<ARRAY_COUNT(walls); ++g)
	{
		walls[g] = BitmapHandler::loadBitmap( getSiegeName(g + 2) );
	}
}

CBattleInterface::SiegeHelper::~SiegeHelper()
{
	if(backWall)
		SDL_FreeSurface(backWall);

	for(int g=0; g<ARRAY_COUNT(walls); ++g)
	{
		SDL_FreeSurface(walls[g]);
	}
}

std::string CBattleInterface::SiegeHelper::getSiegeName(ui16 what, ui16 additInfo) const
{
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
	default:
		return "";
	}
}

void CBattleInterface::SiegeHelper::printSiegeBackground(SDL_Surface * to)
{
	blitAt(backWall, owner->pos.w + owner->pos.x - backWall->w, 50 + owner->pos.y, to);
}

void CBattleInterface::SiegeHelper::printPartOfWall(SDL_Surface * to, int what)
{
	Point pos = Point(-1, -1);
	switch(what)
	{
	case 2: //keep
		pos = Point(owner->pos.w + owner->pos.x - walls[what-2]->w, 154 + owner->pos.y);
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
		pos.x = CGI->heroh->wallPositions[town->town->typeID][what - 3].first + owner->pos.x;
		pos.y = CGI->heroh->wallPositions[town->town->typeID][what - 3].second + owner->pos.y;
		break;
	};

	if(pos.x != -1)
	{
		blitAt(walls[what-2], pos.x, pos.y, to);
	}
}
