/*
 * AdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectLists.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CGGarrison;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
struct Component;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class CAnimImage;
class CShowableAnim;
class CFilledTexture;
class CButton;
class CComponent;
class CHeroTooltip;
class CTownTooltip;
class CTextBox;
class IImage;

/// Base UI Element for hero\town lists
class CList : public CIntObject
{
protected:
	class CListItem : public CIntObject, public std::enable_shared_from_this<CListItem>
	{
		CList * parent;
		std::shared_ptr<CIntObject> selection;
	public:
		CListItem(CList * parent);
		~CListItem();

		void clickRight(tribool down, bool previousState) override;
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

	std::shared_ptr<CListBox> listBox;
	const size_t size;

	/**
	 * @brief CList - protected constructor
	 * @param size - maximal amount of visible at once items
	 * @param position - cordinates
	 * @param btnUp - path to image to use as top button
	 * @param btnDown - path to image to use as bottom button
	 * @param listAmount - amount of items in the list
	 * @param helpUp - index in zelp.txt for button help tooltip
	 * @param helpDown - index in zelp.txt for button help tooltip
	 * @param create - function for creating items in listbox
	 * @param destroy - function for deleting items in listbox
	 */
	CList(int size, Point position, std::string btnUp, std::string btnDown, size_t listAmount, int helpUp, int helpDown, CListBox::CreateFunc create);

	//for selection\deselection
	std::shared_ptr<CListItem> selected;
	void select(std::shared_ptr<CListItem> which);
	friend class CListItem;

	std::shared_ptr<CButton> scrollUp;
	std::shared_ptr<CButton> scrollDown;

	/// should be called when list is invalidated
	void update();

public:
	/// functions that will be called when selection changes
	CFunctionList<void()> onSelect;

	/// return index of currently selected element
	int getSelectedIndex();

	/// set of methods to switch selection
	void selectIndex(int which);
	void selectNext();
	void selectPrev();
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

	std::shared_ptr<CIntObject> createHeroItem(size_t index);
public:
	/**
	 * @brief CHeroList
	 * @param size, position, btnUp, btnDown @see CList::CList
	 */
	CHeroList(int size, Point position, std::string btnUp, std::string btnDown);

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

	std::shared_ptr<CIntObject> createTownItem(size_t index);
public:
	/**
	 * @brief CTownList
	 * @param size, position, btnUp, btnDown @see CList::CList
	 */
	CTownList(int size, Point position, std::string btnUp, std::string btnDown);

	/// Select specific town and scroll if needed
	void select(const CGTownInstance * town = nullptr);

	/// Update town. Will add or remove it from the list if needed
	void update(const CGTownInstance * town = nullptr);
};

