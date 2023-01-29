/*
 * BattleInterfaceClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleConstants.h"
#include "../gui/CIntObject.h"
#include "../../lib/FunctionList.h"
#include "../../lib/battle/BattleHex.h"
#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
struct BattleResult;
class CStack;

namespace battle
{
class Unit;
}

VCMI_LIB_NAMESPACE_END

class Canvas;
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
class BattleRenderer;

/// Class which shows the console at the bottom of the battle screen and manages the text of the console
class BattleConsole : public CIntObject, public IStatusBar
{
private:
	std::shared_ptr<CPicture> background;

	/// List of all texts added during battle, essentially - log of entire battle
	std::vector< std::string > logEntries;

	/// Current scrolling position, to allow showing older entries via scroll buttons
	int scrollPosition;

	/// current hover text set on mouse move, takes priority over log entries
	std::string hoverText;

	/// current text entered via in-game console, takes priority over both log entries and hover text
	std::string consoleText;

	/// if true then we are currently entering console text
	bool enteringText;

	/// splits text into individual strings for battle log
	std::vector<std::string> splitText(const std::string &text);

	/// select line(s) that will be visible in UI
	std::vector<std::string> getVisibleText();
public:
	BattleConsole(std::shared_ptr<CPicture> backgroundSource, const Point & objectPos, const Point & imagePos, const Point &size);

	void showAll(SDL_Surface * to) override;
	void deactivate() override;

	bool addText(const std::string &text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void scrollUp(ui32 by = 1); //scrolls console up by 'by' positions
	void scrollDown(ui32 by = 1); //scrolls console up by 'by' positions

	// IStatusBar interface
	void write(const std::string & Text) override;
	void clearIfMatching(const std::string & Text) override;
	void clear() override;
	void setEnteringMode(bool on) override;
	void setEnteredText(const std::string & text) override;
};

/// Hero battle animation
class BattleHero : public CIntObject
{
	bool defender;

	CFunctionList<void()> phaseFinishedCallback;

	std::shared_ptr<CAnimation> animation;
	std::shared_ptr<CAnimation> flagAnimation;

	const CGHeroInstance * hero; //this animation's hero instance
	const BattleInterface & owner; //battle interface to which this animation is assigned

	EHeroAnimType phase; //stage of animation
	EHeroAnimType nextPhase; //stage of animation to be set after current phase is fully displayed

	float currentSpeed;
	float currentFrame; //frame of animation
	float flagCurrentFrame;

	void switchToNextPhase();

	void render(Canvas & canvas); //prints next frame of animation to to
public:
	const CGHeroInstance * instance();

	void setPhase(EHeroAnimType newPhase); //sets phase of hero animation

	void collectRenderableObjects(BattleRenderer & renderer);

	float getFrame() const;
	void onPhaseFinished(const std::function<void()> &);

	void pause();
	void play();

	void heroLeftClicked();
	void heroRightClicked();

	BattleHero(const BattleInterface & owner, const CGHeroInstance * hero, bool defender);
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
	BattleOptionsWindow(BattleInterface & owner);

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
	BattleInterface & owner;

	std::shared_ptr<CAnimation> icons;
	std::shared_ptr<CAnimation> stateIcons;

	int32_t getSiegeShooterIconID();
public:
	const bool embedded;

	StackQueue(bool Embedded, BattleInterface & owner);
	void update();

	void show(SDL_Surface * to) override;
};
