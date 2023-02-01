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

class CMinimap;

class CMinimapInstance : public CIntObject
{
	CMinimap * parent;
	SDL_Surface * minimap;
	int level;

	//get color of selected tile on minimap
	const SDL_Color & getTileColor(const int3 & pos);

	void blitTileWithColor(const SDL_Color & color, const int3 & pos, SDL_Surface * to, int x, int y);

	//draw minimap already scaled.
	//result is not antialiased. Will result in "missing" pixels on huge maps (>144)
	void drawScaled(int level);
public:
	CMinimapInstance(CMinimap * parent, int level);
	~CMinimapInstance();

	void showAll(SDL_Surface * to) override;
	void tileToPixels (const int3 & tile, int & x, int & y, int toX = 0, int toY = 0);
	void refreshTile(const int3 & pos);
};

/// Minimap which is displayed at the right upper corner of adventure map
class CMinimap : public CIntObject
{
protected:
	std::shared_ptr<CPicture> aiShield; //the graphic displayed during AI turn
	std::shared_ptr<CMinimapInstance> minimap;
	int level;

	//to initialize colors
	std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > loadColors();

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover (bool on) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;

	void moveAdvMapSelection();

public:
	// terrainID -> (normal color, blocked color)
	const std::map<TerrainId, std::pair<SDL_Color, SDL_Color> > colors;

	CMinimap(const Rect & position);

	//should be called to invalidate whole map - different player or level
	int3 translateMousePosition();
	void update();
	void setLevel(int level);
	void setAIRadar(bool on);

	void showAll(SDL_Surface * to) override;

	void hideTile(const int3 &pos); //puts FoW
	void showTile(const int3 &pos); //removes FoW
};

/// Info box which shows next week/day information, hold the current date
class CInfoBar : public CIntObject
{
	//all visible information located in one object - for ease of replacing
	class CVisibleInfo : public CIntObject
	{
	public:
		void show(SDL_Surface * to) override;

	protected:
		std::shared_ptr<CPicture> background;
		std::list<std::shared_ptr<CIntObject>> forceRefresh;

		CVisibleInfo();
	};

	class EmptyVisibleInfo : public CVisibleInfo
	{
	public:
		EmptyVisibleInfo();
	};

	class VisibleHeroInfo : public CVisibleInfo
	{
		std::shared_ptr<CHeroTooltip> heroTooltip;
	public:
		VisibleHeroInfo(const CGHeroInstance * hero);
	};

	class VisibleTownInfo : public CVisibleInfo
	{
		std::shared_ptr<CTownTooltip> townTooltip;
	public:
		VisibleTownInfo(const CGTownInstance * town);
	};

	class VisibleDateInfo : public CVisibleInfo
	{
		std::shared_ptr<CShowableAnim> animation;
		std::shared_ptr<CLabel> label;

		std::string getNewDayName();
	public:
		VisibleDateInfo();
	};

	class VisibleEnemyTurnInfo : public CVisibleInfo
	{
		std::shared_ptr<CAnimImage> banner;
		std::shared_ptr<CShowableAnim> glass;
		std::shared_ptr<CShowableAnim> sand;
	public:
		VisibleEnemyTurnInfo(PlayerColor player);
	};

	class VisibleGameStatusInfo : public CVisibleInfo
	{
		std::shared_ptr<CLabel> allyLabel;
		std::shared_ptr<CLabel> enemyLabel;

		std::vector<std::shared_ptr<CAnimImage>> flags;
		std::vector<std::shared_ptr<CAnimImage>> hallIcons;
		std::vector<std::shared_ptr<CLabel>> hallLabels;
	public:
		VisibleGameStatusInfo();
	};

	class VisibleComponentInfo : public CVisibleInfo
	{
		std::shared_ptr<CComponent> comp;
		std::shared_ptr<CTextBox> text;
	public:
		VisibleComponentInfo(const Component & compToDisplay, std::string message);
	};

	enum EState
	{
		EMPTY, HERO, TOWN, DATE, GAME, AITURN, COMPONENT
	};

	std::shared_ptr<CVisibleInfo> visibleInfo;
	EState state;

	//removes all information about current state, deactivates timer (if any)
	void reset();

	void tick() override;

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover(bool on) override;

	void playNewDaySound();
public:
	CInfoBar(const Rect & pos);

	/// show new day/week animation
	void showDate();

	/// show component for 3 seconds. Used to display picked up resources
	void showComponent(const Component & comp, std::string message);

	/// print enemy turn progress
	void startEnemyTurn(PlayerColor color);

	/// reset to default view - selected object
	void showSelection();

	/// show hero\town information
	void showHeroSelection(const CGHeroInstance * hero);
	void showTownSelection(const CGTownInstance * town);

	/// for 3 seconds shows amount of town halls and players status
	void showGameStatus();
};

/// simple panel that contains other displayable elements; used to separate groups of controls
class CAdvMapPanel : public CIntObject
{
	std::vector<std::shared_ptr<CButton>> colorableButtons;
	std::vector<std::shared_ptr<CIntObject>> otherObjects;
	/// the surface passed to this obj will be freed in dtor
	std::shared_ptr<IImage> background;
public:
	CAdvMapPanel(std::shared_ptr<IImage> bg, Point position);

	void addChildToPanel(std::shared_ptr<CIntObject> obj, ui8 actions = 0);
	void addChildColorableButton(std::shared_ptr<CButton> button);
	/// recolors all buttons to given player color
	void setPlayerColor(const PlayerColor & clr);

	void showAll(SDL_Surface * to) override;
};

/// specialized version of CAdvMapPanel that handles recolorable def-based pictures for world view info panel
class CAdvMapWorldViewPanel : public CAdvMapPanel
{
	/// data that allows reconstruction of panel info icons
	std::vector<std::pair<int, Point>> iconsData;
	/// ptrs to child-pictures constructed from iconsData
	std::vector<std::shared_ptr<CAnimImage>> currentIcons;
	/// surface drawn below world view panel on higher resolutions (won't be needed when world view panel is configured for extraResolutions mod)
	std::shared_ptr<CFilledTexture> backgroundFiller;
	std::shared_ptr<CAnimation> icons;
public:
	CAdvMapWorldViewPanel(std::shared_ptr<CAnimation> _icons, std::shared_ptr<IImage> bg, Point position, int spaceBottom, const PlayerColor &color);
	virtual ~CAdvMapWorldViewPanel();

	void addChildIcon(std::pair<int, Point> data, int indexOffset);
	/// recreates all pictures from given def to recolor them according to current player color
	void recolorIcons(const PlayerColor & color, int indexOffset);
};

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, uint32_t > > texts; //list<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously

	std::weak_ptr<IStatusBar> currentStatusBar;
public:
	std::string enteredText;
	void show(SDL_Surface * to) override;
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key) override; //call-in

	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;

	void startEnteringText();
	void endEnteringText(bool processEnteredText);
	void refreshEnteredText();

	CInGameConsole();
};
