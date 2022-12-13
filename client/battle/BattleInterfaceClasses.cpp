/*
 * BattleInterfaceClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleInterfaceClasses.h"

#include "BattleInterface.h"
#include "BattleActionsController.h"
#include "BattleSiegeController.h"
#include "BattleFieldController.h"
#include "BattleStacksController.h"
#include "BattleControlPanel.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/Canvas.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
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

void BattleConsole::showAll(SDL_Surface * to)
{
	Point consolePos(pos.x + 10,      pos.y + 17);
	Point textPos   (pos.x + pos.w/2, pos.y + 17);

	if (!consoleText.empty())
	{
		graphics->fonts[FONT_SMALL]->renderTextLinesLeft(to, CMessage::breakText(consoleText, pos.w, FONT_SMALL), Colors::WHITE, consolePos);
	}
	else if(!hoverText.empty())
	{
		graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(hoverText, pos.w, FONT_SMALL), Colors::WHITE, textPos);
	}
	else if(logEntries.size())
	{
		if(logEntries.size()==1)
		{
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(logEntries[0], pos.w, FONT_SMALL), Colors::WHITE, textPos);
		}
		else
		{
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(logEntries[scrollPosition - 1], pos.w, FONT_SMALL), Colors::WHITE, textPos);
			textPos.y += 16;
			graphics->fonts[FONT_SMALL]->renderTextLinesCenter(to, CMessage::breakText(logEntries[scrollPosition], pos.w, FONT_SMALL), Colors::WHITE, textPos);
		}
	}
}

bool BattleConsole::addText(const std::string & text)
{
	logGlobal->trace("CBattleConsole message: %s", text);
	if(text.size()>70)
		return false; //text too long!
	int firstInToken = 0;
	for(size_t i = 0; i < text.size(); ++i) //tokenize
	{
		if(text[i] == 10)
		{
			logEntries.push_back( text.substr(firstInToken, i-firstInToken) );
			firstInToken = (int)i+1;
		}
	}

	logEntries.push_back( text.substr(firstInToken, text.size()) );
	scrollPosition = (int)logEntries.size()-1;
	return true;
}
void BattleConsole::scrollUp(ui32 by)
{
	if(scrollPosition > static_cast<int>(by))
		scrollPosition -= by;
}

void BattleConsole::scrollDown(ui32 by)
{
	if(scrollPosition + by < logEntries.size())
		scrollPosition += by;
}

BattleConsole::BattleConsole(const Rect & position)
	: scrollPosition(-1)
	, enteringText(false)
{
	pos += position.topLeft();
	pos.w = position.w;
	pos.h = position.h;
}

BattleConsole::~BattleConsole()
{
	if (enteringText)
		setEnteringMode(false);
}

void BattleConsole::setEnteringMode(bool on)
{
	consoleText.clear();

	if (on)
	{
		assert(enteringText == false);
		CSDL_Ext::startTextInput(&pos);
	}
	else
	{
		assert(enteringText == true);
		CSDL_Ext::stopTextInput();
	}
	enteringText = on;
}

void BattleConsole::setEnteredText(const std::string & text)
{
	assert(enteringText == true);
	consoleText = text;
}

void BattleConsole::write(const std::string & Text)
{
	hoverText = Text;
}

void BattleConsole::clearIfMatching(const std::string & Text)
{
	if (hoverText == Text)
		clear();
}

void BattleConsole::clear()
{
	write({});
}

void BattleHero::render(Canvas & canvas)
{
	auto flagFrame = flagAnimation->getImage(flagAnim, 0, true);

	if(!flagFrame)
		return;

	//animation of flag
	Point flagPosition = pos.topLeft();

	if(flip)
		flagPosition += Point(61, 39);
	else
		flagPosition += Point(72, 39);


	auto heroFrame = animation->getImage(currentFrame, phase, true);

	canvas.draw(flagFrame, flagPosition);
	canvas.draw(heroFrame, pos.topLeft());

	if(++animCount >= 4)
	{
		animCount = 0;
		if(++flagAnim >= flagAnimation->size(0))
			flagAnim = 0;

		if(++currentFrame >= lastFrame)
			switchToNextPhase();
	}
}

void BattleHero::setPhase(int newPhase)
{
	nextPhase = newPhase;
	switchToNextPhase(); //immediately switch to next phase and then restore idling phase
	nextPhase = 0;
}

void BattleHero::hover(bool on)
{
	//TODO: Make lines below work properly
	if (on)
		CCS->curh->changeGraphic(ECursor::COMBAT, 5);
	else
		CCS->curh->changeGraphic(ECursor::COMBAT, 0);
}

void BattleHero::clickLeft(tribool down, bool previousState)
{
	if(myOwner->actionsController->spellcastingModeActive()) //we are casting a spell
		return;

	if(boost::logic::indeterminate(down))
		return;

	if(!myHero || down || !myOwner->myTurn)
		return;

	if(myOwner->getCurrentPlayerInterface()->cb->battleCanCastSpell(myHero, spells::Mode::HERO) == ESpellCastProblem::OK) //check conditions
	{
		BattleHex hoveredHex = myOwner->fieldController->getHoveredHex();
		//do nothing when any hex is hovered - hero's animation overlaps battlefield
		if ( hoveredHex != BattleHex::INVALID )
			return;

		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);

		GH.pushIntT<CSpellWindow>(myHero, myOwner->getCurrentPlayerInterface());
	}
}

void BattleHero::clickRight(tribool down, bool previousState)
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
		GH.pushIntT<HeroInfoWindow>(targetHero, &windowPosition);
	}
}

void BattleHero::switchToNextPhase()
{
	if(phase != nextPhase)
	{
		phase = nextPhase;

		firstFrame = 0;

		lastFrame = static_cast<int>(animation->size(phase));
	}

	currentFrame = firstFrame;
}

BattleHero::BattleHero(const std::string & animationPath, bool flipG, PlayerColor player, const CGHeroInstance * hero, const BattleInterface & owner):
    flip(flipG),
    myHero(hero),
	myOwner(&owner),
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

HeroInfoWindow::HeroInfoWindow(const InfoAboutHero & hero, Point * position)
	: CWindowObject(RCLICK_POPUP | SHADOW_DISABLED, "CHRPOP")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	if (position != nullptr)
		moveTo(*position);
	background->colorize(hero.owner); //maybe add this functionality to base class?

	auto attack = hero.details->primskills[0];
	auto defense = hero.details->primskills[1];
	auto power = hero.details->primskills[2];
	auto knowledge = hero.details->primskills[3];
	auto morale = hero.details->morale;
	auto luck = hero.details->luck;
	auto currentSpellPoints = hero.details->mana;
	auto maxSpellPoints = hero.details->manaLimit;

	icons.push_back(std::make_shared<CAnimImage>("PortraitsLarge", hero.portrait, 0, 10, 6));

	//primary stats
	labels.push_back(std::make_shared<CLabel>(9, 75, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[380] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 87, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[381] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 99, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[382] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 111, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[383] + ":"));

	labels.push_back(std::make_shared<CLabel>(69, 87, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(attack)));
	labels.push_back(std::make_shared<CLabel>(69, 99, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(defense)));
	labels.push_back(std::make_shared<CLabel>(69, 111, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(power)));
	labels.push_back(std::make_shared<CLabel>(69, 123, EFonts::FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(knowledge)));

	//morale+luck
	labels.push_back(std::make_shared<CLabel>(9, 131, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[384] + ":"));
	labels.push_back(std::make_shared<CLabel>(9, 143, EFonts::FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[385] + ":"));

	icons.push_back(std::make_shared<CAnimImage>("IMRL22", morale + 3, 0, 47, 131));
	icons.push_back(std::make_shared<CAnimImage>("ILCK22", luck + 3, 0, 47, 143));

	//spell points
	labels.push_back(std::make_shared<CLabel>(39, 174, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[387]));
	labels.push_back(std::make_shared<CLabel>(39, 186, EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, std::to_string(currentSpellPoints) + "/" + std::to_string(maxSpellPoints)));
}

BattleOptionsWindow::BattleOptionsWindow(BattleInterface & owner):
	CWindowObject(PLAYER_COLORED, "comopbck.bmp")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	auto viewGrid = std::make_shared<CToggleButton>(Point(25, 56), "sysopchk.def", CGI->generaltexth->zelp[427], [&](bool on){owner.setPrintCellBorders(on);} );
	viewGrid->setSelected(settings["battle"]["cellBorders"].Bool());
	toggles.push_back(viewGrid);

	auto movementShadow = std::make_shared<CToggleButton>(Point(25, 89), "sysopchk.def", CGI->generaltexth->zelp[428], [&](bool on){owner.setPrintStackRange(on);});
	movementShadow->setSelected(settings["battle"]["stackRange"].Bool());
	toggles.push_back(movementShadow);

	auto mouseShadow = std::make_shared<CToggleButton>(Point(25, 122), "sysopchk.def", CGI->generaltexth->zelp[429], [&](bool on){owner.setPrintMouseShadow(on);});
	mouseShadow->setSelected(settings["battle"]["mouseShadow"].Bool());
	toggles.push_back(mouseShadow);

	animSpeeds = std::make_shared<CToggleGroup>([&](int value){ owner.setAnimSpeed(value);});

	std::shared_ptr<CToggleButton> toggle;
	toggle = std::make_shared<CToggleButton>(Point( 28, 225), "sysopb9.def", CGI->generaltexth->zelp[422]);
	animSpeeds->addToggle(40, toggle);

	toggle = std::make_shared<CToggleButton>(Point( 92, 225), "sysob10.def", CGI->generaltexth->zelp[423]);
	animSpeeds->addToggle(63, toggle);

	toggle = std::make_shared<CToggleButton>(Point(156, 225), "sysob11.def", CGI->generaltexth->zelp[424]);
	animSpeeds->addToggle(100, toggle);

	animSpeeds->setSelected(owner.getAnimSpeed());

	setToDefault = std::make_shared<CButton>(Point(246, 359), "codefaul.def", CGI->generaltexth->zelp[393], [&](){ bDefaultf(); });
	setToDefault->setImageOrder(1, 0, 2, 3);
	exit = std::make_shared<CButton>(Point(357, 359), "soretrn.def", CGI->generaltexth->zelp[392], [&](){ bExitf();}, SDLK_RETURN);
	exit->setImageOrder(1, 0, 2, 3);

	//creating labels
	labels.push_back(std::make_shared<CLabel>(242,  32, FONT_BIG,    ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[392]));//window title
	labels.push_back(std::make_shared<CLabel>(122, 214, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[393]));//animation speed
	labels.push_back(std::make_shared<CLabel>(122, 293, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[394]));//music volume
	labels.push_back(std::make_shared<CLabel>(122, 359, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[395]));//effects' volume
	labels.push_back(std::make_shared<CLabel>(353,  66, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[396]));//auto - combat options
	labels.push_back(std::make_shared<CLabel>(353, 265, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[397]));//creature info

	//auto - combat options
	labels.push_back(std::make_shared<CLabel>(283,  86, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[398]));//creatures
	labels.push_back(std::make_shared<CLabel>(283, 116, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[399]));//spells
	labels.push_back(std::make_shared<CLabel>(283, 146, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[400]));//catapult
	labels.push_back(std::make_shared<CLabel>(283, 176, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[151]));//ballista
	labels.push_back(std::make_shared<CLabel>(283, 206, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[401]));//first aid tent

	//creature info
	labels.push_back(std::make_shared<CLabel>(283, 285, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[402]));//all stats
	labels.push_back(std::make_shared<CLabel>(283, 315, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[403]));//spells only

	//general options
	labels.push_back(std::make_shared<CLabel>(61,  57, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[404]));
	labels.push_back(std::make_shared<CLabel>(61,  90, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[405]));
	labels.push_back(std::make_shared<CLabel>(61, 123, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[406]));
	labels.push_back(std::make_shared<CLabel>(61, 156, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[407]));
}

void BattleOptionsWindow::bDefaultf()
{
	//TODO: implement
}

void BattleOptionsWindow::bExitf()
{
	close();
}

BattleResultWindow::BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner)
	: owner(_owner)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	pos = genRect(561, 470, (screen->w - 800)/2 + 165, (screen->h - 600)/2 + 19);
	background = std::make_shared<CPicture>("CPRESULT");
	background->colorize(owner.playerID);

	exit = std::make_shared<CButton>(Point(384, 505), "iok6432.def", std::make_pair("", ""), [&](){ bExitf();}, SDLK_RETURN);
	exit->setBorderColor(Colors::METALLIC_GOLD);

	if(br.winner == 0) //attacker won
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[411]));
	}

	if(br.winner == 1)
	{
		labels.push_back(std::make_shared<CLabel>(412, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(408, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[411]));
	}

	labels.push_back(std::make_shared<CLabel>(232, 302, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,  CGI->generaltexth->allTexts[407]));
	labels.push_back(std::make_shared<CLabel>(232, 332, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[408]));
	labels.push_back(std::make_shared<CLabel>(232, 428, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[409]));

	std::string sideNames[2] = {"N/A", "N/A"};

	for(int i = 0; i < 2; i++)
	{
		auto heroInfo = owner.cb->battleGetHeroInfo(i);
		const int xs[] = {21, 392};

		if(heroInfo.portrait >= 0) //attacking hero
		{
			icons.push_back(std::make_shared<CAnimImage>("PortraitsLarge", heroInfo.portrait, 0, xs[i], 38));
			sideNames[i] = heroInfo.name;
		}
		else
		{
			auto stacks = owner.cb->battleGetAllStacks();
			vstd::erase_if(stacks, [i](const CStack * stack) //erase stack of other side and not coming from garrison
			{
				return stack->side != i || !stack->base;
			});

			auto best = vstd::maxElementByFun(stacks, [](const CStack * stack)
			{
				return stack->type->AIValue;
			});

			if(best != stacks.end()) //should be always but to be safe...
			{
				icons.push_back(std::make_shared<CAnimImage>("TWCRPORT", (*best)->type->getIconIndex(), 0, xs[i], 38));
				sideNames[i] = (*best)->type->getPluralName();
			}
		}
	}

	//printing attacker and defender's names
	labels.push_back(std::make_shared<CLabel>(89, 37, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, sideNames[0]));
	labels.push_back(std::make_shared<CLabel>(381, 53, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, sideNames[1]));

	//printing casualties
	for(int step = 0; step < 2; ++step)
	{
		if(br.casualties[step].size()==0)
		{
			labels.push_back(std::make_shared<CLabel>(235, 360 + 97 * step, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[523]));
		}
		else
		{
			int xPos = 235 - ((int)br.casualties[step].size()*32 + ((int)br.casualties[step].size() - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + step * 97;
			for(auto & elem : br.casualties[step])
			{
				icons.push_back(std::make_shared<CAnimImage>("CPRSMALL", CGI->creatures()->getByIndex(elem.first)->getIconIndex(), 0, xPos, yPos));
				std::ostringstream amount;
				amount<<elem.second;
				labels.push_back(std::make_shared<CLabel>(xPos + 16, yPos + 42, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, amount.str()));
				xPos += 42;
			}
		}
	}
	//printing result description
	bool weAreAttacker = !(owner.cb->battleGetMySide());
	if((br.winner == 0 && weAreAttacker) || (br.winner == 1 && !weAreAttacker)) //we've won
	{
		int text = 304;
		switch(br.result)
		{
		case BattleResult::NORMAL:
			break;
		case BattleResult::ESCAPE:
			text = 303;
			break;
		case BattleResult::SURRENDER:
			text = 302;
			break;
		default:
			logGlobal->error("Invalid battle result code %d. Assumed normal.", static_cast<int>(br.result));
			break;
		}

		CCS->musich->playMusic("Music/Win Battle", false, true);
		CCS->videoh->open("WIN3.BIK");
		std::string str = CGI->generaltexth->allTexts[text];

		const CGHeroInstance * ourHero = owner.cb->battleGetMyHero();
		if (ourHero)
		{
			str += CGI->generaltexth->allTexts[305];
			boost::algorithm::replace_first(str, "%s", ourHero->name);
			boost::algorithm::replace_first(str, "%d", boost::lexical_cast<std::string>(br.exp[weAreAttacker ? 0 : 1]));
		}

		description = std::make_shared<CTextBox>(str, Rect(69, 203, 330, 68), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}
	else // we lose
	{
		int text = 311;
		std::string musicName = "Music/LoseCombat";
		std::string videoName = "LBSTART.BIK";
		switch(br.result)
		{
		case BattleResult::NORMAL:
			break;
		case BattleResult::ESCAPE:
			musicName = "Music/Retreat Battle";
			videoName = "RTSTART.BIK";
			text = 310;
			break;
		case BattleResult::SURRENDER:
			musicName = "Music/Surrender Battle";
			videoName = "SURRENDER.BIK";
			text = 309;
			break;
		default:
			logGlobal->error("Invalid battle result code %d. Assumed normal.", static_cast<int>(br.result));
			break;
		}
		CCS->musich->playMusic(musicName, false, true);
		CCS->videoh->open(videoName);

		labels.push_back(std::make_shared<CLabel>(235, 235, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[text]));
	}
}

void BattleResultWindow::activate()
{
	owner.showingDialog->set(true);
	CIntObject::activate();
}

void BattleResultWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
	CCS->videoh->update(pos.x + 107, pos.y + 70, screen, true, false);
}

void BattleResultWindow::bExitf()
{
	CPlayerInterface &intTmp = owner; //copy reference because "this" will be destructed soon

	close();

	if(dynamic_cast<BattleInterface*>(GH.topInt().get()))
		GH.popInts(1); //pop battle interface if present

	//Result window and battle interface are gone. We requested all dialogs to be closed before opening the battle,
	//so we can be sure that there is no dialogs left on GUI stack.
	intTmp.showingDialog->setn(false);
	CCS->videoh->close();
}

void ClickableHex::hover(bool on)
{
	hovered = on;
	//Hoverable::hover(on);
	if(!on && setAlterText)
	{
		myInterface->controlPanel->console->clear();
		setAlterText = false;
	}
}

ClickableHex::ClickableHex() : setAlterText(false), myNumber(-1), strictHovered(false), myInterface(nullptr)
{
	addUsedEvents(LCLICK | RCLICK | HOVER | MOVE);
}

void ClickableHex::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	strictHovered = myInterface->fieldController->isPixelInHex(Point(sEvent.x-pos.x, sEvent.y-pos.y));

	if(hovered && strictHovered) //print attacked creature to console
	{
		const CStack * attackedStack = myInterface->getCurrentPlayerInterface()->cb->battleGetStackByPos(myNumber);
		if( attackedStack != nullptr &&
			attackedStack->owner != myInterface->getCurrentPlayerInterface()->playerID &&
			attackedStack->alive())
		{
			MetaString text;
			text.addTxt(MetaString::GENERAL_TXT, 220);
			attackedStack->addNameReplacement(text);
			myInterface->controlPanel->console->write(text.toString());
			setAlterText = true;
		}
	}
	else if(setAlterText)
	{
		myInterface->controlPanel->console->clear();
		setAlterText = false;
	}
}

void ClickableHex::clickLeft(tribool down, bool previousState)
{
	if(!down && hovered && strictHovered) //we've been really clicked!
	{
		myInterface->hexLclicked(myNumber);
	}
}

void ClickableHex::clickRight(tribool down, bool previousState)
{
	const CStack * myst = myInterface->getCurrentPlayerInterface()->cb->battleGetStackByPos(myNumber); //stack info
	if(hovered && strictHovered && myst!=nullptr)
	{
		if(!myst->alive()) return;
		if(down)
		{
			GH.pushIntT<CStackWindow>(myst, true);
		}
	}
}

StackQueue::StackQueue(bool Embedded, BattleInterface & owner)
	: embedded(Embedded),
	owner(owner)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
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

		background = std::make_shared<CFilledTexture>("DIBOXBCK", Rect(0, 0, pos.w, pos.h));

		icons = std::make_shared<CAnimation>("TWCRPORT");
		stateIcons = std::make_shared<CAnimation>("VCMI/BATTLEQUEUE/STATESSMALL");
		//TODO: where use big icons?
		//stateIcons = std::make_shared<CAnimation>("VCMI/BATTLEQUEUE/STATESBIG");
	}
	stateIcons->preload();

	stackBoxes.resize(QUEUE_SIZE);
	for (int i = 0; i < stackBoxes.size(); i++)
	{
		stackBoxes[i] = std::make_shared<StackBox>(this);
		stackBoxes[i]->moveBy(Point(1 + (embedded ? 36 : 80) * i, 0));
	}
}

void StackQueue::update()
{
	std::vector<battle::Units> queueData;

	owner.getCurrentPlayerInterface()->cb->battleGetTurnOrder(queueData, stackBoxes.size(), 0);

	size_t boxIndex = 0;

	for(size_t turn = 0; turn < queueData.size() && boxIndex < stackBoxes.size(); turn++)
	{
		for(size_t unitIndex = 0; unitIndex < queueData[turn].size() && boxIndex < stackBoxes.size(); boxIndex++, unitIndex++)
			stackBoxes[boxIndex]->setUnit(queueData[turn][unitIndex], turn);
	}

	for(; boxIndex < stackBoxes.size(); boxIndex++)
		stackBoxes[boxIndex]->setUnit(nullptr);
}

int32_t StackQueue::getSiegeShooterIconID()
{
	return owner.siegeController->getSiegedTown()->town->faction->index;
}

StackQueue::StackBox::StackBox(StackQueue * owner):
	owner(owner)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>(owner->embedded ? "StackQueueSmall" : "StackQueueLarge");

	pos.w = background->pos.w;
	pos.h = background->pos.h;

	if(owner->embedded)
	{
		icon = std::make_shared<CAnimImage>(owner->icons, 0, 0, 5, 2);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 7, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}
	else
	{
		icon = std::make_shared<CAnimImage>(owner->icons, 0, 0, 9, 1);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 8, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

		int icon_x = pos.w - 17;
		int icon_y = pos.h - 18;

		stateIcon = std::make_shared<CAnimImage>(owner->stateIcons, 0, 0, icon_x, icon_y);
		stateIcon->visible = false;
	}
}

void StackQueue::StackBox::setUnit(const battle::Unit * unit, size_t turn)
{
	if(unit)
	{
		background->colorize(unit->unitOwner());
		icon->visible = true;

		// temporary code for mod compatibility:
		// first, set icon that should definitely exist (arrow tower icon in base vcmi mod)
		// second, try to switch to icon that should be provided by mod
		// if mod is not up to date and does have arrow tower icon yet - second setFrame call will fail and retain previously set image
		// for 1.2 release & later next line should be moved into 'else' block
		icon->setFrame(unit->creatureIconIndex(), 0);
		if (unit->unitType()->idNumber == CreatureID::ARROW_TOWERS)
			icon->setFrame(owner->getSiegeShooterIconID(), 1);

		amount->setText(makeNumberShort(unit->getCount()));

		if(stateIcon)
		{
			if(unit->defended((int)turn) || (turn > 0 && unit->defended((int)turn - 1)))
			{
				stateIcon->setFrame(0, 0);
				stateIcon->visible = true;
			}
			else if(unit->waited((int)turn))
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
		background->colorize(PlayerColor::NEUTRAL);
		icon->visible = false;
		icon->setFrame(0);
		amount->setText("");

		if(stateIcon)
			stateIcon->visible = false;
	}
}
