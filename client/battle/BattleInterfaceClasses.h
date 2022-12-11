/*
 * CBattleInterfaceClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../../lib/battle/BattleHex.h"
#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
struct BattleResult;
class CStack;
class Canvas;

namespace battle
{
class Unit;
}

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class BattleInterface;
class CPicture;
class CFilledTexture;
class CButton;
class CToggleButton;
class CToggleGroup;
class CLabel;
class CTextBox;
class CAnimImage;
class CPlayerInterface;

/// Class which shows the console at the bottom of the battle screen and manages the text of the console
class BattleConsole : public CIntObject, public IStatusBar
{
private:
	std::vector< std::string > texts; //a place where texts are stored
	int lastShown; //last shown line of text

	std::string ingcAlter; //alternative text set by in-game console - very important!
public:
	BattleConsole(const Rect & position);
	void showAll(SDL_Surface * to = 0) override;

	bool addText(const std::string &text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void scrollUp(ui32 by = 1); //scrolls console up by 'by' positions
	void scrollDown(ui32 by = 1); //scrolls console up by 'by' positions

	void clearMatching(const std::string & Text) override;
	void clear() override;
	void write(const std::string & Text) override;
	void lock(bool shouldLock) override;

};

/// Hero battle animation
class BattleHero : public CIntObject
{
	void switchToNextPhase();
public:
	bool flip; //false if it's attacking hero, true otherwise

	std::shared_ptr<CAnimation> animation;
	std::shared_ptr<CAnimation> flagAnimation;

	const CGHeroInstance * myHero; //this animation's hero instance
	const BattleInterface * myOwner; //battle interface to which this animation is assigned
	int phase; //stage of animation
	int nextPhase; //stage of animation to be set after current phase is fully displayed
	int currentFrame, firstFrame, lastFrame; //frame of animation

	size_t flagAnim;
	ui8 animCount; //for flag animation
	void render(Canvas & canvas); //prints next frame of animation to to
	void setPhase(int newPhase); //sets phase of hero animation
	void hover(bool on) override;
	void clickLeft(tribool down, bool previousState) override; //call-in
	void clickRight(tribool down, bool previousState) override; //call-in
	BattleHero(const std::string & animationPath, bool filpG, PlayerColor player, const CGHeroInstance * hero, const BattleInterface * owner);
};

class HeroInfoWindow : public CWindowObject
{
private:
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CAnimImage>> icons;
public:
	HeroInfoWindow(const InfoAboutHero & hero, Point * position);
};

/// Class which manages the battle options window
class BattleOptionsWindow : public CWindowObject
{
private:
	std::shared_ptr<CButton> setToDefault;
	std::shared_ptr<CButton> exit;
	std::shared_ptr<CToggleGroup> animSpeeds;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CToggleButton>> toggles;
public:
	BattleOptionsWindow(BattleInterface * owner);

	void bDefaultf(); //default button callback
	void bExitf(); //exit button callback
};

/// Class which is responsible for showing the battle result window
class BattleResultWindow : public WindowBase
{
private:
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::shared_ptr<CButton> exit;
	std::vector<std::shared_ptr<CAnimImage>> icons;
	std::shared_ptr<CTextBox> description;
	CPlayerInterface & owner;
public:
	BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner);

	void bExitf(); //exit button callback

	void activate() override;
	void show(SDL_Surface * to = 0) override;
};

/// Class which stands for a single hex field on a battlefield
class ClickableHex : public CIntObject
{
private:
	bool setAlterText; //if true, this hex has set alternative text in console and will clean it
public:
	ui32 myNumber; //number of hex in commonly used format
	//bool accessible; //if true, this hex is accessible for units
	//CStack * ourStack;
	bool strictHovered; //for determining if hex is hovered by mouse (this is different problem than hex's graphic hovering)
	BattleInterface * myInterface; //interface that owns me

	//for user interactions
	void hover (bool on) override;
	void mouseMoved (const SDL_MouseMotionEvent &sEvent) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	ClickableHex();
};

/// Shows the stack queue
class StackQueue : public CIntObject
{
	class StackBox : public CIntObject
	{
		StackQueue * owner;
	public:
		std::shared_ptr<CPicture> background;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CLabel> amount;
		std::shared_ptr<CAnimImage> stateIcon;

		void setUnit(const battle::Unit * unit, size_t turn = 0);
		StackBox(StackQueue * owner);
	};

	static const int QUEUE_SIZE = 10;
	std::shared_ptr<CFilledTexture> background;
	std::vector<std::shared_ptr<StackBox>> stackBoxes;
	BattleInterface * owner;

	std::shared_ptr<CAnimation> icons;
	std::shared_ptr<CAnimation> stateIcons;

	int32_t getSiegeShooterIconID();
public:
	const bool embedded;

	StackQueue(bool Embedded, BattleInterface * _owner);
	void update();
};
