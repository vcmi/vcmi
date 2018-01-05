/*
 * CSelectionBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSelectionBase.h"
#include "CBonusSelection.h"
#include "CLobbyScreen.h"
#include "../mainmenu/CMainMenu.h"
#include "OptionsTab.h"
#include "RandomMapTab.h"
#include "SelectionTab.h"

#include "../../CCallback.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CBitmapHandler.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/NetPacks.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/serializer/Connection.h"

ISelectionScreenInfo::ISelectionScreenInfo(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	assert(!SEL);
	SEL = this;
}

ISelectionScreenInfo::~ISelectionScreenInfo()
{
	assert(SEL == this);
	SEL = nullptr;
}

int ISelectionScreenInfo::getCurrentDifficulty()
{
	return getStartInfo()->difficulty;
}

PlayerInfo ISelectionScreenInfo::getPlayerInfo(int color)
{
	return getMapInfo()->mapHeader->players[color];
}

CSelectionBase::CSelectionBase(ESelectionScreen type)
	: ISelectionScreenInfo(type)
{
	CMainMenu::create(); //we depend on its graphics

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	IShowActivatable::type = BLOCK_ADV_HOTKEYS;
	pos.w = 762;
	pos.h = 584;
	curTab = nullptr;
	tabRand = nullptr;
	tabOpt = nullptr;
	if(screenType == ESelectionScreen::campaignList)
	{
		bordered = false;
		background = new CPicture("CamCust.bmp", 0, 0);
		pos = background->center();
	}
	else
	{
		bordered = true;
		//load random background
		const JsonVector & bgNames = CMainMenuConfig::get().getConfig()["game-select"].Vector();
		background = new CPicture(RandomGeneratorUtil::nextItem(bgNames, CRandomGenerator::getDefault())->String(), 0, 0);
		pos = background->center();
		tabOpt = new OptionsTab();
		tabOpt->recActions = DISPOSE;
	}
	card = new InfoCard();
	buttonSelect = nullptr;
	buttonRMG = nullptr;
	buttonOptions = nullptr;
	buttonStart = nullptr;
	buttonBack = new CButton(Point(581, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], std::bind(&CGuiHandler::popIntTotally, &GH, this), SDLK_ESCAPE);
}

void CSelectionBase::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	if(bordered && (pos.h != to->h || pos.w != to->w))
		CMessage::drawBorder(PlayerColor(1), to, pos.w + 28, pos.h + 30, pos.x - 14, pos.y - 15);
}

void CSelectionBase::toggleTab(CIntObject * tab)
{
	if(curTab && curTab->active)
	{
		curTab->deactivate();
		curTab->recActions = DISPOSE;
	}

	if(curTab != tab)
	{
		tab->recActions = 255;
		tab->activate();
		curTab = tab;
	}
	else
	{
		curTab = nullptr;
	}
	GH.totalRedraw();
}

InfoCard::InfoCard()
	: mapSizeIcons(nullptr), background(nullptr), showChat(true), chat(nullptr), playerListBg(nullptr), difficulty(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CIntObject::type |= REDRAW_PARENT;
	pos.x += 393;
	pos.y += 6;
	addUsedEvents(RCLICK);
	mapDescription = nullptr;

	Rect descriptionRect(26, 149, 320, 115);
	mapDescription = new CTextBox("", descriptionRect, 1);

	if(SEL->screenType == ESelectionScreen::campaignList)
	{
		CLobbyScreen * ss = static_cast<CLobbyScreen *>(parent);
		mapDescription->addChild(new CPicture(*ss->background, descriptionRect + Point(-393, 0)), true); //move subpicture bg to our description control (by default it's our (Infocard) child)
	}
	else
	{
		background = new CPicture("GSELPOP1.bmp", 0, 0);
		parent->addChild(background);
		auto it = vstd::find(parent->children, this); //our position among parent children
		parent->children.insert(it, background); //put BG before us
		parent->children.pop_back();
		pos.w = background->pos.w;
		pos.h = background->pos.h;
		mapSizeIcons = new CAnimImage("SCNRMPSZ", 4, 0, 318, 22); //let it be custom size (frame 4) by default
		mapSizeIcons->recActions &= ~(SHOWALL | UPDATE); //explicit draw

		sFlags = std::make_shared<CAnimation>("ITGFLAGS.DEF");
		sFlags->load();
		difficulty = new CToggleGroup(0);
		{
			static const char * difButns[] = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				auto button = new CToggleButton(Point(110 + i * 32, 450), difButns[i], CGI->generaltexth->zelp[24 + i]);

				difficulty->addToggle(i, button);
				if(SEL->screenType != ESelectionScreen::newGame)
					button->block(true);
			}
		}
	}
	playerListBg = new CPicture("CHATPLUG.bmp", 16, 276);
	chat = new CChatBox(Rect(26, 132, 340, 132));
	setChat(false); // FIXME: This shouln't be needed if chat / description wouldn't overlay each other on init

	victory = new CAnimImage("SCNRVICT", 0, 0, 24, 302);
	victory->recActions &= ~(SHOWALL | UPDATE); //explicit draw
	loss = new CAnimImage("SCNRLOSS", 0, 0, 24, 359);
	loss->recActions &= ~(SHOWALL | UPDATE); //explicit draw
}

InfoCard::~InfoCard()
{
	if(sFlags)
		sFlags->unload();
}

void InfoCard::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	//blit texts
	if(SEL->screenType != ESelectionScreen::campaignList)
	{
		printAtLoc(CGI->generaltexth->allTexts[390] + ":", 24, 400, FONT_SMALL, Colors::WHITE, to); //Allies
		printAtLoc(CGI->generaltexth->allTexts[391] + ":", 190, 400, FONT_SMALL, Colors::WHITE, to); //Enemies
		printAtLoc(CGI->generaltexth->allTexts[494], 33, 430, FONT_SMALL, Colors::YELLOW, to); //"Map Diff:"
		printAtLoc(CGI->generaltexth->allTexts[492] + ":", 133, 430, FONT_SMALL, Colors::YELLOW, to); //player difficulty
		printAtLoc(CGI->generaltexth->allTexts[218] + ":", 290, 430, FONT_SMALL, Colors::YELLOW, to); //"Rating:"
		printAtLoc(CGI->generaltexth->allTexts[495], 26, 22, FONT_SMALL, Colors::YELLOW, to); //Scenario Name:

		if(!showChat)
		{
			printAtLoc(CGI->generaltexth->allTexts[496], 26, 132, FONT_SMALL, Colors::YELLOW, to); //Scenario Description:
			printAtLoc(CGI->generaltexth->allTexts[497], 26, 283, FONT_SMALL, Colors::YELLOW, to); //Victory Condition:
		}
	}
	else
	{
		if(!showChat)
		{
			printAtLoc(CGI->generaltexth->allTexts[38], 26, 132, FONT_SMALL, Colors::YELLOW, to); //Campaign Description:
		}
	}

	if(showChat)
	{
		int playerLeft = 0; // Players with assigned colors
		int playersRight = 0;
		for(auto & p : CSH->playerNames)
		{
			const auto pset = CSH->si->getPlayersSettings(p.first);
			int pid = p.first;
			if(pset)
			{
				auto name = boost::str(boost::format("%s (%d-%d %s)") % p.second.name % p.second.connection % pid % pset->color.getStr());
				printAtLoc(name, 24, 285 + playerLeft++ *graphics->fonts[FONT_SMALL]->getLineHeight(), FONT_SMALL, Colors::WHITE, to);
			}
			else
			{
				auto name = boost::str(boost::format("%s (%d-%d)") % p.second.name % p.second.connection % pid);
				printAtLoc(name, 193, 285 + playersRight++ *graphics->fonts[FONT_SMALL]->getLineHeight(), FONT_SMALL, Colors::WHITE, to);
			}
		}
	}

	if(SEL->getMapInfo())
	{
		if(SEL->screenType != ESelectionScreen::campaignList)
		{
			if(!showChat)
			{
				CMapHeader * header = SEL->getMapInfo()->mapHeader.get();
				//victory conditions
				printAtLoc(header->victoryMessage, 60, 307, FONT_SMALL, Colors::WHITE, to);
				victory->setFrame(header->victoryIconIndex);
				victory->showAll(to);
				//loss conditoins
				printAtLoc(header->defeatMessage, 60, 366, FONT_SMALL, Colors::WHITE, to);
				loss->setFrame(header->defeatIconIndex);
				loss->showAll(to);
			}

			//difficulty
			assert(SEL->getMapInfo()->mapHeader->difficulty <= 4);
			std::string & diff = CGI->generaltexth->arraytxt[142 + SEL->getMapInfo()->mapHeader->difficulty];
			printAtMiddleLoc(diff, 62, 472, FONT_SMALL, Colors::WHITE, to);

			//selecting size icon
			switch(SEL->getMapInfo()->mapHeader->width)
			{
			case 36:
				mapSizeIcons->setFrame(0);
				break;
			case 72:
				mapSizeIcons->setFrame(1);
				break;
			case 108:
				mapSizeIcons->setFrame(2);
				break;
			case 144:
				mapSizeIcons->setFrame(3);
				break;
			default:
				mapSizeIcons->setFrame(4);
				break;
			}
			mapSizeIcons->showAll(to);

			if(SEL->screenType == ESelectionScreen::loadGame)
				printToLoc(SEL->getMapInfo()->date, 308, 34, FONT_SMALL, Colors::WHITE, to);

			//print flags
			int fx = 34 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[390]);
			int ex = 200 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[391]);

			TeamID myT;

			if(CSH->myFirstColor() < PlayerColor::PLAYER_LIMIT)
				myT = SEL->getPlayerInfo(CSH->myFirstColor().getNum()).team;
			else
				myT = TeamID::NO_TEAM;

			for(auto i = CSH->si->playerInfos.cbegin(); i != CSH->si->playerInfos.cend(); i++)
			{
				int * myx = ((i->first == CSH->myFirstColor() || SEL->getPlayerInfo(i->first.getNum()).team == myT) ? &fx : &ex);
				IImage * flag = sFlags->getImage(i->first.getNum(), 0);
				flag->draw(to, pos.x + *myx, pos.y + 399);
				*myx += flag->width();
				flag->decreaseRef();
			}

			std::string tob;
			switch(SEL->getCurrentDifficulty())
			{
			case 0:
				tob = "80%";
				break;
			case 1:
				tob = "100%";
				break;
			case 2:
				tob = "130%";
				break;
			case 3:
				tob = "160%";
				break;
			case 4:
				tob = "200%";
				break;
			}
			printAtMiddleLoc(tob, 311, 472, FONT_SMALL, Colors::WHITE, to);
		}

		//name
		printAtLoc(SEL->getMapInfo()->getName(), 26, 39, FONT_BIG, Colors::YELLOW, to);
	}
}

void InfoCard::clickRight(tribool down, bool previousState)
{
	static const Rect flagArea(19, 397, 335, 23);
	if(SEL->getMapInfo() && down && SEL->getMapInfo() && isItInLoc(flagArea, GH.current->motion.x, GH.current->motion.y))
		showTeamsPopup();
}

void InfoCard::changeSelection()
{
	if(SEL->getMapInfo() && mapDescription)
	{
		mapDescription->setText(SEL->getMapInfo()->getDescription());

		mapDescription->label->scrollTextTo(0);
		if(mapDescription->slider)
			mapDescription->slider->moveToMin();

		if(SEL->screenType != ESelectionScreen::newGame && SEL->screenType != ESelectionScreen::campaignList)
		{
			//difficulty->block(true);
			difficulty->setSelected(SEL->getCurrentDifficulty());
		}
	}
	redraw();
}

void InfoCard::showTeamsPopup()
{
	SDL_Surface * bmp = CMessage::drawDialogBox(256, 90 + 50 * SEL->getMapInfo()->mapHeader->howManyTeams);

	graphics->fonts[FONT_MEDIUM]->renderTextCenter(bmp, CGI->generaltexth->allTexts[657], Colors::YELLOW, Point(128, 30));

	for(int i = 0; i < SEL->getMapInfo()->mapHeader->howManyTeams; i++)
	{
		std::vector<ui8> flags;
		std::string hlp = CGI->generaltexth->allTexts[656]; //Team %d
		hlp.replace(hlp.find("%d"), 2, boost::lexical_cast<std::string>(i + 1));

		graphics->fonts[FONT_SMALL]->renderTextCenter(bmp, hlp, Colors::WHITE, Point(128, 65 + 50 * i));

		for(int j = 0; j < PlayerColor::PLAYER_LIMIT_I; j++)
		{
			if((SEL->getPlayerInfo(j).canHumanPlay || SEL->getPlayerInfo(j).canComputerPlay)
				&& SEL->getPlayerInfo(j).team == TeamID(i))
			{
				flags.push_back(j);
			}
		}

		int curx = 128 - 9 * flags.size();
		for(auto & flag : flags)
		{
			IImage * icon = sFlags->getImage(flag, 0);
			icon->draw(bmp, curx, 75 + 50 * i);
			icon->decreaseRef();
			curx += 18;
		}
	}

	GH.pushInt(new CInfoPopup(bmp, true));
}

void InfoCard::toggleChat()
{
	setChat(!showChat);
}

void InfoCard::setChat(bool activateChat)
{
	if(showChat == activateChat)
		return;

//	assert(active); // FIXME: This shouln't be needed if chat / description wouldn't overlay each other on init

	if(activateChat)
	{
		mapDescription->disable();
		chat->enable();
		playerListBg->enable();
	}
	else
	{
		mapDescription->enable();
		chat->disable();
		playerListBg->disable();
	}

	showChat = activateChat;
	GH.totalRedraw();
}

CChatBox::CChatBox(const Rect & rect)
{
	OBJ_CONSTRUCTION;
	pos += rect;
	addUsedEvents(KEYBOARD | TEXTINPUT);
	captureAllKeys = true;
	type |= REDRAW_PARENT;

	const int height = graphics->fonts[FONT_SMALL]->getLineHeight();
	inputBox = new CTextInput(Rect(0, rect.h - height, rect.w, height));
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = new CTextBox("", Rect(0, 0, rect.w, rect.h - height), 1);

	chatHistory->label->color = Colors::GREEN;
}

void CChatBox::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_RETURN && key.state == SDL_PRESSED && inputBox->text.size())
	{
		CSH->sendMessage(inputBox->text);
		inputBox->setText("");
	}
	else
		inputBox->keyPressed(key);
}

void CChatBox::addNewMessage(const std::string & text)
{
	CCS->soundh->playSound("CHAT");
	chatHistory->setText(chatHistory->label->text + text + "\n");
	if(chatHistory->slider)
		chatHistory->slider->moveToMax();
}
