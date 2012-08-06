#include "StdInc.h"
#include "CBattleInterface.h"

#include "../CGameInfo.h"
#include "../UIFramework/SDL_Extensions.h"
#include "../CAdvmapInterface.h"
#include "../CAnimation.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../CDefHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../CMusicHandler.h"
#include "../CMessage.h"
#include "../../CCallback.h"
#include "../../lib/BattleState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "CCreatureAnimation.h"
#include "../Graphics.h"
#include "../CSpellWindow.h"
#include "../CConfigHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/NetPacks.h"
#include "../CPlayerInterface.h"
#include "../CCreatureWindow.h"
#include "../CVideoHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/map.h"

#include "CBattleAnimations.h"
#include "CBattleInterfaceClasses.h"

#include "../UIFramework/CCursorHandler.h"
#include "../UIFramework/CGuiHandler.h"

#ifndef __GNUC__
const double M_PI = 3.14159265358979323846;
#else
#define _USE_MATH_DEFINES
#include <cmath>
#endif
#include "../../lib/UnlockGuard.h"

using namespace boost::assign;

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

CondSh<bool> CBattleInterface::animsAreDisplayed;

struct CMP_stack2
{
	inline bool operator ()(const CStack& a, const CStack& b)
	{
		return (a.Speed())>(b.Speed());
	}
} cmpst2 ;

static void transformPalette(SDL_Surface * surf, double rCor, double gCor, double bCor)
{
	SDL_Color * colorsToChange = surf->format->palette->colors;
	for(int g=0; g<surf->format->palette->ncolors; ++g)
	{
		if((colorsToChange+g)->b != 132 &&
			(colorsToChange+g)->g != 231 &&
			(colorsToChange+g)->r != 255) //it's not yellow border
		{
			(colorsToChange+g)->r = static_cast<double>((colorsToChange+g)->r) * rCor;
			(colorsToChange+g)->g = static_cast<double>((colorsToChange+g)->g) * gCor;
			(colorsToChange+g)->b = static_cast<double>((colorsToChange+g)->b) * bCor;
		}
	}
}
//////////////////////

void CBattleInterface::addNewAnim(CBattleAnimation * anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	animsAreDisplayed.setn(true);
}

CBattleInterface::CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2, const SDL_Rect & myRect, CPlayerInterface * att, CPlayerInterface * defen)
	: queue(NULL), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0),
	  activeStack(NULL), stackToActivate(NULL), selectedStack(NULL), mouseHoveredStack(-1), lastMouseHoveredStackAnimationTime(-1), previouslyHoveredHex(-1),
	  currentlyHoveredHex(-1), attackingHex(-1), tacticianInterface(NULL),  stackCanCastSpell(false), creatureCasting(false), spellDestSelectMode(false), spellSelMode(NO_LOCATION), spellToCast(NULL), sp(NULL),
	  siegeH(NULL), attackerInt(att), defenderInt(defen), curInt(att), animIDhelper(0),
	  givenCommand(NULL), myTurn(false), resWindow(NULL), moveStarted(false), moveSh(-1), bresult(NULL)

{
	OBJ_CONSTRUCTION;

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
		if(settings["battle"]["showQueue"].Bool())
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
	transformPalette(amountNormal, 0.59, 0.19, 0.93);

	amountPositive = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountPositive);
	transformPalette(amountPositive, 0.18, 1.00, 0.18);

	amountNegative = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNegative);
	transformPalette(amountNegative, 1.00, 0.18, 0.18);

	amountEffNeutral = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountEffNeutral);
	transformPalette(amountEffNeutral, 1.00, 1.00, 0.18);

	////blitting menu background and terrain
// 	blitAt(background, pos.x, pos.y);
// 	blitAt(menu, pos.x, 556 + pos.y);

	//preparing buttons and console
	bOptions = new CAdventureMapButton (CGI->generaltexth->zelp[381].first, CGI->generaltexth->zelp[381].second, boost::bind(&CBattleInterface::bOptionsf,this), 3, 561, "icm003.def", SDLK_o);
	bSurrender = new CAdventureMapButton (CGI->generaltexth->zelp[379].first, CGI->generaltexth->zelp[379].second, boost::bind(&CBattleInterface::bSurrenderf,this), 54, 561, "icm001.def", SDLK_s);
	bFlee = new CAdventureMapButton (CGI->generaltexth->zelp[380].first, CGI->generaltexth->zelp[380].second, boost::bind(&CBattleInterface::bFleef,this), 105, 561, "icm002.def", SDLK_r);
	bFlee->block(!curInt->cb->battleCanFlee());
	bSurrender->block(curInt->cb->battleGetSurrenderCost() < 0);
	bAutofight  = new CAdventureMapButton (CGI->generaltexth->zelp[382].first, CGI->generaltexth->zelp[382].second, boost::bind(&CBattleInterface::bAutofightf,this), 157, 561, "icm004.def", SDLK_a);
	bSpell = new CAdventureMapButton (CGI->generaltexth->zelp[385].first, CGI->generaltexth->zelp[385].second, boost::bind(&CBattleInterface::bSpellf,this), 645, 561, "icm005.def", SDLK_c);
	bSpell->block(true);
	bWait = new CAdventureMapButton (CGI->generaltexth->zelp[386].first, CGI->generaltexth->zelp[386].second, boost::bind(&CBattleInterface::bWaitf,this), 696, 561, "icm006.def", SDLK_w);
	bDefence = new CAdventureMapButton (CGI->generaltexth->zelp[387].first, CGI->generaltexth->zelp[387].second, boost::bind(&CBattleInterface::bDefencef,this), 747, 561, "icm007.def", SDLK_d);
	bDefence->assignedKeys.insert(SDLK_SPACE);
	bConsoleUp = new CAdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleUpf,this), 624, 561, "ComSlide.def", SDLK_UP);
	bConsoleDown = new CAdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleDownf,this), 624, 580, "ComSlide.def", SDLK_DOWN);
	bConsoleDown->setOffset(2);
	console = new CBattleConsole();
	console->pos.x += 211;
	console->pos.y += 560;
	console->pos.w = 406;
	console->pos.h = 38;
	if(tacticsMode)
	{
		btactNext = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bTacticNextStack,this, (CStack*)NULL), 213, 560, "icm011.def", SDLK_SPACE);
		btactEnd = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bEndTacticPhase,this), 419, 560, "icm012.def", SDLK_RETURN);
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
	for(int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		CClickableHex *hex = new CClickableHex();
		hex->myNumber = h;
		hex->pos = hexPosition(h);
		hex->accessible = true;
		hex->myInterface = this;
		bfield.push_back(hex);
	}
	//locking occupied positions on batlefield
	BOOST_FOREACH(const CStack *s, stacks)  //stacks gained at top of this function
		if(s->position >= 0) //turrets have position < 0
			bfield[s->position]->accessible = false;

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
			for(size_t s = 0; s < projectile->ourImages.size(); ++s) //alpha transforming
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
	for(int i=0; i<GameConstants::BFIELD_HEIGHT; ++i) //rows
	{
		for(int j=0; j<GameConstants::BFIELD_WIDTH-2; ++j) //columns
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
	auto obst = curInt->cb->battleGetAllObstacles();
	for(size_t t = 0; t < obst.size(); ++t)
	{
		const int ID = obst[t]->ID;
		if(obst[t]->obstacleType == CObstacleInstance::USUAL)
		{
			idToObstacle[ID] = CDefHandler::giveDef(obst[t]->getInfo().defName);
			for(size_t n = 0; n < idToObstacle[ID]->ourImages.size(); ++n)
			{
				SDL_SetColorKey(idToObstacle[ID]->ourImages[n].bitmap, SDL_SRCCOLORKEY, SDL_MapRGB(idToObstacle[ID]->ourImages[n].bitmap->format,0,255,255));
			}
		}
		else if(obst[t]->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			idToAbsoluteObstacle[ID] = BitmapHandler::loadBitmap(obst[t]->getInfo().defName);
		}
	}

	quicksand = CDefHandler::giveDef("C17SPE1.DEF");
	landMine = CDefHandler::giveDef("C09SPF1.DEF");
	fireWall = CDefHandler::giveDef("C07SPF61");
	bigForceField[0] = CDefHandler::giveDef("C15SPE10.DEF");
	bigForceField[1] = CDefHandler::giveDef("C15SPE7.DEF");
	smallForceField[0] = CDefHandler::giveDef("C15SPE1.DEF");
	smallForceField[1] = CDefHandler::giveDef("C15SPE4.DEF");

	BOOST_FOREACH(auto hex, bfield)
		addChild(hex);

	if(tacticsMode)
		bTacticNextStack();

	CCS->musich->stopMusic();

	int channel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	CCS->soundh->setCallback(channel, boost::bind(&CMusicHandler::playMusicFromSet, CCS->musich, CCS->musich->battleMusics, -1));
    memset(stackCountOutsideHexes, 1, GameConstants::BFIELD_SIZE * sizeof(bool)); //initialize array with trues

	currentAction = INVALID;
	selectedAction = INVALID;
	addUsedEvents(RCLICK | MOVE | KEYBOARD);
}

CBattleInterface::~CBattleInterface()
{
	curInt->battleInt = NULL;
	givenCommand->cond.notify_all(); //that two lines should make any activeStack waiting thread to finish


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

	BOOST_FOREACH(auto hex, bfield)
		delete hex;

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

	delete quicksand;
	delete landMine;
	delete fireWall;
	delete smallForceField[0];
	delete smallForceField[1];
	delete bigForceField[0];
	delete bigForceField[1];

	delete siegeH;

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
	Settings cellBorders = settings.write["battle"]["cellBorders"];
	cellBorders->Bool() = set;

	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintStackRange(bool set)
{
	Settings stackRange = settings.write["battle"]["stackRange"];
	stackRange->Bool() = set;

	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintMouseShadow(bool set)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = set;
}

void CBattleInterface::activate()
{
	CIntObject::activate();
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();

	BOOST_FOREACH(auto hex, bfield)
		hex->activate();

	if(attackingHero)
		attackingHero->activate();
	if(defendingHero)
		defendingHero->activate();
	if(settings["battle"]["showQueue"].Bool())
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
	CIntObject::deactivate();

	bOptions->deactivate();
	bSurrender->deactivate();
	bFlee->deactivate();
	bAutofight->deactivate();
	bSpell->deactivate();
	bWait->deactivate();
	bDefence->deactivate();

	BOOST_FOREACH(auto hex, bfield)
		hex->deactivate();

	if(attackingHero)
		attackingHero->deactivate();
	if(defendingHero)
		defendingHero->deactivate();
	if(settings["battle"]["showQueue"].Bool())
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

void CBattleInterface::showAll(SDL_Surface * to)
{
	show(to);
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
		if(settings["battle"]["cellBorders"].Bool())
		{
			CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, to, &pos);
		}
		//Blit absolute obstacles
		BOOST_FOREACH(auto &oi, curInt->cb->battleGetAllObstacles())
			if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
				blitAt(imageOfObstacle(*oi), pos.x + oi->getInfo().width, pos.y + oi->getInfo().height, to);
	}
	//printing hovered cell
	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
	{
		if(bfield[b]->strictHovered && bfield[b]->hovered)
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
				if (activeStack->attackerOwned)
				{
					if(attackingHeroInstance)
						schoolLevel = attackingHeroInstance->getSpellSchoolLevel(&spToCast);
				}
				else
				{
					if (defendingHeroInstance)
						schoolLevel = defendingHeroInstance->getSpellSchoolLevel(&spToCast);
				}

				//obtaining range and printing it
				auto shaded = spToCast.rangeInHexes(b, schoolLevel, curInt->cb->battleGetMySide());
				BOOST_FOREACH(auto shadedHex, shaded) //for spells with range greater then one hex
				{
					if(settings["battle"]["mouseShadow"].Bool() && (shadedHex % GameConstants::BFIELD_WIDTH != 0) && (shadedHex % GameConstants::BFIELD_WIDTH != 16))
					{
						int x = 14 + ((shadedHex/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(shadedHex%GameConstants::BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (shadedHex/GameConstants::BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &temp_rect);
					}
				}
			}
			else if(settings["battle"]["mouseShadow"].Bool()) //when not casting spell
			{//TODO: do not check it every frame
				if (activeStack) //highlight all attackable hexes
				{
					std::set<BattleHex> set = curInt->cb->battleGetAttackedHexes(activeStack, currentlyHoveredHex, attackingHex);
					BOOST_FOREACH(BattleHex hex, set)
					{
						int x = 14 + ((hex/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(hex%GameConstants::BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (hex/GameConstants::BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &temp_rect);
					}
				}

				//patch by ench0: show enemy stack movement shadow
				// activeStack == NULL means it is opponent's turn...
				if(activeStack && settings["battle"]["stackRange"].Bool())
				{
					// display the movement shadow of the stack at b (i.e. stack under mouse)
					const CStack * const shere = curInt->cb->battleGetStackByPos(b, false);
					if (shere && shere != activeStack && shere->alive())
					{
						std::vector<BattleHex> v = curInt->cb->battleGetAvailableHexes(shere, true );
						BOOST_FOREACH (BattleHex hex, v)
						{
							int x = 14 + ((hex / GameConstants::BFIELD_WIDTH ) % 2 == 0 ? 22 : 0) + 44 * (hex % GameConstants::BFIELD_WIDTH) + pos.x;
							int y = 86 + 42 * (hex / GameConstants::BFIELD_WIDTH) + pos.y;
							SDL_Rect temp_rect = genRect (cellShade->h, cellShade->w, x, y);
							CSDL_Ext::blit8bppAlphaTo24bpp (cellShade, NULL, to, &temp_rect);
						}
					}
				}

				//always highlight pointed hex
				int x = 14 + ((b/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(b%GameConstants::BFIELD_WIDTH) + pos.x;
				int y = 86 + 42 * (b/GameConstants::BFIELD_WIDTH) + pos.y;
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
	auto obstacles = curInt->cb->battleGetAllObstacles();
	std::multimap<BattleHex, int> hexToObstacle;

	for(size_t b = 0; b < obstacles.size(); ++b)
	{
		const auto &oi = obstacles[b];
		if(oi->obstacleType != CObstacleInstance::ABSOLUTE_OBSTACLE  && oi->obstacleType != CObstacleInstance::MOAT)
		{
			//BattleHex position = CGI->heroh->obstacles.find(obstacles[b].ID)->second.getMaxBlocked(obstacles[b].pos);
			hexToObstacle.insert(std::make_pair(oi->pos, b));
		}
	}

	////showing units //a lot of work...
	std::vector<const CStack *> stackAliveByHex[GameConstants::BFIELD_SIZE];
	//double loop because dead stacks should be printed first
	for (size_t i = 0; i < stacks.size(); i++)
	{
		const CStack *s = stacks[i];
		if(creAnims.find(s->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->getType() != 5 && s->position >= 0) //don't show turrets here
			stackAliveByHex[s->position].push_back(s);
	}
	std::vector<const CStack *> stackDeadByHex[GameConstants::BFIELD_SIZE];
	for (size_t i = 0; i < stacks.size(); i++)
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

		bool changedStack = false;

		//activation of next stack
		if(pendingAnims.size() == 0 && stackToActivate != NULL)
		{
			activateStack();
			changedStack = true;

		}
		//anims ended
		animsAreDisplayed.setn(false);

		if(changedStack)
		{
			//we may have changed active interface (another side in hot-seat), 
			// so we can't continue drawing with old setting. So we call ourselves again and end.
			SDL_SetClipRect(to, &buf); //restoring previous clip_rect
			show(to);
			return;
		}
	}

	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b) //showing dead stacks
	{
		for(size_t v=0; v<stackDeadByHex[b].size(); ++v)
		{
			creAnims[stackDeadByHex[b][v]->ID]->nextFrame(to, creAnims[stackDeadByHex[b][v]->ID]->pos.x, creAnims[stackDeadByHex[b][v]->ID]->pos.y, creDir[stackDeadByHex[b][v]->ID], animCount, false); //increment always when moving, never if stack died
		}
	}
	std::vector<const CStack *> flyingStacks; //flying stacks should be displayed later, over other stacks and obstacles
	if (!siegeH)
	{
		for(int b = 0; b < GameConstants::BFIELD_SIZE; ++b) //showing alive stacks
		{
			showObstacles(&hexToObstacle, obstacles, b, to);
			showAliveStacks(stackAliveByHex, b, &flyingStacks, to);
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
					showObstacles(&hexToObstacle, obstacles, hex, to);
					showAliveStacks(stackAliveByHex, hex, &flyingStacks, to);
					showPieceOfWall(to, hex, stacks);
				}

				++runNum;
			}
		}
	}

	for(size_t b = 0; b < flyingStacks.size(); ++b) //showing flying stacks
		showAliveStack(flyingStacks[b], to);

	//units shown

	// Show projectiles
	projectileShowHelper(to);

	//showing spell effects
	if(battleEffects.size())
	{
		for(std::list<BattleEffect>::iterator it = battleEffects.begin(); it!=battleEffects.end(); ++it)
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

	//showing in-game console
	LOCPLINT->cingconsole->show(to);

	Rect posWithQueue = Rect(pos.x, pos.y, 800, 600);

	if(settings["battle"]["showQueue"].Bool())
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

	for(size_t v = 0; v < aliveStacks[hex].size(); ++v)
	{
		const CStack *s = aliveStacks[hex][v];

		if(!s->hasBonusOfType(Bonus::FLYING) || creAnims[s->ID]->getType() != 0)
			showAliveStack(s, to);
		else
			flyingStacks->push_back(s);
	}
}

void CBattleInterface::showObstacles(std::multimap<BattleHex, int> *hexToObstacle, std::vector<shared_ptr<const CObstacleInstance> > &obstacles, int hex, SDL_Surface *to)
{
	std::pair<std::multimap<BattleHex, int>::const_iterator, std::multimap<BattleHex, int>::const_iterator> obstRange =
		hexToObstacle->equal_range(hex);

	for(std::multimap<BattleHex, int>::const_iterator it = obstRange.first; it != obstRange.second; ++it)
	{
		const CObstacleInstance & curOb = *obstacles[it->second];
		SDL_Surface *toBlit = imageOfObstacle(curOb);
		Point p = whereToBlitObstacleImage(toBlit, curOb);
		blitAt(toBlit, p.x, p.y, to);
	}
}

void CBattleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_q && key.state == SDL_PRESSED)
	{
		if(settings["battle"]["showQueue"].Bool()) //hide queue
			hideQueue();
		else
			showQueue();

	}
	else if(key.keysym.sym == SDLK_ESCAPE && spellDestSelectMode)
	{
		endCastingSpell();
	}
}
void CBattleInterface::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	auto hexItr = std::find_if(bfield.begin(), bfield.end(), [](const CClickableHex *hex)
	{
		return hex->hovered && hex->strictHovered;
	});

	handleHex(hexItr == bfield.end() ? -1 : (*hexItr)->myNumber, MOVE);
}

void CBattleInterface::setBattleCursor(const int myNumber)
{
	const CClickableHex & hoveredHex = *bfield[myNumber];
	CCursorHandler *cursor = CCS->curh;

	const double subdividingAngle = 2.0*M_PI/6.0; // Divide a hex into six sectors.
	const double hexMidX = hoveredHex.pos.x + hoveredHex.pos.w/2;
	const double hexMidY = hoveredHex.pos.y + hoveredHex.pos.h/2;
	const double cursorHexAngle = M_PI - atan2(hexMidY - cursor->ypos, cursor->xpos - hexMidX) + subdividingAngle/2; //TODO: refactor this nightmare
	const double sector = fmod(cursorHexAngle/subdividingAngle, 6.0);
	const int zigzagCorrection = !((myNumber/GameConstants::BFIELD_WIDTH)%2); // Off-by-one correction needed to deal with the odd battlefield rows.

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
	if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - 1))
	{
		sectorCursor[0] = -1;
	}
	// Check top left, top right as well as above for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == 0)
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

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
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
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[1] = -1;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[2] = -1;
		}
	}
	// Check to the right.
	if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + 1))
	{
		sectorCursor[3] = -1;
	}
	// Check bottom right, bottom left as well as below for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_HEIGHT - 1)
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

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
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
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[4] = -1;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
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
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //top left
			break;
		case 2:
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection; //top right
			break;
		case 3:
			break;
			attackingHex = myNumber + 1; //right
		case 4:
			break;
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection; //bottom right
		case 5:
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //bottom left
			break;
	}
	BattleHex hex(attackingHex);
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
		curInt->showYesNoDialog(surrenderMessage, boost::bind(&CBattleInterface::reallySurrender,this), 0, false);
	}
}

void CBattleInterface::bFleef()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if( curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = boost::bind(&CBattleInterface::reallyFlee,this);
		curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, 0, false); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<CComponent*> comps;
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

	if ( myTurn && curInt->cb->battleCanCastSpell())
	{
		const CGHeroInstance * chi = NULL;
		if(attackingHeroInstance->tempOwner == curInt->playerID)
			chi = attackingHeroInstance;
		else
			chi = defendingHeroInstance;
		CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), chi, curInt);
		GH.pushInt(spellWindow);
	}
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
	Point coords = CClickableHex::getXYUnitAnim(stack->position, stack->owner == attackingHeroInstance->tempOwner, stack, this);

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

	queue->update();
}

void CBattleInterface::stackActivated(const CStack * stack) //TODO: check it all before game state is changed due to abilities
{
	//givenCommand = NULL;
	stackToActivate = stack;
	waitForAnims();
	//if(pendingAnims.size() == 0)
	if(stackToActivate) //during waiting stack may have gotten activated through show
		activateStack();
}

void CBattleInterface::stackMoved(const CStack * stack, std::vector<BattleHex> destHex, int distance)
{
	addNewAnim(new CMovementAnimation(this, stack, destHex, distance));
	waitForAnims();
}

void CBattleInterface::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	for (size_t h = 0; h < attackedInfos.size(); ++h)
	{
		if (!attackedInfos[h].cloneKilled) //FIXME: play dead animation for cloned creature before it vanishes
			addNewAnim(new CDefenceAnimation(attackedInfos[h], this));
		if (attackedInfos[h].rebirth)
		{
			displayEffect(50, attackedInfos[h].defender->position); //TODO: play reverse death animation
			CCS->soundh->playSound(soundBase::RESURECT);
		}
	}
	waitForAnims();
	int targets = 0, killed = 0, damage = 0;
	for(size_t h = 0; h < attackedInfos.size(); ++h)
	{
		++targets;
		killed += attackedInfos[h].amountKilled;
		damage += attackedInfos[h].dmg;
	}
	if (attackedInfos.front().cloneKilled) //FIXME: cloned stack is already removed
		return;
	if (targets > 1)
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, true); //creatures perish
	else
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, false);

	for(size_t h = 0; h < attackedInfos.size(); ++h)
	{
		if (attackedInfos[h].rebirth)
			creAnims[attackedInfos[h].defender->ID]->setType(CCreatureAnim::HOLDING);
		if (attackedInfos[h].cloneKilled)
			stackRemoved(attackedInfos[h].defender->ID);
	}
}

void CBattleInterface::stackAttacking( const CStack * attacker, BattleHex dest, const CStack * attacked, bool shooting )
{
	if (shooting)
	{
		addNewAnim(new CShootingAnimation(this, attacker, dest, attacked));
	}
	else
	{
		addNewAnim(new CMeleeAttackAnimation(this, attacker, dest, attacked));
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

void CBattleInterface::giveCommand(ui8 action, BattleHex tile, ui32 stackID, si32 additional, si32 selected)
{
	const CStack *stack = curInt->cb->battleGetStackByID(stackID);
	if(!stack && action != BattleAction::HERO_SPELL && action != BattleAction::RETREAT && action != BattleAction::SURRENDER)
	{
		return;
	}

	if(stack && stack != activeStack)
		tlog3 << "Warning: giving an order to a non-active stack?\n";

	BattleAction * ba = new BattleAction(); //is deleted in CPlayerInterface::activeStack()
	ba->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
	ba->actionType = action;
	ba->destinationTile = tile;
	ba->stackNumber = stackID;
	ba->additionalInfo = additional;
	ba->selectedStack = selected;

	//some basic validations
	switch(action)
	{
		case BattleAction::WALK_AND_ATTACK:
			assert(curInt->cb->battleGetStackByPos(additional)); //stack to attack must exist
		case BattleAction::WALK:
		case BattleAction::SHOOT:
		case BattleAction::CATAPULT:
			assert(tile < GameConstants::BFIELD_SIZE);
			break;
	}

	if(!tacticsMode)
	{
		tlog5 << "Setting command for " << (stack ? stack->nodeName() : "hero") << std::endl;
		myTurn = false;
		activeStack = NULL;
		givenCommand->setn(ba);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(ba);
		vstd::clear_pointer(ba);
		activeStack = NULL;
		//next stack will be activated when action ends
	}
}

bool CBattleInterface::isTileAttackable(const BattleHex & number) const
{
	for(size_t b=0; b<occupyableHexes.size(); ++b)
	{
		if(BattleHex::mutualPosition(occupyableHexes[b], number) != -1 || occupyableHexes[b] == number)
			return true;
	}
	return false;
}

bool CBattleInterface::isCatapultAttackable(BattleHex hex) const
{
	if(!siegeH  ||  tacticsMode)
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
	handleHex(whichOne, LCLICK);
}

void CBattleInterface::stackIsCatapulting(const CatapultAttack & ca)
{
	for(std::set< std::pair< std::pair< ui8, si16 >, ui8> >::const_iterator it = ca.attackedParts.begin(); it != ca.attackedParts.end(); ++it)
	{
		const CStack * stack = curInt->cb->battleGetStackByID(ca.attacker);
		addNewAnim(new CShootingAnimation(this, stack, it->first.second, NULL, true, it->second));

		SDL_FreeSurface(siegeH->walls[it->first.first + 2]);
		siegeH->walls[it->first.first + 2] = BitmapHandler::loadBitmap(
			siegeH->getSiegeName(it->first.first + 2, curInt->cb->battleGetWallState(it->first.first)) );
	}
	waitForAnims();
}

void CBattleInterface::battleFinished(const BattleResult& br)
{
	bresult = &br;
	{
		auto unlockPim = vstd::makeUnlockGuard(*LOCPLINT->pim);
		animsAreDisplayed.waitUntil(false);
	}
	displayBattleFinished();
	activeStack = NULL;
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
	case Spells::MAGIC_ARROW:
		{
			//initialization of anims
			anims.push_back("C20SPX0.DEF"); anims.push_back("C20SPX1.DEF"); anims.push_back("C20SPX2.DEF"); anims.push_back("C20SPX3.DEF"); anims.push_back("C20SPX4.DEF");
		}
	case Spells::ICE_BOLT:
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
			Point destcoord = CClickableHex::getXYUnitAnim(sc->tile, !sc->side, curInt->cb->battleGetStackByPos(sc->tile), this); //position attacked by arrow
			destcoord.x += 250; destcoord.y += 240;

			//animation angle
			double angle = atan2(static_cast<double>(destcoord.x - srccoord.x), static_cast<double>(destcoord.y - srccoord.y));
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

			int steps = sqrt(static_cast<double>((destcoord.x - srccoord.x)*(destcoord.x - srccoord.x) + (destcoord.y - srccoord.y) * (destcoord.y - srccoord.y))) / 40;
			if(steps <= 0)
				steps = 1;

			int dx = (destcoord.x - srccoord.x - animDef->ourImages[0].bitmap->w)/steps, dy = (destcoord.y - srccoord.y - animDef->ourImages[0].bitmap->h)/steps;

			delete animDef;
			addNewAnim(new CSpellEffectAnimation(this, animToDisplay, srccoord.x, srccoord.y, dx, dy, Vflip));

			break; //for 15 and 16 cases
		}
	case Spells::LIGHTNING_BOLT:
	case Spells::TITANS_LIGHTNING_BOLT:
	case Spells::THUNDERBOLT:
	case Spells::CHAIN_LIGHTNING: //TODO: zigzag effect
		for (auto it = sc->affectedCres.begin(); it != sc->affectedCres.end(); ++it) //in case we have multiple targets
		{
			displayEffect(1, curInt->cb->battleGetStackByID(*it, false)->position);
			displayEffect(spell.mainEffectAnim, curInt->cb->battleGetStackByID(*it, false)->position);
		}
		break;
	case Spells::DISPEL:
	case Spells::CURE:
	case Spells::RESURRECTION:
	case Spells::ANIMATE_DEAD:
	case Spells::DISPEL_HELPFUL_SPELLS:
	case Spells::SACRIFICE: //TODO: animation upon killed stack
		for(std::set<ui32>::const_iterator it = sc->affectedCres.begin(); it != sc->affectedCres.end(); ++it)
		{
			displayEffect(spell.mainEffectAnim, curInt->cb->battleGetStackByID(*it, false)->position);
		}
		break;
	case Spells::SUMMON_FIRE_ELEMENTAL:
	case Spells::SUMMON_EARTH_ELEMENTAL:
	case Spells::SUMMON_WATER_ELEMENTAL:
	case Spells::SUMMON_AIR_ELEMENTAL:
	case Spells::CLONE:
	case Spells::REMOVE_OBSTACLE:
		addNewAnim(new CDummyAnimation(this, 2)); //interface won't return until animation is played. TODO: make it smarter?
		break;
	} //switch(sc->id)

	//support for resistance
	for(size_t j = 0; j < sc->resisted.size(); ++j)
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
				case Spells::STONE_GAZE:
					customSpell = true;
					plural = true;
					textID = 558;
					break;
				case Spells::POISON:
					customSpell = true;
					plural = true;
					textID = 561;
					break;
				case Spells::BIND:
					customSpell = true;
					text = CGI->generaltexth->allTexts[560];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl );
					break;	//Roots and vines bind the %s to the ground!
				case Spells::DISEASE:
					customSpell = true;
					plural = true;
					textID = 553;
					break;
				case Spells::PARALYZE:
					customSpell = true;
					plural = true;
					textID = 563;
					break;
				case Spells::AGE:
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
				case Spells::THUNDERBOLT:
					text = CGI->generaltexth->allTexts[367];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					console->addText(text);
					text = CGI->generaltexth->allTexts[343].substr(1, CGI->generaltexth->allTexts[343].size() - 1); //Does %d points of damage.
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay)); //no more text afterwards
					console->addText(text);
					customSpell = true;
					text = ""; //yeah, it's a terrible mess
					break;
				case Spells::DISPEL_HELPFUL_SPELLS:
					text = CGI->generaltexth->allTexts[555];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					customSpell = true;
					break;
				case Spells::DEATH_STARE:
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
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->getName());
				}
				else
				{
					text = CGI->generaltexth->allTexts[textID];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->getName());
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
			boost::algorithm::replace_first(text, "%s", "Something");
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
	//mana absorption
	if (sc->manaGained)
	{
		Point leftHero = Point(15, 30) + pos;
		Point rightHero = Point(755, 30) + pos;
		addNewAnim(new CSpellEffectAnimation(this, sc->side ? "SP07_A.DEF" : "SP07_B.DEF", leftHero.x, leftHero.y, 0, 0, false));
		addNewAnim(new CSpellEffectAnimation(this, sc->side ? "SP07_B.DEF" : "SP07_A.DEF", rightHero.x, rightHero.y, 0, 0, false));
	}
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
	sp = CGI->spellh->spells[spellID];
	spellSelMode = ANY_LOCATION;
	if(sp->getTargetType() == CSpell::CREATURE)
	{
		spellSelMode = selectionTypeByPositiveness(*sp);
	}
	if(sp->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(castingHero && castingHero->getSpellSchoolLevel(sp) < 3)
			spellSelMode = selectionTypeByPositiveness(*sp);
		else
			spellSelMode = NO_LOCATION;
	}
	if(sp->getTargetType() == CSpell::OBSTACLE)
	{
		spellSelMode = OBSTACLE;
	} //FIXME: Remove Obstacle has range X, unfortunatelly :(
	else if(sp->range[ castingHero->getSpellSchoolLevel(sp) ] == "X") //spell has no range
	{
		spellSelMode = NO_LOCATION;
	}

	if(sp->range[ castingHero->getSpellSchoolLevel(sp) ].size() > 1) //spell has many-hex range
	{
		spellSelMode = ANY_LOCATION;
	}

	if(spellID == Spells::FIRE_WALL  ||  spellID == Spells::FORCE_FIELD)
	{
		spellSelMode = FREE_LOCATION;
	}

	if (spellSelMode == NO_LOCATION) //user does not have to select location
	{
		spellToCast->destinationTile = -1;
		curInt->cb->battleMakeAction(spellToCast);
		endCastingSpell();
	}
	else
	{
		possibleActions.clear();
		possibleActions.push_back (spellSelMode); //only this one action can be performed at the moment
		GH.fakeMouseMove();//update cursor
	}
}

void CBattleInterface::displayEffect(ui32 effect, int destTile)
{
	addNewAnim(new CSpellEffectAnimation(this, effect, destTile));
}

void CBattleInterface::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	const CStack * stack = curInt->cb->battleGetStackByID(bte.stackID);
	//don't show animation when no HP is regenerated
	switch (bte.effect)
	{
		case Bonus::HP_REGENERATION:
			displayEffect(74, stack->position);
			CCS->soundh->playSound(soundBase::REGENER);
			break;
		case Bonus::MANA_DRAIN:
			displayEffect(77, stack->position);
			CCS->soundh->playSound(soundBase::MANADRAI);
			break;
		case Bonus::POISON:
			displayEffect(67, stack->position);
			CCS->soundh->playSound(soundBase::POISON);
			break;
		case Bonus::FEAR:
			displayEffect(15, stack->position);
			CCS->soundh->playSound(soundBase::FEAR);
			break;
		case Bonus::MORALE:
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(20,stack->position);
			console->addText(hlp);
			break;
		}
		default:
			return;
	}
	//waitForAnims(); //fixme: freezes game :?
}

void CBattleInterface::setAnimSpeed(int set)
{
	Settings speed = settings.write["battle"]["animationSpeed"];
	speed->Float() = set;
}

int CBattleInterface::getAnimSpeed() const
{
	return settings["battle"]["animationSpeed"].Float();
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
	bWait->block(vstd::contains(s->state, EBattleStackState::WAITING)); //block waiting button if stack has been already waiting

	//block cast spell button if hero doesn't have a spellbook
	bSpell->block(!curInt->cb->battleCanCastSpell());
	bSurrender->block((curInt == attackerInt ? defendingHeroInstance : attackingHeroInstance) == NULL);
	bFlee->block(!curInt->cb->battleCanFlee());
	bSurrender->block(curInt->cb->battleGetSurrenderCost() < 0);


	//set casting flag to true if creature can use it to not check it every time
	const Bonus *spellcaster = s->getBonus(Selector::type(Bonus::SPELLCASTER)),
		*randomSpellcaster = s->getBonus(Selector::type(Bonus::RANDOM_SPELLCASTER));
	if (s->casts &&  (spellcaster || randomSpellcaster))
	{
		stackCanCastSpell = true;
		if(randomSpellcaster)
			creatureSpellToCast = -1;
		else
			creatureSpellToCast = curInt->cb->battleGetRandomStackSpell(s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
	}
	else
	{
		stackCanCastSpell = false;
		creatureSpellToCast = -1;
	}

	getPossibleActionsForStack (s);

	if(!pendingAnims.size() && !active)
		activate();

	GH.fakeMouseMove();
}

double CBattleInterface::getAnimSpeedMultiplier() const
{
	switch(getAnimSpeed())
	{
	case 1:
		return 3.5;
	case 2:
		return 2.2;
	case 4:
		return 1.0;
	default:
		return 0.0;
	}
}

void CBattleInterface::endCastingSpell()
{
	assert(spellDestSelectMode);

	delete spellToCast;
	spellToCast = NULL;
	sp = NULL;
	spellDestSelectMode = false;
	CCS->curh->changeGraphic(1, 6);

	if (activeStack)
	{
		getPossibleActionsForStack (activeStack); //restore actions after they were cleared
		myTurn = true;
	}
}

void CBattleInterface::getPossibleActionsForStack(const CStack * stack)
{
	possibleActions.clear();
	if (tacticsMode)
	{
		possibleActions += MOVE_TACTICS, CHOOSE_TACTICS_STACK;
	}
	else
	{
		//first action will be prioritized over later ones
		if (stack->casts) //TODO: check for battlefield effects that prevent casting?
		{
			if (stack->hasBonusOfType (Bonus::SPELLCASTER))
			{
				 //TODO: poll possible spells
				const CSpell * spell;
				BonusList spellBonuses = *stack->getBonuses (Selector::type(Bonus::SPELLCASTER));
				BOOST_FOREACH (Bonus * spellBonus, spellBonuses)
				{
					spell = CGI->spellh->spells[spellBonus->subtype];
					if (spell->isRisingSpell())
					{
						possibleActions.push_back (RISING_SPELL);
					}
					//possibleActions.push_back (NO_LOCATION);
					//possibleActions.push_back (ANY_LOCATION);
					//TODO: allow stacks cast aimed spells
					//possibleActions.push_back (OTHER_SPELL);
					else
					{
						switch (spellBonus->subtype)
						{
							case Spells::REMOVE_OBSTACLE:
								possibleActions.push_back (OBSTACLE);
								break;
							default:
								possibleActions.push_back (selectionTypeByPositiveness (*spell));
								break;
						}
					}

				}
				std::sort(possibleActions.begin(), possibleActions.end());
				auto it = std::unique (possibleActions.begin(), possibleActions.end());
				possibleActions.erase (it, possibleActions.end());
			}
			if (stack->hasBonusOfType (Bonus::RANDOM_SPELLCASTER))
				possibleActions.push_back (RANDOM_GENIE_SPELL);
			if (stack->hasBonusOfType (Bonus::DAEMON_SUMMONING))
				possibleActions.push_back (RISE_DEMONS);
		}
		if (stack->shots && stack->hasBonusOfType (Bonus::SHOOTER))
			possibleActions.push_back (SHOOT);
		if (stack->hasBonusOfType (Bonus::RETURN_AFTER_STRIKE))
			possibleActions.push_back (ATTACK_AND_RETURN);

		possibleActions.push_back(ATTACK); //all active stacks can attack
		possibleActions.push_back(WALK_AND_ATTACK); //not all stacks can always walk, but we will check this elsewhere

		if (stack->canMove() && stack->Speed()) //probably no reason to try move war machines or bound stacks
			possibleActions.push_back (MOVE_STACK); //all active stacks can attack

		if (siegeH && stack->hasBonusOfType (Bonus::CATAPULT)) //TODO: check shots
			possibleActions.push_back (CATAPULT);
		if (stack->hasBonusOfType (Bonus::HEALER))
			possibleActions.push_back (HEAL);
	}
}

void CBattleInterface::showAliveStack(const CStack *stack, SDL_Surface * to)
{
	int ID = stack->ID;
	if(creAnims.find(ID) == creAnims.end()) //eg. for summoned but not yet handled stacks
		return;
	const CCreature *creature = stack->getCreature();
	SDL_Rect unitRect = {creAnims[ID]->pos.x, creAnims[ID]->pos.y, uint16_t(creAnims[ID]->fullWidth), uint16_t(creAnims[ID]->fullHeight)};

	int animType = creAnims[ID]->getType();

	int affectingSpeed = getAnimSpeed();
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
	//bool shootingFinished = true;
	for (std::list<ProjectileInfo>::iterator it = projectiles.begin(); it != projectiles.end(); ++it)
	{
		if (it->stackID == ID)
		{
			//shootingFinished = false;
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
        const BattleHex nextPos = stack->position + (stack->attackerOwned ? 1 : -1);
        const bool edge = stack->position % GameConstants::BFIELD_WIDTH == (stack->attackerOwned ? GameConstants::BFIELD_WIDTH - 2 : 1);
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
			Colors::Cornsilk,
			to
		);
	}
}

void CBattleInterface::showPieceOfWall(SDL_Surface * to, int hex, const std::vector<const CStack*> & stacks)
{
	if(!siegeH)
		return;

#ifdef CPP11_USE_INITIALIZERS_LIST
	//note - std::list<int> must be specified to avoid type deduction by gcc (may not work in other compilers)
	static const std::map<int, std::list<int> > hexToPart = {
		{12,  std::list<int>{8, 1, 7}}, {45,  std::list<int>{12, 6}},
		{101, std::list<int>{10}},      {118, std::list<int>{2}},
		{165, std::list<int>{11}},      {186, std::list<int>{3}}};
#else
	using namespace boost::assign;
	static const std::map<int, std::list<int> > hexToPart = map_list_of<int, std::list<int> >(12, list_of<int>(8)(1)(7))(45, list_of<int>(12)(6))
		(101, list_of<int>(10))(118, list_of<int>(2))(165, list_of<int>(11))(186, list_of<int>(3));
#endif
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

	//draw absolute obstacles (cliffs and so on)
	BOOST_FOREACH(auto &oi, curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE/*  ||  oi.obstacleType == CObstacleInstance::MOAT*/)
			blitAt(imageOfObstacle(*oi), oi->getInfo().width, oi->getInfo().height, backgroundWithHexes);
	}

	if(settings["battle"]["cellBorders"].Bool())
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, NULL, backgroundWithHexes, NULL);

	if(settings["battle"]["stackRange"].Bool())
	{
		std::vector<BattleHex> hexesToShade = occupyableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		BOOST_FOREACH(BattleHex hex, hexesToShade)
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
	char tabh[200] = {0};
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
	std::list< std::list<ProjectileInfo>::iterator > toBeDeleted;
	for(std::list<ProjectileInfo>::iterator it=projectiles.begin(); it!=projectiles.end(); ++it)
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
	for(std::list< std::list<ProjectileInfo>::iterator >::iterator it = toBeDeleted.begin(); it!= toBeDeleted.end(); ++it)
	{
		projectiles.erase(*it);
	}
}

void CBattleInterface::endAction(const BattleAction* action)
{
	const CStack * stack = curInt->cb->battleGetStackByID(action->stackNumber);
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

	if(stack && action->actionType == BattleAction::WALK && creAnims[action->stackNumber]->getType() != 2) //walk or walk & attack
	{
		pendingAnims.push_back(std::make_pair(new CMovementEndAnimation(this, stack, action->destinationTile), false));
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
			addNewAnim(new CReverseAnimation(this, s, s->position, false));
		}
	}

	queue->update();

	if(tacticsMode) //stack ended movement in tactics phase -> select the next one
		bTacticNextStack(stack); 

	if( action->actionType == BattleAction::HERO_SPELL) //we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
		redrawBackgroundWithHexes(activeStack);
}

void CBattleInterface::hideQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = false;

	queue->deactivate();

	if(!queue->embedded)
	{
		moveBy(Point(0, -queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void CBattleInterface::showQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = true;

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
			pendingAnims.push_back(std::make_pair(new CMovementStartAnimation(this, stack), false));
		}
	}

	if(active)
		deactivate();

	char txt[400];

	if (action->actionType == BattleAction::HERO_SPELL) //when hero casts spell
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
	auto unlockPim = vstd::makeUnlockGuard(*LOCPLINT->pim);
	animsAreDisplayed.waitWhileTrue();
}

void CBattleInterface::bEndTacticPhase()
{
	activeStack = NULL;
	btactEnd->block(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed(0, true); //should bound stacks be immobile?
}

void CBattleInterface::bTacticNextStack(const CStack *current /*= NULL*/)
{
	if(!current)
		current = activeStack;

	//no switching stacks when the current one is moving
	waitForAnims();

	TStacks stacksOfMine = tacticianInterface->cb->battleGetStacks(CBattleCallback::ONLY_MINE);
	stacksOfMine.erase(std::remove_if(stacksOfMine.begin(), stacksOfMine.end(), &immobile), stacksOfMine.end());
	TStacks::iterator it = vstd::find(stacksOfMine, current);
	if(it != stacksOfMine.end() && ++it != stacksOfMine.end())
		stackActivated(*it);
	else
		stackActivated(stacksOfMine.front());

}

CBattleInterface::PossibleActions CBattleInterface::selectionTypeByPositiveness(const CSpell & spell)
{
	switch(spell.positiveness)
	{
	case CSpell::NEGATIVE :
		return HOSTILE_CREATURE_SPELL;
	case CSpell::NEUTRAL:
		return ANY_CREATURE;
	case CSpell::POSITIVE:
		return FRIENDLY_CREATURE_SPELL;
	}
	assert(0);
	return NO_LOCATION; //should never happen
}

std::string formatDmgRange(std::pair<ui32, ui32> dmgRange)
{
	if(dmgRange.first != dmgRange.second)
		return (boost::format("%d - %d") % dmgRange.first % dmgRange.second).str();
	else
		return (boost::format("%d") % dmgRange.first).str();
}

bool CBattleInterface::canStackMoveHere (const CStack * activeStack, BattleHex myNumber)
{
	std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes (activeStack, false);
	int shiftedDest = myNumber + (activeStack->attackerOwned ? 1 : -1);

	if (vstd::contains(acc, myNumber))
		return true;
	else if (activeStack->doubleWide() && vstd::contains(acc, shiftedDest))
		return true;
	else
		return false;
}

void CBattleInterface::handleHex(BattleHex myNumber, int eventType)
{
	if(!myTurn) //we are not permit to do anything
		return;

	// This function handles mouse move over hexes and l-clicking on them. 
	// First we decide what happens if player clicks on this hex and set appropriately
	// consoleMsg, cursorFrame/Type and prepare lambda realizeAction.
	// 
	// Then, depending whether it was hover/click we either call the action or set tooltip/cursor.

	//used when hovering -> tooltip message and cursor to be set
	std::string consoleMsg;
	bool setCursor = true; //if we want to suppress setting cursor
	int cursorType = ECursor::COMBAT, cursorFrame = ECursor::COMBAT_POINTER; //TODO: is this line used?
	
	//used when l-clicking -> action to be called upon the click
	std::function<void()> realizeAction;

	//helper lambda that appropriately realizes action / sets cursor and tooltip
	auto realizeThingsToDo = [&]()
	{
		if(eventType == MOVE)
		{
			if(setCursor)
				CCS->curh->changeGraphic(cursorType, cursorFrame);
			this->console->alterText(consoleMsg);
			this->console->whoSetAlter = 0;
		}
		if(eventType == LCLICK && realizeAction)
		{
			//opening creature window shouldn't affect myTurn... 
			if(currentAction != CREATURE_INFO)
			{
				myTurn = false; //tends to crash with empty calls
			}
			realizeAction();
			CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER);
			this->console->alterText("");
		}
	};

	const CStack * const shere = curInt->cb->battleGetStackByPos(myNumber, false);
	const CStack * const sactive = activeStack;

	if (!sactive)
		return;

	bool ourStack = false;
	if (shere)
		ourStack = shere->owner == curInt->playerID;
	
	//TODO: handle
	bool noStackIsHovered = true; //will cause removing a blue glow
	
	localActions.clear();
	illegalActions.clear();

	BOOST_FOREACH (PossibleActions action, possibleActions)
	{
		bool legalAction = false; //this action is legal and can't be performed
		bool notLegal = false; //this action is not legal and should display message
		
		switch (action)
		{ 
			case CHOOSE_TACTICS_STACK:
				if (shere && ourStack)
					legalAction = true;
				break;
			case MOVE_TACTICS:
			case MOVE_STACK:
				if (canStackMoveHere (sactive, myNumber) && !shere)
					legalAction = true;
				break;
			case ATTACK:
			case WALK_AND_ATTACK:
			case ATTACK_AND_RETURN:
			{
				if (shere && !ourStack && shere->alive())
				{
					if (isTileAttackable(myNumber))
					{
						setBattleCursor(myNumber); // temporary - needed for following function :(
						BattleHex attackFromHex = fromWhichHexAttack(myNumber);

						if (attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
							legalAction = true;
					}
				}
			}
				break;
			case SHOOT:
				if(curInt->cb->battleCanShoot (activeStack, myNumber))
					legalAction = true;
				break;
			case ANY_LOCATION:
				if (myNumber > -1) //TODO: this should be checked for all actions
				{
					creatureCasting = stackCanCastSpell && !spellDestSelectMode; //as isCastingPossibleHere is not called
					legalAction = true;
				}
				break;
			case HOSTILE_CREATURE_SPELL:
				if (shere && shere->alive() && !ourStack && isCastingPossibleHere (sactive, shere, myNumber))
					legalAction = true;
				break;
			case FRIENDLY_CREATURE_SPELL:
				if (shere && shere->alive() && ourStack && isCastingPossibleHere (sactive, shere, myNumber))
					legalAction = true;
				break;
			case RISING_SPELL:
				if (shere && shere->canBeHealed() && ourStack && isCastingPossibleHere (sactive, shere, myNumber)) //TODO: at least one stack has to be raised by resurrection / animate dead
					legalAction = true;
				break;
			case RANDOM_GENIE_SPELL:
			{
				if (shere && ourStack && shere != sactive) //only positive spells for other allied creatures
				{
					int spellID = curInt->cb->battleGetRandomStackSpell(shere, CBattleInfoCallback::RANDOM_GENIE);
					if (spellID > -1)
					{
						legalAction = true;
					}
				}
			}
				break;
			case OBSTACLE:
				if (isCastingPossibleHere (sactive, shere, myNumber))
					legalAction = true;
				break;
			case TELEPORT:
			{
				ui8 skill = 0;
				if (creatureCasting)
					skill = sactive->valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, Spells::TELEPORT));
				else
					skill = getActiveHero()->getSpellSchoolLevel (CGI->spellh->spells[spellToCast->additionalInfo]); 
				//TODO: explicitely save power, skill
				if (curInt->cb->battleCanTeleportTo(selectedStack, myNumber, skill))
					legalAction = true;
				else
					notLegal = true;
			}
				break;
			case SACRIFICE: //choose our living stack to sacrifice
				if (shere && shere != selectedStack && ourStack && shere->alive())
					legalAction = true;
				else
					notLegal = true;
				break;
			case FREE_LOCATION:
				{
					ui8 side = curInt->cb->battleGetMySide();
					auto hero = curInt->cb->battleGetFightingHero(side);
					assert(!creatureCasting); //we assume hero casts this spell
					assert(hero);

					legalAction = true;
					bool hexesOutsideBattlefield = false;
					auto tilesThatMustBeClear = sp->rangeInHexes(myNumber, hero->getSpellSchoolLevel(sp), side, &hexesOutsideBattlefield);
					BOOST_FOREACH(BattleHex hex, tilesThatMustBeClear)
					{
						if(curInt->cb->battleGetStackByPos(hex)  ||  !!curInt->cb->battleGetObstacleOnPos(hex, false) 
						 || !hex.isAvailable())
						{
							legalAction = false;
							notLegal = true;
						}
					}

					if(hexesOutsideBattlefield)
					{
						legalAction = false;
						notLegal = true;
					}
				}
				break;
			case CATAPULT:
				if (isCatapultAttackable(myNumber))
					legalAction = true;
				break;
			case HEAL:
				if (shere && ourStack && shere->canBeHealed())
					legalAction = true;
				break;
			case RISE_DEMONS:
				if (shere && ourStack && !shere->alive())
					legalAction = true;
				break;
		}
		if (legalAction)
			localActions.push_back (action);
		else if (notLegal)
			illegalActions.push_back (action);
	}
	illegalAction = INVALID; //clear it in first place

	if (vstd::contains(localActions, selectedAction)) //try to use last selected action by default
		currentAction = selectedAction;
	else if (localActions.size()) //if not possible, select first avaliable action 9they are sorted by suggested priority)
		currentAction = localActions.front();
	else //no legal action possible
	{
		currentAction = INVALID; //don't allow to do anything

		if (vstd::contains(illegalActions, selectedAction))
			illegalAction = selectedAction;
		else if (illegalActions.size())
			illegalAction = illegalActions.front();
		else if (shere && ourStack && shere->alive()) //last possibility - display info about our creature
		{
			currentAction = CREATURE_INFO;
		}
		else
			illegalAction = INVALID; //we should never be here
	}

	bool isCastingPossible = false;
	bool secondaryTarget = false;

	if (currentAction > INVALID)
	{
		switch (currentAction) //display console message, realize selected action
		{
			case CHOOSE_TACTICS_STACK:
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[481]) % shere->getName()).str(); //Select %s
				realizeAction = [=]{ stackActivated(shere); };
				break;
			case MOVE_TACTICS:
			case MOVE_STACK:
				if (activeStack->hasBonusOfType(Bonus::FLYING))
				{
					cursorFrame = ECursor::COMBAT_FLY;
					consoleMsg = (boost::format(CGI->generaltexth->allTexts[295]) % activeStack->getName()).str(); //Fly %s here
				}
				else
				{
					cursorFrame = ECursor::COMBAT_MOVE;
					consoleMsg = (boost::format(CGI->generaltexth->allTexts[294]) % activeStack->getName()).str(); //Move %s here
				}

				realizeAction = [=]
				{
					if (activeStack->doubleWide())
					{
						std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
						int shiftedDest = myNumber + (activeStack->attackerOwned ? 1 : -1);
						if(vstd::contains(acc, myNumber))
							giveCommand (BattleAction::WALK ,myNumber, activeStack->ID);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand (BattleAction::WALK, shiftedDest, activeStack->ID);
					}
					else
					{
						giveCommand (BattleAction::WALK, myNumber, activeStack->ID);
					}
				};
				break;
			case ATTACK:
			case WALK_AND_ATTACK:
			case ATTACK_AND_RETURN: //TODO: allow to disable return
			{
				setBattleCursor(myNumber); //handle direction of cursor and attackable tile
				setCursor = false; //don't overwrite settings from the call above //TODO: what does it mean?
				realizeAction = [=]
				{
					BattleHex attackFromHex = fromWhichHexAttack(myNumber);
					if (attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
					{
						giveCommand(BattleAction::WALK_AND_ATTACK, attackFromHex, activeStack->ID, myNumber);
					}
				};

				std::string estDmgText = formatDmgRange(curInt->cb->battleEstimateDamage(sactive, shere)); //calculating estimated dmg
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[36]) % shere->getName() % estDmgText).str(); //Attack %s (%s damage)
			}
				break;
			case SHOOT:
			{
				if(curInt->cb->battleHasShootingPenalty(activeStack, myNumber))
					cursorFrame = ECursor::COMBAT_SHOOT_PENALTY;
				else
					cursorFrame = ECursor::COMBAT_SHOOT;

				realizeAction = [=] {giveCommand(BattleAction::SHOOT, myNumber, activeStack->ID);};
				std::string estDmgText = formatDmgRange(curInt->cb->battleEstimateDamage(sactive, shere)); //calculating estimated dmg
				//printing - Shoot %s (%d shots left, %s damage)
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[296]) % shere->getName() % sactive->shots % estDmgText).str();
			}
				break;
			case HOSTILE_CREATURE_SPELL:
			case FRIENDLY_CREATURE_SPELL:
			case RISING_SPELL:
				sp = CGI->spellh->spells[creatureCasting ? creatureSpellToCast : spellToCast->additionalInfo]; //necessary if creature has random Genie spell at same time
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[27]) % sp->name % shere->getName()); //Cast %s on %s
				switch (sp->id)
				{
					case Spells::SACRIFICE:
					case Spells::TELEPORT:
						selectedStack = shere; //remember firts target
						secondaryTarget = true;
						break;
				}
				isCastingPossible = true;
				break;
			case ANY_LOCATION:
				sp = CGI->spellh->spells[creatureCasting ? creatureSpellToCast : spellToCast->additionalInfo]; //necessary if creature has random Genie spell at same time
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
				isCastingPossible = true;
				break;
			case RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be avaliable as random spell
				sp = NULL;
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[301]) % shere->getName()); //Cast a spell on %
				creatureCasting = true;
				isCastingPossible = true;
				break;
			case TELEPORT:
				consoleMsg = CGI->generaltexth->allTexts[25]; //Teleport Here
				isCastingPossible = true;
				break;
			case OBSTACLE:
				consoleMsg = CGI->generaltexth->allTexts[550];
				isCastingPossible = true;
				break;
			case SACRIFICE:
				cursorFrame = ECursor::COMBAT_SACRIFICE;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[549]) % shere->getName()).str(); //sacrifice the %s
				spellToCast->selectedStack = shere->ID; //sacrificed creature is selected
				isCastingPossible = true;
				break;
			case FREE_LOCATION:
				//cursorFrame = ECursor::SPELLBOOK;
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
				isCastingPossible = true;
				break;
			case HEAL:
				cursorFrame = ECursor::COMBAT_HEAL;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[419]) % shere->getName()).str(); //Apply first aid to the %s
				realizeAction = [=]{ giveCommand(BattleAction::STACK_HEAL, myNumber, activeStack->ID); }; //command healing
				break;
			case RISE_DEMONS:
				cursorType = ECursor::SPELLBOOK;
				realizeAction = [=]{ giveCommand(BattleAction::DAEMON_SUMMONING, myNumber, activeStack->ID); };
				break;
			case CATAPULT:
				cursorFrame = ECursor::COMBAT_SHOOT_CATAPULT;
				realizeAction = [=]{ giveCommand(BattleAction::CATAPULT, myNumber, activeStack->ID); };
				break;
			case CREATURE_INFO:
			{
				cursorFrame = ECursor::COMBAT_QUERY;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[297]) % shere->getName()).str();
				realizeAction = [=]{ GH.pushInt(createCreWindow(shere, true)); };
	
				//setting console text
				const time_t curTime = time(NULL);
				CCreatureAnimation *hoveredStackAnim = creAnims[shere->ID];

				if (shere->ID != mouseHoveredStack
					&& curTime > lastMouseHoveredStackAnimationTime + HOVER_ANIM_DELTA
					&& hoveredStackAnim->getType() == CCreatureAnim::HOLDING 
					&& hoveredStackAnim->framesInGroup(CCreatureAnim::MOUSEON) > 0)
				{
					hoveredStackAnim->playOnce(CCreatureAnim::MOUSEON);
					lastMouseHoveredStackAnimationTime = curTime;
				}
				noStackIsHovered = false;
				mouseHoveredStack = shere->ID;
			}
				break;
		}
	}
	else //no possible valid action, display message
	{
		switch (illegalAction)
		{
			case HOSTILE_CREATURE_SPELL:
			case FRIENDLY_CREATURE_SPELL:
			case RISING_SPELL:
			case RANDOM_GENIE_SPELL:
				cursorFrame = ECursor::COMBAT_BLOCKED;
				consoleMsg = CGI->generaltexth->allTexts[23];
				break;
			case TELEPORT:
				consoleMsg = CGI->generaltexth->allTexts[24]; //Invalid Teleport Destination
				break;
			case SACRIFICE:
				consoleMsg = CGI->generaltexth->allTexts[543]; //choose army to sacrifice
				break;
			case FREE_LOCATION:
				cursorFrame = ECursor::COMBAT_BLOCKED;
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[181]) % sp->name); //No room to place %s here
				break;
			default:
				if (myNumber == -1)
					CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER); //set neutral cursor over menu etc.
				else
					cursorFrame = ECursor::COMBAT_BLOCKED;
				break;
		}
	}

	if (isCastingPossible) //common part
	{
		cursorType = ECursor::SPELLBOOK;
		cursorFrame = 0;
		if(consoleMsg.empty() && sp)
			consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
		
		realizeAction = [=]
		{
			if (secondaryTarget) //select that target now
			{
				possibleActions.clear();
				switch (sp->id)
				{
					case Spells::TELEPORT: //don't cast spell yet, only select target		
						possibleActions.push_back (TELEPORT);
						spellToCast->selectedStack = selectedStack->ID;
						break;
					case Spells::SACRIFICE:
						possibleActions.push_back (SACRIFICE);
						break;
				}
			}
			else
			{
				if(creatureCasting)
				{
					if (sp)
					{
						giveCommand(BattleAction::MONSTER_SPELL, myNumber, sactive->ID, creatureSpellToCast);
					}
					else //unknown random spell
					{
						giveCommand(BattleAction::MONSTER_SPELL, myNumber, sactive->ID, curInt->cb->battleGetRandomStackSpell(shere, CBattleInfoCallback::RANDOM_GENIE));
					}
				}
				else
				{
					assert (sp);
					switch (sp->id)
					{
						case Spells::SACRIFICE:
							spellToCast->destinationTile = selectedStack->position; //cast on first creature that will be resurrected
							break;
						default:
							spellToCast->destinationTile = myNumber;
							break;
					}
					curInt->cb->battleMakeAction(spellToCast);
					endCastingSpell();
				}
				selectedStack = NULL;
			}
		};
	}

	realizeThingsToDo();
	if(noStackIsHovered)
		mouseHoveredStack = -1;

}

bool CBattleInterface::isCastingPossibleHere (const CStack * sactive, const CStack * shere, BattleHex myNumber)
{
	creatureCasting = stackCanCastSpell && !spellDestSelectMode; //TODO: allow creatures to cast aimed spells
							
	bool isCastingPossible = true;

	int spellID = -1;
	if (creatureCasting)
	{
		if (creatureSpellToCast > -1 && (shere != sactive)) //can't cast on itself
			spellID = creatureSpellToCast; //TODO: merge with SpellTocast?
	}
	else //hero casting
		spellID  = spellToCast->additionalInfo;

	sp = NULL;
	if (spellID >= 0) 
		sp = CGI->spellh->spells[spellID];

	if (sp)
	{
		if (creatureCasting)
			isCastingPossible = (curInt->cb->battleCanCreatureCastThisSpell (sp, myNumber) == ESpellCastProblem::OK);
		else
			isCastingPossible = (curInt->cb->battleCanCastThisSpell (sp, myNumber) == ESpellCastProblem::OK);
	}
	if(!myNumber.isAvailable() && !shere) //empty tile outside battlefield (or in the unavailable border column)
			isCastingPossible = false;

	return isCastingPossible;
}

BattleHex CBattleInterface::fromWhichHexAttack(BattleHex myNumber)
{
	//TODO far too much repeating code
	BattleHex destHex = -1;
	switch(CCS->curh->number)
	{
	case 12: //from bottom right
		{
			bool doubleWide = activeStack->doubleWide();
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 ) +
				(activeStack->attackerOwned && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->attackerOwned) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 7: //from bottom left
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH-1 : GameConstants::BFIELD_WIDTH );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->attackerOwned) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 8: //from left
		{
			if(activeStack->doubleWide() && !activeStack->attackerOwned)
			{
				std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
				if(vstd::contains(acc, myNumber))
					return myNumber - 1;
				else
					return myNumber - 2;
			}
			else
			{
				return myNumber - 1;
			}
			break;
		}
	case 9: //from top left
		{
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH+1 : GameConstants::BFIELD_WIDTH );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->attackerOwned) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 10: //from top right
		{
			bool doubleWide = activeStack->doubleWide();
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 ) +
				(activeStack->attackerOwned && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->attackerOwned) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 11: //from right
		{
			if(activeStack->doubleWide() && activeStack->attackerOwned)
			{
				std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack, false);
				if(vstd::contains(acc, myNumber))
					return myNumber + 1;
				else
					return myNumber + 2;
			}
			else
			{
				return myNumber + 1;
			}
			break;
		}
	case 13: //from bottom
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 14: //from top
		{
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(attackingHeroInstance->tempOwner == curInt->cb->getMyColor()) //if we are attacker
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	}
	return -1;
}

Rect CBattleInterface::hexPosition(BattleHex hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX() + pos.x;
	int y = 86 + 42 * hex.getY() + pos.y;
	int w = cellShade->w;
	int h = cellShade->h;
	return Rect(x, y, w, h);
}

SDL_Surface * CBattleInterface::imageOfObstacle(const CObstacleInstance &oi) const
{
	int frameIndex = (animCount+1) / (40/getAnimSpeed());
	switch(oi.obstacleType)
	{
	case CObstacleInstance::USUAL:
		return vstd::circularAt(idToObstacle.find(oi.ID)->second->ourImages, frameIndex).bitmap;
	case CObstacleInstance::ABSOLUTE_OBSTACLE:
		return idToAbsoluteObstacle.find(oi.ID)->second;
	case CObstacleInstance::QUICKSAND:
		return vstd::circularAt(quicksand->ourImages, frameIndex).bitmap;
	case CObstacleInstance::LAND_MINE:
		return vstd::circularAt(landMine->ourImages, frameIndex).bitmap;
	case CObstacleInstance::FIRE_WALL:
		return vstd::circularAt(fireWall->ourImages, frameIndex).bitmap;
	case CObstacleInstance::FORCE_FIELD:
		{
			auto &forceField = dynamic_cast<const SpellCreatedObstacle &>(oi);
			if(forceField.getAffectedTiles().size() > 2)
				return vstd::circularAt(bigForceField[forceField.casterSide]->ourImages, frameIndex).bitmap;
			else
				return vstd::circularAt(smallForceField[forceField.casterSide]->ourImages, frameIndex).bitmap;
		}

	case CObstacleInstance::MOAT:
		//moat is blitted by SiegeHelper, this shouldn't be called
		assert(0);
	default:
		assert(0);
	}
}

void CBattleInterface::obstaclePlaced(const CObstacleInstance & oi)
{
	//so when multiple obstacles are added, they show up one after another
	waitForAnims();

	int effectID = -1;
	soundBase::soundID sound = soundBase::invalid;//FIXME: variable set but unused. Missing soundh->playSound()?

	std::string defname;

	switch(oi.obstacleType)
	{
	case CObstacleInstance::QUICKSAND:
		effectID = 55;
		sound = soundBase::QUIKSAND;
		break;
	case CObstacleInstance::LAND_MINE:
		effectID = 47;
		sound = soundBase::LANDMINE;
		break;
	case CObstacleInstance::FORCE_FIELD:
		{
			auto &spellObstacle = dynamic_cast<const SpellCreatedObstacle&>(oi);
			if(spellObstacle.casterSide)
			{
				if(oi.getAffectedTiles().size() < 3)
					defname = "C15SPE0.DEF"; //TODO cannot find def for 2-hex force field \ appearing
				else
					defname = "C15SPE6.DEF";
			}
			else
			{
				if(oi.getAffectedTiles().size() < 3)
					defname = "C15SPE0.DEF";
				else
					defname = "C15SPE9.DEF";
			}
		}
		sound = soundBase::FORCEFLD;
		break;
	case CObstacleInstance::FIRE_WALL:
		if(oi.getAffectedTiles().size() < 3)
			effectID = 43; //small fire wall appearing
		else
			effectID = 44; //and the big one
		sound = soundBase::fireWall;
		break;
	default:
		tlog1 << "I don't know how to animate appearing obstacle of type " << (int)oi.obstacleType << std::endl;
		return;
	}

	if(graphics->battleACToDef[effectID].empty())
	{
		tlog1 << "Cannot find def for effect type " << effectID << std::endl;
		return;
	}

	if(defname.empty() && effectID >= 0)
		defname = graphics->battleACToDef[effectID].front();

	assert(!defname.empty());
	//we assume here that effect graphics have the same size as the usual obstacle image
	// -> if we know how to blit obstacle, let's blit the effect in the same place
	Point whereTo = whereToBlitObstacleImage(imageOfObstacle(oi), oi); 
	addNewAnim(new CSpellEffectAnimation(this, defname, whereTo.x, whereTo.y));

	//TODO we need to wait after playing sound till it's finished, otherwise it overlaps and sounds really bad
	//CCS->soundh->playSound(sound);
}

Point CBattleInterface::whereToBlitObstacleImage(SDL_Surface *image, const CObstacleInstance &obstacle) const
{
	int offset = image->h % 42;
	if(obstacle.obstacleType == CObstacleInstance::USUAL)
	{
		if(obstacle.getInfo().blockedTiles.front() < 0  || offset > 37) //second or part is for holy ground ID=62,65,63
			offset -= 42;
	}
	else if(obstacle.obstacleType == CObstacleInstance::QUICKSAND)
	{
		offset -= 42;
	}

	Rect r = hexPosition(obstacle.pos);
	r.y += 42 - image->h + offset;
	return r.topLeft();
}

std::string CBattleInterface::SiegeHelper::townTypeInfixes[GameConstants::F_NUMBER] = {"CS", "RM", "TW", "IN", "NC", "DN", "ST", "FR", "EL"};

CBattleInterface::SiegeHelper::SiegeHelper(const CGTownInstance *siegeTown, const CBattleInterface * _owner)
  : owner(_owner), town(siegeTown)
{
	for(int g = 0; g < ARRAY_COUNT(walls); ++g)
	{
		walls[g] = BitmapHandler::loadBitmap( getSiegeName(g) );
	}
}

CBattleInterface::SiegeHelper::~SiegeHelper()
{
	for(int g = 0; g < ARRAY_COUNT(walls); ++g)
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

double CatapultProjectileInfo::calculateY(double x)
{
	return (facA * pow(10., -3.)) * pow(x, 2.0) + facB * x + facC;
}
