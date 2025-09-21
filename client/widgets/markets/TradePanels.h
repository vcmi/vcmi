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

class CSlider;

enum class EType
{
	RESOURCE, PLAYER, ARTIFACT_TYPE, CREATURE, ARTIFACT
};

class CTradeableItem : public SelectableSlot, public std::enable_shared_from_this<CTradeableItem>
{
public:
	std::shared_ptr<CAnimImage> image;
	AnimationPath getFilename();
	int getIndex();
	using ClickPressedFunctor = std::function<void(const std::shared_ptr<CTradeableItem>&)>;

	EType type;
	int32_t id;
	const int32_t serial;
	std::shared_ptr<CLabel> subtitle;
	ClickPressedFunctor clickPressedCallback;

	void setType(EType newType);
	void setID(int32_t newID);
	void clear();

	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
	void clickPressed(const Point & cursorPosition) override;
	CTradeableItem(const Rect & area, EType Type, int32_t ID, int32_t serial);
};

class TradePanelBase : public CIntObject
{
public:
	using UpdateSlotsFunctor = std::function<void()>;
	using DeleteSlotsCheck = std::function<bool(const std::shared_ptr<CTradeableItem>&)>;

	std::vector<std::shared_ptr<CTradeableItem>> slots;
	UpdateSlotsFunctor updateSlotsCallback;
	DeleteSlotsCheck deleteSlotsCheck;
	const int selectionWidth = 2;
	std::shared_ptr<CTradeableItem> showcaseSlot;		// Separate slot that displays the contents for trading
	std::shared_ptr<CTradeableItem> highlightedSlot;	// One of the slots highlighted by a frame

	virtual void update();
	virtual void deselect();
	virtual void clearSubtitles();
	void updateOffer(CTradeableItem & slot, int, int);
	void setShowcaseSubtitle(const std::string & text);
	int32_t getHighlightedItemId() const;
	void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot);
	bool isHighlighted() const;
};

class ResourcesPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0),   Point(83, 0),   Point(166, 0),
		Point(0, 79),  Point(83, 79),  Point(166, 79),
		Point(0, 158), Point(83, 158), Point(166, 158)
	};
	const Point slotDimension = Point(69, 66);
	const Point selectedPos = Point(83, 267);

	CTradeableItem::ClickPressedFunctor clickPressedCallback;

	std::vector<GameResID> resourcesForTrade;
	std::shared_ptr<CSlider> slider;

	void updateSlots(int line);
public:
	ResourcesPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback, const UpdateSlotsFunctor & updateSubtitles);
};

class ArtifactsPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(165, 0),
		Point(0, 79), Point(83, 79), Point(165, 79),
		Point(83, 158)
	};
	const size_t slotsForTrade = 7;
	const Point slotDimension = Point(69, 68);
	const Point selectedPos = Point(83, 266);

public:
	ArtifactsPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback,
		const UpdateSlotsFunctor & updateSubtitles, const std::vector<TradeItemBuy> & arts);
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
	const Point selectedPos = Point(83, 367);

public:
	explicit PlayersPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback);
};

class CreaturesPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 98), Point(83, 98), Point(166, 98),
		Point(83, 196)
	};
	const Point slotDimension = Point(59, 64);
	const Point selectedPos = Point(83, 327);

public:
	using slotsData = std::vector<std::tuple<CreatureID, SlotID, int>>;

	CreaturesPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback, const slotsData & initialSlots);
	CreaturesPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback,
		const std::vector<std::shared_ptr<CTradeableItem>> & srcSlots, bool emptySlots = true);
};

class ArtifactsAltarPanel : public TradePanelBase
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(54, 0), Point(108, 0),
		Point(162, 0), Point(216, 0), Point(0, 70),
		Point(54, 70), Point(108, 70), Point(162, 70),
		Point(216, 70), Point(0, 140), Point(54, 140),
		Point(108, 140), Point(162, 140), Point(216, 140),
		Point(0, 210), Point(54, 210), Point(108, 210),
		Point(162, 210), Point(216, 210), Point(81, 280),
		Point(135, 280)
	};
	const Point slotDimension = Point(69, 66);
	const Point selectedPos = Point(-48, 389);

public:
	explicit ArtifactsAltarPanel(const CTradeableItem::ClickPressedFunctor & clickPressedCallback);
};
