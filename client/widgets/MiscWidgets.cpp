/*
 * MiscWidgets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MiscWidgets.h"

#include "CComponent.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"

#include "../CPlayerInterface.h"
#include "../CGameInfo.h"
#include "../widgets/TextControls.h"
#include "../windows/CCastleInterface.h"
#include "../windows/InfoWindows.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/TextOperations.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

void CHoverableArea::hover (bool on)
{
	if (on)
		GH.statusbar->write(hoverText);
	else
		GH.statusbar->clearIfMatching(hoverText);
}

CHoverableArea::CHoverableArea()
{
	addUsedEvents(HOVER);
}

CHoverableArea::~CHoverableArea()
{
}

void LRClickableAreaWText::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState && !text.empty())
	{
		LOCPLINT->showInfoDialog(text);
	}
}
void LRClickableAreaWText::clickRight(tribool down, bool previousState)
{
	if (down && !text.empty())
		CRClickPopup::createAndPush(text);
}

LRClickableAreaWText::LRClickableAreaWText()
{
	init();
}

LRClickableAreaWText::LRClickableAreaWText(const Rect &Pos, const std::string &HoverText, const std::string &ClickText)
{
	init();
	pos = Pos + pos.topLeft();
	hoverText = HoverText;
	text = ClickText;
}

LRClickableAreaWText::~LRClickableAreaWText()
{
}

void LRClickableAreaWText::init()
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
}

void LRClickableAreaWTextComp::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		std::vector<std::shared_ptr<CComponent>> comp(1, createComponent());
		LOCPLINT->showInfoDialog(text, comp);
	}
}

LRClickableAreaWTextComp::LRClickableAreaWTextComp(const Rect &Pos, int BaseType)
	: LRClickableAreaWText(Pos), baseType(BaseType), bonusValue(-1)
{
	type = -1;
}

std::shared_ptr<CComponent> LRClickableAreaWTextComp::createComponent() const
{
	if(baseType >= 0)
		return std::make_shared<CComponent>(CComponent::Etype(baseType), type, bonusValue);
	else
		return std::shared_ptr<CComponent>();
}

void LRClickableAreaWTextComp::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(auto comp = createComponent())
		{
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
			return;
		}
	}

	LRClickableAreaWText::clickRight(down, previousState); //only if with-component variant not occurred
}

CHeroArea::CHeroArea(int x, int y, const CGHeroInstance * _hero)
	: CIntObject(LCLICK | RCLICK | HOVER),
	hero(_hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	pos.x += x;
	pos.w = 58;
	pos.y += y;
	pos.h = 64;

	if(hero)
		portrait = std::make_shared<CAnimImage>("PortraitsLarge", hero->portrait);
}

void CHeroArea::clickLeft(tribool down, bool previousState)
{
	if(hero && (!down) && previousState)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::clickRight(tribool down, bool previousState)
{
	if(hero && (!down) && previousState)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::hover(bool on)
{
	if (on && hero)
		GH.statusbar->write(hero->getObjectName());
	else
		GH.statusbar->clear();
}

void LRClickableAreaOpenTown::clickLeft(tribool down, bool previousState)
{
	if(town && (!down) && previousState)
	{
		LOCPLINT->openTownWindow(town);
		if ( type == 2 )
			LOCPLINT->castleInt->builds->buildingClicked(BuildingID::VILLAGE_HALL);
		else if ( type == 3 && town->fortLevel() )
			LOCPLINT->castleInt->builds->buildingClicked(BuildingID::FORT);
	}
}

void LRClickableAreaOpenTown::clickRight(tribool down, bool previousState)
{
	if(town && (!down) && previousState)
		LOCPLINT->openTownWindow(town);//TODO: popup?
}

LRClickableAreaOpenTown::LRClickableAreaOpenTown(const Rect & Pos, const CGTownInstance * Town)
	: LRClickableAreaWTextComp(Pos, -1), town(Town)
{
}

void CMinorResDataBar::show(SDL_Surface * to)
{
}

std::string CMinorResDataBar::buildDateString()
{
	std::string pattern = "%s: %d, %s: %d, %s: %d";

	auto formatted = boost::format(pattern)
		% CGI->generaltexth->translate("core.genrltxt.62") % LOCPLINT->cb->getDate(Date::MONTH)
		% CGI->generaltexth->translate("core.genrltxt.63") % LOCPLINT->cb->getDate(Date::WEEK)
		% CGI->generaltexth->translate("core.genrltxt.64") % LOCPLINT->cb->getDate(Date::DAY_OF_WEEK);

	return boost::str(formatted);
}

void CMinorResDataBar::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	for (Res::ERes i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = std::to_string(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextCenter(to, text, Colors::WHITE, Point(pos.x + 50 + 76 * i, pos.y + pos.h/2));
	}
	graphics->fonts[FONT_SMALL]->renderTextCenter(to, buildDateString(), Colors::WHITE, Point(pos.x+545+(pos.w-545)/2,pos.y+pos.h/2));
}

CMinorResDataBar::CMinorResDataBar()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	pos.x = 7;
	pos.y = 575;

	background = std::make_shared<CPicture>("KRESBAR.bmp");
	background->colorize(LOCPLINT->playerID);

	pos.w = background->pos.w;
	pos.h = background->pos.h;
}

CMinorResDataBar::~CMinorResDataBar() = default;

void CArmyTooltip::init(const InfoAboutArmy &army)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	title = std::make_shared<CLabel>(66, 2, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, army.name);

	std::vector<Point> slotsPos;
	slotsPos.push_back(Point(36, 73));
	slotsPos.push_back(Point(72, 73));
	slotsPos.push_back(Point(108, 73));
	slotsPos.push_back(Point(18, 122));
	slotsPos.push_back(Point(54, 122));
	slotsPos.push_back(Point(90, 122));
	slotsPos.push_back(Point(126, 122));

	for(auto & slot : army.army)
	{
		if(slot.first.getNum() >= GameConstants::ARMY_SIZE)
		{
			logGlobal->warn("%s has stack in slot %d", army.name, slot.first.getNum());
			continue;
		}

		icons.push_back(std::make_shared<CAnimImage>("CPRSMALL", slot.second.type->getIconIndex(), 0, slotsPos[slot.first.getNum()].x, slotsPos[slot.first.getNum()].y));

		std::string subtitle;
		if(army.army.isDetailed)
		{
			subtitle = TextOperations::formatMetric(slot.second.count, 4);
		}
		else
		{
			//if =0 - we have no information about stack size at all
			if(slot.second.count)
			{
				if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
				{
					subtitle = CCreature::getQuantityRangeStringForId((CCreature::CreatureQuantityId)slot.second.count);
				}
				else
				{
					subtitle = CGI->generaltexth->arraytxt[171 + 3*(slot.second.count)];
				}
			}
		}

		subtitles.push_back(std::make_shared<CLabel>(slotsPos[slot.first.getNum()].x + 17, slotsPos[slot.first.getNum()].y + 39, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, subtitle));
	}

}

CArmyTooltip::CArmyTooltip(Point pos, const InfoAboutArmy & army):
	CIntObject(0, pos)
{
	init(army);
}

CArmyTooltip::CArmyTooltip(Point pos, const CArmedInstance * army):
	CIntObject(0, pos)
{
	init(InfoAboutArmy(army, true));
}

void CHeroTooltip::init(const InfoAboutHero & hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	portrait = std::make_shared<CAnimImage>("PortraitsLarge", hero.portrait, 0, 3, 2);

	if(hero.details)
	{
		for(size_t i = 0; i < hero.details->primskills.size(); i++)
			labels.push_back(std::make_shared<CLabel>(75 + 28 * (int)i, 58, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
					   std::to_string(hero.details->primskills[i])));

		labels.push_back(std::make_shared<CLabel>(158, 98, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, std::to_string(hero.details->mana)));

		morale = std::make_shared<CAnimImage>("IMRL22", hero.details->morale + 3, 0, 5, 74);
		luck = std::make_shared<CAnimImage>("ILCK22", hero.details->luck + 3, 0, 5, 91);
	}
}

CHeroTooltip::CHeroTooltip(Point pos, const InfoAboutHero &hero):
	CArmyTooltip(pos, hero)
{
	init(hero);
}

CHeroTooltip::CHeroTooltip(Point pos, const CGHeroInstance * hero):
	CArmyTooltip(pos, InfoAboutHero(hero, InfoAboutHero::EInfoLevel::DETAILED))
{
	init(InfoAboutHero(hero, InfoAboutHero::EInfoLevel::DETAILED));
}

void CTownTooltip::init(const InfoAboutTown & town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	//order of icons in def: fort, citadel, castle, no fort
	size_t fortIndex = town.fortLevel ? town.fortLevel - 1 : 3;

	fort = std::make_shared<CAnimImage>("ITMCLS", fortIndex, 0, 105, 31);

	assert(town.tType);

	size_t iconIndex = town.tType->clientInfo.icons[town.fortLevel > 0][town.built >= CGI->settings()->getInteger(EGameSettings::INT_MAX_BUILDING_PER_TURN)];

	build = std::make_shared<CAnimImage>("itpt", iconIndex, 0, 3, 2);

	if(town.details)
	{
		fort = std::make_shared<CAnimImage>("ITMTLS", town.details->hallLevel, 0, 67, 31);

		if(town.details->goldIncome)
		{
			income = std::make_shared<CLabel>(157, 58, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE,
					   std::to_string(town.details->goldIncome));
		}
		if(town.details->garrisonedHero) //garrisoned hero icon
			garrisonedHero = std::make_shared<CPicture>("TOWNQKGH", 149, 76);

		if(town.details->customRes)//silo is built
		{
			if(town.tType->primaryRes == Res::WOOD_AND_ORE )// wood & ore
			{
				res1 = std::make_shared<CAnimImage>("SMALRES", Res::WOOD, 0, 7, 75);
				res2 = std::make_shared<CAnimImage>("SMALRES", Res::ORE , 0, 7, 88);
			}
			else
			{
				res1 = std::make_shared<CAnimImage>("SMALRES", town.tType->primaryRes, 0, 7, 81);
			}
		}
	}
}

CTownTooltip::CTownTooltip(Point pos, const InfoAboutTown & town)
	: CArmyTooltip(pos, town)
{
	init(town);
}

CTownTooltip::CTownTooltip(Point pos, const CGTownInstance * town)
	: CArmyTooltip(pos, InfoAboutTown(town, true))
{
	init(InfoAboutTown(town, true));
}

void MoraleLuckBox::set(const IBonusBearer * node)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	const int textId[] = {62, 88}; //eg %s \n\n\n {Current Luck Modifiers:}
	const int noneTxtId = 108; //Russian version uses same text for neutral morale\luck
	const int neutralDescr[] = {60, 86}; //eg {Neutral Morale} \n\n Neutral morale means your armies will neither be blessed with extra attacks or freeze in combat.
	const int componentType[] = {CComponent::luck, CComponent::morale};
	const int hoverTextBase[] = {7, 4};
	TConstBonusListPtr modifierList = std::make_shared<const BonusList>();
	bonusValue = 0;

	if(node)
		bonusValue = morale ? node->MoraleValAndBonusList(modifierList) : node->LuckValAndBonusList(modifierList);

	int mrlt = (bonusValue>0)-(bonusValue<0); //signum: -1 - bad luck / morale, 0 - neutral, 1 - good
	hoverText = CGI->generaltexth->heroscrn[hoverTextBase[morale] - mrlt];
	baseType = componentType[morale];
	text = CGI->generaltexth->arraytxt[textId[morale]];
	boost::algorithm::replace_first(text,"%s",CGI->generaltexth->arraytxt[neutralDescr[morale]-mrlt]);

	if (morale && node && (node->hasBonusOfType(Bonus::UNDEAD)
			|| node->hasBonusOfType(Bonus::NON_LIVING)))
	{
		text += CGI->generaltexth->arraytxt[113]; //unaffected by morale
		bonusValue = 0;
	}
	else if(morale && node && node->hasBonusOfType(Bonus::NO_MORALE))
	{
		auto noMorale = node->getBonus(Selector::type()(Bonus::NO_MORALE));
		text += "\n" + noMorale->Description();
		bonusValue = 0;
	}
	else if (!morale && node && node->hasBonusOfType(Bonus::NO_LUCK))
	{
		auto noLuck = node->getBonus(Selector::type()(Bonus::NO_LUCK));
		text += "\n" + noLuck->Description();
		bonusValue = 0;
	}
	else
	{
		std::string addInfo = "";
		for(auto & bonus : * modifierList)
		{
			if(bonus->val)
				addInfo += "\n" + bonus->Description();
		}
		text = addInfo.empty() 
			? text + CGI->generaltexth->arraytxt[noneTxtId] 
			: text + addInfo;
	}
	std::string imageName;
	if (small)
		imageName = morale ? "IMRL30": "ILCK30";
	else
		imageName = morale ? "IMRL42" : "ILCK42";

	image = std::make_shared<CAnimImage>(imageName, bonusValue + 3);
	image->moveBy(Point(pos.w/2 - image->pos.w/2, pos.h/2 - image->pos.h/2));//center icon
}

MoraleLuckBox::MoraleLuckBox(bool Morale, const Rect &r, bool Small)
	: morale(Morale),
	small(Small)
{
	bonusValue = 0;
	pos = r + pos.topLeft();
	defActions = 255-DISPOSE;
}

CCreaturePic::CCreaturePic(int x, int y, const CCreature * cre, bool Big, bool Animated)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.x+=x;
	pos.y+=y;

	TFaction faction = cre->faction;

	assert(CGI->townh->size() > faction);

	if(Big)
		bg = std::make_shared<CPicture>((*CGI->townh)[faction]->creatureBg130);
	else
		bg = std::make_shared<CPicture>((*CGI->townh)[faction]->creatureBg120);
	anim = std::make_shared<CCreatureAnim>(0, 0, cre->animDefName);
	anim->clipRect(cre->isDoubleWide()?170:150, 155, bg->pos.w, bg->pos.h);
	anim->startPreview(cre->hasBonusOfType(Bonus::SIEGE_WEAPON));

	amount = std::make_shared<CLabel>(bg->pos.w, bg->pos.h, FONT_MEDIUM, ETextAlignment::BOTTOMRIGHT, Colors::WHITE);

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
}

void CCreaturePic::show(SDL_Surface * to)
{
	// redraw everything in a proper order
	bg->showAll(to);
	anim->show(to);
	amount->showAll(to);
}

void CCreaturePic::setAmount(int newAmount)
{
	if(newAmount != 0)
		amount->setText(std::to_string(newAmount));
	else
		amount->setText("");
}
