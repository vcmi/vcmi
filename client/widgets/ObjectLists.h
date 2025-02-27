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

VCMI_LIB_NAMESPACE_BEGIN
class Rect;
VCMI_LIB_NAMESPACE_END

class CAnimImage;
class CSlider;
class CLabel;

/// Used as base for Tabs and List classes
class CObjectList : public CIntObject
{
public:
	using CreateFunc = std::function<std::shared_ptr<CIntObject>(size_t)>;

private:
	CreateFunc createObject;

protected:
	//Internal methods for safe creation of items (Children capturing and activation/deactivation if needed)
	void deleteItem(std::shared_ptr<CIntObject> item);
	std::shared_ptr<CIntObject> createItem(size_t index);

	CObjectList(CreateFunc create);
};

/// Window element with multiple tabs
class CTabbedInt : public CObjectList
{
private:
	std::shared_ptr<CIntObject> activeTab;
	size_t activeID;

public:
	//CreateFunc, DestroyFunc - see CObjectList
	//Pos - position of object, all tabs will be moved to this position
	//ActiveID - ID of initially active tab
	CTabbedInt(CreateFunc create, Point position=Point(), size_t ActiveID=0);

	void setActive(size_t which);
	size_t getActive() const;
	//recreate active tab
	void reset();

	//return currently active item
	std::shared_ptr<CIntObject> getItem();
};

/// List of IntObjects with optional slider
class CListBox : public CObjectList
{
private:
	std::list<std::shared_ptr<CIntObject>> items;
	size_t first;
	size_t totalSize;

	Point itemOffset;
	std::shared_ptr<CSlider> slider;

	void updatePositions();
public:
	//CreateFunc, DestroyFunc - see CObjectList
	//Pos - position of first item
	//ItemOffset - distance between items in the list
	//VisibleSize - maximal number of displayable at once items
	//TotalSize
	//Slider - slider style, bit field: 1 = present(disabled), 2=horizontal(vertical), 4=blue(brown)
	//SliderPos - position of slider, if present
	CListBox(CreateFunc create, Point Pos, Point ItemOffset, size_t VisibleSize,
		size_t TotalSize, size_t InitialPos=0, int Slider=0, Rect SliderPos=Rect() );

	//recreate all visible items
	void reset();

	//change or get total amount of items in the list
	void resize(size_t newSize);
	size_t size();

	//return item with index which or null if not present
	std::shared_ptr<CIntObject> getItem(size_t which);

	std::shared_ptr<CSlider> getSlider();

	//return currently active items
	const std::list<std::shared_ptr<CIntObject>> & getItems();

	//get index of this item. -1 if not found
	size_t getIndexOf(std::shared_ptr<CIntObject> item);

	//scroll list to make item which visible
	virtual void scrollTo(size_t which);

	//scroll list to specified position
	virtual void moveToPos(size_t which);
	virtual void moveToNext();
	virtual void moveToPrev();

	size_t getPos();
};

class CListBoxWithCallback : public CListBox
{
public:
	using MovedPosCallback = std::function<void(size_t)>;

	CListBoxWithCallback(MovedPosCallback callback, CreateFunc create, Point pos, Point itemOffset, size_t visibleSize,
		size_t totalSize, size_t initialPos = 0, int slider = 0, Rect sliderPos = Rect());
	void scrollTo(size_t pos) override;
	void moveToPos(size_t pos) override;
	void moveToNext() override;
	void moveToPrev() override;

private:
	MovedPosCallback movedPosCallback;
};
