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
#include "CConfigHandler.h"

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
class CInteractableHeroTooltip;
class CTownTooltip;
class CInteractableTownTooltip;
class CLabel;
class CMultiLineLabel;

/// Info box which shows next week/day information, hold the current date
class CInfoBar : public CIntObject
{
private:
	/// Infobar actually have a fixed size
	/// Declare before to compute correct size of widgets
	static constexpr int width = 192;
	static constexpr int height = 192;
	static constexpr int offset = 4;

	//all visible information located in one object - for ease of replacing
	class CVisibleInfo : public CIntObject
	{
	public:
		static constexpr int offset_x = 8;
		static constexpr int offset_y = 12;

		void show(Canvas & to) override;

	protected:
		std::shared_ptr<CPicture> background;
		std::list<std::shared_ptr<CIntObject>> forceRefresh;

		CVisibleInfo();
	};

	static constexpr int data_width = width - 2 * CVisibleInfo::offset_x;
	static constexpr int data_height = height - 2 * CVisibleInfo::offset_y;

	class EmptyVisibleInfo : public CVisibleInfo
	{
	public:
		EmptyVisibleInfo();
	};

	class VisibleHeroInfo : public CVisibleInfo
	{
		std::shared_ptr<CIntObject> heroTooltip; //should have CHeroTooltip or CInteractableHeroTooltip;
	public:
		VisibleHeroInfo(const CGHeroInstance * hero);
	};

	class VisibleTownInfo : public CVisibleInfo
	{
		std::shared_ptr<CIntObject> townTooltip; //should have CTownTooltip or CInteractableTownTooltip;
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
		std::shared_ptr<CMultiLineLabel> text;
	public:
		struct Cache 
		{
			std::vector<Component> compsToDisplay;
			std::string message;
			int textH;
			bool tiny;
			Cache(std::vector<Component> comps, std::string msg, int textH, bool tiny):
				compsToDisplay(std::move(comps)),
				message(std::move(msg)),
				textH(textH),
				tiny(tiny)
			{}
		};
		VisibleComponentInfo(const Cache & c): VisibleComponentInfo(c.compsToDisplay, c.message, c.textH, c.tiny) {}
		VisibleComponentInfo(const std::vector<Component> & compsToDisplay, std::string message, int textH, bool tiny);
	};

	enum EState
	{
		EMPTY, HERO, TOWN, DATE, GAME, AITURN, COMPONENT
	};

	std::shared_ptr<CVisibleInfo> visibleInfo;
	EState state;
	uint32_t timerCounter;
	bool shouldPopAll = false;
	SettingsListener listener;

	std::queue<std::pair<VisibleComponentInfo::Cache, int>> componentsQueue;

	//private helper for showing components
	void showComponents(const std::vector<Component> & comps, std::string message, int textH, bool tiny, int timer);
	void pushComponents(const std::vector<Component> & comps, std::string message, int textH, bool tiny, int timer);
	void prepareComponents(const std::vector<Component> & comps, std::string message, int timer);
	void popComponents(bool remove = false);

	//removes all information about current state, deactivates timer (if any)
	void reset();

	void tick(uint32_t msPassed) override;

	void clickReleased(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;

	void playNewDaySound();
	void setTimer(uint32_t msToTrigger);
public:
	CInfoBar(const Rect & pos);
	CInfoBar(const Point & pos);

	/// show new day/week animation
	void showDate();

	/// show components for 3 seconds. Used to display picked up resources. Can display up to 8 components
	void pushComponents(const std::vector<Component> & comps, std::string message, int timer = 3000);

	/// Remove all queued components
	void popAll();

	/// Request infobar to pop all after next InfoWindow arrives.
	void requestPopAll();

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

	/// event handler for custom listening on game setting change
	void OnInfoBarCreatureManagementChanged();
};

