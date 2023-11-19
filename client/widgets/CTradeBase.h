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
	using updateSlotsFunctor = std::function<void()>;

	std::vector<std::shared_ptr<CTradeableItem>> slots;
	std::function<void()> updateSlotsCallback;
	std::shared_ptr<CTradeableItem> selected;

	virtual void updateSlots();
	virtual void deselect();
	virtual void clearSubtitles();
	void updateOffer(int slotIdx, int, int);
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

	SResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, updateSlotsFunctor updateSubtitles);
};

struct SArtifactsPanel : public STradePanel
{
	const std::vector<Point> slotsPos =
	{
		Point(0, 0), Point(83, 0), Point(166, 0),
		Point(0, 79), Point(83, 79), Point(166, 79),
		Point(83, 158)
	};
	const size_t artifactsForTrade = 7;

	SArtifactsPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, updateSlotsFunctor updateSubtitles,
		std::vector<TradeItemBuy> & arts);
};

class CTradeBase
{
public:
	const IMarket * market;
	const CGHeroInstance * hero;

	//all indexes: 1 = left, 0 = right
	std::array<std::vector<std::shared_ptr<CTradeableItem>>, 2> items;
	std::vector<std::shared_ptr<STradePanel>> tradePanels;

	//highlighted items (nullptr if no highlight)
	std::shared_ptr<CTradeableItem> hLeft;
	std::shared_ptr<CTradeableItem> hRight;
	std::shared_ptr<CButton> deal;

	CTradeBase(const IMarket * market, const CGHeroInstance * hero);
	void removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove);
	void removeItem(std::shared_ptr<CTradeableItem> item);
	void getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove);
	virtual void makeDeal() = 0;

protected:
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CButton>> buttons;
	std::vector<std::shared_ptr<CTextBox>> texts;
};
