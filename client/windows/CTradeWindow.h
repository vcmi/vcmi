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

#include "../widgets/markets/CTradeBase.h"
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
	void setMode(EMarketMode Mode); //mode setter

	void artifactSelected(CArtPlace * slot); //used when selling artifacts -> called when user clicked on artifact slot
	virtual void selectionChanged(bool side) = 0; //true == left
	virtual Point selectionOffset(bool Left) const = 0;
	virtual std::string updateSlotSubtitle(bool Left) const = 0;
	virtual void updateGarrison() = 0;
	virtual void artifactsChanged(bool left) = 0;
protected:
	std::function<void()> onWindowClosed;
	std::shared_ptr<CGStatusBar> statusBar;
	std::vector<std::shared_ptr<CPicture>> images;
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
	std::string updateSlotSubtitle(bool Left) const override;

	void updateGarrison() override; //removes creatures with count 0 from the list (apparently whole stack has been sold)
	void artifactsChanged(bool left) override;
	void resourceChanged();
	void updateTraderText();
};
