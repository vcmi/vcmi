#include "StdInc.h"
#include "CBattleInterfaceClasses.h"

#include "../UIFramework/SDL_Extensions.h"
#include "CBattleInterface.h"
#include "../CGameInfo.h"
#include "../CDefHandler.h"
#include "../UIFramework/CCursorHandler.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"
#include "../CSpellWindow.h"
#include "../Graphics.h"
#include "../CConfigHandler.h"
#include "../UIFramework/CGuiHandler.h"
#include "../UIFramework/CIntObjectClasses.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/BattleState.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../../lib/CTownHandler.h"
#include "../CBitmapHandler.h"
#include "../CCreatureWindow.h"

CBattleConsole::~CBattleConsole()
{
	texts.clear();
}

void CBattleConsole::show(SDL_Surface * to)
{
	if(ingcAlter.size())
	{
		CSDL_Ext::printAtMiddleWB(ingcAlter, pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, Colors::Cornsilk, to);
	}
	else if(alterTxt.size())
	{
		CSDL_Ext::printAtMiddleWB(alterTxt, pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, Colors::Cornsilk, to);
	}
	else if(texts.size())
	{
		if(texts.size()==1)
		{
			CSDL_Ext::printAtMiddleWB(texts[0], pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, Colors::Cornsilk, to);
		}
		else
		{
			CSDL_Ext::printAtMiddleWB(texts[lastShown-1], pos.x + pos.w/2, pos.y + 11, FONT_SMALL, 80, Colors::Cornsilk, to);
			CSDL_Ext::printAtMiddleWB(texts[lastShown], pos.x + pos.w/2, pos.y + 27, FONT_SMALL, 80, Colors::Cornsilk, to);
		}
	}
}

bool CBattleConsole::addText(const std::string & text)
{
	if(text.size()>70)
		return false; //text too long!
	int firstInToken = 0;
	for(size_t i = 0; i < text.size(); ++i) //tokenize
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

void CBattleConsole::eraseText(ui32 pos)
{
	if(pos < texts.size())
	{
		texts.erase(texts.begin() + pos);
		if(lastShown == texts.size())
			--lastShown;
	}
}

void CBattleConsole::changeTextAt(const std::string & text, ui32 pos)
{
	if(pos >= texts.size()) //no such pos
		return;
	texts[pos] = text;
}

void CBattleConsole::scrollUp(ui32 by)
{
	if(lastShown > static_cast<int>(by))
		lastShown -= by;
}

void CBattleConsole::scrollDown(ui32 by)
{
	if(lastShown + by < texts.size())
		lastShown += by;
}

void CBattleHero::show(SDL_Surface * to)
{
	//animation of flag
	if(flip)
	{
		SDL_Rect temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 61,
			pos.y + 39);
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&temp_rect);
	}
	else
	{
		SDL_Rect temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 72,
			pos.y + 39);
		CSDL_Ext::blit8bppAlphaTo24bpp(
			flag->ourImages[flagAnim].bitmap,
			NULL,
			screen,
			&temp_rect);
	}
	++flagAnimCount;
	if(flagAnimCount%4==0)
	{
		++flagAnim;
		flagAnim %= flag->ourImages.size();
	}
	//animation of hero
	int tick=-1;
	for(size_t i = 0; i < dh->ourImages.size(); ++i)
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

	if(!down && myHero != NULL && myOwner->myTurn && myOwner->curInt->cb->battleCanCastSpell()) //check conditions
	{
		for(int it=0; it<GameConstants::BFIELD_SIZE; ++it) //do nothing when any hex is hovered - hero's animation overlaps battlefield
		{
			if(myOwner->bfield[it].hovered && myOwner->bfield[it].strictHovered)
				return;
		}
		CCS->curh->changeGraphic(0,0);

		CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), myHero, myOwner->curInt);
		GH.pushInt(spellWindow);
	}
}

CBattleHero::CBattleHero(const std::string & defName, int phaseG, int imageG, bool flipG, ui8 player, const CGHeroInstance * hero, const CBattleInterface * owner): flip(flipG), myHero(hero), myOwner(owner), phase(phaseG), nextPhase(-1), image(imageG), flagAnim(0), flagAnimCount(0)
{
	dh = CDefHandler::giveDef( defName );
	for(size_t i = 0; i < dh->ourImages.size(); ++i) //transforming images
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
	for(size_t i = 0; i < flag->ourImages.size(); ++i)
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

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner): myInt(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos = position;
	background = new CPicture("comopbck.bmp");
	background->colorize(owner->curInt->playerID);

	viewGrid = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintCellBorders, owner, true), boost::bind(&CBattleInterface::setPrintCellBorders, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[427].first)(3,CGI->generaltexth->zelp[427].first), CGI->generaltexth->zelp[427].second, false, "sysopchk.def", NULL, 25, 56, false);
	viewGrid->select(settings["battle"]["cellBorders"].Bool());
	movementShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintStackRange, owner, true), boost::bind(&CBattleInterface::setPrintStackRange, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[428].first)(3,CGI->generaltexth->zelp[428].first), CGI->generaltexth->zelp[428].second, false, "sysopchk.def", NULL, 25, 89, false);
	movementShadow->select(settings["battle"]["stackRange"].Bool());
	mouseShadow = new CHighlightableButton(boost::bind(&CBattleInterface::setPrintMouseShadow, owner, true), boost::bind(&CBattleInterface::setPrintMouseShadow, owner, false), boost::assign::map_list_of(0,CGI->generaltexth->zelp[429].first)(3,CGI->generaltexth->zelp[429].first), CGI->generaltexth->zelp[429].second, false, "sysopchk.def", NULL, 25, 122, false);
	mouseShadow->select(settings["battle"]["mouseShadow"].Bool());

	animSpeeds = new CHighlightableButtonsGroup(0);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[422].first),CGI->generaltexth->zelp[422].second, "sysopb9.def", 28, 225, 1);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[423].first),CGI->generaltexth->zelp[423].second, "sysob10.def", 92, 225, 2);
	animSpeeds->addButton(boost::assign::map_list_of(0,CGI->generaltexth->zelp[424].first),CGI->generaltexth->zelp[424].second, "sysob11.def",156, 225, 4);
	animSpeeds->select(owner->getAnimSpeed(), 1);
	animSpeeds->onChange = boost::bind(&CBattleInterface::setAnimSpeed, owner, _1);

	setToDefault = new CAdventureMapButton (CGI->generaltexth->zelp[393], boost::bind(&CBattleOptionsWindow::bDefaultf,this), 246, 359, "codefaul.def");
	setToDefault->swappedImages = true;
	setToDefault->update();
	exit = new CAdventureMapButton (CGI->generaltexth->zelp[392], boost::bind(&CBattleOptionsWindow::bExitf,this), 357, 359, "soretrn.def",SDLK_RETURN);
	exit->swappedImages = true;
	exit->update();

	//creating labels
	labels.push_back(new CLabel(242,  32, FONT_BIG,    CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(new CLabel(122, 214, FONT_MEDIUM, CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(new CLabel(122, 293, FONT_MEDIUM, CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(new CLabel(122, 359, FONT_MEDIUM, CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(new CLabel(353,  66, FONT_MEDIUM, CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(new CLabel(353, 265, FONT_MEDIUM, CENTER, Colors::Jasmine, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(new CLabel(283,  86, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(new CLabel(283, 116, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(new CLabel(283, 146, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(new CLabel(283, 176, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(new CLabel(283, 206, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(new CLabel(283, 285, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(new CLabel(283, 315, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[403]));//spells only

	//general options
	labels.push_back(new CLabel(61,  57, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[404]));
	labels.push_back(new CLabel(61,  90, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[405]));
	labels.push_back(new CLabel(61, 123, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[406]));
	labels.push_back(new CLabel(61, 156, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk, CGI->generaltexth->allTexts[407]));
}

void CBattleOptionsWindow::bDefaultf()
{
}

void CBattleOptionsWindow::bExitf()
{
	GH.popIntTotally(this);
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
	exit = new CAdventureMapButton (std::string(), std::string(), boost::bind(&CBattleResultWindow::bExitf,this), 384 + pos.x, 505 + pos.y, "iok6432.def", SDLK_RETURN);
	exit->borderColor = Colors::MetallicGold;
	exit->borderEnabled = true;

	if(br.winner==0) //attacker won
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 59, 124, FONT_SMALL, Colors::Cornsilk, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 408, 124, FONT_SMALL, Colors::Cornsilk, background);
	}
	else //if(br.winner==1)
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[411], 59, 124, FONT_SMALL, Colors::Cornsilk, background);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[410], 412, 124, FONT_SMALL, Colors::Cornsilk, background);
	}


	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[407], 232, 302, FONT_BIG, Colors::Jasmine, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[408], 232, 332, FONT_BIG, Colors::Cornsilk, background);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[409], 237, 428, FONT_BIG, Colors::Cornsilk, background);

	std::string attackerName, defenderName;

	if(owner->attackingHeroInstance) //a hero attacked
	{
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->attackingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting attackerName
		attackerName = owner->attackingHeroInstance->name;
	}
	else //a monster attacked
	{
		int bestMonsterID = -1;
		ui32 bestPower = 0;
		for(TSlots::const_iterator it = owner->army1->Slots().begin(); it!=owner->army1->Slots().end(); ++it)
		{
			if(it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 21, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->portraitLarge[owner->defendingHeroInstance->portrait], NULL, background, &temp_rect);
		//setting defenderName
		defenderName = owner->defendingHeroInstance->name;
	}
	else //a monster defended
	{
		int bestMonsterID = -1;
		ui32 bestPower = 0;
		for(TSlots::const_iterator it = owner->army2->Slots().begin(); it!=owner->army2->Slots().end(); ++it)
		{
			if( it->second->type->AIValue > bestPower)
			{
				bestPower = it->second->type->AIValue;
				bestMonsterID = it->second->type->idNumber;
			}
		}
		SDL_Rect temp_rect = genRect(64, 58, 392, 38);
		SDL_BlitSurface(graphics->bigImgs[bestMonsterID], NULL, background, &temp_rect);
		//setting defenderName
		defenderName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}

	//printing attacker and defender's names
	CSDL_Ext::printAt(attackerName, 89, 37, FONT_SMALL, Colors::Cornsilk, background);
	CSDL_Ext::printTo(defenderName, 381, 53, FONT_SMALL, Colors::Cornsilk, background);
	//printing casualities
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[523], 235, 360 + 97*step, FONT_SMALL, Colors::Cornsilk, background);
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
				CSDL_Ext::printAtMiddle(amount.str(), xPos+16, yPos + 42, FONT_SMALL, Colors::Cornsilk, background);
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = (owner->curInt->playerID == owner->attackingHeroInstance->tempOwner);
	if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
	{
		int text=-1;
		switch(br.result)
		{
		case 0: text = 304; break;
		case 1: text = 303; break;
		case 2: text = 302; break;
		}

		CCS->musich->playMusic(musicBase::winBattle);
		CCS->videoh->open(VIDEO_WIN);
		std::string str = CGI->generaltexth->allTexts[text];

		const CGHeroInstance * ourHero = weAreAttacker? owner->attackingHeroInstance : owner->defendingHeroInstance;
		if (ourHero)
		{
			str += CGI->generaltexth->allTexts[305];
			boost::algorithm::replace_first(str,"%s",ourHero->name);
			boost::algorithm::replace_first(str,"%d",boost::lexical_cast<std::string>(br.exp[weAreAttacker?0:1]));
		}
		CSDL_Ext::printAtMiddleWB(str, 235, 235, FONT_SMALL, 55, Colors::Cornsilk, background);
	}
	else // we lose
	{
		switch(br.result)
		{
		case 0: //normal victory
			{
				CCS->musich->playMusic(musicBase::loseCombat);
				CCS->videoh->open(VIDEO_LOSE_BATTLE_START);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[311], 235, 235, FONT_SMALL, Colors::Cornsilk, background);
				break;
			}
		case 1: //flee
			{
				CCS->musich->playMusic(musicBase::retreatBattle);
				CCS->videoh->open(VIDEO_RETREAT_START);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[310], 235, 235, FONT_SMALL, Colors::Cornsilk, background);
				break;
			}
		case 2: //surrender
			{
				CCS->musich->playMusic(musicBase::surrenderBattle);
				CCS->videoh->open(VIDEO_SURRENDER);
				CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[309], 235, 220, FONT_SMALL, Colors::Cornsilk, background);
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

void CBattleResultWindow::show(SDL_Surface * to)
{
	//evaluating to
	if(!to)
		to = screen;

	CCS->videoh->update(107, 70, background, false, true);

	SDL_BlitSurface(background, NULL, to, &pos);
	exit->showAll(to);
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

Point CClickableHex::getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * stack, const CBattleInterface * cbi)
{
	Point ret(-500, -500); //returned value
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

void CStackQueue::showAll(SDL_Surface * to)
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

void CStackQueue::StackBox::showAll(SDL_Surface * to)
{
	assert(my);
	if(bg)
	{
		graphics->blueToPlayersAdv(bg, my->owner);
		//SDL_UpdateRect(bg, 0, 0, 0, 0);
		SDL_Rect temp_rect = genRect(bg->h, bg->w, pos.x, pos.y);
		CSDL_Ext::blit8bppAlphaTo24bpp(bg, NULL, to, &temp_rect);
		//blitAt(bg, pos, to);
		blitAt(graphics->bigImgs[my->getCreature()->idNumber], pos.x +9, pos.y + 1, to);
		printAtMiddleLoc(makeNumberShort(my->count), pos.w/2, pos.h - 12, FONT_MEDIUM, Colors::Cornsilk, to);
	}
	else
	{
		blitAt(graphics->smallImgs[-2], pos, to);
		blitAt(graphics->smallImgs[my->getCreature()->idNumber], pos, to);
		const SDL_Color &ownerColor = (my->owner == 255 ? *graphics->neutralColor : graphics->playerColors[my->owner]);
		CSDL_Ext::drawBorder(to, pos, int3(ownerColor.r, ownerColor.g, ownerColor.b));
		printAtMiddleLoc(makeNumberShort(my->count), pos.w/2, pos.h - 8, FONT_TINY, Colors::Cornsilk, to);
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