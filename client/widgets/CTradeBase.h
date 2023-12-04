/*
 * CTradeBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Images.h"

#include "../lib/FunctionList.h"
#include "../lib/networkPacks/TradeItem.h"

VCMI_LIB_NAMESPACE_BEGIN

class IMarket;
class CGHeroInstance;
class SelectableSlot;

VCMI_LIB_NAMESPACE_END

class CButton;
class CTextBox;
class CSlider;

enum EType
{
	RESOURCE, PLAYER, ARTIFACT_TYPE, CREATURE, CREATURE_PLACEHOLDER, ARTIFACT_PLACEHOLDER, ARTIFACT_INSTANCE
};

class CTradeableItem : public CIntObject, public std::enable_shared_from_this<CTradeableItem>
{
	std::shared_ptr<CAnimImage> image;
	AnimationPath getFilename();
	int getIndex();
public:
	using ClickPressedFunctor = std::function<void(std::shared_ptr<CTradeableItem>)>;

	const CArtifactInstance * hlp; //holds ptr to artifact instance id type artifact
	EType type;
	int id;
	const int serial;
	const bool left;
	std::string subtitle; //empty if default
	ClickPressedFunctor clickPressedCallback;

	void setType(EType newType);
	void setID(int newID);

	const CArtifactInstance * getArtInstance() const;
	void setArtInstance(const CArtifactInstance * art);

	std::unique_ptr<SelectableSlot> selection;
	bool downSelection;

	void showAllAt(const Point & dstPos, const std::string & customSub, Canvas & to);

	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
	void clickPressed(const Point & cursorPosition) override;
	std::string getName(int number = -1) const;
	CTradeableItem(Point pos, EType Type, int ID, bool Left, int Serial);
};

struct STradePanel : public CIntObject
{
	using UpdateSlotsFunctor = std::function<void()>;
	using DeleteSlotsCheck = std::function<bool(std::shared_ptr<CTradeableItem> & slot)>;

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

struct SResourcesPanel : public STradePanel
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

	SResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles);
};

struct SArtifactsPanel : public STradePanel
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 79), Point(83, 79), Point(166, 79),
		Point(83, 158)
	};
	const size_t slotsForTrade = 7;

	SArtifactsPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles,
		std::vector<TradeItemBuy> & arts);
};

struct SPlayersPanel : public STradePanel
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 118), Point(83, 118), Point(166, 118),
		Point(83, 236)
	};
	SPlayersPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback);
};

struct SCreaturesPanel : public STradePanel
{
	using slotsData = std::vector<std::tuple<CreatureID, SlotID, int>>;

	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 98), Point(83, 98), Point(166, 98),
		Point(83, 196)
	};
	SCreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, slotsData & initialSlots);
	SCreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback,
		std::vector<std::shared_ptr<CTradeableItem>> & stsSlots, bool emptySlots = true);
};

class CTradeBase
{
public:
	const IMarket * market;
	const CGHeroInstance * hero;

	//all indexes: 1 = left, 0 = right
	std::array<std::vector<std::shared_ptr<CTradeableItem>>, 2> items;
	std::shared_ptr<STradePanel> leftTradePanel, rightTradePanel;

	//highlighted items (nullptr if no highlight)
	std::shared_ptr<CTradeableItem> hLeft;
	std::shared_ptr<CTradeableItem> hRight;
	std::shared_ptr<CButton> deal;
	std::shared_ptr<CSlider> offerSlider;

	CTradeBase(const IMarket * market, const CGHeroInstance * hero);
	void removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove);
	void removeItem(std::shared_ptr<CTradeableItem> item);
	void getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove);
	virtual void makeDeal() = 0;
	virtual void deselect();
	virtual void onSlotClickPressed(std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot);

protected:
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CButton>> buttons;
	std::vector<std::shared_ptr<CTextBox>> texts;
};

// Market subclasses
class CExpAltar : virtual public CTradeBase, virtual public CIntObject
{
public:
	std::shared_ptr<CLabel> expToLevel;
	std::shared_ptr<CLabel> expForHero;
	std::shared_ptr<CButton> sacrificeAllButton;
	const Point dealButtonPos = Point(269, 520);

	CExpAltar();
	virtual void sacrificeAll() = 0;
	virtual TExpType calcExpAltarForHero() = 0;
};

class CCreaturesSelling : virtual public CTradeBase, virtual public CIntObject
{
public:
	CCreaturesSelling();
	bool slotDeletingCheck(std::shared_ptr<CTradeableItem> & slot);
	void updateSubtitle();
};
