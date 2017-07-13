/*
 * ObjectLists.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

struct SDL_Surface;
struct Rect;
class CAnimImage;
class CSlider;
class CLabel;
class CAnimation;

/// Used as base for Tabs and List classes
class CObjectList : public CIntObject
{
public:
	typedef std::function<CIntObject* (size_t)> CreateFunc;
	typedef std::function<void(CIntObject *)> DestroyFunc;

private:
	CreateFunc createObject;
	DestroyFunc destroyObject;

protected:
	//Internal methods for safe creation of items (Children capturing and activation/deactivation if needed)
	void deleteItem(CIntObject* item);
	CIntObject* createItem(size_t index);

	CObjectList(CreateFunc create, DestroyFunc destroy = DestroyFunc());//Protected constructor
};

/// Window element with multiple tabs
class CTabbedInt : public CObjectList
{
private:
	CIntObject * activeTab;
	size_t activeID;

public:
	//CreateFunc, DestroyFunc - see CObjectList
	//Pos - position of object, all tabs will be moved to this position
	//ActiveID - ID of initially active tab
	CTabbedInt(CreateFunc create, DestroyFunc destroy = DestroyFunc(), Point position=Point(), size_t ActiveID=0);

	void setActive(size_t which);
	//recreate active tab
	void reset();

	//return currently active item
	CIntObject * getItem();
};

/// List of IntObjects with optional slider
class CListBox : public CObjectList
{
private:
	std::list< CIntObject* > items;
	size_t first;
	size_t totalSize;

	Point itemOffset;
	CSlider * slider;

	void updatePositions();
public:
	//CreateFunc, DestroyFunc - see CObjectList
	//Pos - position of first item
	//ItemOffset - distance between items in the list
	//VisibleSize - maximal number of displayable at once items
	//TotalSize
	//Slider - slider style, bit field: 1 = present(disabled), 2=horisontal(vertical), 4=blue(brown)
	//SliderPos - position of slider, if present
	CListBox(CreateFunc create, DestroyFunc destroy, Point Pos, Point ItemOffset, size_t VisibleSize,
		size_t TotalSize, size_t InitialPos=0, int Slider=0, Rect SliderPos=Rect() );

	//recreate all visible items
	void reset();

	//change or get total amount of items in the list
	void resize(size_t newSize);
	size_t size();

	//return item with index which or null if not present
	CIntObject * getItem(size_t which);

	//return currently active items
	const std::list< CIntObject * > & getItems();

	//get index of this item. -1 if not found
	size_t getIndexOf(CIntObject * item);

	//scroll list to make item which visible
	void scrollTo(size_t which);

	//scroll list to specified position
	void moveToPos(size_t which);
	void moveToNext();
	void moveToPrev();

	size_t getPos();
};
