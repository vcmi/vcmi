#pragma once

#include "../widgets/AdventureMapClasses.h"
#include "CWindowObject.h"

#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"

class CDefHandler;
class CCallback;
struct CGPath;
class CAdvMapInt;
class CGHeroInstance;
class CGTownInstance;
class CHeroWindow;
class CSpell;
class IShipyard;

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
public:
	int tilesw, tilesh; //width and height of terrain to blit in tiles
	int3 curHoveredTile;
	int moveX, moveY; //shift between actual position of screen and the one we wil blit; ranges from -31 to 31 (in pixels)

	CTerrainRect();
	CGPath * currentPath;
	void deactivate();
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	void hover(bool on);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
	void show(SDL_Surface * to);
	void showPath(const SDL_Rect * extRect, SDL_Surface * to);
	int3 whichTileIsIt(const int & x, const int & y); //x,y are cursor position
	int3 whichTileIsIt(); //uses current cursor pos
};

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar : public CIntObject
{
public:
	SDL_Surface * bg;
	std::vector<std::pair<int,int> > txtpos;
	std::string datetext;

	void clickRight(tribool down, bool previousState);
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw(SDL_Surface * to);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
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

	SDL_Surface * bg;
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

	CTerrainRect terrain; //visible terrain
	CResDataBar resdatabar;
	CHeroList heroList;
	CTownList townList;
	CInfoBar infoBar;

	const CSpell *spellBeingCasted; //nullptr if none

	const CArmedInstance *selection; //currently selected town/hero

	//functions bound to buttons
	void fshowOverview();
	void fswitchLevel();
	void fshowQuestlog();
	void fsleepWake();
	void fmoveHero();
	void fshowSpellbok();
	void fadventureOPtions();
	void fsystemOptions();
	void fnextHero();
	void fendTurn();

	void activate();
	void deactivate();

	void show(SDL_Surface * to); //redraws terrain
	void showAll(SDL_Surface * to); //shows and activates adv. map interface

	void select(const CArmedInstance *sel, bool centerView = true);
	void selectionChanged();
	void centerOn(int3 on);
	void centerOn(const CGObjectInstance *obj);
	int3 verifyPos(int3 ver);
	void handleRightClick(std::string text, tribool down);
	void keyPressed(const SDL_KeyboardEvent & key);
	void mouseMoved (const SDL_MouseMotionEvent & sEvent);
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
	void updateNextHero(const CGHeroInstance *h);
};

extern CAdvMapInt *adventureInt;
