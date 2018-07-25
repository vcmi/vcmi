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
#include "../mainmenu/CMainMenu.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapInfo.h"
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
	: CWindowObject(BORDERED | SHADOW_DISABLED), ISelectionScreenInfo(type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	IShowActivatable::type = BLOCK_ADV_HOTKEYS;
	pos.w = 762;
	pos.h = 584;
	if(screenType == ESelectionScreen::campaignList)
	{
		setBackground("CamCust.bmp");
	}
	else
	{
		const JsonVector & bgNames = CMainMenuConfig::get().getConfig()["game-select"].Vector();
		setBackground(RandomGeneratorUtil::nextItem(bgNames, CRandomGenerator::getDefault())->String());
	}
	pos = background->center();
	card = std::make_shared<InfoCard>();
	buttonBack = std::make_shared<CButton>(Point(581, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], [=](){ close();}, SDLK_ESCAPE);
}

void CSelectionBase::toggleTab(std::shared_ptr<CIntObject> tab)
{
	if(curTab && curTab->active)
	{
		curTab->deactivate();
		curTab->recActions = 0;
	}

	if(curTab != tab)
	{
		tab->recActions = 255 - DISPOSE;
		tab->activate();
		curTab = tab;
	}
	else
	{
		curTab.reset();
	}
	GH.totalRedraw();
}

InfoCard::InfoCard()
	: showChat(true)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	CIntObject::type |= REDRAW_PARENT;
	pos.x += 393;
	pos.y += 6;

	labelSaveDate = std::make_shared<CLabel>(158, 19, FONT_SMALL, TOPLEFT, Colors::WHITE);
	mapName = std::make_shared<CLabel>(26, 39, FONT_BIG, TOPLEFT, Colors::YELLOW);
	Rect descriptionRect(26, 149, 320, 115);
	mapDescription = std::make_shared<CTextBox>("", descriptionRect, 1);
	playerListBg = std::make_shared<CPicture>("CHATPLUG.bmp", 16, 276);
	chat = std::make_shared<CChatBox>(Rect(26, 132, 340, 132));

	if(SEL->screenType == ESelectionScreen::campaignList)
	{
		labelCampaignDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[38]);
	}
	else
	{
		background = std::make_shared<CPicture>("GSELPOP1.bmp", 0, 0);
		parent->addChild(background.get());
		auto it = vstd::find(parent->children, this); //our position among parent children
		parent->children.insert(it, background.get()); //put BG before us
		parent->children.pop_back();
		pos.w = background->pos.w;
		pos.h = background->pos.h;
		iconsMapSizes = std::make_shared<CAnimImage>("SCNRMPSZ", 4, 0, 318, 22); //let it be custom size (frame 4) by default

		iconDifficulty = std::make_shared<CToggleGroup>(0);
		{
			static const char * difButns[] = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				auto button = std::make_shared<CToggleButton>(Point(110 + i * 32, 450), difButns[i], CGI->generaltexth->zelp[24 + i]);

				iconDifficulty->addToggle(i, button);
				if(SEL->screenType != ESelectionScreen::newGame)
					button->block(true);
			}
		}

		flagbox = std::make_shared<CFlagBox>(Rect(24, 400, 335, 23));
		labelMapDiff = std::make_shared<CLabel>(33, 430, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[494]);
		labelPlayerDifficulty = std::make_shared<CLabel>(133, 430, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[492] + ":");
		labelRating = std::make_shared<CLabel>(290, 430, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[218] + ":");
		labelScenarioName = std::make_shared<CLabel>(26, 22, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[495]);
		labelScenarioDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[496]);
		labelVictoryCondition = std::make_shared<CLabel>(26, 283, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[497]);
		labelLossCondition = std::make_shared<CLabel>(26, 339, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[498]);
		iconsVictoryCondition = std::make_shared<CAnimImage>("SCNRVICT", 0, 0, 24, 302);
		iconsLossCondition = std::make_shared<CAnimImage>("SCNRLOSS", 0, 0, 24, 359);

		labelVictoryConditionText = std::make_shared<CLabel>(60, 307, FONT_SMALL, TOPLEFT, Colors::WHITE);
		labelLossConditionText = std::make_shared<CLabel>(60, 366, FONT_SMALL, TOPLEFT, Colors::WHITE);

		labelDifficulty = std::make_shared<CLabel>(62, 472, FONT_SMALL, CENTER, Colors::WHITE);
		labelDifficultyPercent = std::make_shared<CLabel>(311, 472, FONT_SMALL, CENTER, Colors::WHITE);

		labelGroupPlayersAssigned = std::make_shared<CLabelGroup>(FONT_SMALL, TOPLEFT, Colors::WHITE);
		labelGroupPlayersUnassigned = std::make_shared<CLabelGroup>(FONT_SMALL, TOPLEFT, Colors::WHITE);
	}
	setChat(false);
}

void InfoCard::changeSelection()
{
	if(!SEL->getMapInfo())
		return;

	labelSaveDate->setText(SEL->getMapInfo()->date);
	mapName->setText(SEL->getMapInfo()->getName());
	mapDescription->setText(SEL->getMapInfo()->getDescription());

	mapDescription->label->scrollTextTo(0);
	if(mapDescription->slider)
		mapDescription->slider->moveToMin();

	if(SEL->screenType == ESelectionScreen::campaignList)
		return;

	iconsMapSizes->setFrame(SEL->getMapInfo()->getMapSizeIconId());
	const CMapHeader * header = SEL->getMapInfo()->mapHeader.get();
	iconsVictoryCondition->setFrame(header->victoryIconIndex);
	labelVictoryConditionText->setText(header->victoryMessage);
	iconsLossCondition->setFrame(header->defeatIconIndex);
	labelLossConditionText->setText(header->defeatMessage);
	flagbox->recreate();
	labelDifficulty->setText(CGI->generaltexth->arraytxt[142 + SEL->getMapInfo()->mapHeader->difficulty]);
	iconDifficulty->setSelected(SEL->getCurrentDifficulty());
	const std::array<std::string, 5> difficultyPercent = {"80%", "100%", "130%", "160%", "200%"};
	labelDifficultyPercent->setText(difficultyPercent[SEL->getCurrentDifficulty()]);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	// FIXME: We recreate them each time because CLabelGroup don't use smart pointers
	labelGroupPlayersAssigned = std::make_shared<CLabelGroup>(FONT_SMALL, TOPLEFT, Colors::WHITE);
	labelGroupPlayersUnassigned = std::make_shared<CLabelGroup>(FONT_SMALL, TOPLEFT, Colors::WHITE);
	if(!showChat)
	{
		labelGroupPlayersAssigned->disable();
		labelGroupPlayersUnassigned->disable();
	}
	for(auto & p : CSH->playerNames)
	{
		const auto pset = CSH->si->getPlayersSettings(p.first);
		int pid = p.first;
		if(pset)
		{
			auto name = boost::str(boost::format("%s (%d-%d %s)") % p.second.name % p.second.connection % pid % pset->color.getStr());
			labelGroupPlayersAssigned->add(24, 285 + labelGroupPlayersAssigned->currentSize()*graphics->fonts[FONT_SMALL]->getLineHeight(), name);
		}
		else
		{
			auto name = boost::str(boost::format("%s (%d-%d)") % p.second.name % p.second.connection % pid);
			labelGroupPlayersUnassigned->add(193, 285 + labelGroupPlayersUnassigned->currentSize()*graphics->fonts[FONT_SMALL]->getLineHeight(), name);
		}
	}
}

void InfoCard::toggleChat()
{
	setChat(!showChat);
}

void InfoCard::setChat(bool activateChat)
{
	if(showChat == activateChat)
		return;

	if(activateChat)
	{
		if(SEL->screenType == ESelectionScreen::campaignList)
		{
			labelCampaignDescription->disable();
		}
		else
		{
			labelScenarioDescription->disable();
			labelVictoryCondition->disable();
			labelLossCondition->disable();
			iconsVictoryCondition->disable();
			labelVictoryConditionText->disable();
			iconsLossCondition->disable();
			labelLossConditionText->disable();
			labelGroupPlayersAssigned->enable();
			labelGroupPlayersUnassigned->enable();
		}
		mapDescription->disable();
		chat->enable();
		playerListBg->enable();
	}
	else
	{
		mapDescription->enable();
		chat->disable();
		playerListBg->disable();

		if(SEL->screenType == ESelectionScreen::campaignList)
		{
			labelCampaignDescription->enable();
		}
		else
		{
			labelScenarioDescription->enable();
			labelVictoryCondition->enable();
			labelLossCondition->enable();
			iconsVictoryCondition->enable();
			iconsLossCondition->enable();
			labelVictoryConditionText->enable();
			labelLossConditionText->enable();
			labelGroupPlayersAssigned->disable();
			labelGroupPlayersUnassigned->disable();
		}
	}

	showChat = activateChat;
	GH.totalRedraw();
}

CChatBox::CChatBox(const Rect & rect)
	: CIntObject(KEYBOARD | TEXTINPUT)
{
	OBJ_CONSTRUCTION;
	pos += rect;
	captureAllKeys = true;
	type |= REDRAW_PARENT;

	const int height = graphics->fonts[FONT_SMALL]->getLineHeight();
	inputBox = std::make_shared<CTextInput>(Rect(0, rect.h - height, rect.w, height));
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = std::make_shared<CTextBox>("", Rect(0, 0, rect.w, rect.h - height), 1);

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

CFlagBox::CFlagBox(const Rect & rect)
	: CIntObject(RCLICK)
{
	pos += rect;
	pos.w = rect.w;
	pos.h = rect.h;
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	labelAllies = std::make_shared<CLabel>(0, 0, FONT_SMALL, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[390] + ":");
	labelEnemies = std::make_shared<CLabel>(133, 0, FONT_SMALL, EAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[391] + ":");

	iconsTeamFlags = std::make_shared<CAnimation>("ITGFLAGS.DEF");
	iconsTeamFlags->preload();
}

void CFlagBox::recreate()
{
	flagsAllies.clear();
	flagsEnemies.clear();
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const int alliesX = 5 + labelAllies->getWidth();
	const int enemiesX = 5 + 133 + labelEnemies->getWidth();
	for(auto i = CSH->si->playerInfos.cbegin(); i != CSH->si->playerInfos.cend(); i++)
	{
		auto flag = std::make_shared<CAnimImage>(iconsTeamFlags, i->first.getNum(), 0);
		if(i->first == CSH->myFirstColor() || CSH->getPlayerTeamId(i->first) == CSH->getPlayerTeamId(CSH->myFirstColor()))
		{
			flag->moveTo(Point(pos.x + alliesX + flagsAllies.size()*flag->pos.w, pos.y));
			flagsAllies.push_back(flag);
		}
		else
		{
			flag->moveTo(Point(pos.x + enemiesX + flagsEnemies.size()*flag->pos.w, pos.y));
			flagsEnemies.push_back(flag);
		}
	}
}

void CFlagBox::clickRight(tribool down, bool previousState)
{
	if(down && SEL->getMapInfo())
		GH.pushIntT<CFlagBoxTooltipBox>(iconsTeamFlags);
}

CFlagBox::CFlagBoxTooltipBox::CFlagBoxTooltipBox(std::shared_ptr<CAnimation> icons)
	: CWindowObject(BORDERED | RCLICK_POPUP | SHADOW_DISABLED, "DIBOXBCK")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.w = 256;
	pos.h = 90 + 50 * SEL->getMapInfo()->mapHeader->howManyTeams;

	labelTeamAlignment = std::make_shared<CLabel>(128, 30, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[657]);
	labelGroupTeams = std::make_shared<CLabelGroup>(FONT_SMALL, EAlignment::CENTER, Colors::WHITE);
	for(int i = 0; i < SEL->getMapInfo()->mapHeader->howManyTeams; i++)
	{
		std::vector<ui8> flags;
		labelGroupTeams->add(128, 65 + 50 * i, boost::str(boost::format(CGI->generaltexth->allTexts[656]) % (i+1)));

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
			iconsFlags.push_back(std::make_shared<CAnimImage>(icons, flag, 0, curx, 75 + 50 * i));
			curx += 18;
		}
	}
	background->scaleTo(Point(pos.w, pos.h));
	center();
}
