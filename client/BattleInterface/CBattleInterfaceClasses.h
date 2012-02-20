#pragma once

#include "../UIFramework/CIntObject.h"

struct SDL_Surface;
class CDefHandler;
class CGHeroInstance;
class CBattleInterface;
class CPicture;
class CAdventureMapButton;
class CHighlightableButton;
class CHighlightableButtonsGroup;
class CLabel;
class BattleResult;
class CStack;

/*
 * CBattleInterfaceClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which shows the console at the bottom of the battle screen and manages the text of the console
class CBattleConsole : public CIntObject
{
private:
	std::vector< std::string > texts; //a place where texts are stored
	int lastShown; //last shown line of text
public:
	std::string alterTxt; //if it's not empty, this text is displayed
	std::string ingcAlter; //alternative text set by in-game console - very important!
	int whoSetAlter; //who set alter text; 0 - battle interface or none, 1 - button
	CBattleConsole() : lastShown(-1), alterTxt(""), whoSetAlter(0) {}
	~CBattleConsole(); //d-tor
	void show(SDL_Surface * to = 0);
	bool addText(const std::string &text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void alterText(const std::string &text); //place string at alterTxt
	void eraseText(ui32 pos); //erases added text at position pos
	void changeTextAt(const std::string &text, ui32 pos); //if we have more than pos texts, pos-th is changed to given one
	void scrollUp(ui32 by = 1); //scrolls console up by 'by' positions
	void scrollDown(ui32 by = 1); //scrolls console up by 'by' positions
};

/// Hero battle animation
class CBattleHero : public CIntObject
{
public:
	bool flip; //false if it's attacking hero, true otherwise
	CDefHandler *dh, *flag; //animation and flag
	const CGHeroInstance * myHero; //this animation's hero instance
	const CBattleInterface * myOwner; //battle interface to which this animation is assigned
	int phase; //stage of animation
	int nextPhase; //stage of animation to be set after current phase is fully displayed
	int image; //frame of animation
	ui8 flagAnim, flagAnimCount; //for flag animation
	void show(SDL_Surface * to); //prints next frame of animation to to
	void activate();
	void deactivate();
	void setPhase(int newPhase); //sets phase of hero animation
	void clickLeft(tribool down, bool previousState); //call-in
	CBattleHero(const std::string &defName, int phaseG, int imageG, bool filpG, ui8 player, const CGHeroInstance *hero, const CBattleInterface *owner); //c-tor
	~CBattleHero(); //d-tor
};

/// Class which manages the battle options window
class CBattleOptionsWindow : public CIntObject
{
private:
	CBattleInterface * myInt;
	CPicture * background;
	CAdventureMapButton * setToDefault, * exit;
	CHighlightableButton * viewGrid, * movementShadow, * mouseShadow;
	CHighlightableButtonsGroup * animSpeeds;

	std::vector<CLabel*> labels;
public:
	CBattleOptionsWindow(const SDL_Rect &position, CBattleInterface *owner); //c-tor

	void bDefaultf(); //default button callback
	void bExitf(); //exit button callback
};

/// Class which is responsible for showing the battle result window
class CBattleResultWindow : public CIntObject
{
private:
	SDL_Surface *background;
	CAdventureMapButton *exit;
	CBattleInterface *owner;
public:
	CBattleResultWindow(const BattleResult & br, const SDL_Rect & pos, CBattleInterface * _owner); //c-tor
	~CBattleResultWindow(); //d-tor

	void bExitf(); //exit button callback

	void activate();
	void deactivate();
	void show(SDL_Surface * to = 0);
};

/// Class which stands for a single hex field on a battlefield
class CClickableHex : public CIntObject
{
private:
	bool setAlterText; //if true, this hex has set alternative text in console and will clean it
public:
	ui32 myNumber; //number of hex in commonly used format
	bool accessible; //if true, this hex is accessible for units
	//CStack * ourStack;
	bool hovered, strictHovered; //for determining if hex is hovered by mouse (this is different problem than hex's graphic hovering)
	CBattleInterface * myInterface; //interface that owns me
	static Point getXYUnitAnim(const int & hexNum, const bool & attacker, const CStack * creature, const CBattleInterface * cbi); //returns (x, y) of left top corner of animation

	//for user interactions
	void hover (bool on);
	void activate();
	void deactivate();
	void mouseMoved (const SDL_MouseMotionEvent &sEvent);
	void clickLeft(tribool down, bool previousState);
	void clickRight(tribool down, bool previousState);
	CClickableHex();
};

/// Shows the stack queue
class CStackQueue : public CIntObject
{
	class StackBox : public CIntObject
	{
	public:
		const CStack *my;
		SDL_Surface *bg;

		void hover (bool on);
		void showAll(SDL_Surface * to);
		void setStack(const CStack *nStack);
		StackBox(SDL_Surface *BG);
		~StackBox();
	};

public:
	static const int QUEUE_SIZE = 10;
	const bool embedded;
	std::vector<const CStack *> stacksSorted;
	std::vector<StackBox *> stackBoxes;

	SDL_Surface *box;
	SDL_Surface *bg;
	CBattleInterface * owner;

	void showAll(SDL_Surface * to);
	CStackQueue(bool Embedded, CBattleInterface * _owner);
	~CStackQueue();
	void update();
	void blitBg( SDL_Surface * to );
	//void showAll(SDL_Surface * to);
};