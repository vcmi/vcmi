/*
 * CBattleInterfaceClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInterfaceClasses.h"

#include "CBattleInterface.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CSpellWindow.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/NetPacks.h"
#include "../../lib/StartInfo.h"
#include "../../lib/CondSh.h"
#include "../../lib/mapObjects/CGTownInstance.h"

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
	logGlobal->trace("CBattleConsole message: %s", text);
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
	IImage * flagFrame = flagAnimation->getImage(flagAnim, 0, true);

	if(!flagFrame)
		return;

	//animation of flag
	SDL_Rect temp_rect;
	if(flip)
	{
		temp_rect = genRect(
			flagFrame->height(),
			flagFrame->width(),
			pos.x + 61,
			pos.y + 39);

	}
	else
	{
		temp_rect = genRect(
			flagFrame->height(),
			flagFrame->width(),
			pos.x + 72,
			pos.y + 39);
	}

	flagFrame->draw(screen, &temp_rect, nullptr); //FIXME: why screen?

	//animation of hero
	SDL_Rect rect = pos;

	IImage * heroFrame = animation->getImage(currentFrame, phase, true);
	if(!heroFrame)
		return;

	heroFrame->draw(to, &rect, nullptr);

	if(++animCount >= 4)
	{
		animCount = 0;
		if(++flagAnim >= flagAnimation->size(0))
			flagAnim = 0;

		if(++currentFrame >= lastFrame)
			switchToNextPhase();
	}
}

void CBattleHero::setPhase(int newPhase)
{
	nextPhase = newPhase;
	switchToNextPhase(); //immediately switch to next phase and then restore idling phase
	nextPhase = 0;
}

void CBattleHero::hover(bool on)
{
	//TODO: Make lines below work properly
	if (on)
		CCS->curh->changeGraphic(ECursor::COMBAT, 5);
	else
		CCS->curh->changeGraphic(ECursor::COMBAT, 0);
}

void CBattleHero::clickLeft(tribool down, bool previousState)
{
	if(myOwner->spellDestSelectMode) //we are casting a spell
		return;

	if(boost::logic::indeterminate(down))
		return;

	if(!myHero || down || !myOwner->myTurn)
		return;

	if(myOwner->getCurrentPlayerInterface()->cb->battleCanCastSpell(myHero, spells::Mode::HERO) == ESpellCastProblem::OK) //check conditions
	{
		for(int it=0; it<GameConstants::BFIELD_SIZE; ++it) //do nothing when any hex is hovered - hero's animation overlaps battlefield
		{
			if(myOwner->bfield[it]->hovered && myOwner->bfield[it]->strictHovered)
				return;
		}
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);

		GH.pushInt(new CSpellWindow(myHero, myOwner->getCurrentPlayerInterface()));
	}
}

void CBattleHero::clickRight(tribool down, bool previousState)
{
	if(boost::logic::indeterminate(down))
		return;

	Point windowPosition;
	windowPosition.x = (!flip) ? myOwner->pos.topLeft().x + 1 : myOwner->pos.topRight().x - 79;
	windowPosition.y = myOwner->pos.y + 135;

	InfoAboutHero targetHero;
	if(down && (myOwner->myTurn || settings["session"]["spectate"].Bool()))
	{
		auto h = flip ? myOwner->defendingHeroInstance : myOwner->attackingHeroInstance;
		targetHero.initFromHero(h, InfoAboutHero::EInfoLevel::INBATTLE);
		GH.pushInt(new CHeroInfoWindow(targetHero, &windowPosition));
	}
}

void CBattleHero::switchToNextPhase()
{
	if(phase != nextPhase)
	{
		phase = nextPhase;

		firstFrame = 0;

		lastFrame = animation->size(phase);
	}

	currentFrame = firstFrame;
}

CBattleHero::CBattleHero(const std::string & animationPath, bool flipG, PlayerColor player, const CGHeroInstance * hero, const CBattleInterface * owner):
    flip(flipG),
    myHero(hero),
    myOwner(owner),
    phase(1),
    nextPhase(0),
    flagAnim(0),
    animCount(0)
{
	animation = std::make_shared<CAnimation>(animationPath);
	animation->preload();
	if(flipG)
		animation->verticalFlip();

	if(flip)
		flagAnimation = std::make_shared<CAnimation>("CMFLAGR");
	else
		flagAnimation = std::make_shared<CAnimation>("CMFLAGL");

	flagAnimation->preload();
	flagAnimation->playerColored(player);

	addUsedEvents(LCLICK | RCLICK | HOVER);

	switchToNextPhase();
}

CBattleHero::~CBattleHero() = default;

CBattleOptionsWindow::CBattleOptionsWindow(const SDL_Rect & position, CBattleInterface *owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos = position;
	background = new CPicture("comopbck.bmp");
	background->colorize(owner->getCurrentPlayerInterface()->playerID);

	viewGrid = new CToggleButton(Point(25, 56), "sysopchk.def", CGI->generaltexth->zelp[427], [=](bool on){owner->setPrintCellBorders(on);} );
	viewGrid->setSelected(settings["battle"]["cellBorders"].Bool());
	movementShadow = new CToggleButton(Point(25, 89), "sysopchk.def", CGI->generaltexth->zelp[428], [=](bool on){owner->setPrintStackRange(on);});
	movementShadow->setSelected(settings["battle"]["stackRange"].Bool());
	mouseShadow = new CToggleButton(Point(25, 122), "sysopchk.def", CGI->generaltexth->zelp[429], [=](bool on){owner->setPrintMouseShadow(on);});
	mouseShadow->setSelected(settings["battle"]["mouseShadow"].Bool());

	animSpeeds = new CToggleGroup([=](int value){ owner->setAnimSpeed(value);});
	animSpeeds->addToggle(40,  new CToggleButton(Point( 28, 225), "sysopb9.def", CGI->generaltexth->zelp[422]));
	animSpeeds->addToggle(63,  new CToggleButton(Point( 92, 225), "sysob10.def", CGI->generaltexth->zelp[423]));
	animSpeeds->addToggle(100, new CToggleButton(Point(156, 225), "sysob11.def", CGI->generaltexth->zelp[424]));
	animSpeeds->setSelected(owner->getAnimSpeed());

	setToDefault = new CButton (Point(246, 359), "codefaul.def", CGI->generaltexth->zelp[393], [&](){ bDefaultf(); });
	setToDefault->setImageOrder(1, 0, 2, 3);
	exit = new CButton (Point(357, 359), "soretrn.def", CGI->generaltexth->zelp[392], [&](){ bExitf();}, SDLK_RETURN);
	exit->setImageOrder(1, 0, 2, 3);

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
	//TODO: implement
}

void CBattleOptionsWindow::bExitf()
{
	GH.popIntTotally(this);
}

CBattleResultWindow::CBattleResultWindow(const BattleResult &br, const SDL_Rect & pos, CPlayerInterface &_owner)
: owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	this->pos = pos;
	CPicture * bg = new CPicture("CPRESULT");
	bg->colorize(owner.playerID);

	exit = new CButton (Point(384, 505), "iok6432.def", std::make_pair("", ""), [&](){ bExitf();}, SDLK_RETURN);
	exit->borderColor = boost::make_optional(Colors::METALLIC_GOLD);

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

	std::string sideNames[2] = {"N/A", "N/A"};

	for(int i = 0; i < 2; i++)
	{
		auto heroInfo = owner.cb->battleGetHeroInfo(i);
		const int xs[] = {21, 392};

		if(heroInfo.portrait >= 0) //attacking hero
		{
			new CAnimImage("PortraitsLarge", heroInfo.portrait, 0, xs[i], 38);
			sideNames[i] = heroInfo.name;
		}
		else
		{
			auto stacks = owner.cb->battleGetAllStacks();
			vstd::erase_if(stacks, [i](const CStack *stack) //erase stack of other side and not coming from garrison
				{ return stack->side != i  ||  !stack->base; });

			auto best = vstd::maxElementByFun(stacks, [](const CStack *stack){ return stack->type->AIValue; });
			if(best != stacks.end()) //should be always but to be safe...
			{
				new CAnimImage("TWCRPORT", (*best)->type->idNumber+2, 0, xs[i], 38);
				sideNames[i] = CGI->creh->creatures[(*best)->type->idNumber]->namePl;
			}
		}
	}

	//printing attacker and defender's names
	new CLabel( 89, 37, FONT_SMALL, TOPLEFT, Colors::WHITE, sideNames[0]);
	new CLabel( 381, 53, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, sideNames[1]);

	//printing casualties
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
			for(auto & elem : br.casualties[step])
			{
				new CAnimImage("CPRSMALL", CGI->creh->creatures[elem.first]->iconIndex, 0, xPos, yPos);
				std::ostringstream amount;
				amount<<elem.second;
				new CLabel( xPos+16, yPos + 42, FONT_SMALL, CENTER, Colors::WHITE, amount.str());
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = !(owner.cb->battleGetMySide());
	if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
	{
		int text=-1;
		switch(br.result)
		{
		case BattleResult::NORMAL: text = 304; break;
		case BattleResult::ESCAPE: text = 303; break;
		case BattleResult::SURRENDER: text = 302; break;
		}

		CCS->musich->playMusic("Music/Win Battle", false);
		CCS->videoh->open("WIN3.BIK");
		std::string str = CGI->generaltexth->allTexts[text];

		const CGHeroInstance * ourHero = owner.cb->battleGetMyHero();
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
		case BattleResult::NORMAL:
			{
				CCS->musich->playMusic("Music/LoseCombat", false);
				CCS->videoh->open("LBSTART.BIK");
				new CLabel(235, 235, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[311]);
				break;
			}
		case BattleResult::ESCAPE: //flee
			{
				CCS->musich->playMusic("Music/Retreat Battle", false);
				CCS->videoh->open("RTSTART.BIK");
				new CLabel(235, 235, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[310]);
				break;
			}
		case BattleResult::SURRENDER:
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
	owner.showingDialog->set(true);
	CIntObject::activate();
}

void CBattleResultWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	CCS->videoh->update(pos.x + 107, pos.y + 70, screen, true, false);
}

void CBattleResultWindow::bExitf()
{
	CPlayerInterface &intTmp = owner; //copy reference because "this" will be destructed soon
	GH.popIntTotally(this);
	if(dynamic_cast<CBattleInterface*>(GH.topInt()))
		GH.popInts(1); //pop battle interface if present

	//Result window and battle interface are gone. We requested all dialogs to be closed before opening the battle,
	//so we can be sure that there is no dialogs left on GUI stack.
	intTmp.showingDialog->setn(false);
	CCS->videoh->close();
}

Point CClickableHex::getXYUnitAnim(BattleHex hexNum, const CStack * stack, CBattleInterface * cbi)
{
	assert(cbi);

	Point ret(-500, -500); //returned value
	if(stack && stack->initialPosition < 0) //creatures in turrets
	{
		switch(stack->initialPosition)
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
		static const Point basePos(-190, -139); // position of creature in topleft corner
		static const int imageShiftX = 30; // X offset to base pos for facing right stacks, negative for facing left

		ret.x = basePos.x + 22 * ( (hexNum.getY() + 1)%2 ) + 44 * hexNum.getX();
		ret.y = basePos.y + 42 * hexNum.getY();

		if (stack)
		{
			if(cbi->creDir[stack->ID])
				ret.x += imageShiftX;
			else
				ret.x -= imageShiftX;

			//shifting position for double - hex creatures
			if(stack->doubleWide())
			{
				if(stack->side == BattleSide::ATTACKER)
				{
					if(cbi->creDir[stack->ID])
						ret.x -= 44;
				}
				else
				{
					if(!cbi->creDir[stack->ID])
						ret.x += 44;
				}
			}
		}
	}
	//returning
	return ret + CPlayerInterface::battleInt->pos;
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

CClickableHex::CClickableHex() : setAlterText(false), myNumber(-1), accessible(true), hovered(false), strictHovered(false), myInterface(nullptr)
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
		const CStack * attackedStack = myInterface->getCurrentPlayerInterface()->cb->battleGetStackByPos(myNumber);
		if(myInterface->console->alterTxt.size() == 0 &&attackedStack != nullptr &&
			attackedStack->owner != myInterface->getCurrentPlayerInterface()->playerID &&
			attackedStack->alive())
		{
			MetaString text;
			text.addTxt(MetaString::GENERAL_TXT, 220);
			attackedStack->addNameReplacement(text);
			myInterface->console->alterTxt = text.toString();
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
	const CStack * myst = myInterface->getCurrentPlayerInterface()->cb->battleGetStackByPos(myNumber); //stack info
	if(hovered && strictHovered && myst!=nullptr)
	{

		if(!myst->alive()) return;
		if(down)
		{
			GH.pushInt(new CStackWindow(myst, true));
		}
	}
}

CHeroInfoWindow::CHeroInfoWindow(const InfoAboutHero &hero, Point *position) : CWindowObject(RCLICK_POPUP | SHADOW_DISABLED, "CHRPOP")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (position != nullptr)
		moveTo(*position);
	background->colorize(hero.owner); //maybe add this functionality to base class?

	int attack = hero.details->primskills[0];
	int defense = hero.details->primskills[1];
	int power = hero.details->primskills[2];
	int knowledge = hero.details->primskills[3];
	int morale = hero.details->morale;
	int luck = hero.details->luck;
	int currentSpellPoints = hero.details->mana;
	int maxSpellPoints = hero.details->manaLimit;

	new CAnimImage("PortraitsLarge", hero.portrait, 0, 10, 6);

	//primary stats
	new CLabel(9, 75, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[380] + ":");
	new CLabel(9, 87, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[381] + ":");
	new CLabel(9, 99, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[382] + ":");
	new CLabel(9, 111, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[383] + ":");

	new CLabel(69, 87, EFonts::FONT_TINY, EAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(attack));
	new CLabel(69, 99, EFonts::FONT_TINY, EAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(defense));
	new CLabel(69, 111, EFonts::FONT_TINY, EAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(power));
	new CLabel(69, 123, EFonts::FONT_TINY, EAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(knowledge));

	//morale+luck
	new CLabel(9, 131, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[384] + ":");
	new CLabel(9, 143, EFonts::FONT_TINY, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[385] + ":");

	new CAnimImage("IMRL22", morale + 3, 0, 47, 131);
	new CAnimImage("ILCK22", luck + 3, 0, 47, 143);

	//spell points
	new CLabel(39, 174, EFonts::FONT_TINY, EAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[387]);
	new CLabel(39, 186, EFonts::FONT_TINY, EAlignment::CENTER, Colors::WHITE, std::to_string(currentSpellPoints) + "/" + std::to_string(maxSpellPoints));
}

CStackQueue::CStackQueue(bool Embedded, CBattleInterface * _owner)
	: embedded(Embedded),
	owner(_owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if(embedded)
	{
		pos.w = QUEUE_SIZE * 37;
		pos.h = 46;
		pos.x = screen->w/2 - pos.w/2;
		pos.y = (screen->h - 600)/2 + 10;

		icons = std::make_shared<CAnimation>("CPRSMALL");
		stateIcons = std::make_shared<CAnimation>("VCMI/BATTLEQUEUE/STATESSMALL");
	}
	else
	{
		pos.w = 800;
		pos.h = 85;

		new CFilledTexture("DIBOXBCK", Rect(0,0, pos.w, pos.h));

		icons = std::make_shared<CAnimation>("TWCRPORT");
		stateIcons = std::make_shared<CAnimation>("VCMI/BATTLEQUEUE/STATESSMALL");
		//TODO: where use big icons?
		//stateIcons = std::make_shared<CAnimation>("VCMI/BATTLEQUEUE/STATESBIG");
	}
	stateIcons->preload();

	stackBoxes.resize(QUEUE_SIZE);
	for (int i = 0; i < stackBoxes.size(); i++)
	{
		stackBoxes[i] = new StackBox(this);
		stackBoxes[i]->moveBy(Point(1 + (embedded ? 36 : 80)*i, 0));
	}
}

CStackQueue::~CStackQueue()
{
}

void CStackQueue::update()
{
	std::vector<battle::Units> queueData;

	owner->getCurrentPlayerInterface()->cb->battleGetTurnOrder(queueData, stackBoxes.size(), 0);

	size_t boxIndex = 0;

	for(size_t turn = 0; turn < queueData.size() && boxIndex < stackBoxes.size(); turn++)
	{
		for(size_t unitIndex = 0; unitIndex < queueData[turn].size() && boxIndex < stackBoxes.size(); boxIndex++, unitIndex++)
			stackBoxes[boxIndex]->setStack(queueData[turn][unitIndex], turn);
	}

	for(; boxIndex < stackBoxes.size(); boxIndex++)
		stackBoxes[boxIndex]->setStack(nullptr);
}

CStackQueue::StackBox::StackBox(CStackQueue * owner)
	: bg(nullptr),
	icon(nullptr),
	amount(nullptr),
	stateIcon(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture(owner->embedded ? "StackQueueSmall" : "StackQueueLarge" );

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;

	if(owner->embedded)
	{
		icon = new CAnimImage(owner->icons, 0, 0, 5, 2);
		amount = new CLabel(pos.w/2, pos.h - 7, FONT_SMALL, CENTER, Colors::WHITE);
	}
	else
	{
		icon = new CAnimImage(owner->icons, 0, 0, 9, 1);
		amount = new CLabel(pos.w/2, pos.h - 8, FONT_MEDIUM, CENTER, Colors::WHITE);

		int icon_x = pos.w - 17;
		int icon_y = pos.h - 18;

		stateIcon = new CAnimImage(owner->stateIcons, 0, 0, icon_x, icon_y);
		stateIcon->visible = false;
	}
}

void CStackQueue::StackBox::setStack(const battle::Unit * nStack, size_t turn)
{
	if(nStack)
	{
		bg->colorize(nStack->unitOwner());
		icon->visible = true;
		icon->setFrame(nStack->creatureIconIndex());
		amount->setText(makeNumberShort(nStack->getCount()));

		if(stateIcon)
		{
			if(nStack->defended(turn) || (turn > 0 && nStack->defended(turn - 1)))
			{
				stateIcon->setFrame(0, 0);
				stateIcon->visible = true;
			}
			else if(nStack->waited(turn))
			{
				stateIcon->setFrame(1, 0);
				stateIcon->visible = true;
			}
			else
			{
				stateIcon->visible = false;
			}
		}
	}
	else
	{
		bg->colorize(PlayerColor::NEUTRAL);
		icon->visible = false;
		icon->setFrame(0);
		amount->setText("");

		if(stateIcon)
			stateIcon->visible = false;
	}
}
