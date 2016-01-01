#include "StdInc.h"
#include "MiscWidgets.h"

#include "CComponent.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CCursorHandler.h"

#include "../CBitmapHandler.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CGameInfo.h"
#include "../windows/CAdvmapInterface.h"
#include "../windows/CCastleInterface.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/CGameState.h"

/*
 * MiscWidgets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CHoverableArea::hover (bool on)
{
	if (on)
		GH.statusbar->setText(hoverText);
	else if (GH.statusbar->getText()==hoverText)
		GH.statusbar->clear();
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
	if (!text.empty())
		adventureInt->handleRightClick(text, down);
}

LRClickableAreaWText::LRClickableAreaWText()
{
	init();
}

LRClickableAreaWText::LRClickableAreaWText(const Rect &Pos, const std::string &HoverText /*= ""*/, const std::string &ClickText /*= ""*/)
{
	init();
	pos = Pos + pos;
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
		std::vector<CComponent*> comp(1, createComponent());
		LOCPLINT->showInfoDialog(text, comp);
	}
}

LRClickableAreaWTextComp::LRClickableAreaWTextComp(const Rect &Pos, int BaseType)
	: LRClickableAreaWText(Pos), baseType(BaseType), bonusValue(-1)
{
}

CComponent * LRClickableAreaWTextComp::createComponent() const
{
	if(baseType >= 0)
		return new CComponent(CComponent::Etype(baseType), type, bonusValue);
	else
		return nullptr;
}

void LRClickableAreaWTextComp::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if(CComponent *comp = createComponent())
		{
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
			return;
		}
	}

	LRClickableAreaWText::clickRight(down, previousState); //only if with-component variant not occurred
}

CHeroArea::CHeroArea(int x, int y, const CGHeroInstance * _hero):hero(_hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += x;	pos.w = 58;
	pos.y += y;	pos.h = 64;

	if (hero)
		new CAnimImage("PortraitsLarge", hero->portrait);
}

void CHeroArea::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::clickRight(tribool down, bool previousState)
{
	if((!down) && previousState && hero)
		LOCPLINT->openHeroWindow(hero);
}

void CHeroArea::hover(bool on)
{
	if (on && hero)
		GH.statusbar->setText(hero->getObjectName());
	else
		GH.statusbar->clear();
}

void LRClickableAreaOpenTown::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState && town)
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
	if((!down) && previousState && town)
		LOCPLINT->openTownWindow(town);//TODO: popup?
}

LRClickableAreaOpenTown::LRClickableAreaOpenTown()
	: LRClickableAreaWTextComp(Rect(0,0,0,0), -1)
{
}

void CMinorResDataBar::show(SDL_Surface * to)
{
}

void CMinorResDataBar::showAll(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y,to);
	for (Res::ERes i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextCenter(to, text, Colors::WHITE, Point(pos.x + 50 + 76 * i, pos.y + pos.h/2));
	}
	std::vector<std::string> temp;

	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::MONTH)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK)));

	std::string datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
							+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";

	graphics->fonts[FONT_SMALL]->renderTextCenter(to, CSDL_Ext::processStr(datetext,temp), Colors::WHITE, Point(pos.x+545+(pos.w-545)/2,pos.y+pos.h/2));
}

CMinorResDataBar::CMinorResDataBar()
{
	bg = BitmapHandler::loadBitmap("KRESBAR.bmp");
	CSDL_Ext::setDefaultColorKey(bg);
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos.x = 7;
	pos.y = 575;
	pos.w = bg->w;
	pos.h = bg->h;
}

CMinorResDataBar::~CMinorResDataBar()
{
	SDL_FreeSurface(bg);
}

void CArmyTooltip::init(const InfoAboutArmy &army)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	new CLabel(66, 2, FONT_SMALL, TOPLEFT, Colors::WHITE, army.name);

	std::vector<Point> slotsPos;
	slotsPos.push_back(Point(36,73));
	slotsPos.push_back(Point(72,73));
	slotsPos.push_back(Point(108,73));
	slotsPos.push_back(Point(18,122));
	slotsPos.push_back(Point(54,122));
	slotsPos.push_back(Point(90,122));
	slotsPos.push_back(Point(126,122));

	for(auto & slot : army.army)
	{
		if(slot.first.getNum() >= GameConstants::ARMY_SIZE)
		{
			logGlobal->warnStream() << "Warning: " << army.name << " has stack in slot " << slot.first;
			continue;
		}

		new CAnimImage("CPRSMALL", slot.second.type->iconIndex, 0, slotsPos[slot.first.getNum()].x, slotsPos[slot.first.getNum()].y);

		std::string subtitle;
		if(army.army.isDetailed)
			subtitle = boost::lexical_cast<std::string>(slot.second.count);
		else
		{
			//if =0 - we have no information about stack size at all
			if (slot.second.count)
				subtitle = CGI->generaltexth->arraytxt[171 + 3*(slot.second.count)];
		}

		new CLabel(slotsPos[slot.first.getNum()].x + 17, slotsPos[slot.first.getNum()].y + 41, FONT_TINY, CENTER, Colors::WHITE, subtitle);
	}

}

CArmyTooltip::CArmyTooltip(Point pos, const InfoAboutArmy &army):
	CIntObject(0, pos)
{
	init(army);
}

CArmyTooltip::CArmyTooltip(Point pos, const CArmedInstance * army):
	CIntObject(0, pos)
{
	init(InfoAboutArmy(army, true));
}

void CHeroTooltip::init(const InfoAboutHero &hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CAnimImage("PortraitsLarge", hero.portrait, 0, 3, 2);

	if(hero.details)
	{
		for (size_t i = 0; i < hero.details->primskills.size(); i++)
			new CLabel(75 + 28 * i, 58, FONT_SMALL, CENTER, Colors::WHITE,
					   boost::lexical_cast<std::string>(hero.details->primskills[i]));

		new CLabel(158, 98, FONT_TINY, CENTER, Colors::WHITE,
				   boost::lexical_cast<std::string>(hero.details->mana));

		new CAnimImage("IMRL22", hero.details->morale + 3, 0, 5, 74);
		new CAnimImage("ILCK22", hero.details->luck + 3, 0, 5, 91);
	}
}

CHeroTooltip::CHeroTooltip(Point pos, const InfoAboutHero &hero):
	CArmyTooltip(pos, hero)
{
	init(hero);
}

CHeroTooltip::CHeroTooltip(Point pos, const CGHeroInstance * hero):
	CArmyTooltip(pos, InfoAboutHero(hero, true))
{
	init(InfoAboutHero(hero, true));
}

void CTownTooltip::init(const InfoAboutTown &town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	//order of icons in def: fort, citadel, castle, no fort
	size_t fortIndex = town.fortLevel ? town.fortLevel - 1 : 3;

	new CAnimImage("ITMCLS", fortIndex, 0, 105, 31);

	assert(town.tType);

	size_t iconIndex = town.tType->clientInfo.icons[town.fortLevel > 0][town.built >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	new CAnimImage("itpt", iconIndex, 0, 3, 2);

	if(town.details)
	{
		new CAnimImage("ITMTLS", town.details->hallLevel, 0, 67, 31);

		if (town.details->goldIncome)
			new CLabel(157, 58, FONT_TINY, CENTER, Colors::WHITE,
					   boost::lexical_cast<std::string>(town.details->goldIncome));

		if(town.details->garrisonedHero) //garrisoned hero icon
			new CPicture("TOWNQKGH", 149, 76);

		if(town.details->customRes)//silo is built
		{
			if (town.tType->primaryRes == Res::WOOD_AND_ORE )// wood & ore
			{
				new CAnimImage("SMALRES", Res::WOOD, 0, 7, 75);
				new CAnimImage("SMALRES", Res::ORE , 0, 7, 88);
			}
			else
				new CAnimImage("SMALRES", town.tType->primaryRes, 0, 7, 81);
		}
	}
}

CTownTooltip::CTownTooltip(Point pos, const InfoAboutTown &town):
	CArmyTooltip(pos, town)
{
	init(town);
}

CTownTooltip::CTownTooltip(Point pos, const CGTownInstance * town):
	CArmyTooltip(pos, InfoAboutTown(town, true))
{
	init(InfoAboutTown(town, true));
}


void MoraleLuckBox::set(const IBonusBearer *node)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	const int textId[] = {62, 88}; //eg %s \n\n\n {Current Luck Modifiers:}
	const int noneTxtId = 108; //Russian version uses same text for neutral morale\luck
	const int neutralDescr[] = {60, 86}; //eg {Neutral Morale} \n\n Neutral morale means your armies will neither be blessed with extra attacks or freeze in combat.
	const int componentType[] = {CComponent::luck, CComponent::morale};
	const int hoverTextBase[] = {7, 4};
	const Bonus::BonusType bonusType[] = {Bonus::LUCK, Bonus::MORALE};
	int (IBonusBearer::*getValue[])() const = {&IBonusBearer::LuckVal, &IBonusBearer::MoraleVal};
	TBonusListPtr modifierList(new BonusList());

	if (node)
	{
		modifierList = node->getBonuses(Selector::type(bonusType[morale]));
		bonusValue = (node->*getValue[morale])();
	}
	else
		bonusValue = 0;

	int mrlt = (bonusValue>0)-(bonusValue<0); //signum: -1 - bad luck / morale, 0 - neutral, 1 - good
	hoverText = CGI->generaltexth->heroscrn[hoverTextBase[morale] - mrlt];
	baseType = componentType[morale];
	text = CGI->generaltexth->arraytxt[textId[morale]];
	boost::algorithm::replace_first(text,"%s",CGI->generaltexth->arraytxt[neutralDescr[morale]-mrlt]);
	
	if (morale && node && (node->hasBonusOfType(Bonus::UNDEAD) 
			|| node->hasBonusOfType(Bonus::BLOCK_MORALE) 
			|| node->hasBonusOfType(Bonus::NON_LIVING)))
	{
		text += CGI->generaltexth->arraytxt[113]; //unaffected by morale
		bonusValue = 0;
	}
	else if(!morale && node && node->hasBonusOfType(Bonus::BLOCK_LUCK))
	{
		// TODO: there is no text like "Unaffected by luck" so probably we need own text
		text += CGI->generaltexth->arraytxt[noneTxtId];
		bonusValue = 0;
	}
	else if(modifierList->empty())
		text += CGI->generaltexth->arraytxt[noneTxtId];//no modifiers
	else
	{
		for(const Bonus * elem : *modifierList)
		{
			if(elem->val != 0)
				//no bonuses with value 0
				text += "\n" + elem->Description();
		}
	}

	std::string imageName;
	if (small)
		imageName = morale ? "IMRL30": "ILCK30";
	else
		imageName = morale ? "IMRL42" : "ILCK42";

	delete image;
	image = new CAnimImage(imageName, bonusValue + 3);
	image->moveBy(Point(pos.w/2 - image->pos.w/2, pos.h/2 - image->pos.h/2));//center icon
}

MoraleLuckBox::MoraleLuckBox(bool Morale, const Rect &r, bool Small):
	image(nullptr),
	morale(Morale),
	small(Small)
{
	bonusValue = 0;
	pos = r + pos;
}

CCreaturePic::CCreaturePic(int x, int y, const CCreature *cre, bool Big, bool Animated)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x+=x;
	pos.y+=y;

	TFaction faction = cre->faction;

	assert(CGI->townh->factions.size() > faction);

	if(Big)
		bg = new CPicture(CGI->townh->factions[faction]->creatureBg130);
	else
		bg = new CPicture(CGI->townh->factions[faction]->creatureBg120);
	anim = new CCreatureAnim(0, 0, cre->animDefName, Rect());
	anim->clipRect(cre->isDoubleWide()?170:150, 155, bg->pos.w, bg->pos.h);
	anim->startPreview(cre->hasBonusOfType(Bonus::SIEGE_WEAPON));

	amount = new CLabel(bg->pos.w, bg->pos.h, FONT_MEDIUM, BOTTOMRIGHT, Colors::WHITE);

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
}

void CCreaturePic::show(SDL_Surface *to)
{
	// redraw everything in a proper order
	bg->showAll(to);
	anim->show(to);
	amount->showAll(to);
}

void CCreaturePic::setAmount(int newAmount)
{
	if (newAmount != 0)
		amount->setText(boost::lexical_cast<std::string>(newAmount));
	else
		amount->setText("");
}
