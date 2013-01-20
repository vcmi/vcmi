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
#include "../../lib/CConfigHandler.h"
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
#include "../CMessage.h"

void CBattleConsole::showAll(SDL_Surface * to)
{
	Point textPos(pos.x + pos.w/2, pos.y + 17);

	if(ingcAlter.size())
	{
		graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(ingcAlter, pos.w, FONT_SMALL), Colors::WHITE, textPos);
	}
	else if(alterTxt.size())
	{
		graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(alterTxt, pos.w, FONT_SMALL), Colors::WHITE, textPos);
	}
	else if(texts.size())
	{
		if(texts.size()==1)
		{
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(texts[0], pos.w, FONT_SMALL), Colors::WHITE, textPos);
		}
		else
		{
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(texts[lastShown - 1], pos.w, FONT_SMALL), Colors::WHITE, textPos);
			textPos.y += 16;
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(texts[lastShown], pos.w, FONT_SMALL), Colors::WHITE, textPos);
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

void CBattleConsole::alterText(const std::string &text)
{
	//char buf[500];
	//sprintf(buf, text.c_str());
	//alterTxt = buf;
	alterTxt = text;
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

CBattleConsole::CBattleConsole() : lastShown(-1), alterTxt(""), whoSetAlter(0)
{}

void CBattleHero::show(SDL_Surface * to)
{
	//animation of flag
	SDL_Rect temp_rect;
	if(flip)
	{
		temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 61,
			pos.y + 39);

	}
	else
	{
		temp_rect = genRect(
			flag->ourImages[flagAnim].bitmap->h,
			flag->ourImages[flagAnim].bitmap->w,
			pos.x + 72,
			pos.y + 39);
	}
	CSDL_Ext::blit8bppAlphaTo24bpp(
		flag->ourImages[flagAnim].bitmap,
		NULL,
		screen,
		&temp_rect);

	//animation of hero
	SDL_Rect rect = pos;
	CSDL_Ext::blit8bppAlphaTo24bpp(dh->ourImages[currentFrame].bitmap, NULL, to, &rect);

	if ( ++animCount == 4 )
	{
		animCount = 0;
		if ( ++flagAnim >= flag->ourImages.size())
			flagAnim = 0;

		if ( ++currentFrame >= lastFrame)
			switchToNextPhase();
	}
}

void CBattleHero::setPhase(int newPhase)
{
	nextPhase = newPhase;
	switchToNextPhase(); //immediately switch to next phase and then restore idling phase
	nextPhase = 0;
}

void CBattleHero::clickLeft(tribool down, bool previousState)
{
	if(myOwner->spellDestSelectMode) //we are casting a spell
		return;

	if(!down && myHero != NULL && myOwner->myTurn && myOwner->curInt->cb->battleCanCastSpell()) //check conditions
	{
		for(int it=0; it<GameConstants::BFIELD_SIZE; ++it) //do nothing when any hex is hovered - hero's animation overlaps battlefield
		{
			if(myOwner->bfield[it]->hovered && myOwner->bfield[it]->strictHovered)
				return;
		}
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);

		CSpellWindow * spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), myHero, myOwner->curInt);
		GH.pushInt(spellWindow);
	}
}

void CBattleHero::switchToNextPhase()
{
	if (phase != nextPhase)
	{
		phase = nextPhase;

		//find first and last frames of our animation
		for (firstFrame = 0;
		     firstFrame < dh->ourImages.size() && dh->ourImages[firstFrame].groupNumber != phase;
		     firstFrame++);

		for (lastFrame = firstFrame;
			 lastFrame < dh->ourImages.size() && dh->ourImages[lastFrame].groupNumber == phase;
			 lastFrame++);
	}

	currentFrame = firstFrame;
}

CBattleHero::CBattleHero(const std::string & defName, bool flipG, ui8 player, const CGHeroInstance * hero, const CBattleInterface * owner):
    flip(flipG),
    myHero(hero),
    myOwner(owner),
    phase(1),
    nextPhase(0),
    flagAnim(0),
    animCount(0)
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
	addUsedEvents(LCLICK);

	switchToNextPhase();
}

CBattleHero::~CBattleHero()
{
	delete dh;
	delete flag;
}

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner)
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
	labels.push_back(new CLabel(242,  32, FONT_BIG,    CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(new CLabel(122, 214, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(new CLabel(122, 293, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(new CLabel(122, 359, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(new CLabel(353,  66, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(new CLabel(353, 265, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(new CLabel(283,  86, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(new CLabel(283, 116, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(new CLabel(283, 146, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(new CLabel(283, 176, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(new CLabel(283, 206, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(new CLabel(283, 285, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(new CLabel(283, 315, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[403]));//spells only

	//general options
	labels.push_back(new CLabel(61,  57, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[404]));
	labels.push_back(new CLabel(61,  90, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[405]));
	labels.push_back(new CLabel(61, 123, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[406]));
	labels.push_back(new CLabel(61, 156, FONT_MEDIUM, TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[407]));
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
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	this->pos = pos;
	CPicture * bg = new CPicture("CPRESULT");
	bg->colorize(owner->curInt->playerID);

	exit = new CAdventureMapButton ("", "", boost::bind(&CBattleResultWindow::bExitf,this), 384, 505, "iok6432.def", SDLK_RETURN);
	exit->borderColor = Colors::METALLIC_GOLD;
	exit->borderEnabled = true;

	if(br.winner==0) //attacker won
	{
		new CLabel( 59, 124, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[410]);
		new CLabel(408, 124, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[411]);
	}
	else //if(br.winner==1)
	{
		new CLabel( 59, 124, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[411]);
		new CLabel(412, 124, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[410]);
	}

	new CLabel(232, 302, FONT_BIG, CENTER, Colors::YELLOW,  CGI->generaltexth->allTexts[407]);
	new CLabel(232, 332, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[408]);
	new CLabel(232, 428, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[409]);

	std::string attackerName, defenderName;

	if(owner->attackingHeroInstance) //a hero attacked
	{
		new CAnimImage("PortraitsLarge", owner->attackingHeroInstance->portrait, 0, 21, 38);
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
		new CAnimImage("TWCRPORT", bestMonsterID+2, 0, 21, 38);
		//setting attackerName
		attackerName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}
	if(owner->defendingHeroInstance) //a hero defended
	{
		new CAnimImage("PortraitsLarge", owner->defendingHeroInstance->portrait, 0, 392, 38);
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
		new CAnimImage("TWCRPORT", CGI->creh->creatures[bestMonsterID]->iconIndex, 0, 392, 38);
		//setting defenderName
		defenderName =  CGI->creh->creatures[bestMonsterID]->namePl;
	}

	//printing attacker and defender's names
	new CLabel( 89, 37, FONT_SMALL, TOPLEFT, Colors::WHITE, attackerName);

	new CLabel( 381, 53, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, defenderName);

	//printing casualities
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			new CLabel( 235, 360 + 97*step, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[523]);
		}
		else
		{
			int xPos = 235 - (br.casualties[step].size()*32 + (br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + step*97;
			for(std::map<ui32,si32>::const_iterator it=br.casualties[step].begin(); it!=br.casualties[step].end(); ++it)
			{
				new CAnimImage("CPRSMALL", CGI->creh->creatures[it->first]->iconIndex, 0, xPos, yPos);
				std::ostringstream amount;
				amount<<it->second;
				new CLabel( xPos+16, yPos + 42, FONT_SMALL, CENTER, Colors::WHITE, amount.str());
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

		CCS->musich->playMusic("Music/Win Battle", false);
		CCS->videoh->open("WIN3.BIK");
		std::string str = CGI->generaltexth->allTexts[text];

		const CGHeroInstance * ourHero = weAreAttacker? owner->attackingHeroInstance : owner->defendingHeroInstance;
		if (ourHero)
		{
			str += CGI->generaltexth->allTexts[305];
			boost::algorithm::replace_first(str,"%s",ourHero->name);
			boost::algorithm::replace_first(str,"%d",boost::lexical_cast<std::string>(br.exp[weAreAttacker?0:1]));
		}
		
		new CTextBox(str, Rect(69, 203, 330, 68), 0, FONT_SMALL, CENTER, Colors::WHITE);
	}
	else // we lose
	{
		switch(br.result)
		{
		case 0: //normal victory
			{
				CCS->musich->playMusic("Music/LoseCombat", false);
				CCS->videoh->open("LBSTART.BIK");
				new CLabel(235, 235, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[311]);
				break;
			}
		case 1: //flee
			{
				CCS->musich->playMusic("Music/Retreat Battle", false);
				CCS->videoh->open("RTSTART.BIK");
				new CLabel(235, 235, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[310]);
				break;
			}
		case 2: //surrender
			{
				CCS->musich->playMusic("Music/Surrender Battle", false);
				CCS->videoh->open("SURRENDER.BIK");
				new CLabel(235, 235, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[309]);
				break;
			}
		}
	}
}

CBattleResultWindow::~CBattleResultWindow()
{
}

void CBattleResultWindow::activate()
{
	owner->curInt->showingDialog->set(true);
	CIntObject::activate();
}

void CBattleResultWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	CCS->videoh->update(pos.x + 107, pos.y + 70, screen, true, false);
}

void CBattleResultWindow::bExitf()
{
	if(LOCPLINT->cb->getStartInfo()->mode == StartInfo::DUEL)
	{
		CGuiHandler::pushSDLEvent(SDL_QUIT);
		return;
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
			ret = cbi->siegeH->town->town->clientInfo.siegePositions[18];
			break;
		case -3: //lower turret
			ret = cbi->siegeH->town->town->clientInfo.siegePositions[19];
			break;
		case -4: //upper turret
			ret = cbi->siegeH->town->town->clientInfo.siegePositions[20];
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
	addUsedEvents(LCLICK | RCLICK | HOVER | MOVE);
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
	owner->curInt->cb->battleGetStackQueue(stacksSorted, stackBoxes.size());
	if(stacksSorted.size())
	{
		for (int i = 0; i < stackBoxes.size() ; i++)
		{
			stackBoxes[i]->setStack(stacksSorted[i]);
		}
	}
	else
	{
		//no stacks on battlefield... what to do with queue?
	}
}

CStackQueue::CStackQueue(bool Embedded, CBattleInterface * _owner)
:embedded(Embedded), owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if(embedded)
	{
		bg = NULL;
		pos.w = QUEUE_SIZE * 37;
		pos.h = 46;
		pos.x = screen->w/2 - pos.w/2;
		pos.y = (screen->h - 600)/2 + 10;
	}
	else
	{
		bg = BitmapHandler::loadBitmap("DIBOXBCK");
		pos.w = 800;
		pos.h = 85;
	}

	stackBoxes.resize(QUEUE_SIZE);
	for (int i = 0; i < stackBoxes.size(); i++)
	{
		stackBoxes[i] = new StackBox(embedded);
		stackBoxes[i]->moveBy(Point(1 + (embedded ? 36 : 80)*i, 0));
	}
}

CStackQueue::~CStackQueue()
{
	SDL_FreeSurface(bg);
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
		SDL_SetClipRect(to, &pos);
		CSDL_Ext::fillTexture(to, bg);
		SDL_SetClipRect(to, nullptr);
	}
}

void CStackQueue::StackBox::showAll(SDL_Surface * to)
{
	assert(stack);
	bg->colorize(stack->owner);
	CIntObject::showAll(to);

	if(small)
		printAtMiddleLoc(makeNumberShort(stack->count), pos.w/2, pos.h - 7, FONT_SMALL, Colors::WHITE, to);
	else
		printAtMiddleLoc(makeNumberShort(stack->count), pos.w/2, pos.h - 8, FONT_MEDIUM, Colors::WHITE, to);
}

void CStackQueue::StackBox::setStack( const CStack *stack )
{
	this->stack = stack;
	assert(stack);
	icon->setFrame(stack->getCreature()->iconIndex);
}

CStackQueue::StackBox::StackBox(bool small):
    stack(nullptr),
    small(small)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture(small ? "StackQueueBgSmall" : "StackQueueBgBig" );

	if (small)
	{
		icon = new CAnimImage("CPRSMALL", 0, 0, 5, 2);
	}
	else
		icon = new CAnimImage("TWCRPORT", 0, 0, 9, 1);

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
}
