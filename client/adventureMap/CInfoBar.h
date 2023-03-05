/*
 * CInfoBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
struct Component;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

class CAnimImage;
class CShowableAnim;
class CComponent;
class CComponentBox;
class CHeroTooltip;
class CTownTooltip;
class CLabel;
class CTextBox;

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
		std::shared_ptr<CComponentBox> comps;
		std::shared_ptr<CTextBox> text;
	public:
		VisibleComponentInfo(const std::vector<Component> & compsToDisplay, std::string message);
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

	/// show components for 3 seconds. Used to display picked up resources. Can display up to 8 components
	void showComponents(const std::vector<Component> & comps, std::string message);

	/// print enemy turn progress
	void startEnemyTurn(PlayerColor color);

	/// reset to default view - selected object
	void showSelection();

	/// show hero\town information
	void showHeroSelection(const CGHeroInstance * hero);
	void showTownSelection(const CGTownInstance * town);

	/// for 3 seconds shows amount of town halls and players status
	void showGameStatus();

	/// check if infobar is showed something about pickups
	bool showingComponents();
};

