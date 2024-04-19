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

	std::vector<CArtifactsOfHeroPtr> artSets;
	CloseCallback closeCallback;

	explicit CWindowWithArtifacts(const std::vector<CArtifactsOfHeroPtr> * artSets = nullptr);
	void addSet(CArtifactsOfHeroPtr newArtSet);
	void addSetAndCallbacks(CArtifactsOfHeroPtr newArtSet);
	void addCloseCallback(const CloseCallback & callback);
	const CGHeroInstance * getHeroPickedArtifact();
	const CArtifactInstance * getPickedArtifact();
	void clickPressedArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void showPopupArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void gestureArtPlaceHero(const CArtifactsOfHeroBase & artsInst, CArtPlace & artPlace, const Point & cursorPosition);
	void activate() override;
	void deactivate() override;
	void enableArtifactsCostumeSwitcher();

	virtual void artifactRemoved(const ArtifactLocation & artLoc);
	virtual void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw);
	virtual void artifactDisassembled(const ArtifactLocation & artLoc);
	virtual void artifactAssembled(const ArtifactLocation & artLoc);

protected:
	void update() const;
	std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> getState();
	std::optional<CArtifactsOfHeroPtr> findAOHbyRef(const CArtifactsOfHeroBase & artsInst);
	void markPossibleSlots();
	bool checkSpecialArts(const CArtifactInstance & artInst, const CGHeroInstance * hero, bool isTrade) const;
	void setCursorAnimation(const CArtifactInstance & artInst);
};
