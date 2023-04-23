/*
 * CArtifactHolder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "MiscWidgets.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;
class CArtifactSet;
class CArtifactFittingSet;

VCMI_LIB_NAMESPACE_END

class CArtifactsOfHeroBase;
class CAnimImage;
class CButton;

class CArtifactHolder
{
public:
	virtual void artifactRemoved(const ArtifactLocation & artLoc)=0;
	virtual void artifactMoved(const ArtifactLocation & artLoc, const ArtifactLocation & destLoc, bool withRedraw)=0;
	virtual void artifactDisassembled(const ArtifactLocation & artLoc)=0;
	virtual void artifactAssembled(const ArtifactLocation & artLoc)=0;
};

class CArtPlace : public LRClickableAreaWTextComp
{
protected:
	std::shared_ptr<CAnimImage> image;
	const CArtifactInstance * ourArt;

	void setInternals(const CArtifactInstance * artInst);
	virtual void createImage()=0;

public:
	CArtPlace(Point position, const CArtifactInstance * Art = nullptr);
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	const CArtifactInstance * getArt();

	virtual void setArtifact(const CArtifactInstance * art)=0;
};

class CCommanderArtPlace : public CArtPlace
{
protected:
	const CGHeroInstance * commanderOwner;
	ArtifactPosition commanderSlotID;

	void createImage() override;
	void returnArtToHeroCallback();

public:
	CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * Art = nullptr);
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void setArtifact(const CArtifactInstance * art) override;
};

class CHeroArtPlace: public CArtPlace
{
public:
	using ClickHandler = std::function<void(CHeroArtPlace&)>;

	ArtifactPosition slot;
	ClickHandler leftClickCallback;
	ClickHandler rightClickCallback;

	CHeroArtPlace(Point position, const CArtifactInstance * Art = nullptr);
	void lockSlot(bool on);
	bool isLocked();
	void selectSlot(bool on);
	bool isMarked() const;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void showAll(SDL_Surface * to) override;
	void setArtifact(const CArtifactInstance * art) override;
	void addCombinedArtInfo(std::map<const CArtifact*, int> & arts);

protected:
	std::shared_ptr<CAnimImage> selection;
	bool locked;
	bool marked;

	void createImage() override;
};

class CArtifactsOfHeroBase : public CIntObject
{
protected:
	using ArtPlacePtr = std::shared_ptr<CHeroArtPlace>;
	using BpackScrollHandler = std::function<void(int)>;

public:
	using ArtPlaceMap = std::map<ArtifactPosition, ArtPlacePtr>;
	using ClickHandler = std::function<void(CArtifactsOfHeroBase&, CHeroArtPlace&)>;

	const CGHeroInstance * curHero;
	ClickHandler leftClickCallback;
	ClickHandler rightClickCallback;
	
	CArtifactsOfHeroBase();
	virtual ~CArtifactsOfHeroBase();
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
	ArtPlaceMap artWorn;
	std::vector<ArtPlacePtr> backpack;
	std::shared_ptr<CButton> leftBackpackRoll;
	std::shared_ptr<CButton> rightBackpackRoll;
	int backpackPos; // Position to display artifacts in heroes backpack

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

class CArtifactsOfHeroMain : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroMain(const Point & position);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickUpArtifact(CHeroArtPlace & artPlace);
};

class CArtifactsOfHeroKingdom : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroKingdom(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
		std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickUpArtifact(CHeroArtPlace & artPlace);
};

class CArtifactsOfHeroAltar : public CArtifactsOfHeroBase
{
public:
	std::set<const CArtifactInstance*> artifactsOnAltar;
	ArtifactPosition pickedArtFromSlot;
	std::shared_ptr<CArtifactFittingSet> visibleArtSet;

	CArtifactsOfHeroAltar(const Point & position);
	void setHero(const CGHeroInstance * hero) override;
	void updateWornSlots() override;
	void updateBackpackSlots() override;
	void scrollBackpack(int offset) override;
	void pickUpArtifact(CHeroArtPlace & artPlace);
	void swapArtifacts(const ArtifactLocation & srcLoc, const ArtifactLocation & dstLoc);
	void pickedArtMoveToAltar(const ArtifactPosition & slot);
	void deleteFromVisible(const CArtifactInstance * artInst);
};

class CArtifactsOfHeroMarket : public CArtifactsOfHeroBase
{
public:
	std::function<void(CHeroArtPlace*)> selectArtCallback;

	CArtifactsOfHeroMarket(const Point & position);
	void scrollBackpack(int offset) override;
};

class CWindowWithArtifacts : public CArtifactHolder
{
public:
	using CArtifactsOfHeroPtr = std::variant<
		std::weak_ptr<CArtifactsOfHeroMarket>,
		std::weak_ptr<CArtifactsOfHeroAltar>,
		std::weak_ptr<CArtifactsOfHeroKingdom>,
		std::weak_ptr<CArtifactsOfHeroMain>>;

	void addSet(CArtifactsOfHeroPtr artSet);
	const CGHeroInstance * getHeroPickedArtifact();
	const CArtifactInstance * getPickedArtifact();
	void leftClickArtPlaceHero(CArtifactsOfHeroBase & artsInst, CHeroArtPlace & artPlace);
	void rightClickArtPlaceHero(CArtifactsOfHeroBase & artsInst, CHeroArtPlace & artPlace);

	void artifactRemoved(const ArtifactLocation & artLoc) override;
	void artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw) override;
	void artifactDisassembled(const ArtifactLocation & artLoc) override;
	void artifactAssembled(const ArtifactLocation & artLoc) override;

private:
	std::vector<CArtifactsOfHeroPtr> artSets;

	void updateSlots(const ArtifactPosition & slot);
	std::optional<std::tuple<const CGHeroInstance*, const CArtifactInstance*>> getState();
	std::optional<CArtifactsOfHeroPtr> findAOHbyRef(CArtifactsOfHeroBase & artsInst);
};

namespace ArtifactUtils
{
	bool askToAssemble(const CGHeroInstance* hero, const ArtifactPosition& slot);
	bool askToDisassemble(const CGHeroInstance* hero, const ArtifactPosition& slot);
}
