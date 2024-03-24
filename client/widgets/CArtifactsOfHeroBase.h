/*
 * CArtifactsOfHeroBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactHolder.h"

class CButton;

class CArtifactsOfHeroBase : public CIntObject
{
protected:
	using ArtPlacePtr = std::shared_ptr<CHeroArtPlace>;
	using BpackScrollFunctor = std::function<void(int)>;

public:
	using ArtPlaceMap = std::map<ArtifactPosition, ArtPlacePtr>;
	using ClickFunctor = std::function<void(CArtifactsOfHeroBase&, CArtPlace&, const Point&)>;
	using PutBackPickedArtCallback = std::function<void()>;

	ClickFunctor clickPressedCallback;
	ClickFunctor showPopupCallback;
	ClickFunctor gestureCallback;
	
	CArtifactsOfHeroBase();
	virtual void putBackPickedArtifact();
	virtual void setPutBackPickedArtifactCallback(PutBackPickedArtCallback callback);
	virtual void clickPrassedArtPlace(CArtPlace & artPlace, const Point & cursorPosition);
	virtual void showPopupArtPlace(CArtPlace & artPlace, const Point & cursorPosition);
	virtual void gestureArtPlace(CArtPlace & artPlace, const Point & cursorPosition);
	virtual void setHero(const CGHeroInstance * hero);
	virtual const CGHeroInstance * getHero() const;
	virtual void scrollBackpack(bool left);
	virtual void markPossibleSlots(const CArtifactInstance * art, bool assumeDestRemoved = true);
	virtual void unmarkSlots();
	virtual ArtPlacePtr getArtPlace(const ArtifactPosition & slot);
	virtual void updateWornSlots();
	virtual void updateBackpackSlots();
	virtual void updateSlot(const ArtifactPosition & slot);
	virtual const CArtifactInstance * getPickedArtifact();
	void addGestureCallback(CArtPlace::ClickFunctor callback);

protected:
	const CGHeroInstance * curHero;
	ArtPlaceMap artWorn;
	std::vector<ArtPlacePtr> backpack;
	std::shared_ptr<CButton> leftBackpackRoll;
	std::shared_ptr<CButton> rightBackpackRoll;
	PutBackPickedArtCallback putBackPickedArtCallback;

	const std::vector<Point> slotPos =
	{
		Point(509,30),  Point(568,242), Point(509,80),  //0-2
		Point(383,69),  Point(562,184), Point(509,131), //3-5
		Point(431,69),  Point(610,184), Point(515,295), //6-8
		Point(383,143), Point(399,193), Point(415,244), //9-11
		Point(431,295), Point(564,30),  Point(610,30), //12-14
		Point(610,76),  Point(610,122), Point(610,310), //15-17
		Point(381,295) //18
	};

	virtual void init(const CHeroArtPlace::ClickFunctor & lClickCallback, const CHeroArtPlace::ClickFunctor & showPopupCallback,
		const Point & position, const BpackScrollFunctor & scrollCallback);
	// Assigns an artifacts to an artifact place depending on it's new slot ID
	virtual void setSlotData(ArtPlacePtr artPlace, const ArtifactPosition & slot);
};
