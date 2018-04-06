/*
 * InfoWindows.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../Graphics.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"

#include "../windows/CAdvmapInterface.h"
#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"

#include "../gui/SDL_Pixels.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CCursorHandler.h"

#include "../battle/CBattleInterface.h"
#include "../battle/CBattleInterfaceClasses.h"

#include "../../CCallback.h"

#include "../../lib/CGameState.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"

void CSimpleWindow::show(SDL_Surface * to)
{
	if(bitmap)
		blitAt(bitmap,pos.x,pos.y,to);
}
CSimpleWindow::~CSimpleWindow()
{
	if (bitmap)
	{
		SDL_FreeSurface(bitmap);
		bitmap=nullptr;
	}
}

void CSelWindow::selectionChange(unsigned to)
{
	for (unsigned i=0;i<components.size();i++)
	{
		auto pom = std::dynamic_pointer_cast<CSelectableComponent>(components[i]);
		if (!pom)
			continue;
		pom->select(i==to);
	}
	redraw();
}

CSelWindow::CSelWindow(const std::string &Text, PlayerColor player, int charperline, const std::vector<std::shared_ptr<CSelectableComponent>> & comps, const std::vector<std::pair<std::string, CFunctionList<void()> > > &Buttons, QueryID askID)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	ID = askID;
	for (int i = 0; i < Buttons.size(); i++)
	{
		buttons.push_back(std::make_shared<CButton>(Point(0, 0), Buttons[i].first, CButton::tooltip(), Buttons[i].second));
		if (!i  &&  askID.getNum() >= 0)
			buttons.back()->addCallback(std::bind(&CSelWindow::madeChoice, this));
		buttons[i]->addCallback(std::bind(&CInfoWindow::close, this)); //each button will close the window apart from call-defined actions
	}

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	if (buttons.size() > 1 && askID.getNum() >= 0) //cancel button functionality
	{
		buttons.back()->addCallback([askID]() {
			LOCPLINT->cb.get()->selectionMade(0, askID);
		});
		//buttons.back()->addCallback(std::bind(&CCallback::selectionMade, LOCPLINT->cb.get(), 0, askID));
	}

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->recActions = 255-DISPOSE;
		addChild(comps[i].get());
		components.push_back(comps[i]);
		comps[i]->onSelect = std::bind(&CSelWindow::selectionChange,this,i);
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
		if(std::dynamic_pointer_cast<CSelectableComponent>(components[i])->selected)
		{
			ret = i;
		}
	}
	LOCPLINT->cb->selectionMade(ret+1,ID);
}

CInfoWindow::CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo & comps, const TButtonsInfo & Buttons)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	type |= BLOCK_ADV_HOTKEYS;
	ID = QueryID(-1);
	for(auto & Button : Buttons)
	{
		std::shared_ptr<CButton> button = std::make_shared<CButton>(Point(0,0), Button.first, CButton::tooltip(), std::bind(&CInfoWindow::close, this));
		button->setBorderColor(Colors::METALLIC_GOLD);
		button->addCallback(Button.second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);
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
		comp->recActions = 0xff & ~DISPOSE;
		addChild(comp.get());
		comp->recActions &= ~(SHOWALL | UPDATE);
		components.push_back(comp);
	}

	CMessage::drawIWindow(this,Text,player);
}

CInfoWindow::CInfoWindow()
{
	ID = QueryID(-1);
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

CInfoWindow::~CInfoWindow() = default;

void CInfoWindow::showAll(SDL_Surface * to)
{
	CSimpleWindow::show(to);
	CIntObject::showAll(to);
}

void CInfoWindow::showInfoDialog(const std::string &text, const TCompsInfo & components, PlayerColor player)
{
	CInfoWindow * window = CInfoWindow::create(text, player, components);
	GH.pushInt(window);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const TCompsInfo & components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components, pom);

	temp->buttons[0]->addCallback( onYes );
	temp->buttons[1]->addCallback( onNo );

	GH.pushInt(temp);
}

void CInfoWindow::showOkDialog(const std::string & text, const TCompsInfo & components, const std::function<void()> & onOk, PlayerColor player)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components, pom);
	temp->buttons[0]->addCallback(onOk);

	GH.pushInt(temp);
}

CInfoWindow * CInfoWindow::create(const std::string &text, PlayerColor playerID, const TCompsInfo & components)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * ret = new CInfoWindow(text, playerID, components, pom);
	return ret;
}

std::string CInfoWindow::genText(std::string title, std::string description)
{
	return std::string("{") + title + "}" + "\n\n" + description;
}

CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	init(x, y);
}


CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, const Point &p, EAlignment alignment, bool Free)
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
	if(settings["session"]["spectate"].Bool())//TODO: there must be better way to implement this
		player = PlayerColor(1);

	CSimpleWindow * temp = new CInfoWindow(txt, player, comps);
	temp->center(Point(GH.current->motion)); //center on mouse
	temp->fitToScreen(10);
	auto  rcpi = new CRClickPopupInt(temp,true);
	GH.pushInt(rcpi);
}

void CRClickPopup::createAndPush(const std::string & txt, std::shared_ptr<CComponent> component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

void CRClickPopup::createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment)
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

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah, adventureInt->selection); //todo: should this be nearest hero?

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	tooltip = std::make_shared<CTownTooltip>(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "HEROQVBK", toScreen(position))
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah, adventureInt->selection);//todo: should this be nearest hero?

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	tooltip = std::make_shared<CHeroTooltip>(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGGarrison * garr)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(garr, iah);

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	tooltip = std::make_shared<CArmyTooltip>(Point(9, 10), iah);
}

CIntObject * CRClickPopup::createInfoWin(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(nullptr == specific)
		specific = adventureInt->selection;

	if(nullptr == specific)
	{
		logGlobal->error("createInfoWin: no object to describe");
		return nullptr;
	}

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
