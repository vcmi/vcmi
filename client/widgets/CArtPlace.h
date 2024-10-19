/*
 * CArtPlace.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "MiscWidgets.h"

class CAnimImage;

class CArtPlace : public SelectableSlot
{
public:
	using ClickFunctor = std::function<void(CArtPlace&, const Point&)>;

	ArtifactPosition slot;
	
	CArtPlace(Point position, const ArtifactID & artId = ArtifactID::NONE, const SpellID & spellId = SpellID::NONE);
	void setArtifact(const SpellID & spellId);
	void setArtifact(const ArtifactID & artId, const SpellID & spellId = SpellID::NONE);
	ArtifactID getArtifactId() const;
	void lockSlot(bool on);
	bool isLocked() const;
	void setClickPressedCallback(const ClickFunctor & callback);
	void setShowPopupCallback(const ClickFunctor & callback);
	void setGestureCallback(const ClickFunctor & callback);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void addCombinedArtInfo(const std::map<const ArtifactID, std::vector<ArtifactID>> & arts);

private:
	ArtifactID artId;
	SpellID spellId;
	bool locked;
	int32_t imageIndex;
	std::shared_ptr<CAnimImage> image;
	ClickFunctor clickPressedCallback;
	ClickFunctor showPopupCallback;
	ClickFunctor gestureCallback;
};

class CCommanderArtPlace : public CArtPlace
{
private:
	const CGHeroInstance * commanderOwner;
	ArtifactPosition commanderSlotID;

	void returnArtToHeroCallback();

public:
	CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot,
		const ArtifactID & artId = ArtifactID::NONE, const SpellID & spellId = SpellID::NONE);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
};
