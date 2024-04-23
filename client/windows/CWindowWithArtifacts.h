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
	using CArtifactsOfHeroPtr = std::variant<
		std::weak_ptr<CArtifactsOfHeroMarket>,
		std::weak_ptr<CArtifactsOfHeroAltar>,
		std::weak_ptr<CArtifactsOfHeroKingdom>,
		std::weak_ptr<CArtifactsOfHeroMain>,
		std::weak_ptr<CArtifactsOfHeroBackpack>,
		std::weak_ptr<CArtifactsOfHeroQuickBackpack>>;
	using CloseCallback = std::function<void()>;

	explicit CWindowWithArtifacts(const std::vector<CArtifactsOfHeroPtr> * artSets = nullptr);
	void addSet(CArtifactsOfHeroPtr artSet);
	void addSetAndCallbacks(CArtifactsOfHeroPtr artSet);
	void addCloseCallback(CloseCallback callback);
	const CGHeroInstance * getHeroPickedArtifact();
	const CArtifactInstance * getPickedArtifact();
	void clickPressedArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void showPopupArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void gestureArtPlaceHero(CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void activate() override;
	void deactivate() override;

	virtual void artifactRemoved(const ArtifactLocation & artLoc);
	virtual void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw);
	virtual void artifactDisassembled(const ArtifactLocation & artLoc);
	virtual void artifactAssembled(const ArtifactLocation & artLoc);

protected:
	std::vector<CArtifactsOfHeroPtr> artSets;
	CloseCallback closeCallback;

	void updateSlots();
	std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> getState();
	std::optional<CArtifactsOfHeroPtr> findAOHbyRef(CArtifactsOfHeroBase & artsInst);
	void markPossibleSlots();
	bool checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance * hero, bool isTrade);
	void setCursorAnimation(const CArtifactInstance & artInst);
};
