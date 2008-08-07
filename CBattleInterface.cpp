#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "hch/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "hch/CObjectHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CDefHandler.h"
#include "CCursorHandler.h"
#include "CCallback.h"
#include "CGameState.h"
#include "hch/CGeneralTextHandler.h"
#include "client/CCreatureAnimation.h"
#include "client/Graphics.h"
#include <queue>
#include <sstream>
#include "lib/CondSh.h"
#ifndef __GNUC__
const double M_PI = 3.14159265358979323846;
#else
#define _USE_MATH_DEFINES
#include <cmath>
#endif

extern SDL_Surface * screen;
extern TTF_Font * GEOR13;
extern SDL_Color zwykly;
SDL_Surface * CBattleInterface::cellBorder, * CBattleInterface::cellShade;

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2)
: printCellBorders(true), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), activeStack(-1), givenCommand(NULL), attackingInfo(NULL), myTurn(false)
{
	givenCommand = new CondSh<BattleAction *>(NULL);
	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks();
	for(std::map<int, CStack>::iterator b=stacks.begin(); b!=stacks.end(); ++b)
	{
		std::pair <int, int> coords = CBattleHex::getXYUnitAnim(b->second.position, b->second.owner == attackingHeroInstance->tempOwner, b->second.creature);
		creAnims[b->second.ID] = (new CCreatureAnimation(b->second.creature->animDefName));
		creAnims[b->second.ID]->setType(2);
		creAnims[b->second.ID]->pos = genRect(creAnims[b->second.ID]->fullHeight, creAnims[b->second.ID]->fullWidth, coords.first, coords.second);
		creDir[b->second.ID] = b->second.owner==attackingHeroInstance->tempOwner;
	}
	//preparing menu background and terrain
	std::vector< std::string > & backref = graphics->battleBacks[ LOCPLINT->cb->battleGetBattlefieldType() ];
	background = BitmapHandler::loadBitmap(backref[ rand() % backref.size()] );
	menu = BitmapHandler::loadBitmap("CBAR.BMP");
	graphics->blueToPlayersAdv(menu, hero1->tempOwner);

	//preparing graphics for displaying amounts of creatures
	amountBasic = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	amountNormal = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNormal);
	for(int g=0; g<amountNormal->format->palette->ncolors; ++g)
	{
		if((amountNormal->format->palette->colors+g)->b != 132 &&
			(amountNormal->format->palette->colors+g)->g != 231 &&
			(amountNormal->format->palette->colors+g)->r != 255) //it's not yellow border
		{
			(amountNormal->format->palette->colors+g)->r = (float)((amountNormal->format->palette->colors+g)->r) * 0.54f;
			(amountNormal->format->palette->colors+g)->g = (float)((amountNormal->format->palette->colors+g)->g) * 0.19f;
			(amountNormal->format->palette->colors+g)->b = (float)((amountNormal->format->palette->colors+g)->b) * 0.93f;
		}
	}

	////blitting menu background and terrain
	blitAt(background, 0, 0);
	blitAt(menu, 0, 556);
	CSDL_Ext::update();

	//preparing buttons and console
	bOptions = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bOptionsf,this), 3, 561, "icm003.def", false, NULL, false);
	bSurrender = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bSurrenderf,this), 54, 561, "icm001.def", false, NULL, false);
	bFlee = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bFleef,this), 105, 561, "icm002.def", false, NULL, false);
	bAutofight  = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bAutofightf,this), 157, 561, "icm004.def", false, NULL, false);
	bSpell = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bSpellf,this), 645, 561, "icm005.def", false, NULL, false);
	bWait = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bWaitf,this), 696, 561, "icm006.def", false, NULL, false);
	bDefence = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bDefencef,this), 747, 561, "icm007.def", false, NULL, false);
	bConsoleUp = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleUpf,this), 624, 561, "ComSlide.def", false, NULL, false);
	bConsoleDown = new AdventureMapButton (std::string(), std::string(), boost::bind(&CBattleInterface::bConsoleDownf,this), 624, 580, "ComSlide.def", false, NULL, false);
	bConsoleDown->bitmapOffset = 2;
	console = new CBattleConsole();
	console->pos.x = 211;
	console->pos.y = 560;
	console->pos.w = 406;
	console->pos.h = 38;

	//loading hero animations
	if(hero1) // attacking hero
	{
		attackingHero = new CBattleHero(graphics->battleHeroes[hero1->type->heroType], 0, 0, false, hero1->tempOwner);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, -40, 0);
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		defendingHero = new CBattleHero(graphics->battleHeroes[hero2->type->heroType], 0, 0, true, hero2->tempOwner);
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, 690, 0);
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
	for(int h=0; h<187; ++h)
	{
		bfield[h].myNumber = h;

		int x = 14 + ((h/17)%2==0 ? 22 : 0) + 44*(h%17);
		int y = 86 + 42 * (h/17);
		bfield[h].pos = genRect(cellShade->h, cellShade->w, x, y);
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
}

CBattleInterface::~CBattleInterface()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(menu);
	SDL_FreeSurface(amountBasic);
	SDL_FreeSurface(amountNormal);
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
}

void CBattleInterface::activate()
{
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();
	bConsoleUp->activate();
	bConsoleDown->activate();
	for(int b=0; b<187; ++b)
	{
		bfield[b].activate();
	}
}

void CBattleInterface::deactivate()
{
	bOptions->deactivate();
	bSurrender->deactivate();
	bFlee->deactivate();
	bAutofight->deactivate();
	bSpell->deactivate();
	bWait->deactivate();
	bDefence->deactivate();
	bConsoleUp->deactivate();
	bConsoleDown->deactivate();
	for(int b=0; b<187; ++b)
	{
		bfield[b].deactivate();
	}
}

void CBattleInterface::show(SDL_Surface * to)
{
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks(); //used in a few places
	++animCount;
	if(!to) //"evaluating" to
		to = screen;

	//showing background
	blitAt(background, 0, 0, to);
	if(printCellBorders) //printing cell borders
	{
		for(int i=0; i<11; ++i) //rows
		{
			for(int j=0; j<15; ++j) //columns
			{
				int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
				int y = 86 + 42 * i;
				CSDL_Ext::blit8bppAlphaTo24bpp(cellBorder, NULL, to, &genRect(cellBorder->h, cellBorder->w, x, y));
			}
		}
	}
	//printing hovered cell
	for(int b=0; b<187; ++b)
	{
		if(bfield[b].strictHovered && bfield[b].hovered)
		{
			int x = 14 + ((b/17)%2==0 ? 22 : 0) + 44*(b%17);
			int y = 86 + 42 * (b/17);
			CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, NULL, to, &genRect(cellShade->h, cellShade->w, x, y));
		}
	}
	//showing selected unit's range
	if(activeStack != -1 && creAnims[activeStack]->getType() != 0) //don't show if unit is moving
	{
		showRange(to, activeStack);
	}

	//showing menu background and console
	blitAt(menu, 0, 556, to);
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

	//showing hero animations
	if(attackingHero)
		attackingHero->show(to);
	if(defendingHero)
		defendingHero->show(to);

	////showing units //a lot of work...
	std::vector<int> stackAliveByHex[187];
	//double loop because dead stacks should be printed first
	for(std::map<int, CStack>::iterator j=stacks.begin(); j!=stacks.end(); ++j)
	{
		if(j->second.alive)
			stackAliveByHex[j->second.position].push_back(j->second.ID);
	}
	std::vector<int> stackDeadByHex[187];
	for(std::map<int, CStack>::iterator j=stacks.begin(); j!=stacks.end(); ++j)
	{
		if(!j->second.alive)
			stackDeadByHex[j->second.position].push_back(j->second.ID);
	}

	attackingShowHelper(); // handle attack animation

	for(int b=0; b<187; ++b) //showing dead stacks
	{
		for(int v=0; v<stackDeadByHex[b].size(); ++v)
		{
			creAnims[stackDeadByHex[b][v]]->nextFrame(to, creAnims[stackDeadByHex[b][v]]->pos.x, creAnims[stackDeadByHex[b][v]]->pos.y, creDir[stackDeadByHex[b][v]], (animCount%4==0 || creAnims[stackDeadByHex[b][v]]->getType()!=2) && stacks[stackDeadByHex[b][v]].alive, stackDeadByHex[b][v]==activeStack); //increment always when moving, never if stack died
			//printing amount
			if(stacks[stackDeadByHex[b][v]].amount > 0) //don't print if stack is not alive
			{
				int xAdd = stacks[stackDeadByHex[b][v]].attackerOwned ? 220 : 202;

				CSDL_Ext::blit8bppAlphaTo24bpp(amountNormal, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[stackDeadByHex[b][v]]->pos.x + xAdd, creAnims[stackDeadByHex[b][v]]->pos.y + 260));
				std::stringstream ss;
				ss<<stacks[stackDeadByHex[b][v]].amount;
				CSDL_Ext::printAtMiddleWB(ss.str(), creAnims[stackDeadByHex[b][v]]->pos.x + xAdd + 14, creAnims[stackDeadByHex[b][v]]->pos.y + 260 + 4, GEOR13, 20, zwykly, to);
			}
		}
	}
	for(int b=0; b<187; ++b) //showing alive stacks
	{
		for(int v=0; v<stackAliveByHex[b].size(); ++v)
		{
			creAnims[stackAliveByHex[b][v]]->nextFrame(to, creAnims[stackAliveByHex[b][v]]->pos.x, creAnims[stackAliveByHex[b][v]]->pos.y, creDir[stackAliveByHex[b][v]], (animCount%4==0) && stacks[stackAliveByHex[b][v]].alive, stackAliveByHex[b][v]==activeStack); //increment always when moving, never if stack died
			//printing amount
			if(stacks[stackAliveByHex[b][v]].amount > 0) //don't print if stack is not alive
			{
				int xAdd = stacks[stackAliveByHex[b][v]].attackerOwned ? 220 : 202;

				CSDL_Ext::blit8bppAlphaTo24bpp(amountNormal, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[stackAliveByHex[b][v]]->pos.x + xAdd, creAnims[stackAliveByHex[b][v]]->pos.y + 260));
				std::stringstream ss;
				ss<<stacks[stackAliveByHex[b][v]].amount;
				CSDL_Ext::printAtMiddleWB(ss.str(), creAnims[stackAliveByHex[b][v]]->pos.x + xAdd + 14, creAnims[stackAliveByHex[b][v]]->pos.y + 260 + 4, GEOR13, 20, zwykly, to);
			}
		}
	}
	//units shown
	projectileShowHelper(to);//showing projectiles
}

bool CBattleInterface::reverseCreature(int number, int hex, bool wideTrick)
{
	if(creAnims[number]==NULL)
		return false; //there is no such creature
	creAnims[number]->setType(8);
	for(int g=0; g<creAnims[number]->framesInGroup(8); ++g)
	{
		show();
		CSDL_Ext::update();
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
		if((animCount+1)%4)
			creAnims[number]->incrementFrame();
	}
	creDir[number] = !creDir[number];

	CStack curs = *LOCPLINT->cb->battleGetStackByID(number);
	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(hex, creDir[number], curs.creature);
	creAnims[number]->pos.x = coords.first;
	//creAnims[number]->pos.y = coords.second;

	if(wideTrick && curs.creature->isDoubleWide())
	{
		if(!creDir[number])
			creAnims[number]->pos.x -= 44;
	}

	creAnims[number]->setType(7);
	for(int g=0; g<creAnims[number]->framesInGroup(7); ++g)
	{
		show();
		CSDL_Ext::update();
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	creAnims[number]->setType(2);

	return true;
}

void CBattleInterface::bOptionsf()
{
}

void CBattleInterface::bSurrenderf()
{
}

void CBattleInterface::bFleef()
{
	giveCommand(4,0,0);
	CGI->curh->changeGraphic(0, 0);
}

void CBattleInterface::bAutofightf()
{
}

void CBattleInterface::bSpellf()
{
}

void CBattleInterface::bWaitf()
{
}

void CBattleInterface::bDefencef()
{
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

void CBattleInterface::newStack(CStack stack)
{
	creAnims[stack.ID] = new CCreatureAnimation(stack.creature->animDefName);
	creAnims[stack.ID]->setType(2);
	creDir[stack.ID] = stack.owner==attackingHeroInstance->tempOwner;
}

void CBattleInterface::stackRemoved(CStack stack)
{
	delete creAnims[stack.ID];
	creAnims.erase(stack.ID);
}

void CBattleInterface::stackKilled(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	if(creAnims[ID]->getType() != 2)
	{
		return; //something went wrong
	}
	if(byShooting) //delay hit animation
	{
		CStack attacker = *LOCPLINT->cb->battleGetStackByID(IDby);
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
				show();
				CSDL_Ext::update();
				SDL_framerateDelay(LOCPLINT->mainFPSmng);
			}
		}
	}
	creAnims[ID]->setType(5); //death
	for(int i=0; i<creAnims[ID]->framesInGroup(5)-1; ++i)
	{
		show();
		CSDL_Ext::update();
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}

	printConsoleAttacked(ID, dmg, killed, IDby);
}

void CBattleInterface::stackActivated(int number)
{
	//givenCommand = NULL;
	activeStack = number;
	shadedHexes = LOCPLINT->cb->battleGetAvailableHexes(number);
	myTurn = true;
}

void CBattleInterface::stackMoved(int number, int destHex, bool startMoving, bool endMoving)
{
	//a few useful variables
	int curStackPos = LOCPLINT->cb->battleGetPos(number);
	int steps = creAnims[number]->framesInGroup(0);
	int hexWbase = 44, hexHbase = 42;
	bool twoTiles = LOCPLINT->cb->battleGetCreature(number).isDoubleWide();

	if(startMoving) //animation of starting move
	{
		deactivate();
		CGI->curh->hide();
		creAnims[number]->setType(20);
		//LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
		for(int i=0; i<creAnims[number]->framesInGroup(20); ++i)
		{
			show();
			CSDL_Ext::update();
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
			if((animCount+1)%4)
				creAnims[number]->incrementFrame();
		}
	}

	int mutPos = BattleInfo::mutualPosition(curStackPos, destHex);

	{
		switch(mutPos) //reverse unit if necessary
		{
		case 0:	case 4:	case 5:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos, twoTiles);
			break;
		case 1:	case 2: case 3:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos, twoTiles);
			break;
		}
		//moving instructions
		creAnims[number]->setType(0);
		for(int i=0; i<steps; ++i)
		{
			switch(mutPos)
			{
			case 0:
				creAnims[number]->pos.x -= hexWbase/(2*steps);
				creAnims[number]->pos.y -= hexHbase/steps;
				break;
			case 1:
				creAnims[number]->pos.x += hexWbase/(2*steps);
				creAnims[number]->pos.y -= hexHbase/steps;
				break;
			case 2:
				creAnims[number]->pos.x += hexWbase/steps;
				break;
			case 3:
				creAnims[number]->pos.x += hexWbase/(2*steps);
				creAnims[number]->pos.y += hexHbase/steps;
				break;
			case 4:
				creAnims[number]->pos.x -= hexWbase/(2*steps);
				creAnims[number]->pos.y += hexHbase/steps;
				break;
			case 5:
				creAnims[number]->pos.x -= hexWbase/steps;
				break;
			}
			show();
			CSDL_Ext::update();
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
			if((animCount+1)%4)
				creAnims[number]->incrementFrame();
		}
	}

	if(endMoving) //animation of ending move
	{
		creAnims[number]->setType(21);
		for(int i=0; i<creAnims[number]->framesInGroup(21); ++i)
		{
			show();
			CSDL_Ext::update();
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
			if((animCount+1)%4)
				creAnims[number]->incrementFrame();
		}
		creAnims[number]->setType(2); //resetting to default
		activate();
		CGI->curh->show();
	}

	CStack curs = *LOCPLINT->cb->battleGetStackByID(number);
	if(endMoving) //resetting to default
	{
		if(creDir[number] != (curs.owner == attackingHeroInstance->tempOwner))
			reverseCreature(number, destHex, twoTiles);	
	}
	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(destHex, creDir[number], curs.creature);
	creAnims[number]->pos.x = coords.first;
	if(!endMoving && twoTiles && (creDir[number] != (curs.owner == attackingHeroInstance->tempOwner))) //big creature is reversed
		creAnims[number]->pos.x -= 44;
	creAnims[number]->pos.y = coords.second;
}

void CBattleInterface::stackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	if(creAnims[ID]->getType() != 2)
	{
		return; //something went wrong
	}
	if(byShooting) //delay hit animation
	{
		CStack attacker = *LOCPLINT->cb->battleGetStackByID(IDby);
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
				show();
				CSDL_Ext::update();
				SDL_framerateDelay(LOCPLINT->mainFPSmng);
			}
		}
	}
	creAnims[ID]->setType(3); //getting hit
	for(int i=0; i<creAnims[ID]->framesInGroup(3); ++i)
	{
		show();
		CSDL_Ext::update();
		SDL_framerateDelay(LOCPLINT->mainFPSmng);
	}
	creAnims[ID]->setType(2);

	printConsoleAttacked(ID, dmg, killed, IDby);
}

void CBattleInterface::stackAttacking(int ID, int dest)
{
	if(attackingInfo != NULL)
	{
		return; //something went wrong
	}
	CStack aStack = *LOCPLINT->cb->battleGetStackByID(ID); //attacking stack
	if(aStack.creature->isDoubleWide())
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
		}
	}
	else //else for if(aStack.creature->isDoubleWide())
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

	attackingInfo = new CAttHelper;
	attackingInfo->dest = dest;
	attackingInfo->frame = 0;
	attackingInfo->ID = ID;
	attackingInfo->reversing = false;
	attackingInfo->shooting = false;

	switch(BattleInfo::mutualPosition(aStack.position, dest)) //attack direction
	{
		case 0:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(10);
			break;
		case 1:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(10);
			break;
		case 2:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(11);
			break;
		case 3:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(12);
			break;
		case 4:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(12);
			break;
		case 5:
			attackingInfo->maxframe = creAnims[ID]->framesInGroup(11);
			break;
	}
}

void CBattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);
}

void CBattleInterface::giveCommand(ui8 action, ui16 tile, ui32 stack)
{
	BattleAction * ba = new BattleAction(); //to be deleted by engine
	ba->actionType = action;
	ba->destinationTile = tile;
	ba->stackNumber = stack;
	givenCommand->setn(ba);
	myTurn = false;
	activeStack = -1;
}

void CBattleInterface::hexLclicked(int whichOne)
{
	if((whichOne%17)!=0 && (whichOne%17)!=16) //if player is trying to attack enemey unit or move creature stack
	{
		if(!myTurn)
			return; //we are not permit to do anything

		int atCre = LOCPLINT->cb->battleGetStack(whichOne); //creature at destination tile; -1 if there is no one
		//LOCPLINT->cb->battleGetCreature();
		if(atCre==-1) //no creature at that tile
		{
			if(std::find(shadedHexes.begin(),shadedHexes.end(),whichOne)!=shadedHexes.end())// and it's in our range
				giveCommand(2,whichOne,activeStack);
		}
		else if(LOCPLINT->cb->battleGetStackByID(atCre)->owner != attackingHeroInstance->tempOwner
			&& LOCPLINT->cb->battleCanShoot(activeStack, whichOne)) //shooting
		{
			giveCommand(7,whichOne,activeStack);
		}
		else if(LOCPLINT->cb->battleGetStackByID(atCre)->owner != attackingHeroInstance->tempOwner) //attacking
		{
			giveCommand(6,whichOne,activeStack);
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
	projectileAngle = atan2(float(abs(dest - fromHex)/17), float(abs(dest - fromHex)%17));
	if(fromHex < dest)
		projectileAngle = -projectileAngle;

	SProjectileInfo spi;
	spi.creID = LOCPLINT->cb->battleGetStackByID(ID)->creature->idNumber;

	spi.step = 0;
	spi.frameNum = 0;
	spi.spin = CGI->creh->idToProjectileSpin[spi.creID];

	std::pair<int, int> xycoord = CBattleHex::getXYUnitAnim(LOCPLINT->cb->battleGetPos(ID), true, &LOCPLINT->cb->battleGetCreature(ID));
	std::pair<int, int> destcoord = CBattleHex::getXYUnitAnim(dest, false, &LOCPLINT->cb->battleGetCreature(ID)); 
	destcoord.first += 250; destcoord.second += 210; //TODO: find a better place to shoot

	if(projectileAngle > straightAngle) //upper shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).upperRightMissleOffsetX;
		spi.y = xycoord.second + 150 - LOCPLINT->cb->battleGetCreature(ID).upperRightMissleOffsetY;
	}
	else if(projectileAngle < -straightAngle) //lower shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).lowerRightMissleOffsetX;
		spi.y = xycoord.second + 150 - LOCPLINT->cb->battleGetCreature(ID).lowerRightMissleOffsetY;
	}
	else //straight shot
	{
		spi.x = xycoord.first + 200 + LOCPLINT->cb->battleGetCreature(ID).rightMissleOffsetX;
		spi.y = xycoord.second + 150 - LOCPLINT->cb->battleGetCreature(ID).rightMissleOffsetY;
	}
	spi.lastStep = sqrt((float)((destcoord.first - spi.x)*(destcoord.first - spi.x) + (destcoord.second - spi.y) * (destcoord.second - spi.y))) / 40;
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
	attackingInfo->ID = ID;
	attackingInfo->reversing = false;
	attackingInfo->shooting = true;
	if(projectileAngle > straightAngle) //upper shot
		attackingInfo->shootingGroup = 14;
	else if(projectileAngle < -straightAngle) //lower shot
		attackingInfo->shootingGroup = 15;
	else //straight shot
		attackingInfo->shootingGroup = 16;
	attackingInfo->maxframe = creAnims[ID]->framesInGroup(attackingInfo->shootingGroup);
}

void CBattleInterface::showRange(SDL_Surface * to, int ID)
{
	for(int i=0; i<shadedHexes.size(); ++i)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(CBattleInterface::cellShade, NULL, to, &bfield[shadedHexes[i]].pos);
	}
}


void CBattleInterface::attackingShowHelper()
{
	if(attackingInfo && !attackingInfo->reversing)
	{
		if(attackingInfo->frame == 0)
		{
			CStack aStack = *LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(attackingInfo->shooting)
			{
				creAnims[attackingInfo->ID]->setType(attackingInfo->shootingGroup);
			}
			else
			{
				if(aStack.creature->isDoubleWide())
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							creAnims[attackingInfo->ID]->setType(10);
							break;
						case 1:
							creAnims[attackingInfo->ID]->setType(10);
							break;
						case 2:
							creAnims[attackingInfo->ID]->setType(11);
							break;
						case 3:
							creAnims[attackingInfo->ID]->setType(12);
							break;
						case 4:
							creAnims[attackingInfo->ID]->setType(12);
							break;
						case 5:
							creAnims[attackingInfo->ID]->setType(11);
							break;
					}
				}
				else //else for if(aStack.creature->isDoubleWide())
				{
					switch(BattleInfo::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
					{
						case 0:
							creAnims[attackingInfo->ID]->setType(10);
							break;
						case 1:
							creAnims[attackingInfo->ID]->setType(10);
							break;
						case 2:
							creAnims[attackingInfo->ID]->setType(11);
							break;
						case 3:
							creAnims[attackingInfo->ID]->setType(12);
							break;
						case 4:
							creAnims[attackingInfo->ID]->setType(12);
							break;
						case 5:
							creAnims[attackingInfo->ID]->setType(11);
							break;
					}
				}
			}
		}
		else if(attackingInfo->frame == (attackingInfo->maxframe - 1))
		{
			attackingInfo->reversing = true;
			CStack aStack = *LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(aStack.creature->isDoubleWide())
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
				}
			}
			else //else for if(aStack.creature->isDoubleWide())
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
			attackingInfo->reversing = false;
			creAnims[attackingInfo->ID]->setType(2);
			delete attackingInfo;
			attackingInfo = NULL;
		}
		if(attackingInfo)
		{
			attackingInfo->frame++;
		}
	}
}

void CBattleInterface::printConsoleAttacked(int ID, int dmg, int killed, int IDby)
{
	char tabh[200];
	CStack attacker = *LOCPLINT->cb->battleGetStackByID(IDby);
	CStack defender = *LOCPLINT->cb->battleGetStackByID(ID);
	int end = sprintf(tabh, CGI->generaltexth->allTexts[attacker.amount > 1 ? 377 : 376].c_str(),
		(attacker.amount > 1 ? attacker.creature->namePl.c_str() : attacker.creature->nameSing.c_str()),
		dmg);
	if(killed > 0)
	{
		if(killed > 1)
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[379].c_str(), killed, defender.creature->namePl.c_str());
		}
		else //killed == 1
		{
			sprintf(tabh + end, CGI->generaltexth->allTexts[378].c_str(), defender.creature->nameSing.c_str());
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
		CSDL_Ext::blit8bppAlphaTo24bpp(idToProjectile[it->creID]->ourImages[it->frameNum].bitmap, NULL, to, &dst);
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
		CSDL_Ext::blit8bppAlphaTo24bpp(flag->ourImages[flagAnim].bitmap, NULL, screen, &genRect(flag->ourImages[flagAnim].bitmap->h, flag->ourImages[flagAnim].bitmap->w, 752, 39));
	}
	else
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(flag->ourImages[flagAnim].bitmap, NULL, screen, &genRect(flag->ourImages[flagAnim].bitmap->h, flag->ourImages[flagAnim].bitmap->w, 31, 39));
	}
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
			++image;
			if(dh->ourImages[i+1].groupNumber!=phase) //back to appropriate frame
			{
				image = 0;
			}
			break;
		}
	}
}

CBattleHero::CBattleHero(std::string defName, int phaseG, int imageG, bool flipG, unsigned char player): phase(phaseG), image(imageG), flip(flipG), flagAnim(0)
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
		dh->ourImages[i].bitmap = CSDL_Ext::alphaTransform(dh->ourImages[i].bitmap);
	}
	dh->alphaTransformed = true;

	if(flip)
		flag = CDefHandler::giveDef("CMFLAGR.DEF");
	else
		flag = CDefHandler::giveDef("CMFLAGL.DEF");

	//coloring flag and adding transparency
	for(int i=0; i<flag->ourImages.size(); ++i)
	{
		flag->ourImages[i].bitmap = CSDL_Ext::alphaTransform(flag->ourImages[i].bitmap);
		graphics->blueToPlayersAdv(flag->ourImages[i].bitmap, player);
	}
}

CBattleHero::~CBattleHero()
{
	delete dh;
	delete flag;
}

std::pair<int, int> CBattleHex::getXYUnitAnim(int hexNum, bool attacker, CCreature * creature)
{
	std::pair<int, int> ret = std::make_pair(-500, -500); //returned value
	ret.second = -139 + 42 * (hexNum/17); //counting y
	//counting x
	if(attacker)
	{
		ret.first = -160 + 22 * ( ((hexNum/17) + 1)%2 ) + 44 * (hexNum % 17);
	}
	else
	{
		ret.first = -219 + 22 * ( ((hexNum/17) + 1)%2 ) + 44 * (hexNum % 17);
	}
	//shifting position for double - hex creatures
	if(creature->isDoubleWide())
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
	Hoverable::activate();
	MotionInterested::activate();
	ClickableL::activate();
	ClickableR::activate();
}

void CBattleHex::deactivate()
{
	Hoverable::deactivate();
	MotionInterested::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
}

void CBattleHex::hover(bool on)
{
	hovered = on;
	Hoverable::hover(on);
	if(!on && setAlterText)
	{
		myInterface->console->alterTxt = std::string();
		setAlterText = false;
	}
}

CBattleHex::CBattleHex() : myNumber(-1), accesible(true), hovered(false), strictHovered(false), myInterface(NULL), setAlterText(false)
{
}

void CBattleHex::mouseMoved(SDL_MouseMotionEvent &sEvent)
{
	if(CBattleInterface::cellShade)
	{
		if(CSDL_Ext::SDL_GetPixel(CBattleInterface::cellShade, sEvent.x-pos.x, sEvent.y-pos.y) == 0) //hovered pixel is outside hex
		{
			strictHovered = false;
		}
		else //hovered pixel is inside hex
		{
			strictHovered = true;
			if(myInterface->activeStack>=0)
			{
				if(std::find(myInterface->shadedHexes.begin(),myInterface->shadedHexes.end(),myNumber) == myInterface->shadedHexes.end())
				{
					CGI->curh->changeGraphic(1,0);
				}
				else
				{
					if(LOCPLINT->cb->battleGetStackByID(myInterface->activeStack)->creature->isFlying())
						CGI->curh->changeGraphic(1,2);
					else
						CGI->curh->changeGraphic(1,1);
				}
			}
		}
	}

	if(hovered && strictHovered) //print attacked creature to console
	{
		if(myInterface->console->alterTxt.size() == 0 && LOCPLINT->cb->battleGetStack(myNumber) != -1 &&
			LOCPLINT->cb->battleGetStackByPos(myNumber)->owner != LOCPLINT->playerID &&
			LOCPLINT->cb->battleGetStackByPos(myNumber)->alive)
		{
			char tabh[160];
			CStack attackedStack = *LOCPLINT->cb->battleGetStackByPos(myNumber);
			std::string attackedName = attackedStack.amount == 1 ? attackedStack.creature->nameSing : attackedStack.creature->namePl;
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

void CBattleHex::clickLeft(boost::logic::tribool down)
{
	if(!down && hovered && strictHovered) //we've been really clicked!
	{
		myInterface->hexLclicked(myNumber);
	}
}

void CBattleHex::clickRight(boost::logic::tribool down)
{
	int stID = LOCPLINT->cb->battleGetStack(myNumber); //id of stack being on this tile
	if(hovered && strictHovered && stID!=-1)
	{
		CStack myst = *LOCPLINT->cb->battleGetStackByID(stID); //stack info
		StackState *pom = NULL;
		if(down)
		{
			pom = new StackState();
			const CGHeroInstance *h = myst.owner == myInterface->attackingHeroInstance->tempOwner ? myInterface->attackingHeroInstance : myInterface->defendingHeroInstance;
			if(h)
			{
				pom->attackBonus = h->primSkills[0];
				pom->defenseBonus = h->primSkills[1];
				pom->luck = h->getCurrentLuck();
				pom->morale = h->getCurrentMorale();
			}
			(new CCreInfoWindow(myst.creature->idNumber,0,pom,boost::function<void()>(),boost::function<void()>()))
					->activate();
		}
		delete pom;
	}
}

CBattleConsole::CBattleConsole() : lastShown(-1), alterTxt("")
{
}

CBattleConsole::~CBattleConsole()
{
	texts.clear();
}

void CBattleConsole::show(SDL_Surface * to)
{
	if(alterTxt.size())
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

bool CBattleConsole::addText(std::string text)
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

void CBattleConsole::changeTextAt(std::string text, unsigned int pos)
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
