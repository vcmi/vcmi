#pragma once

#include "../widgets/AdventureMapClasses.h"
#include "CWindowObject.h"

#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"

#include "../../lib/spells/ViewSpellInt.h"

class CDefHandler;
class CCallback;
struct CGPath;
class CAdvMapInt;
class CGHeroInstance;
class CGTownInstance;
class CHeroWindow;
class CSpell;
class IShipyard;
enum class EMapAnimRedrawStatus;
class CFadeAnimation;

struct MapDrawingInfo;

/*****************************/

/*
 * CAdvmapInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

enum class EAdvMapMode
{
	NORMAL,
	WORLD_VIEW
};

/// Adventure options dialogue where you can view the world, dig, play the replay of the last turn,...
class CAdventureOptions : public CWindowObject
{
public:
	CButton *exit, *viewWorld, *puzzle, *dig, *scenInfo, *replay;

	CAdventureOptions();
	static void showScenarioInfo();
};

/// Holds information about which tiles of the terrain are shown/not shown at the screen
class CTerrainRect
	:  public CIntObject
{
	SDL_Surface * fadeSurface;
	EMapAnimRedrawStatus lastRedrawStatus;
	CFadeAnimation * fadeAnim;
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)

	CTerrainRect();
	virtual ~CTerrainRect();
	CGPath * currentPath;
	void deactivate() override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void hover(bool on) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	void showAnim(SDL_Surface * to);
	void showPath(const SDL_Rect * extRect, SDL_Surface * to);
	int3 whichTileIsIt(const int x, const int y); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
	/// @returns number of visible tiles on screen respecting current map scaling
	int3 tileCountOnScreen();
	/// animates view by caching current surface and crossfading it with normal screen
	void fadeFromCurrentView();
	bool needsAnimUpdate();
};

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar : public CIntObject
{
public:
	SDL_Surface * bg;
	std::vector<std::pair<int,int> > txtpos;
	std::string datetext;

	void clickRight(tribool down, bool previousState) override;
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw(SDL_Surface * to);
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

/// That's a huge class which handles general adventure map actions and
/// shows the right menu(questlog, spellbook, end turn,..) from where you
/// can get to the towns and heroes.
class CAdvMapInt : public CIntObject
{
	//Return object that must be active at this tile (=clickable)
	const CGObjectInstance *getActiveObject(const int3 &tile);

public:
	CAdvMapInt();
	~CAdvMapInt();

	int3 position; //top left corner of visible map part
	PlayerColor player;

	bool duringAITurn;

	enum{LEFT=1, RIGHT=2, UP=4, DOWN=8};
	ui8 scrollingDir; //uses enum: LEFT RIGHT, UP, DOWN

	enum{NA, INGAME, WAITING} state;

	bool updateScreen;
	ui8 anim, animValHitCount; //animation frame
	ui8 heroAnim, heroAnimValHitCount; //animation frame

	EAdvMapMode mode;
	float worldViewScale;
	
	struct WorldViewOptions
	{
		bool showAllTerrain; //for expert viewEarth
		
		std::vector<ObjectPosInfo> iconPositions;
		
		WorldViewOptions();
		
		void clear();
		
		void adjustDrawingInfo(MapDrawingInfo & info);		
	};
	
	WorldViewOptions worldViewOptions; 	

	SDL_Surface * bg;
	SDL_Surface * bgWorldView;
	std::vector<CDefHandler *> gems;
	CMinimap minimap;
	CGStatusBar statusbar;

	CButton * kingOverview;
	CButton * underground;
	CButton * questlog;
	CButton * sleepWake;
	CButton * moveHero;
	CButton * spellbook;
	CButton * advOptions;
	CButton * sysOptions;
	CButton * nextHero;
	CButton * endTurn;

	CButton * worldViewUnderground;

	CTerrainRect terrain; //visible terrain
	CResDataBar resdatabar;
	CHeroList heroList;
	CTownList townList;
	CInfoBar infoBar;

	CAdvMapPanel *panelMain; // panel that holds all right-side buttons in normal view
	CAdvMapWorldViewPanel *panelWorldView; // panel that holds all buttons and other ui in world view
	CAdvMapPanel *activeMapPanel; // currently active panel (either main or world view, depending on current mode)

	CDefHandler * worldViewIconsDef; // images for world view overlay

	const CSpell *spellBeingCasted; //nullptr if none

	const CArmedInstance *selection; //currently selected town/hero

	//functions bound to buttons
	void fshowOverview();
	void fworldViewBack();
	void fworldViewScale1x();
	void fworldViewScale2x();
	void fworldViewScale4x();
	void fswitchLevel();
	void fshowQuestlog();
	void fsleepWake();
	void fmoveHero();
	void fshowSpellbok();
	void fadventureOPtions();
	void fsystemOptions();
	void fnextHero();
	void fendTurn();

	void activate() override;
	void deactivate() override;

	void show(SDL_Surface * to) override; //redraws terrain
	void showAll(SDL_Surface * to) override; //shows and activates adv. map interface

	void select(const CArmedInstance *sel, bool centerView = true);
	void selectionChanged();
	void centerOn(int3 on, bool fade = false);
	void centerOn(const CGObjectInstance *obj, bool fade = false);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down);
	void keyPressed(const SDL_KeyboardEvent & key) override;
	void mouseMoved (const SDL_MouseMotionEvent & sEvent) override;
	bool isActive();

	bool isHeroSleeping(const CGHeroInstance *hero);
	void setHeroSleeping(const CGHeroInstance *hero, bool sleep);
	int getNextHeroIndex(int startIndex); //for Next Hero button - cycles awake heroes with movement only

	void setPlayer(PlayerColor Player);
	void startHotSeatWait(PlayerColor Player);
	void startTurn();
	void endingTurn();
	void aiTurnStarted();

	void adjustActiveness(bool aiTurnStart); //should be called every time at AI/human turn transition; blocks GUI during AI turn
	void quickCombatLock(); //should be called when quick battle started
	void quickCombatUnlock();
	void tileLClicked(const int3 &mapPos);
	void tileHovered(const int3 &mapPos);
	void tileRClicked(const int3 &mapPos);
	void enterCastingMode(const CSpell * sp);
	void leaveCastingMode(bool cast = false, int3 dest = int3(-1, -1, -1));
	const CGHeroInstance * curHero() const;
	const CGTownInstance * curTown() const;
	const IShipyard * ourInaccessibleShipyard(const CGObjectInstance *obj) const; //checks if obj is our ashipyard and cursor is 0,0 -> returns shipyard or nullptr else
	//button updates
	void updateSleepWake(const CGHeroInstance *h);
	void updateMoveHero(const CGHeroInstance *h, tribool hasPath = boost::logic::indeterminate);
	void updateSpellbook(const CGHeroInstance *h);
	void updateNextHero(const CGHeroInstance *h);

	/// changes current adventure map mode; used to switch between default view and world view; scale is ignored if EAdvMapMode == NORMAL
	void changeMode(EAdvMapMode newMode, float newScale = 0.36f);
};

extern CAdvMapInt *adventureInt;
