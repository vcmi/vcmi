/*
 * CMarketBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "TradePanels.h"
#include "../../widgets/Slider.h"
#include "../../render/EFont.h"
#include "../../render/Colors.h"

VCMI_LIB_NAMESPACE_BEGIN

class IMarket;

VCMI_LIB_NAMESPACE_END

class CMarketBase : public CIntObject
{
public:
	struct ShowcaseParams
	{
		std::string text;
		int imageIndex;
	};
	struct MarketShowcasesParams
	{
		std::optional<const ShowcaseParams> bidParams;
		std::optional<const ShowcaseParams> offerParams;
	};
	using ShowcasesParamsFunctor = std::function<const MarketShowcasesParams()>;

	const IMarket * market;
	const CGHeroInstance * hero;

	std::shared_ptr<TradePanelBase> bidTradePanel;
	std::shared_ptr<TradePanelBase> offerTradePanel;

	std::shared_ptr<CButton> deal;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CTextBox>> texts;
	int bidQty;
	int offerQty;
	const Point dealButtonPos = Point(270, 520);
	const Point titlePos = Point(299, 27);

	CMarketBase(const IMarket * market, const CGHeroInstance * hero);
	virtual void makeDeal() = 0;
	virtual void deselect();
	virtual void update();

protected:
	virtual void onSlotClickPressed(const std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<TradePanelBase> & curPanel);
	virtual void updateSubtitlesForBid(EMarketMode marketMode, int bidId);
	virtual void updateShowcases();
	virtual MarketShowcasesParams getShowcasesParams() const;
	virtual void highlightingChanged();
};

// Market subclasses
class CExperienceAltar : virtual public CMarketBase
{
public:
	std::shared_ptr<CLabel> expToLevel;
	std::shared_ptr<CLabel> expForHero;
	std::shared_ptr<CButton> sacrificeAllButton;

	CExperienceAltar();
	void deselect() override;
	void update() override;
	virtual void sacrificeAll() = 0;
	virtual TExpType calcExpAltarForHero() = 0;
};

class CCreaturesSelling : virtual public CMarketBase
{
public:
	CCreaturesSelling();
	bool slotDeletingCheck(const std::shared_ptr<CTradeableItem> & slot) const;
	void updateSubtitles() const;
};

class CResourcesBuying : virtual public CMarketBase
{
public:
	CResourcesBuying(const CTradeableItem::ClickPressedFunctor & clickPressedCallback,
		const TradePanelBase::UpdateSlotsFunctor & updSlotsCallback);
};

class CResourcesSelling : virtual public CMarketBase
{
public:
	explicit CResourcesSelling(const CTradeableItem::ClickPressedFunctor & clickPressedCallback);
	void updateSubtitles() const;
};

class CMarketSlider : virtual public CMarketBase
{
public:
	std::shared_ptr<CSlider> offerSlider;
	std::shared_ptr<CButton> maxAmount;
	const Point dealButtonPosWithSlider = Point(306, 520);

	explicit CMarketSlider(const CSlider::SliderMovingFunctor & movingCallback);
	void deselect() override;
	virtual void onOfferSliderMoved(int newVal);
};

class CMarketTraderText : virtual public CMarketBase
{
public:
	CMarketTraderText(const Point & pos = Point(316, 48), const EFonts & font = EFonts::FONT_SMALL, const ColorRGBA & Color = Colors::WHITE);
	void deselect() override;
	void makeDeal() override;

	const Point traderTextDimensions = Point(256, 75);
	std::shared_ptr<CTextBox> traderText;
	bool madeTransaction;

protected:
	void highlightingChanged() override;
	virtual std::string getTraderText() = 0;
};
