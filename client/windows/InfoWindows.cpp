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

#include "../CGameInfo.h"
#include "../PlayerLocalState.h"
#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"

#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../gui/CGuiHandler.h"
#include "../battle/BattleInterface.h"
#include "../battle/BattleInterfaceClasses.h"
#include "../adventureMap/CAdventureMapInterface.h"
#include "../windows/CMessage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"

#include "../../CCallback.h"

#include "../../lib/CGameState.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"

#include <SDL_surface.h>

void CSimpleWindow::show(SDL_Surface * to)
{
	if(bitmap)
		CSDL_Ext::blitAt(bitmap,pos.x,pos.y,to);
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

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);

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
		if(i<8)
			comps[i]->assignedKey = vstd::next(EShortcut::SELECT_INDEX_1,i);
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

	text = std::make_shared<CTextBox>(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
	if(!text->slider)
	{
		text->resize(text->label->textSize);
	}

	if(buttons.size() == 1)
		buttons.front()->assignedKey = EShortcut::GLOBAL_RETURN;

	if(buttons.size() == 2)
	{
		buttons.front()->assignedKey = EShortcut::GLOBAL_ACCEPT;
		buttons.back()->assignedKey = EShortcut::GLOBAL_CANCEL;
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
	WindowBase::close();

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
	GH.pushInt(CInfoWindow::create(text, player, components));
}

void CInfoWindow::showYesNoDialog(const std::string & text, const TCompsInfo & components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	std::shared_ptr<CInfoWindow> temp =  std::make_shared<CInfoWindow>(text, player, components, pom);

	temp->buttons[0]->addCallback( onYes );
	temp->buttons[1]->addCallback( onNo );

	GH.pushInt(temp);
}

std::shared_ptr<CInfoWindow> CInfoWindow::create(const std::string &text, PlayerColor playerID, const TCompsInfo & components)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	return std::make_shared<CInfoWindow>(text, playerID, components, pom);
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


CInfoPopup::CInfoPopup(SDL_Surface * Bitmap, const Point &p, ETextAlignment alignment, bool Free)
 : free(Free),bitmap(Bitmap)
{
	switch(alignment)
	{
	case ETextAlignment::BOTTOMRIGHT:
		init(p.x - Bitmap->w, p.y - Bitmap->h);
		break;
	case ETextAlignment::CENTER:
		init(p.x - Bitmap->w/2, p.y - Bitmap->h/2);
		break;
	case ETextAlignment::TOPLEFT:
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
		pos.x = GH.screenDimensions().x / 2 - bitmap->w / 2;
		pos.y = GH.screenDimensions().y / 2 - bitmap->h / 2;
		pos.h = bitmap->h;
		pos.w = bitmap->w;
	}
}

void CInfoPopup::close()
{
	if(free)
		SDL_FreeSurface(bitmap);
	WindowBase::close();
}

void CInfoPopup::show(SDL_Surface * to)
{
	CSDL_Ext::blitAt(bitmap,pos.x,pos.y,to);
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
	vstd::amin(pos.x, GH.screenDimensions().x - bitmap->w);
	vstd::amin(pos.y, GH.screenDimensions().y - bitmap->h);
}


void CRClickPopup::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	close();
}

void CRClickPopup::close()
{
	WindowBase::close();
}

void CRClickPopup::createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps)
{
	PlayerColor player = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1); //if no player, then use blue
	if(settings["session"]["spectate"].Bool())//TODO: there must be better way to implement this
		player = PlayerColor(1);

	auto temp = std::make_shared<CInfoWindow>(txt, player, comps);
	temp->center(GH.getCursorPosition()); //center on mouse
#ifdef VCMI_IOS
    // TODO: enable also for android?
    temp->moveBy({0, -temp->pos.h / 2});
#endif
	temp->fitToScreen(10);

	GH.pushIntT<CRClickPopupInt>(temp);
}

void CRClickPopup::createAndPush(const std::string & txt, std::shared_ptr<CComponent> component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

void CRClickPopup::createAndPush(const CGObjectInstance * obj, const Point & p, ETextAlignment alignment)
{
	auto iWin = createInfoWin(p, obj); //try get custom infowindow for this obj
	if(iWin)
	{
		GH.pushInt(iWin);
	}
	else
	{
		if(LOCPLINT->localState->getCurrentHero())
			CRClickPopup::createAndPush(obj->getHoverText(LOCPLINT->localState->getCurrentHero()));
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

CRClickPopupInt::CRClickPopupInt(std::shared_ptr<CIntObject> our)
{
	CCS->curh->hide();
	defActions = SHOWALL | UPDATE;
	our->recActions = defActions;
	inner = our;
	addChild(our.get(), false);
}

CRClickPopupInt::~CRClickPopupInt()
{
	CCS->curh->show();
}

Point CInfoBoxPopup::toScreen(Point p)
{
	auto bounds = adventureInt->terrainAreaPixels();

	vstd::abetween(p.x, bounds.top() + 100, bounds.bottom() - 100);
	vstd::abetween(p.y, bounds.left() + 100, bounds.right() - 100);

	return p;
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah, LOCPLINT->localState->getCurrentTown()); //todo: should this be nearest hero?

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	tooltip = std::make_shared<CTownTooltip>(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero)
	: CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "HEROQVBK", toScreen(position))
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah, LOCPLINT->localState->getCurrentHero());//todo: should this be nearest hero?

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

std::shared_ptr<WindowBase> CRClickPopup::createInfoWin(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(nullptr == specific)
		specific = LOCPLINT->localState->getCurrentArmy();

	if(nullptr == specific)
	{
		logGlobal->error("createInfoWin: no object to describe");
		return nullptr;
	}

	switch(specific->ID)
	{
	case Obj::HERO:
		return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGHeroInstance *>(specific));
	case Obj::TOWN:
		return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGTownInstance *>(specific));
	case Obj::GARRISON:
	case Obj::GARRISON2:
		return std::make_shared<CInfoBoxPopup>(position, dynamic_cast<const CGGarrison *>(specific));
	default:
		return std::shared_ptr<WindowBase>();
	}
}
