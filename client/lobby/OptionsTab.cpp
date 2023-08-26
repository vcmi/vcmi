/*
 * OptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "OptionsTab.h"

#include "CSelectionBase.h"

#include "../CGameInfo.h"
#include "../CServerHandler.h"
#include "../CMusicHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../widgets/CComponent.h"
#include "../widgets/ComboBox.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../eventsSDL/InputHandler.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"

OptionsTab::OptionsTab() : humanPlayers(0)
{
	recActions = 0;
		
	addCallback("setTimerPreset", [&](int index){
		if(!variables["timerPresets"].isNull())
		{
			auto tpreset = variables["timerPresets"].Vector().at(index).Vector();
			TurnTimerInfo tinfo;
			tinfo.baseTimer = tpreset.at(0).Integer() * 1000;
			tinfo.turnTimer = tpreset.at(1).Integer() * 1000;
			tinfo.battleTimer = tpreset.at(2).Integer() * 1000;
			tinfo.creatureTimer = tpreset.at(3).Integer() * 1000;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	
	auto parseTimerString = [](const std::string & str){
		auto sc = str.find(":");
		if(sc == std::string::npos)
			return str.empty() ? 0 : std::stoi(str);
		
		auto l = str.substr(0, sc);
		auto r = str.substr(sc + 1, std::string::npos);
		if(r.length() == 3) //symbol added
		{
			l.push_back(r.front());
			r.erase(r.begin());
		}
		else if(r.length() == 1) //symbol removed
		{
			r.insert(r.begin(), l.back());
			l.pop_back();
		}
		else if(r.empty())
			r = "0";
		
		int sec = std::stoi(r);
		if(sec >= 60)
		{
			if(l.empty()) //9:00 -> 0:09
				return sec / 10;
			
			l.push_back(r.front()); //0:090 -> 9:00
			r.erase(r.begin());
		}
		else if(l.empty())
			return sec;
		
		return std::stoi(l) * 60 + std::stoi(r);
	};
	
	addCallback("parseAndSetTimer_base", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.baseTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_turn", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.turnTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_battle", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.battleTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_creature", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.creatureTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	
	const JsonNode config(ResourceID("config/widgets/optionsTab.json"));
	build(config);
	
	//set timers combo box callbacks
	if(auto w = widget<ComboBox>("timerModeSwitch"))
	{
		w->onConstructItems = [&](std::vector<const void *> & curItems){
			if(variables["timers"].isNull())
				return;
			
			for(auto & p : variables["timers"].Vector())
			{
				curItems.push_back(&p);
			}
		};
		
		w->onSetItem = [&](const void * item){
			if(item)
			{
				if(auto * tObj = reinterpret_cast<const JsonNode *>(item))
				{
					for(auto wname : (*tObj)["hideWidgets"].Vector())
					{
						if(auto w = widget<CIntObject>(wname.String()))
							w->setEnabled(false);
					}
					for(auto wname : (*tObj)["showWidgets"].Vector())
					{
						if(auto w = widget<CIntObject>(wname.String()))
							w->setEnabled(true);
					}
					if((*tObj)["default"].isVector())
					{
						TurnTimerInfo tinfo;
						tinfo.baseTimer = (*tObj)["default"].Vector().at(0).Integer() * 1000;
						tinfo.turnTimer = (*tObj)["default"].Vector().at(1).Integer() * 1000;
						tinfo.battleTimer = (*tObj)["default"].Vector().at(2).Integer() * 1000;
						tinfo.creatureTimer = (*tObj)["default"].Vector().at(3).Integer() * 1000;
						CSH->setTurnTimerInfo(tinfo);
					}
				}
				redraw();
			}
		};
		
		w->getItemText = [this](int idx, const void * item){
			if(item)
			{
				if(auto * tObj = reinterpret_cast<const JsonNode *>(item))
					return readText((*tObj)["text"]);
			}
			return std::string("");
		};
		
		w->setItem(0);
	}
}

void OptionsTab::recreate()
{
	entries.clear();
	humanPlayers = 0;

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	for(auto & pInfo : SEL->getStartInfo()->playerInfos)
	{
		if(pInfo.second.isControlledByHuman())
			humanPlayers++;

		entries.insert(std::make_pair(pInfo.first, std::make_shared<PlayerOptionsEntry>(pInfo.second, * this)));
	}
	
	const auto & turnTimerRemote = SEL->getStartInfo()->turnTimerInfo;
	
	//classic timer
	if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
	{
		if(!variables["timerPresets"].isNull() && !turnTimerRemote.battleTimer && !turnTimerRemote.creatureTimer && !turnTimerRemote.baseTimer)
		{
			for(int idx = 0; idx < variables["timerPresets"].Vector().size(); ++idx)
			{
				auto & tpreset = variables["timerPresets"].Vector()[idx];
				if(tpreset.Vector().at(1).Integer() == turnTimerRemote.turnTimer / 1000)
				{
					turnSlider->scrollTo(idx);
					if(auto w = widget<CLabel>("labelTurnDurationValue"))
						w->setText(CGI->generaltexth->turnDurations[idx]);
				}
			}
		}
	}
	
	//chess timer
	auto timeToString = [](int time) -> std::string
	{
		std::stringstream ss;
		ss << time / 1000 / 60 << ":" << std::setw(2) << std::setfill('0') << time / 1000 % 60;
		return ss.str();
	};
	
	if(auto ww = widget<CTextInput>("chessFieldBase"))
		ww->setText(timeToString(turnTimerRemote.baseTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldTurn"))
		ww->setText(timeToString(turnTimerRemote.turnTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldBattle"))
		ww->setText(timeToString(turnTimerRemote.battleTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldCreature"))
		ww->setText(timeToString(turnTimerRemote.creatureTimer), false);
	
	if(auto w = widget<ComboBox>("timerModeSwitch"))
	{
		if(turnTimerRemote.battleTimer || turnTimerRemote.creatureTimer || turnTimerRemote.baseTimer)
		{
			if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
				if(turnSlider->isActive())
					w->setItem(1);
		}
	}
}

size_t OptionsTab::CPlayerSettingsHelper::getImageIndex(bool big)
{
	enum EBonusSelection //frames of bonuses file
	{
		WOOD_ORE = 0,   CRYSTAL = 1,    GEM  = 2,
		MERCURY  = 3,   SULFUR  = 5,    GOLD = 8,
		ARTIFACT = 9,   RANDOM  = 10,
		WOOD = 0,       ORE     = 0,    MITHRIL = 10, // resources unavailable in bonuses file

		TOWN_RANDOM = 38,  TOWN_NONE = 39, // Special frames in ITPA
		HERO_RANDOM = 163, HERO_NONE = 164 // Special frames in PortraitsSmall
	};
	auto factionIndex = settings.castle.getNum() >= CGI->townh->size() ? 0 : settings.castle.getNum();

	switch(type)
	{
	case TOWN:
		switch(settings.castle)
		{
		case PlayerSettings::NONE:
			return TOWN_NONE;
		case PlayerSettings::RANDOM:
			return TOWN_RANDOM;
		default:
			return (*CGI->townh)[factionIndex]->town->clientInfo.icons[true][false] + (big ? 0 : 2);
		}
	case HERO:
		switch(settings.hero)
		{
		case PlayerSettings::NONE:
			return HERO_NONE;
		case PlayerSettings::RANDOM:
			return HERO_RANDOM;
		default:
		{
			if(settings.heroPortrait != HeroTypeID::NONE)
				return settings.heroPortrait;
			auto index = settings.hero.getNum() >= CGI->heroh->size() ? 0 : settings.hero.getNum();
			return (*CGI->heroh)[index]->imageIndex;
		}
		}
	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return RANDOM;
		case PlayerSettings::ARTIFACT:
			return ARTIFACT;
		case PlayerSettings::GOLD:
			return GOLD;
		case PlayerSettings::RESOURCE:
		{
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::WOOD_AND_ORE:
				return WOOD_ORE;
			case EGameResID::WOOD:
				return WOOD;
			case EGameResID::MERCURY:
				return MERCURY;
			case EGameResID::ORE:
				return ORE;
			case EGameResID::SULFUR:
				return SULFUR;
			case EGameResID::CRYSTAL:
				return CRYSTAL;
			case EGameResID::GEMS:
				return GEM;
			case EGameResID::GOLD:
				return GOLD;
			case EGameResID::MITHRIL:
				return MITHRIL;
			}
		}
		}
	}
	}
	return 0;
}

std::string OptionsTab::CPlayerSettingsHelper::getImageName(bool big)
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return big ? "ITPt": "ITPA";
	case OptionsTab::HERO:
		return big ? "PortraitsLarge": "PortraitsSmall";
	case OptionsTab::BONUS:
		return "SCNRSTAR";
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getName()
{
	switch(type)
	{
	case TOWN:
	{
		switch(settings.castle)
		{
		case PlayerSettings::NONE:
			return CGI->generaltexth->allTexts[523];
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
		{
			auto factionIndex = settings.castle.getNum() >= CGI->townh->size() ? 0 : settings.castle.getNum();
			return (*CGI->townh)[factionIndex]->getNameTranslated();
		}
	}
	}
	case HERO:
	{
		switch(settings.hero)
		{
		case PlayerSettings::NONE:
			return CGI->generaltexth->allTexts[523];
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
		{
			if(!settings.heroName.empty())
				return settings.heroName;
			auto index = settings.hero.getNum() >= CGI->heroh->size() ? 0 : settings.hero.getNum();
			return (*CGI->heroh)[index]->getNameTranslated();
		}
		}
	}
	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
			return CGI->generaltexth->arraytxt[214 + settings.bonus];
		}
	}
	}
	return "";
}


std::string OptionsTab::CPlayerSettingsHelper::getTitle()
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return (settings.castle.getNum() < 0) ? CGI->generaltexth->allTexts[103] : CGI->generaltexth->allTexts[80];
	case OptionsTab::HERO:
		return (settings.hero.getNum() < 0) ? CGI->generaltexth->allTexts[101] : CGI->generaltexth->allTexts[77];
	case OptionsTab::BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[86]; //{Random Bonus}
		case PlayerSettings::ARTIFACT:
			return CGI->generaltexth->allTexts[83]; //{Artifact Bonus}
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[84]; //{Gold Bonus}
		case PlayerSettings::RESOURCE:
			return CGI->generaltexth->allTexts[85]; //{Resource Bonus}
		}
	}
	}
	return "";
}
std::string OptionsTab::CPlayerSettingsHelper::getSubtitle()
{
	auto factionIndex = settings.castle.getNum() >= CGI->townh->size() ? 0 : settings.castle.getNum();
	auto heroIndex = settings.hero.getNum() >= CGI->heroh->size() ? 0 : settings.hero.getNum();

	switch(type)
	{
	case TOWN:
		return getName();
	case HERO:
	{
		if(settings.hero.getNum() >= 0)
			return getName() + " - " + (*CGI->heroh)[heroIndex]->heroClass->getNameTranslated();
		return getName();
	}

	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[87]; //500-1000
		case PlayerSettings::RESOURCE:
		{
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return CGI->generaltexth->allTexts[694];
			case EGameResID::SULFUR:
				return CGI->generaltexth->allTexts[695];
			case EGameResID::CRYSTAL:
				return CGI->generaltexth->allTexts[692];
			case EGameResID::GEMS:
				return CGI->generaltexth->allTexts[693];
			case EGameResID::WOOD_AND_ORE:
				return CGI->generaltexth->allTexts[89]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
			}
		}
		}
	}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getDescription()
{
	auto factionIndex = settings.castle.getNum() >= CGI->townh->size() ? 0 : settings.castle.getNum();

	switch(type)
	{
	case TOWN:
		return CGI->generaltexth->allTexts[104];
	case HERO:
		return CGI->generaltexth->allTexts[102];
	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
		case PlayerSettings::ARTIFACT:
			return CGI->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
		case PlayerSettings::RESOURCE:
		{
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return CGI->generaltexth->allTexts[690];
			case EGameResID::SULFUR:
				return CGI->generaltexth->allTexts[691];
			case EGameResID::CRYSTAL:
				return CGI->generaltexth->allTexts[688];
			case EGameResID::GEMS:
				return CGI->generaltexth->allTexts[689];
			case EGameResID::WOOD_AND_ORE:
				return CGI->generaltexth->allTexts[93]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
			}
		}
		}
	}
	}
	return "";
}

OptionsTab::CPlayerOptionTooltipBox::CPlayerOptionTooltipBox(CPlayerSettingsHelper & helper)
	: CWindowObject(BORDERED | RCLICK_POPUP), CPlayerSettingsHelper(helper)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	int value = PlayerSettings::NONE;

	switch(CPlayerSettingsHelper::type)
	{
		break;
	case TOWN:
		value = settings.castle;
		break;
	case HERO:
		value = settings.hero;
		break;
	case BONUS:
		value = settings.bonus;
	}

	if(value == PlayerSettings::RANDOM)
		genBonusWindow();
	else if(CPlayerSettingsHelper::type == BONUS)
		genBonusWindow();
	else if(CPlayerSettingsHelper::type == HERO)
		genHeroWindow();
	else if(CPlayerSettingsHelper::type == TOWN)
		genTownWindow();

	center();
}

void OptionsTab::CPlayerOptionTooltipBox::genHeader()
{
	backgroundTexture = std::make_shared<CFilledTexture>("DIBOXBCK", pos);
	updateShadow();

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, getTitle());
	labelSubTitle = std::make_shared<CLabel>(pos.w / 2, 88, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, getSubtitle());
	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex(), 0, pos.w / 2 - 24, 45);
}

void OptionsTab::CPlayerOptionTooltipBox::genTownWindow()
{
	pos = Rect(0, 0, 228, 290);
	genHeader();
	labelAssociatedCreatures = std::make_shared<CLabel>(pos.w / 2 + 8, 122, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[79]);
	auto factionIndex = settings.castle.getNum() >= CGI->townh->size() ? 0 : settings.castle.getNum();
	std::vector<std::shared_ptr<CComponent>> components;
	const CTown * town = (*CGI->townh)[factionIndex]->town;

	for(auto & elem : town->creatures)
	{
		if(!elem.empty())
			components.push_back(std::make_shared<CComponent>(CComponent::creature, elem.front(), 0, CComponent::tiny));
	}
	boxAssociatedCreatures = std::make_shared<CComponentBox>(components, Rect(10, 140, pos.w - 20, 140));
}

void OptionsTab::CPlayerOptionTooltipBox::genHeroWindow()
{
	pos = Rect(0, 0, 292, 226);
	genHeader();
	labelHeroSpeciality = std::make_shared<CLabel>(pos.w / 2 + 4, 117, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	auto heroIndex = settings.hero.getNum() >= CGI->heroh->size() ? 0 : settings.hero.getNum();

	imageSpeciality = std::make_shared<CAnimImage>("UN44", (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());
}

void OptionsTab::CPlayerOptionTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	textBonusDescription = std::make_shared<CTextBox>(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

OptionsTab::SelectionWindow::SelectionWindow(PlayerColor _color, SelType _type)
	: CWindowObject(BORDERED)
{
	addUsedEvents(LCLICK | SHOW_POPUP);

	color = _color;
	type = _type;

	initialFaction = SEL->getStartInfo()->playerInfos.find(color)->second.castle;
	initialHero = SEL->getStartInfo()->playerInfos.find(color)->second.hero;
	initialBonus = SEL->getStartInfo()->playerInfos.find(color)->second.bonus;
	selectedFaction = initialFaction;
	selectedHero = initialHero;
	selectedBonus = initialBonus;
	allowedFactions = SEL->getPlayerInfo(color.getNum()).allowedFactions;
	std::vector<bool> allowedHeroesFlag = SEL->getMapInfo()->mapHeader->allowedHeroes;
	for(int i = 0; i < allowedHeroesFlag.size(); i++)
		if(allowedHeroesFlag[i])
			allowedHeroes.insert(HeroTypeID(i));

	allowedBonus.push_back(-1); // random
	if(initialHero.getNum() >= -1)
		allowedBonus.push_back(0); // artifact
	allowedBonus.push_back(1); // gold
	if(initialFaction.getNum() >= 0)
		allowedBonus.push_back(2); // resource

	recreate();
}

int OptionsTab::SelectionWindow::calcLines(FactionID faction)
{
	double additionalItems = 1; // random

	if(faction.getNum() < 0)
		return std::ceil(((double)allowedFactions.size() + additionalItems) / elementsPerLine);

	int count = 0;
	for(auto & elemh : allowedHeroes)
	{
		CHero * type = VLC->heroh->objects[elemh];
		if(type->heroClass->faction == faction)
			count++;
	}

	return std::ceil(std::max((double)count + additionalItems, (double)allowedFactions.size() + additionalItems) / (double)elementsPerLine);
}

void OptionsTab::SelectionWindow::apply()
{
	if(GH.windows().isTopWindow(this))
	{
		GH.input().hapticFeedback();
		CCS->soundh->playSound(soundBase::button);

		close();

		setSelection();
	}
}

void OptionsTab::SelectionWindow::setSelection()
{
	if(selectedFaction != initialFaction)
		CSH->setPlayerOption(LobbyChangePlayerOption::TOWN_ID, selectedFaction, color);

	if(selectedHero != initialHero)
		CSH->setPlayerOption(LobbyChangePlayerOption::HERO_ID, selectedHero, color);

	if(selectedBonus != initialBonus)
		CSH->setPlayerOption(LobbyChangePlayerOption::BONUS_ID, selectedBonus, color);
}

void OptionsTab::SelectionWindow::recreate()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	int amountLines = 1;
	if(type == SelType::BONUS)
		elementsPerLine = allowedBonus.size();
	else
	{
		// try to make squarish
		if(type == SelType::TOWN)
			elementsPerLine = floor(sqrt(allowedFactions.size()));
		if(type == SelType::HERO)
		{
			int count = 0;
			for(auto & elem : allowedHeroes)
			{
				CHero * type = VLC->heroh->objects[elem];
				if(type->heroClass->faction == selectedFaction)
				{
					count++;
				}
			}
			elementsPerLine = floor(sqrt(count));
		}

		amountLines = calcLines((type > SelType::TOWN) ? selectedFaction : static_cast<FactionID>(PlayerSettings::RANDOM));
	}

	int x = (elementsPerLine) * (ICON_BIG_WIDTH-1);
	int y = (amountLines) * (ICON_BIG_HEIGHT-1);

	pos = Rect(0, 0, x, y);

	backgroundTexture = std::make_shared<FilledTexturePlayerColored>("DiBoxBck", pos);
	backgroundTexture->playerColored(PlayerColor(1));
	updateShadow();

	if(type == SelType::TOWN)
		genContentFactions();
	if(type == SelType::HERO)
		genContentHeroes();
	if(type == SelType::BONUS)
		genContentBonus();
	genContentGrid(amountLines);

	center();
}

void OptionsTab::SelectionWindow::drawOutlinedText(int x, int y, ColorRGBA color, std::string text)
{
	components.push_back(std::make_shared<CLabel>(x-1, y, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text));
	components.push_back(std::make_shared<CLabel>(x+1, y, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text));
	components.push_back(std::make_shared<CLabel>(x, y-1, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text));
	components.push_back(std::make_shared<CLabel>(x, y+1, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text));
	components.push_back(std::make_shared<CLabel>(x, y, FONT_TINY, ETextAlignment::CENTER, color, text));
}

void OptionsTab::SelectionWindow::genContentGrid(int lines)
{
	for(int y = 0; y < lines; y++)
	{
		for(int x = 0; x < elementsPerLine; x++)
		{
			components.push_back(std::make_shared<CPicture>("lobby/townBorderBig", x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		}
	}
}

void OptionsTab::SelectionWindow::genContentFactions()
{
	int i = 1;

	// random
	PlayerSettings set = PlayerSettings();
	set.castle = PlayerSettings::RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, 6, (ICON_SMALL_HEIGHT/2)));
	drawOutlinedText(TEXT_POS_X, TEXT_POS_Y, (selectedFaction.getNum() == PlayerSettings::RANDOM) ? Colors::YELLOW : Colors::WHITE, helper.getName());
	if(selectedFaction.getNum() == PlayerSettings::RANDOM)
		components.push_back(std::make_shared<CPicture>("lobby/townBorderSmallActivated", 6, (ICON_SMALL_HEIGHT/2)));

	for(auto & elem : allowedFactions)
	{
		int x = i % elementsPerLine;
		int y = i / elementsPerLine;

		PlayerSettings set = PlayerSettings();
		set.castle = elem;

		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);

		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		components.push_back(std::make_shared<CPicture>(selectedFaction == elem ? "lobby/townBorderBigActivated" : "lobby/townBorderBig", x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, (selectedFaction == elem) ? Colors::YELLOW : Colors::WHITE, helper.getName());
		factions.push_back(elem);

		i++;
	}
}

void OptionsTab::SelectionWindow::genContentHeroes()
{
	int i = 1;

	// random
	PlayerSettings set = PlayerSettings();
	set.hero = PlayerSettings::RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, 6, (ICON_SMALL_HEIGHT/2)));
	drawOutlinedText(TEXT_POS_X, TEXT_POS_Y, (selectedHero.getNum() == PlayerSettings::RANDOM) ? Colors::YELLOW : Colors::WHITE, helper.getName());
	if(selectedHero.getNum() == PlayerSettings::RANDOM)
		components.push_back(std::make_shared<CPicture>("lobby/townBorderSmallActivated", 6, (ICON_SMALL_HEIGHT/2)));

	for(auto & elem : allowedHeroes)
	{
		CHero * type = VLC->heroh->objects[elem];

		if(type->heroClass->faction == selectedFaction)
		{

			int x = i % elementsPerLine;
			int y = i / elementsPerLine;

			PlayerSettings set = PlayerSettings();
			set.hero = elem;

			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);

			components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
			components.push_back(std::make_shared<CPicture>(selectedHero == elem ? "lobby/townBorderBigActivated" : "lobby/townBorderBig", x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
			drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, (selectedHero == elem) ? Colors::YELLOW : Colors::WHITE, helper.getName());
			heroes.push_back(elem);

			i++;
		}
	}
}

void OptionsTab::SelectionWindow::genContentBonus()
{
	PlayerSettings set = PlayerSettings();

	int i = 0;
	for(auto elem : allowedBonus)
	{
		int x = i;
		int y = 0;

		set.bonus = static_cast<PlayerSettings::Ebonus>(elem);
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, x * (ICON_BIG_WIDTH-1) + 6, y * (ICON_BIG_HEIGHT-1) + (ICON_SMALL_HEIGHT/2)));
		drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, Colors::WHITE , helper.getName());
		if(selectedBonus == elem)
		{
			components.push_back(std::make_shared<CPicture>("lobby/townBorderSmallActivated", x * (ICON_BIG_WIDTH-1) + 6, y * (ICON_BIG_HEIGHT-1) + (ICON_SMALL_HEIGHT/2)));
			drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, Colors::YELLOW , helper.getName());
		}

		i++;
	}
}

int OptionsTab::SelectionWindow::getElement(const Point & cursorPosition)
{
	int x = (cursorPosition.x - pos.x) / (ICON_BIG_WIDTH-1);
	int y = (cursorPosition.y - pos.y) / (ICON_BIG_HEIGHT-1);

	return x + y * elementsPerLine;
}

void OptionsTab::SelectionWindow::setElement(int elem, bool doApply)
{
	PlayerSettings set = PlayerSettings();
	if(type == SelType::TOWN)
	{
		if(elem > 0)
		{
			elem--;
			if(elem >= factions.size())
				return;
			set.castle = factions[elem];
		}
		else
		{
			set.castle = PlayerSettings::RANDOM;
		}
		if(set.castle.getNum() != PlayerSettings::NONE)
		{
			if(!doApply)
			{
				CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
				GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
			}
			else
				selectedFaction = set.castle;
		}
	}
	if(type == SelType::HERO)
	{
		if(elem > 0)
		{
			elem--;
			if(elem >= heroes.size())
				return;
			set.hero = heroes[elem];
		}
		else
		{
			set.hero = PlayerSettings::RANDOM;
		}
		if(set.hero.getNum() != PlayerSettings::NONE)
		{
			if(!doApply)
			{
				CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
				GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
			}
			else
				selectedHero = set.hero;
		}
	}
	if(type == SelType::BONUS)
	{
		if(elem >= 4)
			return;
		set.bonus = static_cast<PlayerSettings::Ebonus>(elem-1);
		if(set.bonus != PlayerSettings::NONE)
		{
			if(!doApply)
			{
				CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
				GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
			}
			else
				selectedBonus = set.bonus;
		}
	}

	if(doApply)
		apply();
}

bool OptionsTab::SelectionWindow::receiveEvent(const Point & position, int eventType) const
{
	return true;  // capture click also outside of window
}

void OptionsTab::SelectionWindow::clickReleased(const Point & cursorPosition)
{
	if(!pos.isInside(cursorPosition))
	{
		close();
		return;
	}

	int elem = getElement(cursorPosition);

	setElement(elem, true);
}

void OptionsTab::SelectionWindow::showPopupWindow(const Point & cursorPosition)
{
	if(!pos.isInside(cursorPosition))
		return;

	int elem = getElement(cursorPosition);

	setElement(elem, false);
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & settings, SelType type)
	: Scrollable(LCLICK | SHOW_POPUP, position, Orientation::HORIZONTAL)
	, CPlayerSettingsHelper(settings, type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex());
	subtitle = std::make_shared<CLabel>(23, 39, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, getName());

	pos = image->pos;

	setPanningStep(pos.w);
}

void OptionsTab::SelectedBox::update()
{
	image->setFrame(getImageIndex());
	subtitle->setText(getName());
}

void OptionsTab::SelectedBox::showPopupWindow(const Point & cursorPosition)
{
	// cases when we do not need to display a message
	if(settings.castle.getNum() == PlayerSettings::NONE && CPlayerSettingsHelper::type == TOWN)
		return;
	if(settings.hero.getNum() == PlayerSettings::NONE && !SEL->getPlayerInfo(settings.color.getNum()).hasCustomMainHero() && CPlayerSettingsHelper::type == HERO)
		return;

	GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(*this);
}

void OptionsTab::SelectedBox::clickReleased(const Point & cursorPosition)
{
	PlayerInfo pi = SEL->getPlayerInfo(settings.color.getNum());
	const bool foreignPlayer = CSH->isGuest() && !CSH->isMyColor(settings.color);

	if(type == SelType::TOWN && ((pi.allowedFactions.size() < 2 && !pi.isFactionRandom) || foreignPlayer))
		return;

	if(type == SelType::HERO && ((pi.defaultHero() != -1 || settings.castle.getNum() < 0) || foreignPlayer))
		return;

	if(type == SelType::BONUS && foreignPlayer)
		return;

	GH.input().hapticFeedback();
	GH.windows().createAndPushWindow<SelectionWindow>(settings.color, type);
}

void OptionsTab::SelectedBox::scrollBy(int distance)
{
	// FIXME: currently options tab is completely recreacted from scratch whenever we receive any information from server
	// because of that, panning event gets interrupted (due to destruction of element)
	// so, currently, gesture will always move selection only by 1, and then wait for recreation from server info
	distance = std::clamp(distance, -1, 1);

	switch(CPlayerSettingsHelper::type)
	{
		case TOWN:
			CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, distance, settings.color);
			break;
		case HERO:
			CSH->setPlayerOption(LobbyChangePlayerOption::HERO, distance, settings.color);
			break;
		case BONUS:
			CSH->setPlayerOption(LobbyChangePlayerOption::BONUS, distance, settings.color);
			break;
	}

	setScrollingEnabled(false);
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry(const PlayerSettings & S, const OptionsTab & parent)
	: pi(std::make_unique<PlayerInfo>(SEL->getPlayerInfo(S.color.getNum())))
	, s(std::make_unique<PlayerSettings>(S))
	, parentTab(parent)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;

	int serial = 0;
	for(int g = 0; g < s->color.getNum(); ++g)
	{
		auto itred = SEL->getPlayerInfo(g);
		if(itred.canComputerPlay || itred.canHumanPlay)
			serial++;
	}

	pos.x += 54;
	pos.y += 128 + serial * 50;

	assert(CSH->mi && CSH->mi->mapHeader);
	const PlayerInfo & p = SEL->getPlayerInfo(s->color.getNum());
	assert(p.canComputerPlay || p.canHumanPlay); //someone must be able to control this player
	if(p.canHumanPlay && p.canComputerPlay)
		whoCanPlay = HUMAN_OR_CPU;
	else if(p.canComputerPlay)
		whoCanPlay = CPU;
	else
		whoCanPlay = HUMAN;

	static const std::array<std::string, PlayerColor::PLAYER_LIMIT_I> flags =
	{{
		"AOFLGBR.DEF", "AOFLGBB.DEF", "AOFLGBY.DEF", "AOFLGBG.DEF",
		"AOFLGBO.DEF", "AOFLGBP.DEF", "AOFLGBT.DEF", "AOFLGBS.DEF"
	}};
	static const std::array<std::string, PlayerColor::PLAYER_LIMIT_I> bgs =
	{{
		"ADOPRPNL.bmp", "ADOPBPNL.bmp", "ADOPYPNL.bmp", "ADOPGPNL.bmp",
		"ADOPOPNL.bmp", "ADOPPPNL.bmp", "ADOPTPNL.bmp", "ADOPSPNL.bmp"
	}};

	background = std::make_shared<CPicture>(bgs[s->color.getNum()], 0, 0);
	labelPlayerName = std::make_shared<CLabel>(55, 10, EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, s->name);
	labelWhoCanPlay = std::make_shared<CMultiLineLabel>(Rect(6, 23, 45, (int)graphics->fonts[EFonts::FONT_TINY]->getLineHeight()*2), EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->arraytxt[206 + whoCanPlay]);

	if(SEL->screenType == ESelectionScreen::newGame)
	{
		buttonTownLeft = std::make_shared<CButton>(Point(107, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[132], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, -1, s->color));
		buttonTownRight = std::make_shared<CButton>(Point(168, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[133], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, +1, s->color));
		buttonHeroLeft = std::make_shared<CButton>(Point(183, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[148], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, -1, s->color));
		buttonHeroRight = std::make_shared<CButton>(Point(244, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[149], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, +1, s->color));
		buttonBonusLeft = std::make_shared<CButton>(Point(259, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[164], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, -1, s->color));
		buttonBonusRight = std::make_shared<CButton>(Point(320, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[165], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, +1, s->color));
	}

	hideUnavailableButtons();

	if(SEL->screenType != ESelectionScreen::scenarioInfo && SEL->getPlayerInfo(s->color.getNum()).canHumanPlay)
	{
		flag = std::make_shared<CButton>(
			Point(-43, 2),
			flags[s->color.getNum()],
			CGI->generaltexth->zelp[180],
			std::bind(&OptionsTab::onSetPlayerClicked, &parentTab, *s)
		);
		flag->hoverable = true;
		flag->block(CSH->isGuest());
	}
	else
		flag = nullptr;

	town = std::make_shared<SelectedBox>(Point(119, 2), *s, TOWN);
	hero = std::make_shared<SelectedBox>(Point(195, 2), *s, HERO);
	bonus = std::make_shared<SelectedBox>(Point(271, 2), *s, BONUS);
}

void OptionsTab::onSetPlayerClicked(const PlayerSettings & ps) const
{
	if(ps.isControlledByAI() || humanPlayers > 0)
		CSH->setPlayer(ps.color);
}

void OptionsTab::PlayerOptionsEntry::hideUnavailableButtons()
{
	if(!buttonTownLeft)
		return;

	const bool foreignPlayer = CSH->isGuest() && !CSH->isMyColor(s->color);

	if((pi->allowedFactions.size() < 2 && !pi->isFactionRandom) || foreignPlayer)
	{
		buttonTownLeft->disable();
		buttonTownRight->disable();
	}
	else
	{
		buttonTownLeft->enable();
		buttonTownRight->enable();
	}

	if((pi->defaultHero() != -1 || s->castle.getNum() < 0) //fixed hero
		|| foreignPlayer) //or not our player
	{
		buttonHeroLeft->disable();
		buttonHeroRight->disable();
	}
	else
	{
		buttonHeroLeft->enable();
		buttonHeroRight->enable();
	}

	if(foreignPlayer)
	{
		buttonBonusLeft->disable();
		buttonBonusRight->disable();
	}
	else
	{
		buttonBonusLeft->enable();
		buttonBonusRight->enable();
	}
}
