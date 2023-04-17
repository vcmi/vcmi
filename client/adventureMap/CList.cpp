/*
 * CList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CList.h"

#include "CAdventureMapInterface.h"

#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../gui/CGuiHandler.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CList::CListItem::CListItem(CList * Parent)
	: CIntObject(LCLICK | RCLICK | HOVER),
	parent(Parent),
	selection()
{
	defActions = 255-DISPOSE;
}

CList::CListItem::~CListItem()
{
}

void CList::CListItem::clickRight(tribool down, bool previousState)
{
	if (down == true)
		showTooltip();
}

void CList::CListItem::clickLeft(tribool down, bool previousState)
{
	if(down == true)
	{
		//second click on already selected item
		if(parent->selected == this->shared_from_this())
		{
			open();
		}
		else
		{
			//first click - switch selection
			parent->select(this->shared_from_this());
		}
	}
}

void CList::CListItem::hover(bool on)
{
	if (on)
		GH.statusbar->write(getHoverText());
	else
		GH.statusbar->clear();
}

void CList::CListItem::onSelect(bool on)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	selection.reset();
	if(on)
		selection = genSelection();
	select(on);
	redraw();
}

CList::CList(int Size, Point position, std::string btnUp, std::string btnDown, size_t listAmount, int helpUp, int helpDown, CListBox::CreateFunc create)
	: CIntObject(0, position),
	size(Size),
	selected(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	scrollUp = std::make_shared<CButton>(Point(0, 0), btnUp, CGI->generaltexth->zelp[helpUp]);
	scrollDown = std::make_shared<CButton>(Point(0, scrollUp->pos.h + 32*(int)size), btnDown, CGI->generaltexth->zelp[helpDown]);

	listBox = std::make_shared<CListBox>(create, Point(1,scrollUp->pos.h), Point(0, 32), size, listAmount);

	//assign callback only after list was created
	scrollUp->addCallback(std::bind(&CListBox::moveToPrev, listBox));
	scrollDown->addCallback(std::bind(&CListBox::moveToNext, listBox));

	scrollUp->addCallback(std::bind(&CList::update, this));
	scrollDown->addCallback(std::bind(&CList::update, this));

	update();
}

void CList::update()
{
	bool onTop = listBox->getPos() == 0;
	bool onBottom = listBox->getPos() + size >= listBox->size();

	scrollUp->block(onTop);
	scrollDown->block(onBottom);
}

void CList::select(std::shared_ptr<CListItem> which)
{
	if(selected == which)
		return;

	if(selected)
		selected->onSelect(false);

	selected = which;
	if(which)
	{
		which->onSelect(true);
		onSelect();
	}
}

int CList::getSelectedIndex()
{
	return static_cast<int>(listBox->getIndexOf(selected));
}

void CList::selectIndex(int which)
{
	if(which < 0)
	{
		if(selected)
			select(nullptr);
	}
	else
	{
		listBox->scrollTo(which);
		update();
		select(std::dynamic_pointer_cast<CListItem>(listBox->getItem(which)));
	}
}

void CList::selectNext()
{
	int index = getSelectedIndex() + 1;
	if(index >= listBox->size())
		index = 0;
	selectIndex(index);
}

void CList::selectPrev()
{
	int index = getSelectedIndex();
	if(index <= 0)
		selectIndex(0);
	else
		selectIndex(index-1);
}

CHeroList::CEmptyHeroItem::CEmptyHeroItem()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	movement = std::make_shared<CAnimImage>("IMOBIL", 0, 0, 0, 1);
	portrait = std::make_shared<CPicture>("HPSXXX", movement->pos.w + 1, 0);
	mana = std::make_shared<CAnimImage>("IMANA", 0, 0, movement->pos.w + portrait->pos.w + 2, 1 );

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);
}

CHeroList::CHeroItem::CHeroItem(CHeroList *parent, const CGHeroInstance * Hero)
	: CListItem(parent),
	hero(Hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	movement = std::make_shared<CAnimImage>("IMOBIL", 0, 0, 0, 1);
	portrait = std::make_shared<CAnimImage>("PortraitsSmall", hero->portrait, 0, movement->pos.w + 1);
	mana = std::make_shared<CAnimImage>("IMANA", 0, 0, movement->pos.w + portrait->pos.w + 2, 1);

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);

	update();
}

void CHeroList::CHeroItem::update()
{
	movement->setFrame(std::min<size_t>(movement->size()-1, hero->movement / 100));
	mana->setFrame(std::min<size_t>(mana->size()-1, hero->mana / 5));
	redraw();
}

std::shared_ptr<CIntObject> CHeroList::CHeroItem::genSelection()
{
	return std::make_shared<CPicture>("HPSYYY", movement->pos.w + 1, 0);
}

void CHeroList::CHeroItem::select(bool on)
{
	if(on && LOCPLINT->localState->getCurrentHero() != hero)
		LOCPLINT->setSelection(hero);
}

void CHeroList::CHeroItem::open()
{
	LOCPLINT->openHeroWindow(hero);
}

void CHeroList::CHeroItem::showTooltip()
{
	CRClickPopup::createAndPush(hero, GH.getCursorPosition());
}

std::string CHeroList::CHeroItem::getHoverText()
{
	return boost::str(boost::format(CGI->generaltexth->allTexts[15]) % hero->getNameTranslated() % hero->type->heroClass->getNameTranslated());
}

std::shared_ptr<CIntObject> CHeroList::createHeroItem(size_t index)
{
	if (LOCPLINT->localState->getWanderingHeroes().size() > index)
		return std::make_shared<CHeroItem>(this, LOCPLINT->localState->getWanderingHero(index));
	return std::make_shared<CEmptyHeroItem>();
}

CHeroList::CHeroList(int size, Point position, std::string btnUp, std::string btnDown):
	CList(size, position, btnUp, btnDown, LOCPLINT->localState->getWanderingHeroes().size(), 303, 304, std::bind(&CHeroList::createHeroItem, this, _1))
{
}

void CHeroList::select(const CGHeroInstance * hero)
{
	selectIndex(vstd::find_pos(LOCPLINT->localState->getWanderingHeroes(), hero));
}

void CHeroList::update(const CGHeroInstance * hero)
{
	//this hero is already present, update its status
	for(auto & elem : listBox->getItems())
	{
		auto item = std::dynamic_pointer_cast<CHeroItem>(elem);
		if(item && item->hero == hero && vstd::contains(LOCPLINT->localState->getWanderingHeroes(), hero))
		{
			item->update();
			return;
		}
	}
	//simplest solution for now: reset list and restore selection

	listBox->resize(LOCPLINT->localState->getWanderingHeroes().size());
	if (LOCPLINT->localState->getCurrentHero())
		select(LOCPLINT->localState->getCurrentHero());

	CList::update();
}

std::shared_ptr<CIntObject> CTownList::createTownItem(size_t index)
{
	if (LOCPLINT->localState->ownedTowns.size() > index)
		return std::make_shared<CTownItem>(this, LOCPLINT->localState->ownedTowns[index]);
	return std::make_shared<CAnimImage>("ITPA", 0);
}

CTownList::CTownItem::CTownItem(CTownList *parent, const CGTownInstance *Town):
	CListItem(parent),
	town(Town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	picture = std::make_shared<CAnimImage>("ITPA", 0);
	pos = picture->pos;
	update();
}

std::shared_ptr<CIntObject> CTownList::CTownItem::genSelection()
{
	return std::make_shared<CAnimImage>("ITPA", 1);
}

void CTownList::CTownItem::update()
{
	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->settings()->getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP)];

	picture->setFrame(iconIndex + 2);
	redraw();
}

void CTownList::CTownItem::select(bool on)
{
	if (on && LOCPLINT->localState->getCurrentTown() != town)
		LOCPLINT->localState->setSelection(town);
}

void CTownList::CTownItem::open()
{
	LOCPLINT->openTownWindow(town);
}

void CTownList::CTownItem::showTooltip()
{
	CRClickPopup::createAndPush(town, GH.getCursorPosition());
}

std::string CTownList::CTownItem::getHoverText()
{
	return town->getObjectName();
}

CTownList::CTownList(int size, Point position, std::string btnUp, std::string btnDown):
	CList(size, position, btnUp, btnDown, LOCPLINT->localState->ownedTowns.size(),  306, 307, std::bind(&CTownList::createTownItem, this, _1))
{
}

void CTownList::select(const CGTownInstance * town)
{
	selectIndex(vstd::find_pos(LOCPLINT->localState->ownedTowns, town));
}

void CTownList::update(const CGTownInstance *)
{
	//simplest solution for now: reset list and restore selection

	listBox->resize(LOCPLINT->localState->ownedTowns.size());
	if (LOCPLINT->localState->getCurrentTown())
		select(LOCPLINT->localState->getCurrentTown());

	CList::update();
}

