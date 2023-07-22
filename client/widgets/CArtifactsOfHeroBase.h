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

class CArtifactsOfHeroBase : public CIntObject
{
protected:
	using ArtPlacePtr = std::shared_ptr<CHeroArtPlace>;
	using BpackScrollHandler = std::function<void(int)>;

public:
	using ArtPlaceMap = std::map<ArtifactPosition, ArtPlacePtr>;
	using ClickHandler = std::function<void(CArtifactsOfHeroBase&, CHeroArtPlace&)>;
	using PutBackPickedArtCallback = std::function<void()>;

	ClickHandler leftClickCallback;
	ClickHandler rightClickCallback;
	
	CArtifactsOfHeroBase();
	virtual void putBackPickedArtifact();
	virtual void setPutBackPickedArtifactCallback(PutBackPickedArtCallback callback);
	virtual void leftClickArtPlace(CHeroArtPlace & artPlace);
	virtual void rightClickArtPlace(CHeroArtPlace & artPlace);
	virtual void setHero(const CGHeroInstance * hero);
	virtual const CGHeroInstance * getHero() const;
	virtual void scrollBackpack(int offset);
	virtual void safeRedraw();
	virtual void markPossibleSlots(const CArtifactInstance * art, bool assumeDestRemoved = true);
	virtual void unmarkSlots();
	virtual ArtPlacePtr getArtPlace(const ArtifactPosition & slot);
	virtual void updateWornSlots();
	virtual void updateBackpackSlots();
	virtual void updateSlot(const ArtifactPosition & slot);
	virtual const CArtifactInstance * getPickedArtifact();

protected:
	const CGHeroInstance * curHero;
	ArtPlaceMap artWorn;
	std::vector<ArtPlacePtr> backpack;
	std::shared_ptr<CButton> leftBackpackRoll;
	std::shared_ptr<CButton> rightBackpackRoll;
	int backpackPos; // Position to display artifacts in heroes backpack
	PutBackPickedArtCallback putBackPickedArtCallback;

	const std::vector<Point> slotPos =
	{
		Point(509,30),  Point(567,240), Point(509,80),  //0-2
		Point(383,68),  Point(564,183), Point(509,130), //3-5
		Point(431,68),  Point(610,183), Point(515,295), //6-8
		Point(383,143), Point(399,194), Point(415,245), //9-11
		Point(431,296), Point(564,30),  Point(610,30), //12-14
		Point(610,76),  Point(610,122), Point(610,310), //15-17
		Point(381,296) //18
	};

	virtual void init(CHeroArtPlace::ClickHandler lClickCallback, CHeroArtPlace::ClickHandler rClickCallback,
		const Point & position, BpackScrollHandler scrollHandler);
	// Assigns an artifacts to an artifact place depending on it's new slot ID
	virtual void setSlotData(ArtPlacePtr artPlace, const ArtifactPosition & slot, const CArtifactSet & artSet);
	virtual void scrollBackpackForArtSet(int offset, const CArtifactSet & artSet);
};
