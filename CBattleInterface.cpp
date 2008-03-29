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
#include <queue>

extern SDL_Surface * screen;
SDL_Surface * CBattleInterface::cellBorder, * CBattleInterface::cellShade;

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, CGHeroInstance *hero1, CGHeroInstance *hero2) 
: printCellBorders(true), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0), activeStack(-1), curStackActed(false)
{
	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	std::map<int, CStack> stacks = LOCPLINT->cb->battleGetStacks();
	for(std::map<int, CStack>::iterator b=stacks.begin(); b!=stacks.end(); ++b)
	{
		creAnims[b->second.ID] = (new CCreatureAnimation(b->second.creature->animDefName));
		creAnims[b->second.ID]->setType(2);
	}
	//preparing menu background and terrain
	std::vector< std::string > & backref = CGI->mh->battleBacks[ LOCPLINT->cb->battleGetBattlefieldType() ];
	background = CGI->bitmaph->loadBitmap(backref[ rand() % backref.size()] );
	menu = CGI->bitmaph->loadBitmap("CBAR.BMP");
	CSDL_Ext::blueToPlayersAdv(menu, hero1->tempOwner);

	//blitting menu background and terrain
	blitAt(background, 0, 0);
	blitAt(menu, 0, 556);
	CSDL_Ext::update();
	
	//preparing buttons
	bOptions = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bOptionsf, 3, 561, "icm003.def", this, false, NULL, false);
	bSurrender = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bSurrenderf, 54, 561, "icm001.def", this, false, NULL, false);
	bFlee = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bFleef, 105, 561, "icm002.def", this, false, NULL, false);
	bAutofight  = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bAutofightf, 157, 561, "icm004.def", this, false, NULL, false);
	bSpell = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bSpellf, 645, 561, "icm005.def", this, false, NULL, false);
	bWait = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bWaitf, 696, 561, "icm006.def", this, false, NULL, false);
	bDefence = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bDefencef, 747, 561, "icm007.def", this, false, NULL, false);
	bConsoleUp = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bConsoleUpf, 624, 561, "ComSlide.def", this, false, NULL, false);
	bConsoleDown = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bConsoleDownf, 624, 580, "ComSlide.def", this, false, NULL, false);
	bConsoleDown->bitmapOffset = 2;
	
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
	cellBorder = CSDL_Ext::alphaTransform(cellBorder);
	cellShade = CGI->bitmaph->loadBitmap("CCELLSHD.BMP");
	cellShade = CSDL_Ext::alphaTransform(cellShade);
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
	delete bOptions;
	delete bSurrender;
	delete bFlee;
	delete bAutofight;
	delete bSpell;
	delete bWait;
	delete bDefence;
	delete bConsoleUp;
	delete bConsoleDown;

	delete attackingHero;
	delete defendingHero;

	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	for(int g=0; g<creAnims.size(); ++g)
		delete creAnims[g];
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
	showRange(to, activeStack);

	//showing menu background
	blitAt(menu, 0, 556, to);

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
	for(std::map<int, CCreatureAnimation*>::iterator j=creAnims.begin(); j!=creAnims.end(); ++j)
	{
		std::pair <int, int> coords = CBattleHex::getXYUnitAnim(stacks[j->first].position, stacks[j->first].owner == attackingHeroInstance->tempOwner);
		j->second->nextFrame(to, coords.first, coords.second, stacks[j->first].owner == attackingHeroInstance->tempOwner, animCount%2==0, j->first==activeStack);
	}
	//units shown

	CSDL_Ext::update();
}

void CBattleInterface::bOptionsf()
{
}

void CBattleInterface::bSurrenderf()
{
}

void CBattleInterface::bFleef()
{
	for(int i=0; i<LOCPLINT->objsToBlit.size(); ++i)
	{
		if( dynamic_cast<CBattleInterface*>( LOCPLINT->objsToBlit[i] ) )
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+i);
		}
	}
	deactivate();

	LOCPLINT->adventureInt->activate();
	delete this;
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
}

void CBattleInterface::bConsoleUpf()
{
}

void CBattleInterface::bConsoleDownf()
{
}

void CBattleInterface::newStack(CStack stack)
{
	creAnims[stack.ID] = new CCreatureAnimation(stack.creature->animDefName);
	creAnims[stack.ID]->setType(2);
}

void CBattleInterface::stackRemoved(CStack stack)
{
	delete creAnims[stack.ID];
	creAnims.erase(stack.ID);
}

void CBattleInterface::stackActivated(int number)
{
	curStackActed = false;
	activeStack = number;
}

void CBattleInterface::stackMoved(int number, int destHex)
{
	int curStackPos = LOCPLINT->cb->battleGetPos(number);
	for(int i=0; i<6; ++i)
	{
		//creAnims[number]->setType(0);
	}
}

void CBattleInterface::hexLclicked(int whichOne)
{
	if((whichOne%17)!=0 && (whichOne%17)!=16)
	{
		LOCPLINT->cb->battleMoveCreature(activeStack, whichOne);
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

void CBattleHero::show(SDL_Surface *to)
{
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
}

CBattleHero::CBattleHero(std::string defName, int phaseG, int imageG, bool flipG, unsigned char player): phase(phaseG), image(imageG), flip(flipG), flagAnim(0)
{
	dh = CGI->spriteh->giveDef( defName );
	for(int i=0; i<dh->ourImages.size(); ++i) //transforming images
	{
		if(flip)
			dh->ourImages[i].bitmap = CSDL_Ext::rotate01(dh->ourImages[i].bitmap); 
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

std::pair<int, int> CBattleHex::getXYUnitAnim(int hexNum, bool attacker)
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
	//returning
	return ret;
}

void CBattleHex::activate()
{
	Hoverable::activate();
	MotionInterested::activate();
	ClickableL::activate();
}

void CBattleHex::deactivate()
{
	Hoverable::deactivate();
	MotionInterested::deactivate();
	ClickableL::deactivate();
}

void CBattleHex::hover(bool on)
{
	hovered = on;
	Hoverable::hover(on);
}

CBattleHex::CBattleHex() : myNumber(-1), accesible(true), hovered(false), strictHovered(false), myInterface(NULL)
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
}

void CBattleHex::clickLeft(boost::logic::tribool down)
{
	if(!down && hovered && strictHovered) //we've been really clicked!
	{
		myInterface->hexLclicked(myNumber);
	}
}
