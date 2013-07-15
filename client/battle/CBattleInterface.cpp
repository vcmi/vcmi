#include "StdInc.h"
#include "CBattleInterface.h"

#include "../CGameInfo.h"
#include "../gui/SDL_Extensions.h"
#include "../CAdvmapInterface.h"
#include "../CAnimation.h"
#include "../CBitmapHandler.h"
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
#include "../../lib/CConfigHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/NetPacks.h"
#include "../CPlayerInterface.h"
#include "../CCreatureWindow.h"
#include "../CVideoHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/mapping/CMap.h"

#include "CBattleAnimations.h"
#include "CBattleInterfaceClasses.h"

#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../CMT.h"

#ifndef __GNUC__
const double M_PI = 3.14159265358979323846;
#else
#define _USE_MATH_DEFINES
#include <cmath>
#endif
#include "../../lib/UnlockGuard.h"

using namespace boost::assign;

/*
 * CBattleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CondSh<bool> CBattleInterface::animsAreDisplayed;

static void onAnimationFinished(const CStack *stack, CCreatureAnimation * anim)
{
	if (anim->isIdle())
	{
		const CCreature *creature = stack->getCreature();

		if (anim->framesInGroup(CCreatureAnim::MOUSEON) > 0)
		{
			if (float(rand() % 100) < creature->animation.timeBetweenFidgets * 10)
				anim->playOnce(CCreatureAnim::MOUSEON);
			else
				anim->setType(CCreatureAnim::HOLDING);
		}
		else
		{
			anim->setType(CCreatureAnim::HOLDING);
		}
	}
	// always reset callback
	anim->onAnimationReset += std::bind(&onAnimationFinished, stack, anim);
}

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

void CBattleInterface::addNewAnim(CBattleAnimation * anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	animsAreDisplayed.setn(true);
}

CBattleInterface::CBattleInterface(const CCreatureSet * army1, const CCreatureSet * army2, 
								   CGHeroInstance *hero1, CGHeroInstance *hero2, 
								   const SDL_Rect & myRect, 
								   shared_ptr<CPlayerInterface> att, shared_ptr<CPlayerInterface> defen)
	: background(nullptr), queue(nullptr), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0),
      activeStack(nullptr), mouseHoveredStack(nullptr), stackToActivate(nullptr), selectedStack(nullptr), previouslyHoveredHex(-1),
	  currentlyHoveredHex(-1), attackingHex(-1), stackCanCastSpell(false), creatureCasting(false), spellDestSelectMode(false), spellSelMode(NO_LOCATION), spellToCast(nullptr), sp(nullptr),
	  siegeH(nullptr), attackerInt(att), defenderInt(defen), curInt(att), animIDhelper(0),
	  givenCommand(nullptr), myTurn(false), resWindow(nullptr), moveStarted(false), moveSoundHander(-1), bresult(nullptr)
{
	OBJ_CONSTRUCTION;

	if(!curInt)
	{
		//May happen when we are defending during network MP game -> attacker interface is just not present
		curInt = defenderInt; 
	}

	animsAreDisplayed.setn(false);
	pos = myRect;
	strongInterest = true;
	givenCommand = new CondSh<BattleAction *>(nullptr);

	if(attackerInt && attackerInt->cb->battleGetTacticDist()) //hot-seat -> check tactics for both players (defender may be local human)
		tacticianInterface = attackerInt;
	else if(defenderInt && defenderInt->cb->battleGetTacticDist())
		tacticianInterface = defenderInt;

	tacticsMode = static_cast<bool>(tacticianInterface);  //if we found interface of player with tactics, then enter tactics mode

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
	for(const CStack *s : stacks)
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

			auto & info = siegeH->town->town->clientInfo;
			Point moatPos(info.siegePositions[13].x, info.siegePositions[13].y);
			Point mlipPos(info.siegePositions[14].x, info.siegePositions[14].y);

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
		auto bfieldType = (int)curInt->cb->battleGetBattlefieldType();
		if(graphics->battleBacks.size() <= bfieldType || bfieldType < 0)
			logGlobal->errorStream() << bfieldType << " is not valid battlefield type index!";
		else if(graphics->battleBacks[bfieldType].empty())
			logGlobal->errorStream() << bfieldType << " battlefield type does not have any backgrounds!";
		else
		{
			const std::string bgName = vstd::pickRandomElementOf(graphics->battleBacks[bfieldType], rand);
			background = BitmapHandler::loadBitmap(bgName, false);
		}
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
	bFlee->block(!curInt->cb->getMyColor() || !curInt->cb->battleCanFlee());
	bSurrender->block(!curInt->cb->getMyColor() || curInt->cb->battleGetSurrenderCost() < 0);
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
		btactNext = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bTacticNextStack,this, (CStack*)nullptr), 213, 560, "icm011.def", SDLK_SPACE);
		btactEnd = new CAdventureMapButton(std::string(), std::string(), boost::bind(&CBattleInterface::bEndTacticPhase,this), 419, 560, "icm012.def", SDLK_RETURN);
		bDefence->block(true);
		bWait->block(true);
		menu = BitmapHandler::loadBitmap("COPLACBR.BMP");
	}
	else
	{
		menu = BitmapHandler::loadBitmap("CBAR.BMP");
		btactEnd = btactNext = nullptr;
	}
	graphics->blueToPlayersAdv(menu, curInt->playerID);

	//loading hero animations
	if(hero1) // attacking hero
	{
		std::string battleImage;
		if ( hero1->sex )
			battleImage = hero1->type->heroClass->imageBattleFemale;
		else
			battleImage = hero1->type->heroClass->imageBattleMale;

		attackingHero = new CBattleHero(battleImage, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : nullptr, this);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, pos.x - 43, pos.y - 19);
	}
	else
	{
		attackingHero = nullptr;
	}
	if(hero2) // defending hero
	{
		std::string battleImage;
		if ( hero2->sex )
			battleImage = hero2->type->heroClass->imageBattleFemale;
		else
			battleImage = hero2->type->heroClass->imageBattleMale;

		defendingHero = new CBattleHero(battleImage, true, hero2->tempOwner, hero2->tempOwner == curInt->playerID ? hero2 : nullptr, this);
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, pos.x + 693, pos.y - 19);
	}
	else
	{
		defendingHero = nullptr;
	}

	//preparing cells and hexes
	cellBorder = BitmapHandler::loadBitmap("CCELLGRD.BMP");
	CSDL_Ext::alphaTransform(cellBorder);
	cellShade = BitmapHandler::loadBitmap("CCELLSHD.BMP");
	CSDL_Ext::alphaTransform(cellShade);
	for(int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		auto hex = new CClickableHex();
		hex->myNumber = h;
		hex->pos = hexPosition(h);
		hex->accessible = true;
		hex->myInterface = this;
		bfield.push_back(hex);
	}
	//locking occupied positions on batlefield
	for(const CStack *s : stacks)  //stacks gained at top of this function
		if(s->position >= 0) //turrets have position < 0
			bfield[s->position]->accessible = false;

	//loading projectiles for units
	for(const CStack *s : stacks)
	{
		if(s->getCreature()->isShooting())
		{
			CDefHandler *&projectile = idToProjectile[s->getCreature()->idNumber];

			const CCreature * creature;//creature whose shots should be loaded
			if (s->getCreature()->idNumber == CreatureID::ARROW_TOWERS)
				creature = CGI->creh->creatures[siegeH->town->town->clientInfo.siegeShooter];
			else
				creature = s->getCreature();

			projectile = CDefHandler::giveDef(creature->animation.projectileImageName);

			for(auto & elem : projectile->ourImages) //alpha transforming
			{
				CSDL_Ext::alphaTransform(elem.bitmap);
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
	for(auto & elem : obst)
	{
		const int ID = elem->ID;
		if(elem->obstacleType == CObstacleInstance::USUAL)
		{
			idToObstacle[ID] = CDefHandler::giveDef(elem->getInfo().defName);
			for(auto & _n : idToObstacle[ID]->ourImages)
			{
				SDL_SetColorKey(_n.bitmap, SDL_SRCCOLORKEY, SDL_MapRGB(_n.bitmap->format,0,255,255));
			}
		}
		else if(elem->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			idToAbsoluteObstacle[ID] = BitmapHandler::loadBitmap(elem->getInfo().defName);
		}
	}

	quicksand = CDefHandler::giveDef("C17SPE1.DEF");
	landMine = CDefHandler::giveDef("C09SPF1.DEF");
	fireWall = CDefHandler::giveDef("C07SPF61");
	bigForceField[0] = CDefHandler::giveDef("C15SPE10.DEF");
	bigForceField[1] = CDefHandler::giveDef("C15SPE7.DEF");
	smallForceField[0] = CDefHandler::giveDef("C15SPE1.DEF");
	smallForceField[1] = CDefHandler::giveDef("C15SPE4.DEF");

	for(auto hex : bfield)
		addChild(hex);

	if(tacticsMode)
		bTacticNextStack();

	CCS->musich->stopMusic();

	int channel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	auto onIntroPlayed = []()
	{
		if (LOCPLINT->battleInt)
			CCS->musich->playMusicFromSet("battle", true);
	};

	CCS->soundh->setCallback(channel, onIntroPlayed);
	memset(stackCountOutsideHexes, 1, GameConstants::BFIELD_SIZE * sizeof(bool)); //initialize array with trues

	currentAction = INVALID;
	selectedAction = INVALID;
	addUsedEvents(RCLICK | MOVE | KEYBOARD);
}

CBattleInterface::~CBattleInterface()
{
	curInt->battleInt = nullptr;
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

	for(auto hex : bfield)
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

	for(auto & elem : creAnims)
		delete elem.second;

	for(auto & elem : idToProjectile)
		delete elem.second;

	for(auto & elem : idToObstacle)
		delete elem.second;

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
		int terrain = LOCPLINT->cb->getTile(adventureInt->selection->visitablePos())->terType;
		CCS->musich->playMusicFromSet("terrain", terrain, true);
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
	if(curInt->isAutoFightOn)
	{
		bAutofight->activate();
		return;
	}

	CIntObject::activate();
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();

	for(auto hex : bfield)
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

	for(auto hex : bfield)
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
	if(activeStack != nullptr && creAnims[activeStack->ID]->getType() != CCreatureAnim::MOVING) //show everything with range
	{
		blitAt(backgroundWithHexes, pos.x, pos.y, to);
	}
	else
	{
		//showing background
		blitAt(background, pos.x, pos.y, to);
		if(settings["battle"]["cellBorders"].Bool())
		{
			CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, nullptr, to, &pos);
		}
		//Blit absolute obstacles
		for(auto &oi : curInt->cb->battleGetAllObstacles())
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
				for(auto shadedHex : shaded) //for spells with range greater then one hex
				{
					if(settings["battle"]["mouseShadow"].Bool() && (shadedHex % GameConstants::BFIELD_WIDTH != 0) && (shadedHex % GameConstants::BFIELD_WIDTH != 16))
					{
						int x = 14 + ((shadedHex/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(shadedHex%GameConstants::BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (shadedHex/GameConstants::BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, nullptr, to, &temp_rect);
					}
				}
			}
			else if(settings["battle"]["mouseShadow"].Bool()) //when not casting spell
			{//TODO: do not check it every frame
				if (activeStack) //highlight all attackable hexes
				{
					std::set<BattleHex> set = curInt->cb->battleGetAttackedHexes(activeStack, currentlyHoveredHex, attackingHex);
					for(BattleHex hex : set)
					{
						int x = 14 + ((hex/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(hex%GameConstants::BFIELD_WIDTH) + pos.x;
						int y = 86 + 42 * (hex/GameConstants::BFIELD_WIDTH) + pos.y;
						SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
						CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, nullptr, to, &temp_rect);
					}
				}

				//patch by ench0: show enemy stack movement shadow
				// activeStack == nullptr means it is opponent's turn...
				if(activeStack && settings["battle"]["stackRange"].Bool())
				{
					// display the movement shadow of the stack at b (i.e. stack under mouse)
					const CStack * const shere = curInt->cb->battleGetStackByPos(b, false);
					if (shere && shere != activeStack && shere->alive())
					{
						std::vector<BattleHex> v = curInt->cb->battleGetAvailableHexes(shere, true );
						for (BattleHex hex : v)
						{
							int x = 14 + ((hex / GameConstants::BFIELD_WIDTH ) % 2 == 0 ? 22 : 0) + 44 * (hex % GameConstants::BFIELD_WIDTH) + pos.x;
							int y = 86 + 42 * (hex / GameConstants::BFIELD_WIDTH) + pos.y;
							SDL_Rect temp_rect = genRect (cellShade->h, cellShade->w, x, y);
							CSDL_Ext::blit8bppAlphaTo24bpp (cellShade, nullptr, to, &temp_rect);
						}
					}
				}

				if (active)
				{
					//always highlight pointed hex
					int x = 14 + ((b/GameConstants::BFIELD_WIDTH)%2==0 ? 22 : 0) + 44*(b%GameConstants::BFIELD_WIDTH) + pos.x;
					int y = 86 + 42 * (b/GameConstants::BFIELD_WIDTH) + pos.y;
					SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
					CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, nullptr, to, &temp_rect);
				}
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
	for (auto & stack : stacks)
	{
		const CStack *s = stack;
		if(creAnims.find(s->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->getType() != CCreatureAnim::DEAD && s->position >= 0) //don't show turrets here
			stackAliveByHex[s->position].push_back(s);
	}
	std::vector<const CStack *> stackDeadByHex[GameConstants::BFIELD_SIZE];
	for (auto & stack : stacks)
	{
		const CStack *s = stack;
		if(creAnims.find(s->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;
		if(creAnims[s->ID]->isDead())
			stackDeadByHex[s->position].push_back(s);
	}

	//handle animations
	for(auto & elem : pendingAnims)
	{
		if(!elem.first) //this animation should be deleted
			continue;

		if(!elem.second)
		{
			elem.second = elem.first->init();
		}
		if(elem.second && elem.first)
			elem.first->nextFrame();
	}

	//delete anims
	int preSize = pendingAnims.size();
	for(auto it = pendingAnims.begin(); it != pendingAnims.end(); ++it)
	{
		if(it->first == nullptr)
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
		if(pendingAnims.size() == 0 && stackToActivate != nullptr)
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

	//Get all the spell effects that need to be drawn with the stack
	std::vector<const BattleEffect *> battleEffectByHex[GameConstants::BFIELD_SIZE];
	std::vector<const BattleEffect *> topBattleEffects;
	for (auto & battleEffect : battleEffects)
	{
		const BattleEffect *e = &battleEffect;
		if(e->position.isValid())
			battleEffectByHex[e->position].push_back(e);
		else
			topBattleEffects.push_back(e);
	}

	for(auto & elem : stackDeadByHex) //showing dead stacks
	{
		for(size_t v=0; v<elem.size(); ++v)
		{
			creAnims[elem[v]->ID]->nextFrame(to, creDir[elem[v]->ID]);
			creAnims[elem[v]->ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);
		}
	}
	std::vector<const CStack *> flyingStacks; //flying stacks should be displayed later, over other stacks and obstacles
	if (!siegeH)
	{
		for(int b = 0; b < GameConstants::BFIELD_SIZE; ++b) //showing alive stacks and spells
		{
			showObstacles(&hexToObstacle, obstacles, b, to);
			showAliveStacks(stackAliveByHex, b, &flyingStacks, to);
			showBattleEffects(battleEffectByHex[b], to);
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
					showBattleEffects(battleEffectByHex[hex], to);
					showPieceOfWall(to, hex, stacks);
				}

				++runNum;
			}
		}
	}

	for(auto & flyingStack : flyingStacks) //showing flying stacks
		showAliveStack(flyingStack, to);

	//units shown

	// Show projectiles
	projectileShowHelper(to);

	//showing spell effects
	showBattleEffects(topBattleEffects, to);

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
			queue->blitBg(to);
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

	for(auto & elem : aliveStacks[hex])
	{
		const CStack *s = elem;

		if(!s->hasBonusOfType(Bonus::FLYING) || creAnims[s->ID]->getType() != CCreatureAnim::DEAD)
			showAliveStack(s, to);
		else
			flyingStacks->push_back(s);
	}
}

void CBattleInterface::showObstacles(std::multimap<BattleHex, int> *hexToObstacle, std::vector<shared_ptr<const CObstacleInstance> > &obstacles, int hex, SDL_Surface *to)
{
	auto obstRange = hexToObstacle->equal_range(hex);

	for(auto it = obstRange.first; it != obstRange.second; ++it)
	{
		const CObstacleInstance & curOb = *obstacles[it->second];
		SDL_Surface *toBlit = imageOfObstacle(curOb);
		Point p = whereToBlitObstacleImage(toBlit, curOb);
		blitAt(toBlit, p.x, p.y, to);
	}
}

void CBattleInterface::showBattleEffects(const std::vector<const BattleEffect *> &battleEffects, SDL_Surface *to)
{
	for(auto & elem : battleEffects)
	{
		SDL_Surface * bitmapToBlit = elem->anim->ourImages[(elem->frame)%elem->anim->ourImages.size()].bitmap;
		SDL_Rect temp_rect = genRect(bitmapToBlit->h, bitmapToBlit->w, elem->x, elem->y);
		SDL_BlitSurface(bitmapToBlit, nullptr, to, &temp_rect);
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

	// Generally should NEVER happen, but to avoid the possibility of having endless loop below... [#1016]
	if(!vstd::contains_if(sectorCursor, [](int sc) { return sc != -1; }))
	{
        logGlobal->errorStream() << "Error: for hex " << myNumber << " cannot find a hex to attack from!";
		attackingHex = -1;
		return;
	}

	// Find the closest direction attackable, starting with the right one.
	// FIXME: Is this really how the original H3 client does it?
	int i = 0;
	while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1) //Why hast thou forsaken me?
		i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
	int index = (cursorIndex + i)%sectorCursor.size(); //hopefully we get elements from sectorCursor
	cursor->changeGraphic(ECursor::COMBAT, sectorCursor[index]);
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

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	Rect tempRect = genRect(431, 481, 160, 84);
	tempRect += pos.topLeft();
	auto  optionsWin = new CBattleOptionsWindow(tempRect, this);
	GH.pushInt(optionsWin);
}

void CBattleInterface::bSurrenderf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	int cost = curInt->cb->battleGetSurrenderCost();
	if(cost >= 0)
	{
		std::string enemyHeroName = curInt->cb->battleGetEnemyHero().name;
		if(enemyHeroName.empty())
			enemyHeroName = "#ENEMY#"; //TODO: should surrendering without enemy hero be enabled?

		std::string surrenderMessage = boost::str(boost::format(CGI->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		curInt->showYesNoDialog(surrenderMessage, [this]{ reallySurrender(); }, 0, false);
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
	giveCommand(Battle::RETREAT,0,0);
	CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
}

void CBattleInterface::reallySurrender()
{
	if(curInt->cb->getResourceAmount(Res::GOLD) < curInt->cb->battleGetSurrenderCost())
	{
		curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		giveCommand(Battle::SURRENDER,0,0);
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
	}
}

void CBattleInterface::bAutofightf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;
	
	//Stop auto-fight mode
	if(curInt->isAutoFightOn)
	{
		assert(curInt->autofightingAI);
		curInt->isAutoFightOn = false;
		logGlobal->traceStream() << "Stopping the autofight...";
	}
	else
	{
		curInt->isAutoFightOn = true;	
		deactivate();
		bAutofight->activate();

		auto ai = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
		ai->init(curInt->cb);
		ai->battleStart(army1, army2, int3(0,0,0), attackingHeroInstance, defendingHeroInstance, curInt->cb->battleGetMySide());
		curInt->autofightingAI = ai;
		curInt->cb->registerBattleInterface(ai);

		requestAutofightingAIToTakeAction();
	}
}

void CBattleInterface::bSpellf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	if(!myTurn)
		return;

	auto myHero = currentHero();
	ESpellCastProblem::ESpellCastProblem spellCastProblem;
	if (curInt->cb->battleCanCastSpell(&spellCastProblem))
	{
		auto  spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), myHero, curInt.get());
		GH.pushInt(spellWindow);
	}
	else if(spellCastProblem == ESpellCastProblem::MAGIC_IS_BLOCKED)
	{
		//Handle Orb of Inhibition-like effects -> we want to display dialog with info, why casting is impossible
		auto blockingBonus = currentHero()->getBonusLocalFirst(Selector::type(Bonus::BLOCK_ALL_MAGIC));
		if(!blockingBonus)
			return;;
		
		if(blockingBonus->source == Bonus::ARTIFACT)
		{
			const int artID = blockingBonus->sid;
			//If we have artifact, put name of our hero. Otherwise assume it's the enemy.
			//TODO check who *really* is source of bonus
			std::string heroName = myHero->hasArt(artID) ? myHero->name : enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683]) 
										% heroName % CGI->arth->artifacts[artID]->Name()));
		}
	}
}

void CBattleInterface::bWaitf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if(activeStack != nullptr)
		giveCommand(Battle::WAIT,0,activeStack->ID);
}

void CBattleInterface::bDefencef()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	if(activeStack != nullptr)
		giveCommand(Battle::DEFEND,0,activeStack->ID);
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
	creDir[stack->ID] = stack->attackerOwned; // must be set before getting stack position

	Point coords = CClickableHex::getXYUnitAnim(stack->position, stack, this);

	if(stack->position < 0) //turret
	{
		const CCreature * turretCreature = CGI->creh->creatures[siegeH->town->town->clientInfo.siegeShooter];

		creAnims[stack->ID] = AnimationControls::getAnimation(turretCreature);

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
			coords.x = siegeH->town->town->clientInfo.siegePositions[posID].x + this->pos.x;
			coords.y = siegeH->town->town->clientInfo.siegePositions[posID].y + this->pos.y;
		}
		creAnims[stack->ID]->pos.h = siegeH->town->town->clientInfo.siegeShooterCropHeight;
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
	
}

void CBattleInterface::stackRemoved(int stackID)
{
	delete creAnims[stackID];
	creAnims.erase(stackID);
	creDir.erase(stackID);

	redrawBackgroundWithHexes(activeStack);
	queue->update();
}

void CBattleInterface::stackActivated(const CStack * stack) //TODO: check it all before game state is changed due to abilities
{
	//givenCommand = nullptr;
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
	for (auto & attackedInfo : attackedInfos)
	{
		if (!attackedInfo.cloneKilled) //FIXME: play dead animation for cloned creature before it vanishes
			addNewAnim(new CDefenceAnimation(attackedInfo, this));
		if (attackedInfo.rebirth)
		{
			displayEffect(50, attackedInfo.defender->position); //TODO: play reverse death animation
			CCS->soundh->playSound(soundBase::RESURECT);
		}
	}
	waitForAnims();
	int targets = 0, killed = 0, damage = 0;
	for(auto & attackedInfo : attackedInfos)
	{
		++targets;
		killed += attackedInfo.amountKilled;
		damage += attackedInfo.dmg;
	}
	if (attackedInfos.front().cloneKilled) //FIXME: cloned stack is already removed
		return;
	if (targets > 1)
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, true); //creatures perish
	else
		printConsoleAttacked(attackedInfos.front().defender, damage, killed, attackedInfos.front().attacker, false);

	for(auto & attackedInfo : attackedInfos)
	{
		if (attackedInfo.rebirth)
			creAnims[attackedInfo.defender->ID]->setType(CCreatureAnim::HOLDING);
		if (attackedInfo.cloneKilled)
			stackRemoved(attackedInfo.defender->ID);
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
//	for(const CStack *s : stacks)
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

void CBattleInterface::giveCommand(Battle::ActionType action, BattleHex tile, ui32 stackID, si32 additional, si32 selected)
{
	const CStack *stack = curInt->cb->battleGetStackByID(stackID);
	if(!stack && action != Battle::HERO_SPELL && action != Battle::RETREAT && action != Battle::SURRENDER)
	{
		return;
	}

	if(stack && stack != activeStack)
        logGlobal->warnStream() << "Warning: giving an order to a non-active stack?";

	auto  ba = new BattleAction(); //is deleted in CPlayerInterface::activeStack()
	ba->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
	ba->actionType = action;
	ba->destinationTile = tile;
	ba->stackNumber = stackID;
	ba->additionalInfo = additional;
	ba->selectedStack = selected;

	//some basic validations
	switch(action)
	{
		case Battle::WALK_AND_ATTACK:
			assert(curInt->cb->battleGetStackByPos(additional)); //stack to attack must exist
		case Battle::WALK:
		case Battle::SHOOT:
		case Battle::CATAPULT:
			assert(tile < GameConstants::BFIELD_SIZE);
			break;
	}

	if(!tacticsMode)
	{
        logGlobal->traceStream() << "Setting command for " << (stack ? stack->nodeName() : "hero");
		myTurn = false;
		setActiveStack(nullptr);
		givenCommand->setn(ba);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(ba);
		vstd::clear_pointer(ba);
		setActiveStack(nullptr);
		//next stack will be activated when action ends
	}
}

bool CBattleInterface::isTileAttackable(const BattleHex & number) const
{
	for(auto & elem : occupyableHexes)
	{
		if(BattleHex::mutualPosition(elem, number) != -1 || elem == number)
			return true;
	}
	return false;
}

bool CBattleInterface::isCatapultAttackable(BattleHex hex) const
{
	if(!siegeH  ||  tacticsMode)
		return false;

	int wallUnder = curInt->cb->battleHexToWallPart(hex);
	if(wallUnder < 0) //invalid or indestructible
		return false;

	return curInt->cb->battleGetWallState(wallUnder) < EWallState::DESTROYED;
}

const CGHeroInstance * CBattleInterface::getActiveHero()
{
	const CStack * attacker = activeStack;
	if (!attacker)
	{
		return nullptr;
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
	for(auto it = ca.attackedParts.begin(); it != ca.attackedParts.end(); ++it)
	{
		const CStack * stack = curInt->cb->battleGetStackByID(ca.attacker);
		addNewAnim(new CShootingAnimation(this, stack, it->first.second, nullptr, true, it->second));

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
	setActiveStack(nullptr);
}

void CBattleInterface::displayBattleFinished()
{
	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	SDL_Rect temp_rect = genRect(561, 470, (screen->w - 800)/2 + 165, (screen->h - 600)/2 + 19);
	resWindow = new CBattleResultWindow(*bresult, temp_rect, *this->curInt);
	GH.pushInt(resWindow);
}

void CBattleInterface::spellCast( const BattleSpellCast * sc )
{
	const CSpell &spell = *CGI->spellh->spells[sc->id];

	//spell opening battle is cast when no stack is active
	if(sc->castedByHero && ( activeStack == nullptr || sc->side == !activeStack->attackerOwned) )
		bSpell->block(true);

	std::vector< std::string > anims; //for magic arrow and ice bolt

	if (vstd::contains(CCS->soundh->spellSounds, &spell))
		CCS->soundh->playSound(CCS->soundh->spellSounds[&spell]);

	switch(sc->id)
	{
	case SpellID::MAGIC_ARROW:
		{
			//initialization of anims
			anims.push_back("C20SPX0.DEF"); anims.push_back("C20SPX1.DEF"); anims.push_back("C20SPX2.DEF"); anims.push_back("C20SPX3.DEF"); anims.push_back("C20SPX4.DEF");
		}
	case SpellID::ICE_BOLT:
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
			Point destcoord = CClickableHex::getXYUnitAnim(sc->tile, curInt->cb->battleGetStackByPos(sc->tile), this); //position attacked by arrow
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
	case SpellID::LIGHTNING_BOLT:
	case SpellID::TITANS_LIGHTNING_BOLT:
	case SpellID::THUNDERBOLT:
	case SpellID::CHAIN_LIGHTNING: //TODO: zigzag effect
		for (auto & elem : sc->affectedCres) //in case we have multiple targets
		{
			displayEffect(1, curInt->cb->battleGetStackByID(elem, false)->position);
			displayEffect(spell.mainEffectAnim, curInt->cb->battleGetStackByID(elem, false)->position);
		}
		break;
	case SpellID::DISPEL:
	case SpellID::CURE:
	case SpellID::RESURRECTION:
	case SpellID::ANIMATE_DEAD:
	case SpellID::DISPEL_HELPFUL_SPELLS:
	case SpellID::SACRIFICE: //TODO: animation upon killed stack
		for(auto & elem : sc->affectedCres)
		{
			displayEffect(spell.mainEffectAnim, curInt->cb->battleGetStackByID(elem, false)->position);
		}
		break;
	case SpellID::SUMMON_FIRE_ELEMENTAL:
	case SpellID::SUMMON_EARTH_ELEMENTAL:
	case SpellID::SUMMON_WATER_ELEMENTAL:
	case SpellID::SUMMON_AIR_ELEMENTAL:
	case SpellID::CLONE:
	case SpellID::REMOVE_OBSTACLE:
		addNewAnim(new CDummyAnimation(this, 2)); //interface won't return until animation is played. TODO: make it smarter?
		break;
	} //switch(sc->id)

	if (spell.isDamageSpell() && sc->affectedCres.empty()) //for example Inferno that causes no BattleStackAttacked
	{
		displayEffect (spell.mainEffectAnim, sc->tile);
	}

	//support for resistance
	for(auto & elem : sc->resisted)
	{
		int tile = curInt->cb->battleGetStackByID(elem)->position;
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
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetHeroInfo(sc->side).name);
			boost::algorithm::replace_first(text, "%s", CGI->spellh->spells[sc->id]->name); //spell name
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl ); //target
		}
		else
		{
			switch(sc->id)
			{
				case SpellID::STONE_GAZE:
					customSpell = true;
					plural = true;
					textID = 558;
					break;
				case SpellID::POISON:
					customSpell = true;
					plural = true;
					textID = 561;
					break;
				case SpellID::BIND:
					customSpell = true;
					text = CGI->generaltexth->allTexts[560];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin(), false)->getCreature()->namePl );
					break;	//Roots and vines bind the %s to the ground!
				case SpellID::DISEASE:
					customSpell = true;
					plural = true;
					textID = 553;
					break;
				case SpellID::PARALYZE:
					customSpell = true;
					plural = true;
					textID = 563;
					break;
				case SpellID::AGE:
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
				case SpellID::THUNDERBOLT:
					text = CGI->generaltexth->allTexts[367];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					console->addText(text);
					text = CGI->generaltexth->allTexts[343].substr(1, CGI->generaltexth->allTexts[343].size() - 1); //Does %d points of damage.
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(sc->dmgToDisplay)); //no more text afterwards
					console->addText(text);
					customSpell = true;
					text = ""; //yeah, it's a terrible mess
					break;
				case SpellID::DISPEL_HELPFUL_SPELLS:
					text = CGI->generaltexth->allTexts[555];
					boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetStackByID(*sc->affectedCres.begin())->type->namePl);
					customSpell = true;
					break;
				case SpellID::DEATH_STARE:
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
			boost::algorithm::replace_first(text, "%s", curInt->cb->battleGetHeroInfo(sc->side).name);
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
		for(auto & elem : sse.stacks)
		{
			bool areaEffect(CGI->spellh->spells[effID]->getTargetType() == CSpell::ETargetType::NO_TARGET);
			displayEffect(CGI->spellh->spells[effID]->mainEffectAnim, curInt->cb->battleGetStackByID(elem)->position, areaEffect);
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


	if (activeStack != nullptr) //it can be -1 when a creature casts effect
	{
		redrawBackgroundWithHexes(activeStack);
	}
}

void CBattleInterface::castThisSpell(int spellID)
{
	auto  ba = new BattleAction;
	ba->actionType = Battle::HERO_SPELL;
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

	if(spellID == SpellID::FIRE_WALL  ||  spellID == SpellID::FORCE_FIELD)
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

void CBattleInterface::displayEffect(ui32 effect, int destTile, bool areaEffect)
{
	addNewAnim(new CSpellEffectAnimation(this, effect, destTile, 0, 0, false, areaEffect));
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
			CCS->soundh->playSound(soundBase::GOODMRLE);
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
	speed->Float() = float(set) / 100;
}

int CBattleInterface::getAnimSpeed() const
{
	return vstd::round(settings["battle"]["animationSpeed"].Float() * 100);
}

void CBattleInterface::setActiveStack(const CStack * stack)
{
	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getGoldBorder());
}

void CBattleInterface::setHoveredStack(const CStack * stack)
{
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

void CBattleInterface::activateStack()
{
	setActiveStack(stackToActivate);
	stackToActivate = nullptr;
	const CStack *s = activeStack;

	myTurn = true;
	if(!!attackerInt && defenderInt) //hotseat -> need to pick which interface "takes over" as active
		curInt = attackerInt->playerID == s->owner ? attackerInt : defenderInt;

	queue->update();
	redrawBackgroundWithHexes(activeStack);
	bWait->block(s->waited() || tacticsMode); //block waiting button if stack has been already waiting

	//block cast spell button if hero doesn't have a spellbook
	ESpellCastProblem::ESpellCastProblem spellcastingProblem;
	bool canCastSpells = curInt->cb->battleCanCastSpell(&spellcastingProblem);
	bSpell->block(!canCastSpells && spellcastingProblem != ESpellCastProblem::MAGIC_IS_BLOCKED); //if magic is blocked, we leave button active, so the message can be displayed (cf bug #97)
	bSurrender->block((curInt == attackerInt ? defendingHeroInstance : attackingHeroInstance) == nullptr);
	bFlee->block(!curInt->cb->battleCanFlee());
	bSurrender->block(curInt->cb->battleGetSurrenderCost() < 0);


	//set casting flag to true if creature can use it to not check it every time
	const Bonus *spellcaster = s->getBonusLocalFirst(Selector::type(Bonus::SPELLCASTER)),
		*randomSpellcaster = s->getBonusLocalFirst(Selector::type(Bonus::RANDOM_SPELLCASTER));
	if (s->casts &&  (spellcaster || randomSpellcaster))
	{
		stackCanCastSpell = true;
		if(randomSpellcaster)
			creatureSpellToCast = -1; //spell will be set later on cast

		creatureSpellToCast = curInt->cb->battleGetRandomStackSpell(s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
		//TODO: what if creature can cast BOTH random genie spell and aimed spell?
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

void CBattleInterface::endCastingSpell()
{
	assert(spellDestSelectMode);

	delete spellToCast;
	spellToCast = nullptr;
	sp = nullptr;
	spellDestSelectMode = false;
	CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER);

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
				for (Bonus * spellBonus : spellBonuses)
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
							case SpellID::REMOVE_OBSTACLE:
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

	creAnims[ID]->nextFrame(to, creDir[ID]);
	creAnims[ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);

	//printing amount
	if(stack->count > 0 //don't print if stack is not alive
		&& (!curInt->curAction
			|| (curInt->curAction->stackNumber != ID //don't print if stack is currently taking an action
				&& (curInt->curAction->actionType != Battle::WALK_AND_ATTACK  ||  stack->position != curInt->curAction->additionalInfo) //nor if it's an object of attack
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
		SDL_Surface *amountBG = nullptr;
		TBonusListPtr spellEffects = stack->getSpellBonuses();
		if(!spellEffects->size())
		{
			amountBG = amountNormal;
		}
		else
		{
			int pos=0; //determining total positiveness of effects
			std::vector<si32> spellIds = stack->activeSpells();
			for(auto & spellId : spellIds)
			{
				pos += CGI->spellh->spells[ spellId ]->positiveness;
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
		SDL_BlitSurface(amountBG, nullptr, to, &temp_rect);
		//blitting amount

		Point textPos(creAnims[ID]->pos.x + xAdd + 15, creAnims[ID]->pos.y + yAdd + 5);
		graphics->fonts[FONT_TINY]->renderTextCenter(to, makeNumberShort(stack->count), Colors::WHITE, textPos);
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
	std::map<int, std::list<int> > hexToPart;
	hexToPart[12] = list_of<int>(8)(1)(7);
	hexToPart[45] = list_of<int>(12)(6);
	hexToPart[101] = list_of<int>(10);
	hexToPart[118] = list_of<int>(2);
	hexToPart[165] = list_of<int>(11);
	hexToPart[186] = list_of<int>(3);
#endif

	auto it = hexToPart.find(hex);
	if(it != hexToPart.end())
	{
		for(int wallNum : it->second)
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
				const CStack *turret = nullptr;

				for(const CStack *s : stacks)
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
	for(auto &oi : curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE/*  ||  oi.obstacleType == CObstacleInstance::MOAT*/)
			blitAt(imageOfObstacle(*oi), oi->getInfo().width, oi->getInfo().height, backgroundWithHexes);
	}

	if(settings["battle"]["cellBorders"].Bool())
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, nullptr, backgroundWithHexes, nullptr);

	if(settings["battle"]["stackRange"].Bool())
	{
		std::vector<BattleHex> hexesToShade = occupyableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		for(BattleHex hex : hexesToShade)
		{
			int i = hex.getY(); //row
			int j = hex.getX()-1; //column
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 * i;
			SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
			CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, nullptr, backgroundWithHexes, &temp_rect);
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
	if(to == nullptr)
		to = screen;
	std::list< std::list<ProjectileInfo>::iterator > toBeDeleted;
	for(auto it = projectiles.begin(); it!=projectiles.end(); ++it)
	{
		// Check if projectile is already visible (shooter animation did the shot)
		if (!it->shotDone)
		{
			if (creAnims[it->stackID]->getCurrentFrame() > it->animStartDelay)
			{
				//at this point projectile should become visible
				creAnims[it->stackID]->pause(); // pause animation
				it->shotDone = true;
			}
			else
				continue; // wait...
		}

		SDL_Rect dst;
		dst.h = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->h;
		dst.w = idToProjectile[it->creID]->ourImages[it->frameNum].bitmap->w;
		dst.x = it->x - dst.w / 2;
		dst.y = it->y - dst.h / 2;

		if(it->reverse)
		{
			SDL_Surface * rev = CSDL_Ext::rotate01(idToProjectile[it->creID]->ourImages[it->frameNum].bitmap);
			CSDL_Ext::blit8bppAlphaTo24bpp(rev, nullptr, to, &dst);
			SDL_FreeSurface(rev);
		}
		else
		{
			CSDL_Ext::blit8bppAlphaTo24bpp(idToProjectile[it->creID]->ourImages[it->frameNum].bitmap, nullptr, to, &dst);
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
				it->y = it->catapultInfo->calculateY(it->x);

				++(it->frameNum);
				it->frameNum %= idToProjectile[it->creID]->ourImages.size();
			}
			else
			{
				// Normal projectile, just add the calculated "deltas" to the x and y positions.
				it->x += it->dx;
				it->y += it->dy;
			}
		}
	}
	for(auto & elem : toBeDeleted)
	{
		// resume animation
		creAnims[elem->stackID]->play();
		projectiles.erase(elem);
	}
}

void CBattleInterface::endAction(const BattleAction* action)
{
	const CStack * stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(action->actionType == Battle::HERO_SPELL)
	{
		if(action->side)
			defendingHero->setPhase(0);
		else
			attackingHero->setPhase(0);
	}

	if(stack && action->actionType == Battle::WALK &&
		!creAnims[action->stackNumber]->isIdle()) //walk or walk & attack
	{
		pendingAnims.push_back(std::make_pair(new CMovementEndAnimation(this, stack, action->destinationTile), false));
	}

	//check if we should reverse stacks
	//for some strange reason, it's not enough
	TStacks stacks = curInt->cb->battleGetStacks(CBattleCallback::MINE_AND_ENEMY);

	for(const CStack *s : stacks)
	{
		if(s && creDir[s->ID] != bool(s->attackerOwned) && s->alive()
		   && creAnims[s->ID]->isIdle())
		{
			addNewAnim(new CReverseAnimation(this, s, s->position, false));
		}
	}

	queue->update();

	if(tacticsMode) //stack ended movement in tactics phase -> select the next one
		bTacticNextStack(stack); 

	if( action->actionType == Battle::HERO_SPELL) //we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
		redrawBackgroundWithHexes(activeStack);

	if(activeStack && !animsAreDisplayed.get() && pendingAnims.empty() && !active)
	{
		logGlobal->warnStream() << "Something wrong... interface was deactivated but there is no animation. Reactivating...";
		activate();
	}
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
	setActiveStack(nullptr);
	setHoveredStack(nullptr);

	if(action->actionType == Battle::END_TACTIC_PHASE)
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

		return;
	}

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(stack)
	{
		queue->update();
	}
	else
	{
		assert(action->actionType == Battle::HERO_SPELL); //only cast spell is valid action without acting stack number
	}

	if(action->actionType == Battle::WALK
		|| (action->actionType == Battle::WALK_AND_ATTACK && action->destinationTile != stack->position))
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
	redraw(); // redraw after deactivation, including proper handling of hovered hexes

	char txt[400];

	if (action->actionType == Battle::HERO_SPELL) //when hero casts spell
	{
		if(action->side)
			defendingHero->setPhase(4);
		else
			attackingHero->setPhase(4);
		return;
	}
	if(!stack)
	{
        logGlobal->errorStream()<<"Something wrong with stackNumber in actionStarted. Stack number: "<<action->stackNumber;
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case Battle::WAIT:
		txtid = 136;
		break;
	case Battle::BAD_MORALE:
		txtid = -34; //negative -> no separate singular/plural form
		displayEffect(30,stack->position);
		CCS->soundh->playSound(soundBase::BADMRLE);
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
		case Battle::STACK_HEAL:
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
	setActiveStack(nullptr);
	btactEnd->block(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed(0, true); //should bound stacks be immobile?
}

void CBattleInterface::bTacticNextStack(const CStack *current /*= nullptr*/)
{
	if(!current)
		current = activeStack;

	//no switching stacks when the current one is moving
	waitForAnims();

	TStacks stacksOfMine = tacticianInterface->cb->battleGetStacks(CBattleCallback::ONLY_MINE);
	stacksOfMine.erase(std::remove_if(stacksOfMine.begin(), stacksOfMine.end(), &immobile), stacksOfMine.end());
	auto it = vstd::find(stacksOfMine, current);
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
	ECursor::ECursorTypes cursorType = ECursor::COMBAT;
	int cursorFrame = ECursor::COMBAT_POINTER; //TODO: is this line used?
	
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

	const CStack * const sactive = activeStack;
	//Get stack on the hex - first try to grab the alive one, if not found -> allow dead stacks.
	const CStack *shere = curInt->cb->battleGetStackByPos(myNumber, true);
	if(!shere)
		shere = curInt->cb->battleGetStackByPos(myNumber, false);

	if (!sactive)
		return;

	bool ourStack = false;
	if (shere)
		ourStack = shere->owner == curInt->playerID;
	
	//stack changed, update selection border
	if (shere != mouseHoveredStack)
	{
		setHoveredStack(shere);
	}

	localActions.clear();
	illegalActions.clear();

	for (PossibleActions action : possibleActions)
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
			{
				if (!(shere && shere->alive())) //we can walk on dead stacks
				{
					if (canStackMoveHere (sactive, myNumber))
						legalAction = true;
				}
				break;
			}
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
					skill = sactive->valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, SpellID::TELEPORT));
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
					auto hero = curInt->cb->battleGetMyHero();
					assert(!creatureCasting); //we assume hero casts this spell
					assert(hero);

					legalAction = true;
					bool hexesOutsideBattlefield = false;
					auto tilesThatMustBeClear = sp->rangeInHexes(myNumber, hero->getSpellSchoolLevel(sp), side, &hexesOutsideBattlefield);
					for(BattleHex hex : tilesThatMustBeClear)
					{
						if(curInt->cb->battleGetStackByPos(hex, false)  ||  !!curInt->cb->battleGetObstacleOnPos(hex, false) 
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
							giveCommand (Battle::WALK ,myNumber, activeStack->ID);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand (Battle::WALK, shiftedDest, activeStack->ID);
					}
					else
					{
						giveCommand (Battle::WALK, myNumber, activeStack->ID);
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
						giveCommand(Battle::WALK_AND_ATTACK, attackFromHex, activeStack->ID, myNumber);
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

				realizeAction = [=] {giveCommand(Battle::SHOOT, myNumber, activeStack->ID);};
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
					case SpellID::SACRIFICE:
					case SpellID::TELEPORT:
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
				sp = nullptr;
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
				realizeAction = [=]{ giveCommand(Battle::STACK_HEAL, myNumber, activeStack->ID); }; //command healing
				break;
			case RISE_DEMONS:
				cursorType = ECursor::SPELLBOOK;
				realizeAction = [=]{ giveCommand(Battle::DAEMON_SUMMONING, myNumber, activeStack->ID); };
				break;
			case CATAPULT:
				cursorFrame = ECursor::COMBAT_SHOOT_CATAPULT;
				realizeAction = [=]{ giveCommand(Battle::CATAPULT, myNumber, activeStack->ID); };
				break;
			case CREATURE_INFO:
			{
				cursorFrame = ECursor::COMBAT_QUERY;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[297]) % shere->getName()).str();
				realizeAction = [=]{ GH.pushInt(createCreWindow(shere, true)); };
				break;
			}
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
				switch (sp->id.toEnum())
				{
					case SpellID::TELEPORT: //don't cast spell yet, only select target		
						possibleActions.push_back (TELEPORT);
						spellToCast->selectedStack = selectedStack->ID;
						break;
					case SpellID::SACRIFICE:
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
						giveCommand(Battle::MONSTER_SPELL, myNumber, sactive->ID, creatureSpellToCast);
					}
					else //unknown random spell
					{
						giveCommand(Battle::MONSTER_SPELL, myNumber, sactive->ID, curInt->cb->battleGetRandomStackSpell(shere, CBattleInfoCallback::RANDOM_GENIE));
					}
				}
				else
				{
					assert (sp);
					switch (sp->id.toEnum())
					{
						case SpellID::SACRIFICE:
							spellToCast->destinationTile = selectedStack->position; //cast on first creature that will be resurrected
							break;
						default:
							spellToCast->destinationTile = myNumber;
							break;
					}
					curInt->cb->battleMakeAction(spellToCast);
					endCastingSpell();
				}
				selectedStack = nullptr;
			}
		};
	}
	realizeThingsToDo();
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

	sp = nullptr;
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
	switch(CCS->curh->frame)
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
	int frameIndex = (animCount+1) * 25 / getAnimSpeed();
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

	case CObstacleInstance::MOAT://moat is blitted by SiegeHelper, this shouldn't be called
	default:
		assert(0);
		return nullptr;
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
        logGlobal->errorStream() << "I don't know how to animate appearing obstacle of type " << (int)oi.obstacleType;
		return;
	}

	if(graphics->battleACToDef[effectID].empty())
	{
        logGlobal->errorStream() << "Cannot find def for effect type " << effectID;
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

const CGHeroInstance * CBattleInterface::currentHero() const
{
	if(attackingHeroInstance->tempOwner == curInt->playerID)
		return attackingHeroInstance;
	else
		return defendingHeroInstance;
}

InfoAboutHero CBattleInterface::enemyHero() const
{
	InfoAboutHero ret;
	if(attackingHeroInstance->tempOwner == curInt->playerID)
		curInt->cb->getHeroInfo(defendingHeroInstance, ret);
	else
		curInt->cb->getHeroInfo(attackingHeroInstance, ret);

	return ret;
}

void CBattleInterface::requestAutofightingAIToTakeAction()
{
	assert(curInt->isAutoFightOn);

	boost::thread aiThread([&] 
	{
		auto  ba = new BattleAction(curInt->autofightingAI->activeStack(activeStack));

		if(curInt->isAutoFightOn)
		{
			givenCommand->setn(ba);
		}
		else
		{
			delete ba;
			boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
			activateStack();
		}
	});
	
	aiThread.detach();
}

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
	for(auto & elem : walls)
	{
		SDL_FreeSurface(elem);
	}
}

std::string CBattleInterface::SiegeHelper::getSiegeName(ui16 what, ui16 additInfo) const
{
	if(what == 2 || what == 3 || what == 8)
		vstd::amin(additInfo, 2);
	else
		vstd::amin(additInfo, 3);

	std::string & prefix = town->town->clientInfo.siegePrefix;
	std::string addit = boost::lexical_cast<std::string>(additInfo);

	switch(what)
	{
	case 0: //background
		return prefix + "BACK.BMP";
	case 1: //background wall
		{
			switch(town->town->faction->index)
			{
			case 5: case 4: case 1: case 6:
				return prefix + "TPW1.BMP";
			default:
				return prefix + "TPWL.BMP";
			}
		}
	case 2: //keep
		return prefix + "MAN" + addit + ".BMP";
	case 3: //bottom tower
		return prefix + "TW1" + addit + ".BMP";
	case 4: //bottom wall
		return prefix + "WA1" + addit + ".BMP";
	case 5: //below gate
		return prefix + "WA3" + addit + ".BMP";
	case 6: //over gate
		return prefix + "WA4" + addit + ".BMP";
	case 7: //upper wall
		return prefix + "WA6" + addit + ".BMP";
	case 8: //upper tower
		return prefix + "TW2" + addit + ".BMP";
	case 9: //gate
		return prefix + "DRW" + addit + ".BMP";
	case 10: //gate arch
		return prefix + "ARCH.BMP";
	case 11: //bottom static wall
		return prefix + "WA2.BMP";
	case 12: //upper static wall
		return prefix + "WA5.BMP";
	case 13: //moat
		return prefix + "MOAT.BMP";
	case 14: //mlip
		return prefix + "MLIP.BMP";
	case 15: //keep creature cover
		return prefix + "MANC.BMP";
	case 16: //bottom turret creature cover
		return prefix + "TW1C.BMP";
	case 17: //upper turret creature cover
		return prefix + "TW2C.BMP";
	default:
		return "";
	}
}

/// What: 1. background wall, 2. keep, 3. bottom tower, 4. bottom wall, 5. wall below gate,
/// 6. wall over gate, 7. upper wall, 8. upper tower, 9. gate, 10. gate arch, 11. bottom static wall, 12. upper static wall, 13. moat, 14. mlip,
/// 15. keep turret cover, 16. lower turret cover, 17. upper turret cover
void CBattleInterface::SiegeHelper::printPartOfWall(SDL_Surface * to, int what)
{
	Point pos = Point(-1, -1);

	if (what >= 1 && what <= 17)
	{
		pos.x = owner->siegeH->town->town->clientInfo.siegePositions[what].x + owner->pos.x;
		pos.y = owner->siegeH->town->town->clientInfo.siegePositions[what].y + owner->pos.y;
	}

	if(pos.x != -1)
	{
		blitAt(walls[what], pos.x, pos.y, to);
	}
}

CatapultProjectileInfo::CatapultProjectileInfo(Point from, Point dest)
{
	facA = 0.005; // seems to be constant

	// system of 2 linear equations, solutions of which are missing coefficients
	// for quadratic equation a*x*x + b*x + c
	double eq[2][3] = {
		{ static_cast<double>(from.x), 1.0, from.y - facA*from.x*from.x },
		{ static_cast<double>(dest.x), 1.0, dest.y - facA*dest.x*dest.x }
	};

	// solve system via determinants
	double det  = eq[0][0] * eq[1][1] - eq[1][0] * eq[0][1];
	double detB = eq[0][2] * eq[1][1] - eq[1][2] * eq[0][1];
	double detC = eq[0][0] * eq[1][2] - eq[1][0] * eq[0][2];

	facB = detB / det;
	facC = detC / det;

	// make sure that parabola is correct e.g. passes through from and dest
	assert(fabs(calculateY(from.x) - from.y) < 1.0);
	assert(fabs(calculateY(dest.x) - dest.y) < 1.0);
}

double CatapultProjectileInfo::calculateY(double x)
{
	return facA * pow(x, 2.0) + facB * x + facC;
}
