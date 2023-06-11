/*
 * CList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/Scrollable.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;

VCMI_LIB_NAMESPACE_END

class CListBox;
class CButton;
class CAnimImage;

/// Base UI Element for hero\town lists
class CList : public Scrollable
{
protected:
	class CListItem : public CIntObject, public std::enable_shared_from_this<CListItem>
	{
		CList * parent;
		std::shared_ptr<CIntObject> selection;
	public:
		CListItem(CList * parent);
		~CListItem();

		void showPopupWindow() override;
		void clickLeft(tribool down, bool previousState) override;
		void hover(bool on) override;
		void onSelect(bool on);

		/// create object with selection rectangle
		virtual std::shared_ptr<CIntObject> genSelection()=0;
		/// reaction on item selection (e.g. enable selection border)
		/// NOTE: item may be deleted in selected state
		virtual void select(bool on)=0;
		/// open item (town or hero screen)
		virtual void open()=0;
		/// show right-click tooltip
		virtual void showTooltip()=0;
		/// get hover text for status bar
		virtual std::string getHoverText()=0;
	};

private:
	const size_t size;

	//for selection\deselection
	std::shared_ptr<CListItem> selected;
	void select(std::shared_ptr<CListItem> which);
	friend class CListItem;

	std::shared_ptr<CButton> scrollUp;
	std::shared_ptr<CButton> scrollDown;

	void scrollBy(int distance) override;
	void scrollPrev() override;
	void scrollNext() override;

protected:
	std::shared_ptr<CListBox> listBox;

	CList(int size, Rect widgetDimensions);

	void createList(Point firstItemPosition, Point itemPositionDelta, size_t listAmount);

	virtual std::shared_ptr<CIntObject> createItem(size_t index) = 0;

public:
	/// functions that will be called when selection changes
	CFunctionList<void()> onSelect;

	/// return index of currently selected element
	int getSelectedIndex();

	void setScrollUpButton(std::shared_ptr<CButton> button);
	void setScrollDownButton(std::shared_ptr<CButton> button);

	/// should be called when list is invalidated
	void update();

	/// set of methods to switch selection
	void selectIndex(int which);
	void selectNext();
	void selectPrev();

	void showAll(Canvas & to) override;
};

/// List of heroes which is shown at the right of the adventure map screen
class CHeroList	: public CList
{
	/// Empty hero item used as placeholder for unused entries in list
	class CEmptyHeroItem : public CIntObject
	{
		std::shared_ptr<CAnimImage> movement;
		std::shared_ptr<CAnimImage> mana;
		std::shared_ptr<CPicture> portrait;
	public:
		CEmptyHeroItem();
	};

	class CHeroItem : public CListItem
	{
		std::shared_ptr<CAnimImage> movement;
		std::shared_ptr<CAnimImage> mana;
		std::shared_ptr<CAnimImage> portrait;
	public:
		const CGHeroInstance * const hero;

		CHeroItem(CHeroList * parent, const CGHeroInstance * hero);

		std::shared_ptr<CIntObject> genSelection() override;
		void update();
		void select(bool on) override;
		void open() override;
		void showTooltip() override;
		std::string getHoverText() override;
	};

	std::shared_ptr<CIntObject> createItem(size_t index);
public:
	CHeroList(int visibleItemsCount, Rect widgetPosition, Point firstItemOffset, Point itemOffsetDelta, size_t initialItemsCount);

	/// Select specific hero and scroll if needed
	void select(const CGHeroInstance * hero = nullptr);

	/// Update hero. Will add or remove it from the list if needed
	void update(const CGHeroInstance * hero = nullptr);
};

/// List of towns which is shown at the right of the adventure map screen or in the town screen
class CTownList	: public CList
{
	class CTownItem : public CListItem
	{
		std::shared_ptr<CAnimImage> picture;
	public:
		const CGTownInstance * const town;

		CTownItem(CTownList *parent, const CGTownInstance * town);

		std::shared_ptr<CIntObject> genSelection() override;
		void update();
		void select(bool on) override;
		void open() override;
		void showTooltip() override;
		std::string getHoverText() override;
	};

	std::shared_ptr<CIntObject> createItem(size_t index) override;
public:
	CTownList(int visibleItemsCount, Rect widgetPosition, Point firstItemOffset, Point itemOffsetDelta, size_t initialItemsCount);

	/// Select specific town and scroll if needed
	void select(const CGTownInstance * town = nullptr);

	/// Update town. Will add or remove it from the list if needed
	void update(const CGTownInstance * town = nullptr);
};

