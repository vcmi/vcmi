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

VCMI_LIB_NAMESPACE_END

class CArtifactsOfHero;
class CAnimImage;
class CButton;

class CArtifactHolder
{
public:
	CArtifactHolder();

	virtual void artifactRemoved(const ArtifactLocation &artLoc)=0;
	virtual void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc, bool withRedraw)=0;
	virtual void artifactDisassembled(const ArtifactLocation &artLoc)=0;
	virtual void artifactAssembled(const ArtifactLocation &artLoc)=0;
};

class CArtPlace : public LRClickableAreaWTextComp
{
protected:
	std::shared_ptr<CAnimImage> image;
	virtual void createImage()=0;
public:
	const CArtifactInstance * ourArt; // should be changed only with setArtifact()

	CArtPlace(Point position, const CArtifactInstance * Art = nullptr);
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;

	virtual void setArtifact(const CArtifactInstance *art)=0;
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

	virtual void setArtifact(const CArtifactInstance * art) override;

};

/// Artifacts can be placed there. Gets shown at the hero window
class CHeroArtPlace: public CArtPlace
{
	std::shared_ptr<CAnimImage> selection;

	void createImage() override;

public:
	// consider these members as const - change them only with appropriate methods e.g. lockSlot()
	bool locked;
	bool picked;
	bool marked;

	ArtifactPosition slotID; //Arts::EPOS enum + backpack starting from Arts::BACKPACK_START

	CArtifactsOfHero * ourOwner;

	CHeroArtPlace(Point position, const CArtifactInstance * Art = nullptr);

	void lockSlot(bool on);
	void pickSlot(bool on);
	void selectSlot(bool on);

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void select();
	void showAll(SDL_Surface * to) override;
	bool fitsHere (const CArtifactInstance * art) const; //returns true if given artifact can be placed here

	void setMeAsDest(bool backpackAsVoid = true);
	void setArtifact(const CArtifactInstance *art) override;
	static bool askToAssemble(const CArtifactInstance *art, ArtifactPosition slot,
	                          const CGHeroInstance *hero);
};

/// Contains artifacts of hero. Distincts which artifacts are worn or backpacked
class CArtifactsOfHero : public CIntObject
{
public:
	using ArtPlacePtr = std::shared_ptr<CHeroArtPlace>;
	using ArtPlaceMap = std::map<ArtifactPosition, ArtPlacePtr>;

	struct SCommonPart
	{
		struct Artpos
		{
			ArtifactPosition slotID;
			const CArtifactsOfHero *AOH;
			const CArtifactInstance *art;

			void clear();
			void setTo(const CHeroArtPlace *place, bool dontTakeBackpack);
			bool valid();
			bool operator==(const ArtifactLocation &al) const;
		} src, dst;

		std::set<CArtifactsOfHero *> participants; // Needed to mark slots.

		void reset();
	};
	std::shared_ptr<SCommonPart> commonInfo; //when we have more than one CArtifactsOfHero in one window with exchange possibility, we use this (eg. in exchange window); to be provided externally

	std::shared_ptr<CButton> leftArtRoll;
	std::shared_ptr<CButton> rightArtRoll;
	bool allowedAssembling;

	std::multiset<const CArtifactInstance*> artifactsOnAltar; //artifacts id that are technically present in backpack but in GUI are moved to the altar - they'll be omitted in backpack slots
	std::function<void(CHeroArtPlace*)> highlightModeCallback; //if set, clicking on art place doesn't pick artifact but highlights the slot and calls this function

	void realizeCurrentTransaction(); //calls callback with parameters stored in commonInfo
	void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst, bool withUIUpdate);
	void artifactRemoved(const ArtifactLocation &al);
	void artifactUpdateSlots(const ArtifactLocation &al);
	ArtPlacePtr getArtPlace(ArtifactPosition slot);//may return null

	void setHero(const CGHeroInstance * hero);
	const CGHeroInstance *getHero() const;
	void dispose(); //free resources not needed after closing windows and reset state
	void scrollBackpack(int dir); //dir==-1 => to left; dir==1 => to right

	void activate() override;
	void deactivate() override;

	void safeRedraw();
	void markPossibleSlots(const CArtifactInstance * art, bool withRedraw = false);
	void unmarkSlots(bool withRedraw = false); //unmarks slots in all visible AOHs
	void unmarkLocalSlots(bool withRedraw = false); //unmarks slots in that particular AOH
	void updateWornSlots(bool redrawParent = false);
	void updateBackpackSlots(bool redrawParent = false);

	void updateSlot(ArtifactPosition i);

	CArtifactsOfHero(const Point& position, bool createCommonPart = false);
	//Alternative constructor, used if custom artifacts positioning required (Kingdom interface)
	CArtifactsOfHero(ArtPlaceMap ArtWorn, std::vector<ArtPlacePtr> Backpack,
		std::shared_ptr<CButton> leftScroll, std::shared_ptr<CButton> rightScroll, bool createCommonPart = false);
	~CArtifactsOfHero();
	void updateParentWindow();
	friend class CHeroArtPlace;

private:

	const CGHeroInstance * curHero;

	ArtPlaceMap artWorn;

	std::vector<ArtPlacePtr> backpack; //hero's visible backpack (only 5 elements!)
	int backpackPos; //number of first art visible in backpack (in hero's vector)

	void eraseSlotData(ArtPlacePtr artPlace, ArtifactPosition slotID);
	void setSlotData(ArtPlacePtr artPlace, ArtifactPosition slotID);
};

class CWindowWithArtifacts : public CArtifactHolder
{
	std::vector<std::weak_ptr<CArtifactsOfHero>> artSets;
public:
	void addSet(std::shared_ptr<CArtifactsOfHero> artSet);

	std::shared_ptr<CArtifactsOfHero::SCommonPart> getCommonPart();

	void artifactRemoved(const ArtifactLocation &artLoc) override;
	void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc, bool withRedraw) override;
	void artifactDisassembled(const ArtifactLocation &artLoc) override;
	void artifactAssembled(const ArtifactLocation &artLoc) override;
};
