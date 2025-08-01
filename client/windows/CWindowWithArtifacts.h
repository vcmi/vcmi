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

class CWindowWithArtifacts : virtual public CWindowObject, public IArtifactsHolder
{
public:
	using CArtifactsOfHeroPtr = std::shared_ptr<CArtifactsOfHeroBase>;

	std::vector<CArtifactsOfHeroPtr> artSets;

	explicit CWindowWithArtifacts(const std::vector<CArtifactsOfHeroPtr> * artSets = nullptr);
	void addSet(const std::shared_ptr<CArtifactsOfHeroBase> & newArtSet);
	const CGHeroInstance * getHeroPickedArtifact() const;
	const CArtifactInstance * getPickedArtifact() const;
	void clickPressedOnArtPlace(const CGHeroInstance * hero, const ArtifactPosition & slot,
		bool allowExchange, bool altarTrading, bool closeWindow, const Point & cursorPosition);
	void swapArtifactAndClose(const CArtifactsOfHeroBase & artsInst, const ArtifactPosition & slot, const ArtifactLocation & dstLoc);
	void showArtifactAssembling(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition) const;
	void showQuickBackpackWindow(const CGHeroInstance * hero, const ArtifactPosition & slot, const Point & cursorPosition) const;
	void activate() override;
	void deactivate() override;
	void enableKeyboardShortcuts() const;

	void updateArtifacts() override;

protected:
	void markPossibleSlots() const;
	bool checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance & hero, bool isTrade) const;
	void setCursorAnimation(const CArtifactInstance & artInst) const;
	void putPickedArtifact(const CGHeroInstance & curHero, const ArtifactPosition & targetSlot) const;
	void onClickPressedCommonArtifact(const CGHeroInstance & curHero, const ArtifactPosition & slot, bool closeWindow);
};
