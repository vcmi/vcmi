/*
 * CTradeWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CTradeBase.h"
#include "../widgets/CWindowWithArtifacts.h"
#include "CWindowObject.h"

class CSlider;
class CGStatusBar;

class CTradeWindow : public CTradeBase, public CWindowObject, public CWindowWithArtifacts //base for markets and altar of sacrifice
{
public:
	EType itemsType[2];

	EMarketMode mode;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CButton> max;

	std::shared_ptr<CSlider> slider; //for choosing amount to be exchanged
	bool readyToTrade;

	CTradeWindow(const ImagePath & bgName, const IMarket * Market, const CGHeroInstance * Hero, const std::function<void()> & onWindowClosed, EMarketMode Mode); //c

	void showAll(Canvas & to) override;
	void close() override;

	void initSubs(bool Left);
	void initTypes();
	void initItems(bool Left);
	std::vector<int> *getItemsIds(bool Left); //nullptr if default
	void getPositionsFor(std::vector<Rect> &poss, bool Left, EType type) const;
	void setMode(EMarketMode Mode); //mode setter

	void artifactSelected(CHeroArtPlace *slot); //used when selling artifacts -> called when user clicked on artifact slot

	virtual void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const = 0;
	virtual void selectionChanged(bool side) = 0; //true == left
	virtual Point selectionOffset(bool Left) const = 0;
	virtual std::string selectionSubtitle(bool Left) const = 0;
	virtual void garrisonChanged() = 0;
	virtual void artifactsChanged(bool left) = 0;
protected:
	std::function<void()> onWindowClosed;
	std::shared_ptr<CGStatusBar> statusBar;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CPicture>> images;
	std::vector<std::shared_ptr<CButton>> buttons;
	std::vector<std::shared_ptr<CTextBox>> texts;
};

class CMarketplaceWindow : public CTradeWindow
{
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CArtifactsOfHeroMarket> arts;

	bool printButtonFor(EMarketMode M) const;

	ImagePath getBackgroundForMode(EMarketMode mode);
public:
	int r1, r2; //suggested amounts of traded resources
	bool madeTransaction; //if player made at least one transaction
	std::shared_ptr<CTextBox> traderText;

	void setMax();
	void sliderMoved(int to);
	void makeDeal() override;
	void selectionChanged(bool side) override; //true == left
	CMarketplaceWindow(const IMarket * Market, const CGHeroInstance * Hero, const std::function<void()> & onWindowClosed, EMarketMode Mode);
	~CMarketplaceWindow();

	Point selectionOffset(bool Left) const override;
	std::string selectionSubtitle(bool Left) const override;

	void garrisonChanged() override; //removes creatures with count 0 from the list (apparently whole stack has been sold)
	void artifactsChanged(bool left) override;
	void resourceChanged();

	void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const override;
	void updateTraderText();
};

class CAltarWindow : public CTradeWindow
{
	std::shared_ptr<CAnimImage> artIcon;
public:
	std::vector<int> sacrificedUnits; //[slot_nr] -> how many creatures from that slot will be sacrificed
	std::vector<int> expPerUnit;

	std::shared_ptr<CButton> sacrificeAll;
	std::shared_ptr<CButton> sacrificeBackpack;
	std::shared_ptr<CLabel> expToLevel;
	std::shared_ptr<CLabel> expOnAltar;
	std::shared_ptr<CArtifactsOfHeroAltar> arts;

	CAltarWindow(const IMarket * Market, const CGHeroInstance * Hero, const std::function<void()> & onWindowClosed, EMarketMode Mode);
	~CAltarWindow();

	void getExpValues();

	void selectionChanged(bool side) override; //true == left
	void selectOppositeItem(bool side);
	void SacrificeAll();
	void SacrificeBackpack();

	void putOnAltar(int backpackIndex);
	bool putOnAltar(std::shared_ptr<CTradeableItem> altarSlot, const CArtifactInstance * art);
	void makeDeal() override;
	void showAll(Canvas & to) override;

	void blockTrade();
	void sliderMoved(int to);
	void getBaseForPositions(EType type, int &dx, int &dy, int &x, int &y, int &h, int &w, bool Right, int &leftToRightOffset) const override;
	void mimicCres();

	Point selectionOffset(bool Left) const override;
	std::string selectionSubtitle(bool Left) const override;
	void garrisonChanged() override;
	void artifactsChanged(bool left) override;
	void calcTotalExp();
	void setExpToLevel();
	void updateRight(std::shared_ptr<CTradeableItem> toUpdate);

	void artifactPicked();
	int firstFreeSlot();
	void moveArtToAltar(std::shared_ptr<CTradeableItem>, const CArtifactInstance * art);
};
