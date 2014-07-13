#include "StdInc.h"
#include "MiscWidgets.h"

#include "CGuiHandler.h"
#include "CCursorHandler.h"

#include "../CBitmapHandler.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CGameInfo.h"
#include "../CAdvmapInterface.h"
#include "../CCastleInterface.h"

#include "../../CCallback.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"

/*
 * MiscWidgets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CSelWindow::selectionChange(unsigned to)
{
	for (unsigned i=0;i<components.size();i++)
	{
		CSelectableComponent * pom = dynamic_cast<CSelectableComponent*>(components[i]);
		if (!pom)
			continue;
		pom->select(i==to);
	}
	redraw();
}

CSelWindow::CSelWindow(const std::string &Text, PlayerColor player, int charperline, const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string,CFunctionList<void()> > > &Buttons, QueryID askID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ID = askID;
	for(int i=0;i<Buttons.size();i++)
	{
		buttons.push_back(new CAdventureMapButton("","",Buttons[i].second,0,0,Buttons[i].first));
		if(!i  &&  askID.getNum() >= 0)
			buttons.back()->callback += boost::bind(&CSelWindow::madeChoice,this);
		buttons[i]->callback += boost::bind(&CInfoWindow::close,this); //each button will close the window apart from call-defined actions
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	if(buttons.size() > 1  &&  askID.getNum() >= 0) //cancel button functionality
		buttons.back()->callback += boost::bind(&CCallback::selectionMade,LOCPLINT->cb.get(),0,askID);

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->recActions = 255;
		addChild(comps[i]);
		components.push_back(comps[i]);
		comps[i]->onSelect = boost::bind(&CSelWindow::selectionChange,this,i);
		if(i<9)
			comps[i]->assignedKeys.insert(SDLK_1+i);
	}
	CMessage::drawIWindow(this, Text, player);
}

void CSelWindow::madeChoice()
{
	if(ID.getNum() < 0)
		return;
	int ret = -1;
	for (int i=0;i<components.size();i++)
	{
		if(dynamic_cast<CSelectableComponent*>(components[i])->selected)
		{
			ret = i;
		}
	}
	LOCPLINT->cb->selectionMade(ret+1,ID);
}

CInfoWindow::CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo &comps, const TButtonsInfo &Buttons, bool delComps)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	type |= BLOCK_ADV_HOTKEYS;
	ID = QueryID(-1);
	for(auto & Button : Buttons)
	{
		CAdventureMapButton *button = new CAdventureMapButton("","",boost::bind(&CInfoWindow::close,this),0,0,Button.first);
		button->borderColor = Colors::METALLIC_GOLD;
		button->borderEnabled = true;
		button->callback.add(Button.second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);
	if(!text->slider)
	{
		text->resize(text->label->textSize);
	}

	if(buttons.size())
	{
		buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
		buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape
	}

	for(auto & comp : comps)
	{
		comp->recActions = 0xff;
		addChild(comp);
		comp->recActions &= ~(SHOWALL | UPDATE);
		components.push_back(comp);
	}
	setDelComps(delComps);
	CMessage::drawIWindow(this,Text,player);
}

CInfoWindow::CInfoWindow()
{
	ID = QueryID(-1);
	setDelComps(false);
	text = nullptr;
}

void CInfoWindow::close()
{
	GH.popIntTotally(this);
	if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);
}

void CInfoWindow::show(SDL_Surface * to)
{
	CIntObject::show(to);
}

CInfoWindow::~CInfoWindow()
{
	if(!delComps)
	{
		for (auto & elem : components)
			removeChild(elem);
	}
}

void CInfoWindow::showAll(SDL_Surface * to)
{
	CSimpleWindow::show(to);
	CIntObject::showAll(to);
}

void CInfoWindow::showInfoDialog(const std::string &text, const std::vector<CComponent *> *components, bool DelComps, PlayerColor player)
{
	CInfoWindow * window = CInfoWindow::create(text, player, components, DelComps);
	GH.pushInt(window);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const std::vector<CComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components ? *components : std::vector<CComponent*>(), pom, DelComps);
	for(auto & elem : onYes.funcs)
		temp->buttons[0]->callback += elem;
	for(auto & elem : onNo.funcs)
		temp->buttons[1]->callback += elem;

	GH.pushInt(temp);
}

void CInfoWindow::showOkDialog(const std::string & text, const std::vector<CComponent*> *components, const boost::function<void()> & onOk, bool delComps, PlayerColor player)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, *components, pom, delComps);
	temp->buttons[0]->callback += onOk;

	GH.pushInt(temp);
}

CInfoWindow * CInfoWindow::create(const std::string &text, PlayerColor playerID /*= 1*/, const std::vector<CComponent*> *components /*= nullptr*/, bool DelComps)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * ret = new CInfoWindow(text, playerID, components ? *components : std::vector<CComponent*>(), pom, DelComps);
	return ret;
}

std::string CInfoWindow::genText(std::string title, std::string description)
{
	return std::string("{") + title + "}" + "\n\n" + description;
}

void CInfoWindow::setDelComps(bool DelComps)
{
	delComps = DelComps;
	for(CComponent *comp : components)
	{
		if(delComps)
			comp->recActions |= DISPOSE;
		else
			comp->recActions &= ~DISPOSE;
	}
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	init(x, y);
}


CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free/*=false*/)
 : free(Free),bitmap(Bitmap)
{
	switch(alignment)
	{
	case BOTTOMRIGHT:
		init(p.x - Bitmap->w, p.y - Bitmap->h);
		break;
	case CENTER:
		init(p.x - Bitmap->w/2, p.y - Bitmap->h/2);
		break;
	case TOPLEFT:
		init(p.x, p.y);
		break;
	default:
		assert(0); //not implemented
	}
}

CInfoPopup::CInfoPopup(SDL_Surface *Bitmap, bool Free)
{
	CCS->curh->hide();

	free=Free;
	bitmap=Bitmap;

	if(bitmap)
	{
		pos.x = screen->w/2 - bitmap->w/2;
		pos.y = screen->h/2 - bitmap->h/2;
		pos.h = bitmap->h;
		pos.w = bitmap->w;
	}
}

void CInfoPopup::close()
{
	if(free)
		SDL_FreeSurface(bitmap);
	GH.popIntTotally(this);
}

void CInfoPopup::show(SDL_Surface * to)
{
	blitAt(bitmap,pos.x,pos.y,to);
}

CInfoPopup::~CInfoPopup()
{
	CCS->curh->show();
}

void CInfoPopup::init(int x, int y)
{
	CCS->curh->hide();

	pos.x = x;
	pos.y = y;
	pos.h = bitmap->h;
	pos.w = bitmap->w;

	// Put the window back on screen if necessary
	vstd::amax(pos.x, 0);
	vstd::amax(pos.y, 0);
	vstd::amin(pos.x, screen->w - bitmap->w);
	vstd::amin(pos.y, screen->h - bitmap->h);
}


void CRClickPopup::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	close();
}

void CRClickPopup::close()
{
	GH.popIntTotally(this);
}

void CRClickPopup::createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps)
{
	PlayerColor player = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1); //if no player, then use blue

	CSimpleWindow * temp = new CInfoWindow(txt, player, comps);
	temp->center(Point(GH.current->motion)); //center on mouse
	temp->fitToScreen(10);
	auto  rcpi = new CRClickPopupInt(temp,true);
	GH.pushInt(rcpi);
}

void CRClickPopup::createAndPush(const std::string &txt, CComponent * component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

void CRClickPopup::createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment /*= BOTTOMRIGHT*/)
{
	CIntObject *iWin = createInfoWin(p, obj); //try get custom infowindow for this obj
	if(iWin)
		GH.pushInt(iWin);
	else
	{
		if (adventureInt->curHero())
			CRClickPopup::createAndPush(obj->getHoverText(adventureInt->curHero()));
		else
			CRClickPopup::createAndPush(obj->getHoverText(LOCPLINT->playerID));
	}
}

CRClickPopup::CRClickPopup()
{
	addUsedEvents(RCLICK);
}

CRClickPopup::~CRClickPopup()
{
}

void CRClickPopupInt::show(SDL_Surface * to)
{
	inner->show(to);
}

CRClickPopupInt::CRClickPopupInt( IShowActivatable *our, bool deleteInt )
{
	CCS->curh->hide();
	inner = our;
	delInner = deleteInt;
}

CRClickPopupInt::~CRClickPopupInt()
{
	if(delInner)
		delete inner;

	CCS->curh->show();
}

void CRClickPopupInt::showAll(SDL_Surface * to)
{
	inner->showAll(to);
}

Point CInfoBoxPopup::toScreen(Point p)
{
	vstd::abetween(p.x, adventureInt->terrain.pos.x + 100, adventureInt->terrain.pos.x + adventureInt->terrain.pos.w - 100);
	vstd::abetween(p.y, adventureInt->terrain.pos.y + 100, adventureInt->terrain.pos.y + adventureInt->terrain.pos.h - 100);

	return p;
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CTownTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "HEROQVBK", toScreen(position))
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CHeroTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGGarrison * garr):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(garr, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CArmyTooltip(Point(9, 10), iah);
}

CIntObject * CRClickPopup::createInfoWin(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(!specific)
		specific = adventureInt->selection;

	assert(specific);

	switch(specific->ID)
	{
	case Obj::HERO:
		return new CInfoBoxPopup(position, dynamic_cast<const CGHeroInstance *>(specific));
	case Obj::TOWN:
		return new CInfoBoxPopup(position, dynamic_cast<const CGTownInstance *>(specific));
	case Obj::GARRISON:
	case Obj::GARRISON2:
		return new CInfoBoxPopup(position, dynamic_cast<const CGGarrison *>(specific));
	default:
		return nullptr;
	}
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

	int mrlt = -9;
	TModDescr mrl;

	if (node)
	{
		node->getModifiersWDescr(mrl, bonusType[morale]);
		bonusValue = (node->*getValue[morale])();
	}
	else
		bonusValue = 0;

	mrlt = (bonusValue>0)-(bonusValue<0); //signum: -1 - bad luck / morale, 0 - neutral, 1 - good
	hoverText = CGI->generaltexth->heroscrn[hoverTextBase[morale] - mrlt];
	baseType = componentType[morale];
	text = CGI->generaltexth->arraytxt[textId[morale]];
	boost::algorithm::replace_first(text,"%s",CGI->generaltexth->arraytxt[neutralDescr[morale]-mrlt]);
	if (!mrl.size())
		text += CGI->generaltexth->arraytxt[noneTxtId];
	else
	{
		//it's a creature window
		if ((morale && node->hasBonusOfType(Bonus::UNDEAD)) ||
			node->hasBonusOfType(Bonus::BLOCK_MORALE) || node->hasBonusOfType(Bonus::NON_LIVING))
		{
			text += CGI->generaltexth->arraytxt[113]; //unaffected by morale
		}
		else
		{
			for(auto & elem : mrl)
			{
				if (elem.first) //no bonuses with value 0
					text += "\n" + elem.second;
			}
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
	bg->needRefresh = true;
	anim = new CCreatureAnim(0, 0, cre->animDefName, Rect());
	anim->clipRect(cre->isDoubleWide()?170:150, 155, bg->pos.w, bg->pos.h);
	anim->startPreview(cre->hasBonusOfType(Bonus::SIEGE_WEAPON));

	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
}
