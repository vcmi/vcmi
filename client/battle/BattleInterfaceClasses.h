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
struct InfoAboutHero;
class CStack;

namespace battle
{
class Unit;
}

VCMI_LIB_NAMESPACE_END

class CAnimation;
class Canvas;
class BattleInterface;
class CPicture;
class CFilledTexture;
class CButton;
class CToggleButton;
class CLabel;
class CTextBox;
class CAnimImage;
class TransparentFilledRectangle;
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

	void showAll(Canvas & to) override;
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
	void tick(uint32_t msPassed) override;

	float getFrame() const;
	void onPhaseFinished(const std::function<void()> &);

	void pause();
	void play();

	void heroLeftClicked();
	void heroRightClicked();

	BattleHero(const BattleInterface & owner, const CGHeroInstance * hero, bool defender);
};

class HeroInfoBasicPanel : public CIntObject //extracted from InfoWindow to fit better as non-popup embed element
{
private:
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CAnimImage>> icons;
public:
	HeroInfoBasicPanel(const InfoAboutHero & hero, Point * position, bool initializeBackground = true);

	void show(Canvas & to) override;

	void initializeData(const InfoAboutHero & hero);
	void update(const InfoAboutHero & updatedInfo);
};

class HeroInfoWindow : public CWindowObject
{
private:
	std::shared_ptr<HeroInfoBasicPanel> content;
public:
	HeroInfoWindow(const InfoAboutHero & hero, Point * position);
};

/// Class which is responsible for showing the battle result window
class BattleResultWindow : public WindowBase
{
private:
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> repeat;
	std::vector<std::shared_ptr<CAnimImage>> icons;
	std::shared_ptr<CTextBox> description;
	CPlayerInterface & owner;

	enum BattleResultVideo
	{
		NONE,
		WIN,
		SURRENDER,
		RETREAT,
		RETREAT_LOOP,
		DEFEAT,
		DEFEAT_LOOP,
		DEFEAT_SIEGE,
		DEFEAT_SIEGE_LOOP,
		WIN_SIEGE,
		WIN_SIEGE_LOOP,
	};
	BattleResultVideo currentVideo;

	void playVideo(bool startLoop = false);
	
	void buttonPressed(int button); //internal function for button callbacks
public:
	BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner, bool allowReplay = false);

	void bExitf(); //exit button callback
	void bRepeatf(); //repeat button callback
	std::function<void(int result)> resultCallback; //callback receiving which button was pressed

	void activate() override;
	void show(Canvas & to) override;
};

/// Shows the stack queue
class StackQueue : public CIntObject
{
	class StackBox : public CIntObject
	{
		StackQueue * owner;
		std::optional<uint32_t> boundUnitID;

		std::shared_ptr<CPicture> background;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CLabel> amount;
		std::shared_ptr<CAnimImage> stateIcon;
		std::shared_ptr<CLabel> round;
		std::shared_ptr<TransparentFilledRectangle> roundRect;

		void show(Canvas & to) override;
		void showAll(Canvas & to) override;

		bool isBoundUnitHighlighted() const;
	public:
		StackBox(StackQueue * owner);
		void setUnit(const battle::Unit * unit, size_t turn = 0, std::optional<ui32> currentTurn = std::nullopt);
		std::optional<uint32_t> getBoundUnitID() const;
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
	std::optional<uint32_t> getHoveredUnitIdIfAny() const;

	void show(Canvas & to) override;
};
