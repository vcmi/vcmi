/*
 * TradePanels.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../MiscWidgets.h"
#include "../Images.h"

#include "../../../lib/networkPacks/TradeItem.h"

enum class EType
{
	RESOURCE, PLAYER, ARTIFACT_TYPE, CREATURE, CREATURE_PLACEHOLDER, ARTIFACT_PLACEHOLDER, ARTIFACT_INSTANCE
};

class CTradeableItem : public SelectableSlot, public std::enable_shared_from_this<CTradeableItem>
{
public:
	std::shared_ptr<CAnimImage> image;
	AnimationPath getFilename();
	int getIndex();
	using ClickPressedFunctor = std::function<void(const std::shared_ptr<CTradeableItem>&)>;

	const CArtifactInstance * artInstance; //holds ptr to artifact instance id type artifact
	EType type;
	int id;
	const int serial;
	const bool left;
	std::string subtitle;
	ClickPressedFunctor clickPressedCallback;

	void setType(EType newType);
	void setID(int newID);

	const CArtifactInstance * getArtInstance() const;
	void setArtInstance(const CArtifactInstance * art);

	bool downSelection;

	void showAllAt(const Point & dstPos, const std::string & customSub, Canvas & to);

	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
	void clickPressed(const Point & cursorPosition) override;
	std::string getName(int number = -1) const;
	CTradeableItem(const Rect & area, EType Type, int ID, bool Left, int Serial);
};

class TradePanelBase : public CIntObject
{
public:
	using UpdateSlotsFunctor = std::function<void()>;
	using DeleteSlotsCheck = std::function<bool(const std::shared_ptr<CTradeableItem>&)>;

	std::vector<std::shared_ptr<CTradeableItem>> slots;
	UpdateSlotsFunctor updateSlotsCallback;
	DeleteSlotsCheck deleteSlotsCheck;
	std::shared_ptr<CTradeableItem> selected;
	const int selectionWidth = 2;

	virtual void updateSlots();
	virtual void deselect();
	virtual void clearSubtitles();
	void updateOffer(CTradeableItem & slot, int, int);
	void deleteSlots();
};

class ResourcesPanel : public TradePanelBase
{
	const std::vector<GameResID> resourcesForTrade =
	{
		GameResID::WOOD, GameResID::MERCURY, GameResID::ORE,
		GameResID::SULFUR, GameResID::CRYSTAL, GameResID::GEMS,
		GameResID::GOLD
	};
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 79), Point(83, 79), Point(166, 79),
		Point(83, 158)
	};
	const Point slotDimension = Point(69, 66);

public:
	ResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles);
};

class ArtifactsPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 79), Point(83, 79), Point(166, 79),
		Point(83, 158)
	};
	const size_t slotsForTrade = 7;
	const Point slotDimension = Point(69, 66);

public:
	ArtifactsPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles,
		const std::vector<TradeItemBuy> & arts);
};

class PlayersPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 118), Point(83, 118), Point(166, 118),
		Point(83, 236)
	};
	const Point slotDimension = Point(58, 64);

public:
	explicit PlayersPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback);
};

class CreaturesPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 98), Point(83, 98), Point(166, 98),
		Point(83, 196)
	};
	const Point slotDimension = Point(58, 64);

public:
	using slotsData = std::vector<std::tuple<CreatureID, SlotID, int>>;

	CreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, const slotsData & initialSlots);
	CreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback,
		const std::vector<std::shared_ptr<CTradeableItem>> & srcSlots, bool emptySlots = true);
};
