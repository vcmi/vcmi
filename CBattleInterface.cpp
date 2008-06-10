#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"
#include "hch\CHeroHandler.h"
#include "hch\CDefHandler.h"
#include "CCallback.h"
#include "CGameState.h"
#include "hch\CGeneralTextHandler.h"
#include <queue>
#include <sstream>

extern SDL_Surface * screen;
extern TTF_Font * GEOR13;
extern SDL_Color zwykly;
SDL_Surface * CBattleInterface::cellBorder, * CBattleInterface::cellShade;

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2) 
: printCellBorders(true), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), activeStack(-1), givenCommand(NULL), attackingInfo(NULL)
{
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
	std::vector< std::string > & backref = CGI->mh->battleBacks[ LOCPLINT->cb->battleGetBattlefieldType() ];
	background = CGI->bitmaph->loadBitmap(backref[ rand() % backref.size()] );
	menu = CGI->bitmaph->loadBitmap("CBAR.BMP");
	CSDL_Ext::blueToPlayersAdv(menu, hero1->tempOwner);

	//preparing graphics for displaying amounts of creatures
	amountBasic = CGI->bitmaph->loadBitmap("CMNUMWIN.BMP");
	amountNormal = CGI->bitmaph->loadBitmap("CMNUMWIN.BMP");
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
		attackingHero = new CBattleHero(CGI->mh->battleHeroes[hero1->type->heroType], 0, 0, false, hero1->tempOwner);
		attackingHero->pos = genRect(attackingHero->dh->ourImages[0].bitmap->h, attackingHero->dh->ourImages[0].bitmap->w, -40, 0);
	}
	else
	{
		attackingHero = NULL;
	}
	if(hero2) // defending hero
	{
		defendingHero = new CBattleHero(CGI->mh->battleHeroes[hero2->type->heroType], 0, 0, true, hero2->tempOwner);
		defendingHero->pos = genRect(defendingHero->dh->ourImages[0].bitmap->h, defendingHero->dh->ourImages[0].bitmap->w, 690, 0);
	}
	else
	{
		defendingHero = NULL;
	}

	//preparing cells and hexes
	cellBorder = CGI->bitmaph->loadBitmap("CCELLGRD.BMP");
	CSDL_Ext::alphaTransform(cellBorder);
	cellShade = CGI->bitmaph->loadBitmap("CCELLSHD.BMP");
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

	delete attackingHero;
	delete defendingHero;

	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	for(std::map< int, CCreatureAnimation * >::iterator g=creAnims.begin(); g!=creAnims.end(); ++g)
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
	if(creAnims[activeStack]->getType() != 0) //don't show if unit is moving
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

	//showing units //a lot of work...
	int stackByHex[187];
	for(int b=0; b<187; ++b)
		stackByHex[b] = -1;
	for(std::map<int, CStack>::iterator j=stacks.begin(); j!=stacks.end(); ++j)
	{
		stackByHex[j->second.position] = j->second.ID;
	}

	attackingShowHelper(); // handle attack animation

	for(int b=0; b<187; ++b)
	{
		if(stackByHex[b]!=-1)
		{
			creAnims[stackByHex[b]]->nextFrame(to, creAnims[stackByHex[b]]->pos.x, creAnims[stackByHex[b]]->pos.y, creDir[stackByHex[b]], (animCount%2==0 || creAnims[stackByHex[b]]->getType()!=2) && stacks[stackByHex[b]].alive, stackByHex[b]==activeStack); //increment always when moving, never if stack died
			//printing amount
			if(stacks[stackByHex[b]].amount > 0) //don't print if stack is not alive
			{
				if(stacks[stackByHex[b]].attackerOwned)
				{
					CSDL_Ext::blit8bppAlphaTo24bpp(amountNormal, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[stackByHex[b]]->pos.x + 220, creAnims[stackByHex[b]]->pos.y + 260));
					std::stringstream ss;
					ss<<stacks[stackByHex[b]].amount;
					CSDL_Ext::printAtMiddleWB(ss.str(), creAnims[stackByHex[b]]->pos.x + 220 + 14, creAnims[stackByHex[b]]->pos.y + 260 + 4, GEOR13, 20, zwykly, to);
				}
				else
				{
					CSDL_Ext::blit8bppAlphaTo24bpp(amountNormal, NULL, to, &genRect(amountNormal->h, amountNormal->w, creAnims[stackByHex[b]]->pos.x + 202, creAnims[stackByHex[b]]->pos.y + 260));
					std::stringstream ss;
					ss<<stacks[stackByHex[b]].amount;
					CSDL_Ext::printAtMiddleWB(ss.str(), creAnims[stackByHex[b]]->pos.x + 202 + 14, creAnims[stackByHex[b]]->pos.y + 260 + 4, GEOR13, 20, zwykly, to);
				}
			}
		}
	}
	//units shown
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
	}
	creDir[number] = !creDir[number];

	CStack curs = LOCPLINT->cb->battleGetStackByID(number);
	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(hex, creDir[number], curs.creature);
	creAnims[number]->pos.x = coords.first;
	//creAnims[number]->pos.y = coords.second;

	if(wideTrick && curs.creature->isDoubleWide())
	{
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
	BattleAction * ba = new BattleAction;
	ba->actionType = 4;
	givenCommand = ba;
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
	BattleAction * ba = new BattleAction;
	ba->actionType = 3;
	ba->stackNumber = activeStack;
	givenCommand = ba;
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

void CBattleInterface::stackKilled(int ID, int dmg, int killed, int IDby)
{
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
	givenCommand = NULL;
	activeStack = number;
}

void CBattleInterface::stackMoved(int number, int destHex, bool startMoving, bool endMoving)
{
	//a few useful variables
	int curStackPos = LOCPLINT->cb->battleGetPos(number);
	int steps = creAnims[number]->framesInGroup(0);
	int hexWbase = 44, hexHbase = 42;

	if(startMoving) //animation of starting move
	{
		for(int i=0; i<creAnims[number]->framesInGroup(20); ++i)
		{
			show();
			CSDL_Ext::update();
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
		}
	}

	int mutPos = CBattleHex::mutualPosition(curStackPos, destHex);

	if(LOCPLINT->cb->battleGetCreature(number).isDoubleWide() && 
		((creDir[number] && mutPos == 5) || (creDir[number] && mutPos == 0) || (creDir[number] && mutPos == 4))) //for special cases
	{
		switch(CBattleHex::mutualPosition(curStackPos, destHex)) //reverse unit if necessary
		{
		case 0:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos, true);
			break;
		case 1:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos, true);
			break;
		case 2:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos, true);
			break;
		case 3:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos, true);
			break;
		case 4:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos, true);
			break;
		case 5:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos, true);
			break;
		}
		//moving instructions
		creAnims[number]->setType(0);
		for(int i=0; i<steps; ++i)
		{
			switch(CBattleHex::mutualPosition(curStackPos, destHex))
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
		}
		if( (LOCPLINT->cb->battleGetStackByID(number).owner == attackingHeroInstance->tempOwner ) != creDir[number])
		{
			reverseCreature(number, curStackPos, true);
		}

	}
	else //normal move instructions
	{
		switch(CBattleHex::mutualPosition(curStackPos, destHex)) //reverse unit if necessary
		{
		case 0:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos);
			break;
		case 1:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos);
			break;
		case 2:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos);
			break;
		case 3:
			if(creDir[number] == false)
				reverseCreature(number, curStackPos);
			break;
		case 4:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos);
			break;
		case 5:
			if(creDir[number] == true)
				reverseCreature(number, curStackPos);
			break;
		}
		//moving instructions
		creAnims[number]->setType(0);
		for(int i=0; i<steps; ++i)
		{
			switch(CBattleHex::mutualPosition(curStackPos, destHex))
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
		}
	}

	if(endMoving) //animation of ending move
	{
		for(int i=0; i<creAnims[number]->framesInGroup(21); ++i)
		{
			show();
			CSDL_Ext::update();
			SDL_framerateDelay(LOCPLINT->mainFPSmng);
		}
	}

	creAnims[number]->setType(2); //resetting to default
	CStack curs = LOCPLINT->cb->battleGetStackByID(number);
	if(endMoving) //resetting to default
	{
		if(creDir[number] != (curs.owner == attackingHeroInstance->tempOwner))
			reverseCreature(number, destHex);
		//creDir[number] = (curs.owner == attackingHeroInstance->tempOwner);
	}
	
	std::pair <int, int> coords = CBattleHex::getXYUnitAnim(destHex, creDir[number], curs.creature);
	creAnims[number]->pos.x = coords.first;
	creAnims[number]->pos.y = coords.second;
}

void CBattleInterface::stackIsAttacked(int ID, int dmg, int killed, int IDby)
{
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
	CStack aStack = LOCPLINT->cb->battleGetStackByID(ID); //attacking stack
	if(aStack.creature->isDoubleWide())
	{
		switch(CBattleHex::mutualPosition(aStack.position, dest)) //attack direction
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
		switch(CBattleHex::mutualPosition(aStack.position, dest)) //attack direction
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

	if(aStack.creature->isDoubleWide())
	{
		switch(CBattleHex::mutualPosition(aStack.position, dest)) //attack direction
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
	else //else for if(aStack.creature->isDoubleWide())
	{
		switch(CBattleHex::mutualPosition(aStack.position, dest)) //attack direction
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
}

void CBattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);
}

void CBattleInterface::hexLclicked(int whichOne)
{
	if((whichOne%17)!=0 && (whichOne%17)!=16) //if player is trying to attack enemey unit or move creature stack
	{
		int atCre = LOCPLINT->cb->battleGetStack(whichOne); //creature at destination tile; -1 if there is no one
		//LOCPLINT->cb->battleGetCreature();
		if(atCre==-1) //normal move action
		{
			BattleAction * ba = new BattleAction(); //to be deleted by engine
			ba->actionType = 2;
			ba->destinationTile = whichOne;
			ba->stackNumber = activeStack;
			givenCommand = ba;
		}
		else if(LOCPLINT->cb->battleGetStackByID(atCre).owner != attackingHeroInstance->tempOwner) //attacking
		{
			BattleAction * ba = new BattleAction(); //to be deleted by engine
			ba->actionType = 6;
			ba->destinationTile = whichOne;
			ba->stackNumber = activeStack;
			givenCommand = ba;
		}
	}
}

void CBattleInterface::showRange(SDL_Surface * to, int ID)
{
	std::vector<int> shadedHexes = LOCPLINT->cb->battleGetAvailableHexes(ID);
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
			CStack aStack = LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(aStack.creature->isDoubleWide())
			{
				switch(CBattleHex::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
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
				switch(CBattleHex::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
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
		else if(attackingInfo->frame == (attackingInfo->maxframe - 1))
		{
			attackingInfo->reversing = true;
			CStack aStack = LOCPLINT->cb->battleGetStackByID(attackingInfo->ID); //attacking stack
			if(aStack.creature->isDoubleWide())
			{
				switch(CBattleHex::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
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
				switch(CBattleHex::mutualPosition(aStack.position, attackingInfo->dest)) //attack direction
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
	CStack attacker = LOCPLINT->cb->battleGetStackByID(IDby);
	CStack defender = LOCPLINT->cb->battleGetStackByID(ID);
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
	dh = CGI->spriteh->giveDef( defName );
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
		flag = CGI->spriteh->giveDef("CMFLAGR.DEF");
	else
		flag = CGI->spriteh->giveDef("CMFLAGL.DEF");

	//coloring flag and adding transparency
	for(int i=0; i<flag->ourImages.size(); ++i)
	{
		flag->ourImages[i].bitmap = CSDL_Ext::alphaTransform(flag->ourImages[i].bitmap);
		CSDL_Ext::blueToPlayersAdv(flag->ourImages[i].bitmap, player);
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

signed char CBattleHex::mutualPosition(int hex1, int hex2)
{
	if(hex2 == hex1 - ( (hex1/17)%2 ? 18 : 17 )) //top left
		return 0;
	if(hex2 == hex1 - ( (hex1/17)%2 ? 17 : 16 )) //top right
		return 1;
	if(hex2 == hex1 - 1 && hex1%17 != 0) //left
		return 5;
	if(hex2 == hex1 + 1 && hex1%17 != 16) //right
		return 2;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 16 : 17 )) //bottom left
		return 4;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 17 : 18 )) //bottom right
		return 3;
	return -1;
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
		}
	}

	if(hovered && strictHovered) //print attacked creature to console
	{
		if(myInterface->console->alterTxt.size() == 0 && LOCPLINT->cb->battleGetStack(myNumber) != -1 &&
			LOCPLINT->cb->battleGetStackByPos(myNumber).owner != LOCPLINT->playerID &&
			LOCPLINT->cb->battleGetStackByPos(myNumber).alive)
		{
			char tabh[160];
			CStack attackedStack = LOCPLINT->cb->battleGetStackByPos(myNumber);
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
		CStack myst = LOCPLINT->cb->battleGetStackByID(stID); //stack info
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
			lastShown++;
		}
	}

	texts.push_back( text.substr(firstInToken, text.size()) );
	lastShown++;
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
