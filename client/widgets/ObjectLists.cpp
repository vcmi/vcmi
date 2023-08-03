/*
 * ObjectLists.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ObjectLists.h"

#include "../gui/CGuiHandler.h"
#include "Slider.h"

CObjectList::CObjectList(CreateFunc create)
	: createObject(create)
{
}

void CObjectList::deleteItem(std::shared_ptr<CIntObject> item)
{
	if(!item)
		return;
	item->deactivate();
	removeChild(item.get());
}

std::shared_ptr<CIntObject> CObjectList::createItem(size_t index)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	std::shared_ptr<CIntObject> item = createObject(index);
	if(!item)
		item = std::make_shared<CIntObject>();

	item->recActions = defActions;
	addChild(item.get());
	if (isActive())
		item->activate();
	return item;
}

CTabbedInt::CTabbedInt(CreateFunc create, Point position, size_t ActiveID)
	: CObjectList(create),
	activeTab(nullptr),
	activeID(ActiveID)
{
	defActions &= ~DISPOSE;
	pos += position;
	reset();
}

void CTabbedInt::setActive(size_t which)
{
	if(which != activeID)
	{
		activeID = which;
		reset();
	}
}

size_t CTabbedInt::getActive() const
{
	return activeID;
}

void CTabbedInt::reset()
{
	deleteItem(activeTab);
	activeTab = createItem(activeID);
	activeTab->moveTo(pos.topLeft());

	if(isActive())
		redraw();
}

std::shared_ptr<CIntObject> CTabbedInt::getItem()
{
	return activeTab;
}

CListBox::CListBox(CreateFunc create, Point Pos, Point ItemOffset, size_t VisibleSize,
		size_t TotalSize, size_t InitialPos, int Slider, Rect SliderPos)
	: CObjectList(create),
	first(InitialPos),
	totalSize(TotalSize),
	itemOffset(ItemOffset)
{
	pos += Pos;
	items.resize(VisibleSize, nullptr);

	if(Slider & 1)
	{
		OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
		slider = std::make_shared<CSlider>(
			SliderPos.topLeft(),
			SliderPos.w,
			std::bind(&CListBox::moveToPos, this, _1),
			(int)VisibleSize,
			(int)TotalSize,
			(int)InitialPos,
			Slider & 2 ? Orientation::HORIZONTAL : Orientation::VERTICAL,
			Slider & 4 ? CSlider::BLUE : CSlider::BROWN
		);

		slider->setPanningStep(itemOffset.x + itemOffset.y);
	}
	reset();
}

// Used to move active items after changing list position
void CListBox::updatePositions()
{
	Point itemPos = pos.topLeft();
	for (auto & elem : items)
	{
		(elem)->moveTo(itemPos);
		itemPos += itemOffset;
	}
	if (isActive())
	{
		redraw();
		if (slider)
			slider->scrollTo((int)first);
	}
}

void CListBox::reset()
{
	size_t current = first;
	for (auto & elem : items)
	{
		deleteItem(elem);
		elem = createItem(current++);
	}
	updatePositions();
}

void CListBox::resize(size_t newSize)
{
	if (totalSize == newSize)
		return;

	totalSize = newSize;
	if (slider)
		slider->setAmount((int)totalSize);
	reset();
}

size_t CListBox::size()
{
	return totalSize;
}

std::shared_ptr<CIntObject> CListBox::getItem(size_t which)
{
	if(which < first || which > first + items.size() || which > totalSize)
		return std::shared_ptr<CIntObject>();

	size_t i=first;
	for (auto iter = items.begin(); iter != items.end(); iter++, i++)
		if( i == which)
			return *iter;
	return std::shared_ptr<CIntObject>();
}

size_t CListBox::getIndexOf(std::shared_ptr<CIntObject> item)
{
	size_t i=first;
	for(auto iter = items.begin(); iter != items.end(); iter++, i++)
		if(*iter == item)
			return i;
	return size_t(-1);
}

void CListBox::scrollTo(size_t which)
{
	//scroll up
	if(first > which)
		moveToPos(which);
	//scroll down
	else if (first + items.size() <= which && which < totalSize)
		moveToPos(which - items.size() + 1);
}

void CListBox::moveToPos(size_t which)
{
	//Calculate new position
	size_t maxPossible;
	if(totalSize > items.size())
		maxPossible = totalSize - items.size();
	else
		maxPossible = 0;

	size_t newPos = std::min(which, maxPossible);

	//If move distance is 1 (most of calls from Slider) - use faster shifts instead of resetting all items
	if(first - newPos == 1)
	{
		moveToPrev();
	}
	else if(newPos - first == 1)
	{
		moveToNext();
	}
	else if(newPos != first)
	{
		first = newPos;
		reset();
	}
}

void CListBox::moveToNext()
{
	//Remove front item and insert new one to end
	if(first + items.size() < totalSize)
	{
		first++;
		deleteItem(items.front());
		items.pop_front();
		items.push_back(createItem(first+items.size()));
		updatePositions();
	}
}

void CListBox::moveToPrev()
{
	//Remove last item and insert new one at start
	if(first)
	{
		first--;
		deleteItem(items.back());
		items.pop_back();
		items.push_front(createItem(first));
		updatePositions();
	}
}

size_t CListBox::getPos()
{
	return first;
}

const std::list<std::shared_ptr<CIntObject>> & CListBox::getItems()
{
	return items;
}

CListBoxWithCallback::CListBoxWithCallback(CListBoxWithCallback::MovedPosCallback callback, CreateFunc create, Point pos, Point itemOffset, 
	size_t visibleSize,	size_t totalSize, size_t initialPos, int slider, Rect sliderPos)
	: CListBox(create, pos, itemOffset, visibleSize, totalSize, initialPos, slider, sliderPos)
{
	movedPosCallback = callback;
}

void CListBoxWithCallback::scrollTo(size_t pos)
{
	CListBox::scrollTo(pos);
	if(movedPosCallback)
		movedPosCallback(getPos());
}

void CListBoxWithCallback::moveToPos(size_t pos)
{
	CListBox::moveToPos(pos);
	if(movedPosCallback)
		movedPosCallback(getPos());
}

void CListBoxWithCallback::moveToNext()
{
	CListBox::moveToNext();
	if(movedPosCallback)
		movedPosCallback(getPos());
}

void CListBoxWithCallback::moveToPrev()
{
	CListBox::moveToPrev();
	if(movedPosCallback)
		movedPosCallback(getPos());
}