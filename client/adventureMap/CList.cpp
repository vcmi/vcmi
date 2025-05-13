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
#include "../widgets/RadialMenu.h"
#include "../windows/InfoWindows.h"
#include "../windows/CCastleInterface.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CList::CListItem::CListItem(CList * Parent)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER),
	parent(Parent),
	selection()
{
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
		ENGINE->statusbar()->write(getHoverText());
	else
		ENGINE->statusbar()->clear();
}

void CList::CListItem::onSelect(bool on)
{
	OBJECT_CONSTRUCTION;
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
	OBJECT_CONSTRUCTION;
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
	OBJECT_CONSTRUCTION;
	movement = std::make_shared<CAnimImage>(AnimationPath::builtin("IMOBIL"), 0, 0, 0, 1);
	portrait = std::make_shared<CPicture>(ImagePath::builtin("HPSXXX"), movement->pos.w + 1, 0);
	mana = std::make_shared<CAnimImage>(AnimationPath::builtin("IMANA"), 0, 0, movement->pos.w + portrait->pos.w + 2, 1 );

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);
}

CHeroList::CHeroItem::CHeroItem(CHeroList *parent, const CGHeroInstance * Hero)
	: CListItem(parent),
	hero(Hero)
{
	OBJECT_CONSTRUCTION;
	movement = std::make_shared<CAnimImage>(AnimationPath::builtin("IMOBIL"), 0, 0, 0, 1);
	portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsSmall"), hero->getIconIndex(), 0, movement->pos.w + 1);
	mana = std::make_shared<CAnimImage>(AnimationPath::builtin("IMANA"), 0, 0, movement->pos.w + portrait->pos.w + 2, 1);

	pos.w = mana->pos.w + mana->pos.x - pos.x;
	pos.h = std::max(std::max<int>(movement->pos.h + 1, mana->pos.h + 1), portrait->pos.h);

	update();

	addUsedEvents(GESTURE | KEYBOARD);
}

void CHeroList::CHeroItem::update()
{
	movement->setFrame(std::min<size_t>(movement->size()-1, hero->movementPointsRemaining() / 100));
	mana->setFrame(std::min<size_t>(mana->size()-1, hero->mana / 5));
	redraw();
}

std::shared_ptr<CIntObject> CHeroList::CHeroItem::genSelection()
{
	return std::make_shared<CPicture>(ImagePath::builtin("HPSYYY"), movement->pos.w + 1, 0);
}

void CHeroList::CHeroItem::select(bool on)
{
	if(on)
		GAME->interface()->localState->setSelection(hero);
}

void CHeroList::CHeroItem::open()
{
	GAME->interface()->openHeroWindow(hero);
}

void CHeroList::CHeroItem::showTooltip()
{
	CRClickPopup::createAndPush(hero, ENGINE->getCursorPosition());
}

std::string CHeroList::CHeroItem::getHoverText()
{
	return boost::str(boost::format(LIBRARY->generaltexth->allTexts[15]) % hero->getNameTranslated() % hero->getClassNameTranslated()) + hero->getMovementPointsTextIfOwner(hero->getOwner());
}

void CHeroList::CHeroItem::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	if(!hero)
		return;

	auto & heroes = GAME->interface()->localState->getWanderingHeroes();

	if(heroes.size() < 2)
		return;

	size_t heroPos = vstd::find_pos(heroes, hero);
	const CGHeroInstance * heroUpper = (heroPos < 1) ? nullptr : heroes.at(heroPos - 1);
	const CGHeroInstance * heroLower = (heroPos > heroes.size() - 2) ? nullptr : heroes.at(heroPos + 1);

	std::vector<RadialMenuConfig> menuElements = {
		{ RadialMenuConfig::ITEM_ALT_NN, heroUpper != nullptr, "altUpTop", "vcmi.radialWheel.moveTop", [heroPos]()
		{
			for (size_t i = heroPos; i > 0; i--)
				GAME->interface()->localState->swapWanderingHero(i, i - 1);
		} },
		{ RadialMenuConfig::ITEM_ALT_NW, heroUpper != nullptr, "altUp", "vcmi.radialWheel.moveUp", [heroPos](){GAME->interface()->localState->swapWanderingHero(heroPos, heroPos - 1); } },
		{ RadialMenuConfig::ITEM_ALT_SW, heroLower != nullptr, "altDown", "vcmi.radialWheel.moveDown", [heroPos](){ GAME->interface()->localState->swapWanderingHero(heroPos, heroPos + 1); } },
		{ RadialMenuConfig::ITEM_ALT_SS, heroLower != nullptr, "altDownBottom", "vcmi.radialWheel.moveBottom", [heroPos, heroes]()
		{
			for (int i = heroPos; i < heroes.size() - 1; i++)
				GAME->interface()->localState->swapWanderingHero(i, i + 1);
		} },
	};

	ENGINE->windows().createAndPushWindow<RadialMenu>(pos.center(), menuElements, true);
}

void CHeroList::CHeroItem::keyPressed(EShortcut key)
{
	if(!hero)
		return;

	if(parent->selected != this->shared_from_this())
		return;

	auto & heroes = GAME->interface()->localState->getWanderingHeroes();

	if(key == EShortcut::LIST_HERO_DISMISS)
	{
		GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[22], [this](){ GAME->interface()->cb->dismissHero(hero); }, nullptr);
		return;
	}

	if(heroes.size() < 2)
		return;

	size_t heroPos = vstd::find_pos(heroes, hero);
	const CGHeroInstance * heroUpper = (heroPos < 1) ? nullptr : heroes.at(heroPos - 1);
	const CGHeroInstance * heroLower = (heroPos > heroes.size() - 2) ? nullptr : heroes.at(heroPos + 1);

	switch(key)
	{
	case EShortcut::LIST_HERO_UP:
		if(heroUpper)
			GAME->interface()->localState->swapWanderingHero(heroPos, heroPos - 1);
		break;

	case EShortcut::LIST_HERO_DOWN:
		if(heroLower)
			GAME->interface()->localState->swapWanderingHero(heroPos, heroPos + 1);
		break;

	case EShortcut::LIST_HERO_TOP:
		if(heroUpper)
			for (size_t i = heroPos; i > 0; i--)
				GAME->interface()->localState->swapWanderingHero(i, i - 1);
		break;

	case EShortcut::LIST_HERO_BOTTOM:
		if(heroLower)
			for (int i = heroPos; i < heroes.size() - 1; i++)
				GAME->interface()->localState->swapWanderingHero(i, i + 1);
		break;
	}
}

std::shared_ptr<CIntObject> CHeroList::createItem(size_t index)
{
	if (GAME->interface()->localState->getWanderingHeroes().size() > index)
		return std::make_shared<CHeroItem>(this, GAME->interface()->localState->getWanderingHero(index));
	return std::make_shared<CEmptyHeroItem>();
}

CHeroList::CHeroList(int visibleItemsCount, Rect widgetPosition, Point firstItemOffset, Point itemOffsetDelta, size_t initialItemsCount)
	: CList(visibleItemsCount, widgetPosition)
{
	createList(firstItemOffset, itemOffsetDelta, initialItemsCount);
}

void CHeroList::select(const CGHeroInstance * hero)
{
	selectIndex(vstd::find_pos(GAME->interface()->localState->getWanderingHeroes(), hero));
}

void CHeroList::updateElement(const CGHeroInstance * hero)
{
	updateWidget();
}

void CHeroList::updateWidget()
{
	const auto & heroes = GAME->interface()->localState->getWanderingHeroes();

	listBox->resize(heroes.size());

	for (size_t i = 0; i < heroes.size(); ++i)
	{
		auto item = std::dynamic_pointer_cast<CHeroItem>(listBox->getItem(i));

		if (!item)
			continue;

		if (item->hero == heroes.at(i))
		{
			item->update();
		}
		else
		{
			listBox->reset();
			break;
		}
	}

	if (GAME->interface()->localState->getCurrentHero())
		select(GAME->interface()->localState->getCurrentHero());

	CList::update();
}

std::shared_ptr<CIntObject> CTownList::createItem(size_t index)
{
	if (GAME->interface()->localState->getOwnedTowns().size() > index)
		return std::make_shared<CTownItem>(this, GAME->interface()->localState->getOwnedTown(index));
	return std::make_shared<CAnimImage>(AnimationPath::builtin("ITPA"), 0);
}

CTownList::CTownItem::CTownItem(CTownList *parent, const CGTownInstance *Town):
	CListItem(parent),
	town(Town)
{
	OBJECT_CONSTRUCTION;
	picture = std::make_shared<CAnimImage>(AnimationPath::builtin("ITPA"), 0);
	pos = picture->pos;
	update();

	addUsedEvents(GESTURE | KEYBOARD);
}

std::shared_ptr<CIntObject> CTownList::CTownItem::genSelection()
{
	return std::make_shared<CAnimImage>(AnimationPath::builtin("ITPA"), 1);
}

void CTownList::CTownItem::update()
{
	size_t iconIndex = town->getTown()->clientInfo.icons[town->hasFort()][town->built >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP)];

	picture->setFrame(iconIndex + 2);
	redraw();
}

void CTownList::CTownItem::select(bool on)
{
	if(on)
		GAME->interface()->localState->setSelection(town);
}

void CTownList::CTownItem::open()
{
	GAME->interface()->openTownWindow(town);
}

void CTownList::CTownItem::showTooltip()
{
	CRClickPopup::createAndPush(town, pos.center());
}

void CTownList::CTownItem::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	const std::vector<const CGTownInstance *> towns = GAME->interface()->localState->getOwnedTowns();
	size_t townIndex = vstd::find_pos(towns, town);

	if(townIndex + 1 > towns.size() || !towns.at(townIndex))
		return;

	if(towns.size() < 2)
		return;

	int townUpperPos = (townIndex < 1) ? -1 : townIndex - 1;
	int townLowerPos = (townIndex > towns.size() - 2) ? -1 : townIndex + 1;

	auto updateList = [](){
		for (auto ci : ENGINE->windows().findWindows<CCastleInterface>())
		{
			ci->townlist->updateWidget();
			ci->townlist->select(ci->town);
		}
	};

	std::vector<RadialMenuConfig> menuElements = {
		{ RadialMenuConfig::ITEM_ALT_NN, townUpperPos > -1, "altUpTop", "vcmi.radialWheel.moveTop", [updateList, townIndex]()
		{
			for (int i = townIndex; i > 0; i--)
				GAME->interface()->localState->swapOwnedTowns(i, i - 1);
			updateList();
		} },
		{ RadialMenuConfig::ITEM_ALT_NW, townUpperPos > -1, "altUp", "vcmi.radialWheel.moveUp", [updateList, townIndex, townUpperPos](){GAME->interface()->localState->swapOwnedTowns(townIndex, townUpperPos); updateList(); } },
		{ RadialMenuConfig::ITEM_ALT_SW, townLowerPos > -1, "altDown", "vcmi.radialWheel.moveDown", [updateList, townIndex, townLowerPos](){ GAME->interface()->localState->swapOwnedTowns(townIndex, townLowerPos); updateList(); } },
		{ RadialMenuConfig::ITEM_ALT_SS, townLowerPos > -1, "altDownBottom", "vcmi.radialWheel.moveBottom", [updateList, townIndex, towns]()
		{
			for (int i = townIndex; i < towns.size() - 1; i++)
				GAME->interface()->localState->swapOwnedTowns(i, i + 1);
			updateList();
		} },
	};

	ENGINE->windows().createAndPushWindow<RadialMenu>(pos.center(), menuElements, true);
}

void CTownList::CTownItem::keyPressed(EShortcut key)
{
	if(parent->selected != this->shared_from_this())
		return;

	const std::vector<const CGTownInstance *> towns = GAME->interface()->localState->getOwnedTowns();
	size_t townIndex = vstd::find_pos(towns, town);

	if(townIndex + 1 > towns.size() || !towns.at(townIndex))
		return;

	if(towns.size() < 2)
		return;

	int townUpperPos = (townIndex < 1) ? -1 : townIndex - 1;
	int townLowerPos = (townIndex > towns.size() - 2) ? -1 : townIndex + 1;

	switch(key)
	{
	case EShortcut::LIST_TOWN_UP:
		if(townUpperPos > -1)
			GAME->interface()->localState->swapOwnedTowns(townIndex, townUpperPos);
		break;

	case EShortcut::LIST_TOWN_DOWN:
		if(townLowerPos > -1)
			GAME->interface()->localState->swapOwnedTowns(townIndex, townLowerPos);
		break;

	case EShortcut::LIST_TOWN_TOP:
		if(townUpperPos > -1)
			for (int i = townIndex; i > 0; i--)
				GAME->interface()->localState->swapOwnedTowns(i, i - 1);
		break;

	case EShortcut::LIST_TOWN_BOTTOM:
		if(townLowerPos > -1)
			for (int i = townIndex; i < towns.size() - 1; i++)
				GAME->interface()->localState->swapOwnedTowns(i, i + 1);
		break;
	}

	for (auto ki : ENGINE->windows().findWindows<CCastleInterface>())
		ki->townChange(); //update list
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
	selectIndex(vstd::find_pos(GAME->interface()->localState->getOwnedTowns(), town));
}

void CTownList::updateElement(const CGTownInstance * town)
{
	updateWidget();
}

void CTownList::updateWidget()
{
	auto & towns = GAME->interface()->localState->getOwnedTowns();

	listBox->resize(towns.size());

	for (size_t i = 0; i < towns.size(); ++i)
	{
		auto item = std::dynamic_pointer_cast<CTownItem>(listBox->getItem(i));

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

	if (GAME->interface()->localState->getCurrentTown())
		select(GAME->interface()->localState->getCurrentTown());

	CList::update();
}
