#pragma once

#include "ObjectLists.h"
#include "../../lib/FunctionList.h"

class CArmedInstance;
class CShowableAnim;
class CGGarrison;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
class CButton;
struct Component;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

/*
 * CAdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Base UI Element for hero\town lists
class CList : public CIntObject
{
protected:
	class CListItem : public CIntObject
	{
		CList * parent;
		CIntObject * selection;
	public:
		CListItem(CList * parent);
		~CListItem();

		void clickRight(tribool down, bool previousState);
		void clickLeft(tribool down, bool previousState);
		void hover(bool on);
		void onSelect(bool on);

		/// create object with selection rectangle
		virtual CIntObject * genSelection()=0;
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

	CListBox * list;
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
	CList(int size, Point position, std::string btnUp, std::string btnDown, size_t listAmount, int helpUp, int helpDown,
	      CListBox::CreateFunc create, CListBox::DestroyFunc destroy = CListBox::DestroyFunc());

	//for selection\deselection
	CListItem *selected;
	void select(CListItem * which);
	friend class CListItem;

	/// should be called when list is invalidated
	void update();

public:

	CButton * scrollUp;
	CButton * scrollDown;

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
	public:
		CEmptyHeroItem();
	};

	class CHeroItem : public CListItem
	{
		CAnimImage * movement;
		CAnimImage * mana;
		CAnimImage * portrait;
	public:
		const CGHeroInstance * const hero;

		CHeroItem(CHeroList *parent, const CGHeroInstance * hero);

		CIntObject * genSelection();
		void update();
		void select(bool on);
		void open();
		void showTooltip();
		std::string getHoverText();
	};

	CIntObject * createHeroItem(size_t index);
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
		CAnimImage * picture;
	public:
		const CGTownInstance * const town;

		CTownItem(CTownList *parent, const CGTownInstance * town);

		CIntObject * genSelection();
		void update();
		void select(bool on);
		void open();
		void showTooltip();
		std::string getHoverText();
	};

	CIntObject * createTownItem(size_t index);
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

class CMinimap;

class CMinimapInstance : public CIntObject
{
	CMinimap *parent;
	SDL_Surface * minimap;
	int level;

	//get color of selected tile on minimap
	const SDL_Color & getTileColor(const int3 & pos);

	void blitTileWithColor(const SDL_Color & color, const int3 & pos, SDL_Surface *to, int x, int y);

	//draw minimap already scaled.
	//result is not antialiased. Will result in "missing" pixels on huge maps (>144)
	void drawScaled(int level);
public:
	CMinimapInstance(CMinimap * parent, int level);
	~CMinimapInstance();

	void showAll(SDL_Surface *to);
	void tileToPixels (const int3 &tile, int &x, int &y,int toX = 0, int toY = 0);

	void refreshTile(const int3 &pos);
};

/// Minimap which is displayed at the right upper corner of adventure map
class CMinimap : public CIntObject
{
protected:

	CPicture *aiShield; //the graphic displayed during AI turn
	CMinimapInstance * minimap;
	int level;

	//to initialize colors
	std::map<int, std::pair<SDL_Color, SDL_Color> > loadColors(std::string from);

	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover (bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);

	void moveAdvMapSelection();

public:
	// terrainID -> (normal color, blocked color)
	const std::map<int, std::pair<SDL_Color, SDL_Color> > colors;

	CMinimap(const Rect & position);

	//should be called to invalidate whole map - different player or level
	int3 translateMousePosition();
	void update();
	void setLevel(int level);
	void setAIRadar(bool on);

	void showAll(SDL_Surface * to);

	void hideTile(const int3 &pos); //puts FoW
	void showTile(const int3 &pos); //removes FoW
};

/// Info box which shows next week/day information, hold the current date
class CInfoBar : public CIntObject
{
	//all visible information located in one object - for ease of replacing
	class CVisibleInfo : public CIntObject
	{
		//list of objects that must be redrawed on each frame on a top of animation
		std::list<CIntObject *> forceRefresh;

		//the only part of gui we need to know about for updating - AI progress
		CAnimImage *aiProgress;

		std::string getNewDayName();
		void playNewDaySound();

	public:
		CVisibleInfo(Point position);

		void show(SDL_Surface *to);

		//functions that must be called only once
		void loadHero(const CGHeroInstance * hero);
		void loadTown(const CGTownInstance * town);
		void loadDay();
		void loadEnemyTurn(PlayerColor player);
		void loadGameStatus();
		void loadComponent(const Component &comp, std::string message);

		//can be called multiple times
		void updateEnemyTurn(double progress);
	};

	enum EState
	{
		EMPTY, HERO, TOWN, DATE, GAME, AITURN, COMPONENT
	};

	CVisibleInfo * visibleInfo;
	EState state;
	//currently displayed object. May be null if state is not hero or town
	const CGObjectInstance * currentObject;

	//removes all information about current state, deactivates timer (if any)
	void reset(EState newState);

	void tick();

	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);

public:
	CInfoBar(const Rect & pos);

	/// show new day/week animation
	void showDate();

	/// show component for 3 seconds. Used to display picked up resources
	void showComponent(const Component & comp, std::string message);

	/// print enemy turn progress
	void startEnemyTurn(PlayerColor color);
	/// updates enemy turn.
	/// NOTE: currently DISABLED. Check comments in CInfoBar::CVisibleInfo::loadEnemyTurn()
	void updateEnemyTurn(double progress);

	/// reset to default view - selected object
	void showSelection();

	/// show hero\town information
	void showHeroSelection(const CGHeroInstance * hero);
	void showTownSelection(const CGTownInstance * town);

	/// for 3 seconds shows amount of town halls and players status
	void showGameStatus();
};

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, int > > texts; //list<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously
public:
	std::string enteredText;
	void show(SDL_Surface * to);
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key); //call-in

#ifndef VCMI_SDL1
	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;
#endif // VCMI_SDL1

	void startEnteringText();
	void endEnteringText(bool printEnteredText);
	void refreshEnteredText();

	CInGameConsole(); //c-tor
};
