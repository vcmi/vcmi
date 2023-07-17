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

#include "AdventureMapInterface.h"

#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/ObjectLists.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../gui/CGuiHandler.h"
#include "../render/Canvas.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/GameSettings.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CList::CListItem::CListItem(CList * Parent)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER),
	parent(Parent),
	selection()
{
	defActions = 255-DISPOSE;
}

CList::CListItem::~CListItem() = default;

void CList::CListItem::showPopupWindow(const Point & cursorPosition)
{
	showTooltip();
}

void CList::CListItem::clickPressed(const Point & cursorPosition)
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

void CList::CListItem::hover(bool on)
{
	if (on)
		GH.statusbar()->write(getHoverText());
	else
		GH.statusbar()->clear();
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

CList::CList(int Size, Rect widgetDimensions)
	: Scrollable(0, widgetDimensions.topLeft(), Orientation::VERTICAL),
	size(Size),
	selected(nullptr)
{
	pos.w = widgetDimensions.w;
	pos.h = widgetDimensions.h;
}

void CList::showAll(Canvas & to)
{
	to.drawColor(pos, Colors::BLACK);
	CIntObject::showAll(to);
}

void CList::createList(Point firstItemPosition, Point itemPositionDelta, size_t listAmount)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	listBox = std::make_shared<CListBox>(std::bind(&CList::createItem, this, _1), firstItemPosition, itemPositionDelta, size, listAmount);
}

void CList::setScrollUpButton(std::shared_ptr<CButton> button)
{
	addChild(button.get());

	scrollUp = button;
	scrollUp->addCallback(std::bind(&CList::scrollPrev, this));
	update();
}

void CList::setScrollDownButton(std::shared_ptr<CButton> button)
{
	addChild(button.get());

	scrollDown = button;
	scrollDown->addCallback(std::bind(&CList::scrollNext, this));
	update();
}

void CList::scrollBy(int distance)
{
	if (distance < 0 && listBox->getPos() < -distance)
		listBox->moveToPos(0);
	else
		listBox->moveToPos(static_cast<int>(listBox->getPos()) + distance);

	update();
}

void CList::scrollPrev()
{
	listBox->moveToPrev();
	update();
}

void CList::scrollNext()
{
	listBox->moveToNext();
	update();
}

void CList::update()
{
	bool onTop = listBox->getPos() == 0;
	bool onBottom = listBox->getPos() + size >= listBox->size();

	if (scrollUp)
		scrollUp->block(onTop);

	if (scrollDown)
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
	movement->setFrame(std::min<size_t>(movement->size()-1, hero->movementPointsRemaining() / 100));
	mana->setFrame(std::min<size_t>(mana->size()-1, hero->mana / 5));
	redraw();
}

std::shared_ptr<CIntObject> CHeroList::CHeroItem::genSelection()
{
	return std::make_shared<CPicture>("HPSYYY", movement->pos.w + 1, 0);
}

void CHeroList::CHeroItem::select(bool on)
{
	if(on)
		LOCPLINT->localState->setSelection(hero);
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

std::shared_ptr<CIntObject> CHeroList::createItem(size_t index)
{
	if (LOCPLINT->localState->getWanderingHeroes().size() > index)
		return std::make_shared<CHeroItem>(this, LOCPLINT->localState->getWanderingHero(index));
	return std::make_shared<CEmptyHeroItem>();
}

CHeroList::CHeroList(int visibleItemsCount, Rect widgetPosition, Point firstItemOffset, Point itemOffsetDelta, size_t initialItemsCount)
	: CList(visibleItemsCount, widgetPosition)
{
	createList(firstItemOffset, itemOffsetDelta, initialItemsCount);
}

void CHeroList::select(const CGHeroInstance * hero)
{
	selectIndex(vstd::find_pos(LOCPLINT->localState->getWanderingHeroes(), hero));
}

void CHeroList::updateElement(const CGHeroInstance * hero)
{
	updateWidget();
}

void CHeroList::updateWidget()
{
	auto & heroes = LOCPLINT->localState->getWanderingHeroes();

	listBox->resize(heroes.size());

	for (size_t i = 0; i < heroes.size(); ++i)
	{
		auto item =  std::dynamic_pointer_cast<CHeroItem>(listBox->getItem(i));

		if (!item)
			continue;

		if (item->hero == heroes[i])
		{
			item->update();
		}
		else
		{
			listBox->reset();
			break;
		}
	}

	if (LOCPLINT->localState->getCurrentHero())
		select(LOCPLINT->localState->getCurrentHero());

	CList::update();
}

std::shared_ptr<CIntObject> CTownList::createItem(size_t index)
{
	if (LOCPLINT->localState->getOwnedTowns().size() > index)
		return std::make_shared<CTownItem>(this, LOCPLINT->localState->getOwnedTown(index));
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
	if(on)
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

CTownList::CTownList(int visibleItemsCount, Rect widgetPosition, Point firstItemOffset, Point itemOffsetDelta, size_t initialItemsCount)
	: CList(visibleItemsCount, widgetPosition)
{
	createList(firstItemOffset, itemOffsetDelta, initialItemsCount);
}

void CTownList::select(const CGTownInstance * town)
{
	selectIndex(vstd::find_pos(LOCPLINT->localState->getOwnedTowns(), town));
}

void CTownList::updateElement(const CGTownInstance * town)
{
	updateWidget();
}

void CTownList::updateWidget()
{
	auto & towns = LOCPLINT->localState->getOwnedTowns();

	listBox->resize(towns.size());

	for (size_t i = 0; i < towns.size(); ++i)
	{
		auto item =  std::dynamic_pointer_cast<CTownItem>(listBox->getItem(i));

		if (!item)
			continue;

		if (item->town == towns[i])
		{
			item->update();
		}
		else
		{
			listBox->reset();
			break;
		}
	}

	if (LOCPLINT->localState->getCurrentTown())
		select(LOCPLINT->localState->getCurrentTown());

	CList::update();
}
