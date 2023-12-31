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

#include "CArtifactHolder.h"
#include "CArtifactsOfHeroMain.h"
#include "CArtifactsOfHeroKingdom.h"
#include "CArtifactsOfHeroAltar.h"
#include "CArtifactsOfHeroMarket.h"
#include "CArtifactsOfHeroBackpack.h"

class CWindowWithArtifacts : public CArtifactHolder
{
public:
	using CArtifactsOfHeroPtr = std::variant<
		std::weak_ptr<CArtifactsOfHeroMarket>,
		std::weak_ptr<CArtifactsOfHeroAltar>,
		std::weak_ptr<CArtifactsOfHeroKingdom>,
		std::weak_ptr<CArtifactsOfHeroMain>,
		std::weak_ptr<CArtifactsOfHeroBackpack>,
		std::weak_ptr<CArtifactsOfHeroQuickBackpack>>;
	using CloseCallback = std::function<void()>;

	void addSet(CArtifactsOfHeroPtr artSet);
	void addSetAndCallbacks(CArtifactsOfHeroPtr artSet);
	void addCloseCallback(CloseCallback callback);
	const CGHeroInstance * getHeroPickedArtifact();
	const CArtifactInstance * getPickedArtifact();
	void clickPressedArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void showPopupArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void gestureArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);

	void artifactRemoved(const ArtifactLocation & artLoc) override;
	void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw) override;
	void artifactDisassembled(const ArtifactLocation & artLoc) override;
	void artifactAssembled(const ArtifactLocation & artLoc) override;

protected:
	std::vector<CArtifactsOfHeroPtr> artSets;
	CloseCallback closeCallback;

	void updateSlots();
	std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> getState();
	std::optional<CArtifactsOfHeroPtr> findAOHbyRef(CArtifactsOfHeroBase & artsInst);
	void markPossibleSlots();
};
