/*
 * CWindowWithArtifacts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CArtifactsOfHeroMain.h"
#include "../widgets/CArtifactsOfHeroKingdom.h"
#include "../widgets/CArtifactsOfHeroAltar.h"
#include "../widgets/CArtifactsOfHeroMarket.h"
#include "../widgets/CArtifactsOfHeroBackpack.h"
#include "CWindowObject.h"

class CWindowWithArtifacts : virtual public CWindowObject
{
public:
	using ArtifactsOfHeroVar = std::variant<
		std::shared_ptr<CArtifactsOfHeroMarket>,
		std::shared_ptr<CArtifactsOfHeroAltar>,
		std::shared_ptr<CArtifactsOfHeroKingdom>,
		std::shared_ptr<CArtifactsOfHeroMain>,
		std::shared_ptr<CArtifactsOfHeroBackpack>,
		std::shared_ptr<CArtifactsOfHeroQuickBackpack>>;
	using CloseCallback = std::function<void()>;

	std::vector<ArtifactsOfHeroVar> artSets;
	CloseCallback closeCallback;

	explicit CWindowWithArtifacts(const std::vector<ArtifactsOfHeroVar> * artSets = nullptr);
	void addSet(ArtifactsOfHeroVar newArtSet);
	void addSetAndCallbacks(ArtifactsOfHeroVar newArtSet);
	void addCloseCallback(const CloseCallback & callback);
	const CGHeroInstance * getHeroPickedArtifact();
	const CArtifactInstance * getPickedArtifact();
	void clickPressedOnArtPlace(const CGHeroInstance & hero, const ArtifactPosition & slot,
		bool allowExchange, bool altarTrading, bool closeWindow);
	void swapArtifactAndClose(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const ArtifactLocation & dstLoc);
	void showArtifactAssembling(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void showArifactInfo(CArtPlace & artPlace, const Point & cursorPosition);
	void showQuickBackpackWindow(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void activate() override;
	void deactivate() override;
	void enableArtifactsCostumeSwitcher() const;

	virtual void artifactRemoved(const ArtifactLocation & artLoc);
	virtual void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw);
	virtual void artifactDisassembled(const ArtifactLocation & artLoc);
	virtual void artifactAssembled(const ArtifactLocation & artLoc);

protected:
	void update();
	void markPossibleSlots();
	bool checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance & hero, bool isTrade) const;
	void setCursorAnimation(const CArtifactInstance & artInst);
	void putPickedArtifact(const CGHeroInstance & curHero, const ArtifactPosition & targetSlot);
};
