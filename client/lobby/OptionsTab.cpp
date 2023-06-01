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
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

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
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>("ADVOPTBK", 0, 6);
	pos = background->pos;
	labelTitle = std::make_shared<CLabel>(222, 30, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[515]);
	labelSubTitle = std::make_shared<CMultiLineLabel>(Rect(60, 44, 320, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[516]);

	labelPlayerNameAndHandicap = std::make_shared<CMultiLineLabel>(Rect(58, 86, 100, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[517]);
	labelStartingTown = std::make_shared<CMultiLineLabel>(Rect(163, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[518]);
	labelStartingHero = std::make_shared<CMultiLineLabel>(Rect(239, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[519]);
	labelStartingBonus = std::make_shared<CMultiLineLabel>(Rect(315, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[520]);
	if(SEL->screenType == ESelectionScreen::newGame || SEL->screenType == ESelectionScreen::loadGame || SEL->screenType == ESelectionScreen::scenarioInfo)
	{
		sliderTurnDuration = std::make_shared<CSlider>(Point(55, 551), 194, std::bind(&IServerAPI::setTurnLength, CSH, _1), 1, (int)GameConstants::POSSIBLE_TURNTIME.size(), (int)GameConstants::POSSIBLE_TURNTIME.size(), true, CSlider::BLUE);
		labelPlayerTurnDuration = std::make_shared<CLabel>(222, 538, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[521]);
		labelTurnDurationValue = std::make_shared<CLabel>(319, 559, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
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

	if(sliderTurnDuration)
	{
		sliderTurnDuration->moveTo(vstd::find_pos(GameConstants::POSSIBLE_TURNTIME, SEL->getStartInfo()->turnTime));
		labelTurnDurationValue->setText(CGI->generaltexth->turnDurations[sliderTurnDuration->getValue()]);
	}
}

size_t OptionsTab::CPlayerSettingsHelper::getImageIndex()
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;

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
			return (*CGI->townh)[factionIndex]->town->clientInfo.icons[true][false] + 2;
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
			if(settings.heroPortrait >= 0)
				return settings.heroPortrait;
			auto index = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;
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

std::string OptionsTab::CPlayerSettingsHelper::getImageName()
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return "ITPA";
	case OptionsTab::HERO:
		return "PortraitsSmall";
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
			auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
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
			auto index = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;
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
		return (settings.castle < 0) ? CGI->generaltexth->allTexts[103] : CGI->generaltexth->allTexts[80];
	case OptionsTab::HERO:
		return (settings.hero < 0) ? CGI->generaltexth->allTexts[101] : CGI->generaltexth->allTexts[77];
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
	auto heroIndex = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;

	switch(type)
	{
	case TOWN:
		return getName();
	case HERO:
	{
		if(settings.hero >= 0)
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;

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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
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
	auto heroIndex = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;

	imageSpeciality = std::make_shared<CAnimImage>("UN44", (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());
}

void OptionsTab::CPlayerOptionTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	textBonusDescription = std::make_shared<CTextBox>(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & settings, SelType type)
	: CIntObject(RCLICK, position), CPlayerSettingsHelper(settings, type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex());
	subtitle = std::make_shared<CLabel>(23, 39, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, getName());

	pos = image->pos;
}

void OptionsTab::SelectedBox::update()
{
	image->setFrame(getImageIndex());
	subtitle->setText(getName());
}

void OptionsTab::SelectedBox::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		// cases when we do not need to display a message
		if(settings.castle == -2 && CPlayerSettingsHelper::type == TOWN)
			return;
		if(settings.hero == -2 && !SEL->getPlayerInfo(settings.color.getNum()).hasCustomMainHero() && CPlayerSettingsHelper::type == HERO)
			return;

		GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(*this);
	}
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
	pos.y += 122 + serial * 50;

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

	if((pi->defaultHero() != -1 || s->castle < 0) //fixed hero
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
