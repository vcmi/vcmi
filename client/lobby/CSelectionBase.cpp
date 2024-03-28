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
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../globalLobby/GlobalLobbyClient.h"
#include "../mainmenu/CMainMenu.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/MetaString.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/CMapInfo.h"

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

PlayerInfo ISelectionScreenInfo::getPlayerInfo(PlayerColor color)
{
	return getMapInfo()->mapHeader->players.at(color.getNum());
}

CSelectionBase::CSelectionBase(ESelectionScreen type)
	: CWindowObject(BORDERED | SHADOW_DISABLED), ISelectionScreenInfo(type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.w = 762;
	pos.h = 584;
	if(screenType == ESelectionScreen::campaignList)
	{
		setBackground(ImagePath::builtin("CamCust.bmp"));
	}
	else
	{
		const JsonVector & bgNames = CMainMenuConfig::get().getConfig()["game-select"].Vector();
		setBackground(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(bgNames, CRandomGenerator::getDefault())));
	}
	pos = background->center();
	card = std::make_shared<InfoCard>();
	buttonBack = std::make_shared<CButton>(Point(581, 535), AnimationPath::builtin("SCNRBACK.DEF"), CGI->generaltexth->zelp[105], [=](){ close();}, EShortcut::GLOBAL_CANCEL);
}

void CSelectionBase::toggleTab(std::shared_ptr<CIntObject> tab)
{
	if(curTab && curTab->isActive())
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

	if(tabSel->showRandom && tab != tabOpt)
	{
		tabSel->curFolder = "";
		tabSel->showRandom = false;
		tabSel->filter(0, true);
	}

	GH.windows().totalRedraw();
}

InfoCard::InfoCard()
	: showChat(true)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	setRedrawParent(true);
	pos.x += 393;
	pos.y += 6;

	labelSaveDate = std::make_shared<CLabel>(310, 38, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE);
	labelMapSize = std::make_shared<CLabel>(333, 56, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE);
	mapName = std::make_shared<CLabel>(26, 39, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW);
	Rect descriptionRect(26, 149, 320, 115);
	mapDescription = std::make_shared<CTextBox>("", descriptionRect, 1);
	playerListBg = std::make_shared<CPicture>(ImagePath::builtin("CHATPLUG.bmp"), 16, 276);
	chat = std::make_shared<CChatBox>(Rect(18, 126, 335, 143));

	buttonInvitePlayers = std::make_shared<CButton>(Point(20, 365), AnimationPath::builtin("pregameInvitePlayers"), CGI->generaltexth->zelp[105], [](){ CSH->getGlobalLobby().activateRoomInviteInterface(); } );
	buttonOpenGlobalLobby = std::make_shared<CButton>(Point(188, 365), AnimationPath::builtin("pregameReturnToLobby"), CGI->generaltexth->zelp[105], [](){ CSH->getGlobalLobby().activateInterface(); });

	buttonInvitePlayers->setTextOverlay  (MetaString::createFromTextID("vcmi.lobby.invite.header").toString(), EFonts::FONT_SMALL, Colors::WHITE);
	buttonOpenGlobalLobby->setTextOverlay(MetaString::createFromTextID("vcmi.lobby.backToLobby").toString(), EFonts::FONT_SMALL, Colors::WHITE);

	if(SEL->screenType == ESelectionScreen::campaignList)
	{
		labelCampaignDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[38]);
	}
	else
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("GSELPOP1.bmp"), 0, 0);
		parent->addChild(background.get());
		auto it = vstd::find(parent->children, this); //our position among parent children
		parent->children.insert(it, background.get()); //put BG before us
		parent->children.pop_back();
		pos.w = background->pos.w;
		pos.h = background->pos.h;
		iconsMapSizes = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRMPSZ"), 4, 0, 318, 22); //let it be custom size (frame 4) by default

		iconDifficulty = std::make_shared<CToggleGroup>(0);
		{
			constexpr std::array difButns = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				auto button = std::make_shared<CToggleButton>(Point(110 + i * 32, 450), AnimationPath::builtin(difButns[i]), CGI->generaltexth->zelp[24 + i]);

				iconDifficulty->addToggle(i, button);
				if(SEL->screenType != ESelectionScreen::newGame)
					button->block(true);
			}
		}

		flagbox = std::make_shared<CFlagBox>(Rect(24, 400, 335, 23));
		labelMapDiff = std::make_shared<CLabel>(33, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[494]);
		labelPlayerDifficulty = std::make_shared<CLabel>(133, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[492] + ":");
		labelRating = std::make_shared<CLabel>(290, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[218] + ":");
		labelScenarioName = std::make_shared<CLabel>(26, 22, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[495]);
		labelScenarioDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[496]);
		labelVictoryCondition = std::make_shared<CLabel>(26, 283, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[497]);
		labelLossCondition = std::make_shared<CLabel>(26, 339, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[498]);
		iconsVictoryCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRVICT"), 0, 0, 24, 302);
		iconsLossCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRLOSS"), 0, 0, 24, 359);

		labelVictoryConditionText = std::make_shared<CLabel>(60, 307, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		labelLossConditionText = std::make_shared<CLabel>(60, 366, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

		labelDifficulty = std::make_shared<CLabel>(62, 472, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
		labelDifficultyPercent = std::make_shared<CLabel>(311, 472, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		labelGroupPlayers = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		disableLabelRedraws();
	}
	setChat(false);
	if (CSH->inLobbyRoom())
		setChat(true); // FIXME: less ugly version?
}

void InfoCard::disableLabelRedraws()
{
	labelSaveDate->setAutoRedraw(false);
	labelMapSize->setAutoRedraw(false);
	mapName->setAutoRedraw(false);
	mapDescription->label->setAutoRedraw(false);
	labelVictoryConditionText->setAutoRedraw(false);
	labelLossConditionText->setAutoRedraw(false);
	labelDifficulty->setAutoRedraw(false);
	labelDifficultyPercent->setAutoRedraw(false);
}

void InfoCard::changeSelection()
{
	auto mapInfo = SEL->getMapInfo();
	if(!mapInfo)
		return;

	labelSaveDate->setText(mapInfo->date);
	mapName->setText(mapInfo->getNameTranslated());
	mapDescription->setText(mapInfo->getDescriptionTranslated());

	mapDescription->label->scrollTextTo(0, false);
	if(mapDescription->slider)
		mapDescription->slider->scrollToMin();

	if(SEL->screenType == ESelectionScreen::campaignList)
		return;

	const CMapHeader * header = mapInfo->mapHeader.get();

	labelMapSize->setText(std::to_string(header->width) + "x" + std::to_string(header->height));
	iconsMapSizes->setFrame(mapInfo->getMapSizeIconId());

	iconsVictoryCondition->setFrame(header->victoryIconIndex);
	labelVictoryConditionText->setText(header->victoryMessage.toString());
	iconsLossCondition->setFrame(header->defeatIconIndex);
	labelLossConditionText->setText(header->defeatMessage.toString());
	flagbox->recreate();
	labelDifficulty->setText(CGI->generaltexth->arraytxt[142 + vstd::to_underlying(mapInfo->mapHeader->difficulty)]);
	iconDifficulty->setSelected(SEL->getCurrentDifficulty());
	if(SEL->screenType == ESelectionScreen::loadGame || SEL->screenType == ESelectionScreen::saveGame)
		for(auto & button : iconDifficulty->buttons)
			button.second->setEnabled(button.first == SEL->getCurrentDifficulty());

	const std::array<std::string, 5> difficultyPercent = {"80%", "100%", "130%", "160%", "200%"};
	labelDifficultyPercent->setText(difficultyPercent[SEL->getCurrentDifficulty()]);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	// FIXME: We recreate them each time because CLabelGroup don't use smart pointers
	labelGroupPlayers = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
	if(!showChat)
		labelGroupPlayers->disable();

	for(const auto & p : CSH->playerNames)
	{
		int slotsUsed = labelGroupPlayers->currentSize();
		Point labelPosition;

		if(slotsUsed < 4)
			labelPosition = Point(24, 285 + slotsUsed * graphics->fonts[FONT_SMALL]->getLineHeight()); // left column
		else
			labelPosition = Point(193, 285 + (slotsUsed - 4) * graphics->fonts[FONT_SMALL]->getLineHeight()); // right column

		labelGroupPlayers->add(labelPosition.x, labelPosition.y, p.second.name);
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
			labelGroupPlayers->enable();
		}
		if (CSH->inLobbyRoom())
		{
			buttonInvitePlayers->enable();
			buttonOpenGlobalLobby->enable();
		}
		mapDescription->disable();
		chat->enable();
		playerListBg->enable();
	}
	else
	{
		buttonInvitePlayers->disable();
		buttonOpenGlobalLobby->disable();
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
			labelGroupPlayers->disable();
		}
	}

	showChat = activateChat;
	GH.windows().totalRedraw();
}

CChatBox::CChatBox(const Rect & rect)
	: CIntObject(KEYBOARD | TEXTINPUT)
{
	OBJ_CONSTRUCTION;
	pos += rect.topLeft();
	setRedrawParent(true);

	const int height = static_cast<int>(graphics->fonts[FONT_SMALL]->getLineHeight());
	Rect textInputArea(1, rect.h - height, rect.w - 1, height);
	Rect chatHistoryArea(3, 1, rect.w - 3, rect.h - height - 1);
	inputBackground = std::make_shared<TransparentFilledRectangle>(textInputArea, ColorRGBA(0,0,0,192));
	inputBox = std::make_shared<CTextInput>(textInputArea, EFonts::FONT_SMALL, nullptr, ETextAlignment::TOPLEFT, true);
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = std::make_shared<CTextBox>("", chatHistoryArea, 1);

	chatHistory->label->color = Colors::GREEN;
}

bool CChatBox::captureThisKey(EShortcut key)
{
	return !inputBox->getText().empty() && key == EShortcut::GLOBAL_ACCEPT;
}

void CChatBox::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_ACCEPT && inputBox->getText().size())
	{
		CSH->sendMessage(inputBox->getText());
		inputBox->setText("");
	}
	else
		inputBox->keyPressed(key);
}

void CChatBox::addNewMessage(const std::string & text)
{
	CCS->soundh->playSound(AudioPath::builtin("CHAT"));
	chatHistory->setText(chatHistory->label->getText() + text + "\n");
	if(chatHistory->slider)
		chatHistory->slider->scrollToMax();
}

CFlagBox::CFlagBox(const Rect & rect)
	: CIntObject(SHOW_POPUP)
{
	pos += rect.topLeft();
	pos.w = rect.w;
	pos.h = rect.h;
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	labelAllies = std::make_shared<CLabel>(0, 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[390] + ":");
	labelEnemies = std::make_shared<CLabel>(133, 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[391] + ":");

	iconsTeamFlags = GH.renderHandler().loadAnimation(AnimationPath::builtin("ITGFLAGS.DEF"));
	iconsTeamFlags->preload();
}

void CFlagBox::recreate()
{
	flagsAllies.clear();
	flagsEnemies.clear();
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const int alliesX = 5 + (int)labelAllies->getWidth();
	const int enemiesX = 5 + 133 + (int)labelEnemies->getWidth();
	for(auto i = CSH->si->playerInfos.cbegin(); i != CSH->si->playerInfos.cend(); i++)
	{
		auto flag = std::make_shared<CAnimImage>(iconsTeamFlags, i->first.getNum(), 0);
		if(i->first == CSH->myFirstColor() || CSH->getPlayerTeamId(i->first) == CSH->getPlayerTeamId(CSH->myFirstColor()))
		{
			flag->moveTo(Point(pos.x + alliesX + (int)flagsAllies.size()*flag->pos.w, pos.y));
			flagsAllies.push_back(flag);
		}
		else
		{
			flag->moveTo(Point(pos.x + enemiesX + (int)flagsEnemies.size()*flag->pos.w, pos.y));
			flagsEnemies.push_back(flag);
		}
	}
}

void CFlagBox::showPopupWindow(const Point & cursorPosition)
{
	if(SEL->getMapInfo())
		GH.windows().createAndPushWindow<CFlagBoxTooltipBox>(iconsTeamFlags);
}

CFlagBox::CFlagBoxTooltipBox::CFlagBoxTooltipBox(std::shared_ptr<CAnimation> icons)
	: CWindowObject(BORDERED | RCLICK_POPUP | SHADOW_DISABLED, ImagePath::builtin("DIBOXBCK"))
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	labelTeamAlignment = std::make_shared<CLabel>(128, 30, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[657]);
	labelGroupTeams = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	std::vector<std::set<PlayerColor>> teams(PlayerColor::PLAYER_LIMIT_I);

	for(PlayerColor j(0); j < PlayerColor::PLAYER_LIMIT; j++)
	{
		if(SEL->getPlayerInfo(j).canHumanPlay || SEL->getPlayerInfo(j).canComputerPlay)
		{
			teams[SEL->getPlayerInfo(j).team].insert(j);
		}
	}

	auto curIdx = 0;
	for(const auto & team : teams)
	{
		if(team.empty())
			continue;

		labelGroupTeams->add(128, 65 + 50 * curIdx, boost::str(boost::format(CGI->generaltexth->allTexts[656]) % (curIdx + 1)));
		int curx = 128 - 9 * team.size();
		for(const auto & player : team)
		{
			iconsFlags.push_back(std::make_shared<CAnimImage>(icons, player, 0, curx, 75 + 50 * curIdx));
			curx += 18;
		}
		++curIdx;
	}
	pos.w = 256;
	pos.h = 90 + 50 * curIdx;

	background->scaleTo(Point(pos.w, pos.h));
	center();
}
