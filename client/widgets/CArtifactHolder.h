#pragma once

//#include "CComponent.h"
#include "MiscWidgets.h"

/*
 * CArtifactHolder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CArtifactsOfHero;
class CAnimImage;
class CButton;

struct ArtifactLocation;

class CArtifactHolder
{
public:
	CArtifactHolder();

	virtual void artifactRemoved(const ArtifactLocation &artLoc)=0;
	virtual void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)=0;
	virtual void artifactDisassembled(const ArtifactLocation &artLoc)=0;
	virtual void artifactAssembled(const ArtifactLocation &artLoc)=0;
};

class CWindowWithArtifacts : public CArtifactHolder
{
public:
	std::vector<CArtifactsOfHero *> artSets;

	void artifactRemoved(const ArtifactLocation &artLoc) override;
	void artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc) override;
	void artifactDisassembled(const ArtifactLocation &artLoc) override;
	void artifactAssembled(const ArtifactLocation &artLoc) override;
};

/// Artifacts can be placed there. Gets shown at the hero window
class CArtPlace: public LRClickableAreaWTextComp
{
	CAnimImage *image;
	CAnimImage *selection;

	void createImage();

public:
	// consider these members as const - change them only with appropriate methods e.g. lockSlot()
	bool locked;
	bool picked;
	bool marked;

	ArtifactPosition slotID; //Arts::EPOS enum + backpack starting from Arts::BACKPACK_START

	void lockSlot(bool on);
	void pickSlot(bool on);
	void selectSlot(bool on);

	CArtifactsOfHero * ourOwner;
	const CArtifactInstance * ourArt; // should be changed only with setArtifact()

	CArtPlace(Point position, const CArtifactInstance * Art = nullptr); //c-tor
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void select ();
	void deselect ();
	void showAll(SDL_Surface * to) override;
	bool fitsHere (const CArtifactInstance * art) const; //returns true if given artifact can be placed here

	void setMeAsDest(bool backpackAsVoid = true);
	void setArtifact(const CArtifactInstance *art);
	static bool askToAssemble(const CArtifactInstance *art, ArtifactPosition slot,
	                          const CGHeroInstance *hero);
};

/// Contains artifacts of hero. Distincts which artifacts are worn or backpacked
class CArtifactsOfHero : public CIntObject
{
	const CGHeroInstance * curHero;

	std::map<ArtifactPosition, CArtPlace *> artWorn;

	std::vector<CArtPlace *> backpack; //hero's visible backpack (only 5 elements!)
	int backpackPos; //number of first art visible in backpack (in hero's vector)

public:
	struct SCommonPart
	{
		struct Artpos
		{
			ArtifactPosition slotID;
			const CArtifactsOfHero *AOH;
			const CArtifactInstance *art;

			Artpos();
			void clear();
			void setTo(const CArtPlace *place, bool dontTakeBackpack);
			bool valid();
			bool operator==(const ArtifactLocation &al) const;
		} src, dst;

		std::set<CArtifactsOfHero *> participants; // Needed to mark slots.

		void reset();
	} * commonInfo; //when we have more than one CArtifactsOfHero in one window with exchange possibility, we use this (eg. in exchange window); to be provided externally

	bool updateState; // Whether the commonInfo should be updated on setHero or not.

	CButton * leftArtRoll, * rightArtRoll;
	bool allowedAssembling;
	std::multiset<const CArtifactInstance*> artifactsOnAltar; //artifacts id that are technically present in backpack but in GUI are moved to the altar - they'll be omitted in backpack slots
	std::function<void(CArtPlace*)> highlightModeCallback; //if set, clicking on art place doesn't pick artifact but highlights the slot and calls this function

	void realizeCurrentTransaction(); //calls callback with parameters stored in commonInfo
	void artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst);
	void artifactRemoved(const ArtifactLocation &al);
	void artifactAssembled(const ArtifactLocation &al);
	void artifactDisassembled(const ArtifactLocation &al);
	CArtPlace *getArtPlace(int slot);//may return null

	void setHero(const CGHeroInstance * hero);
	const CGHeroInstance *getHero() const;
	void dispose(); //free resources not needed after closing windows and reset state
	void scrollBackpack(int dir); //dir==-1 => to left; dir==1 => to right

	void safeRedraw();
	void markPossibleSlots(const CArtifactInstance* art);
	void unmarkSlots(bool withRedraw = true); //unmarks slots in all visible AOHs
	void unmarkLocalSlots(bool withRedraw = true); //unmarks slots in that particular AOH
	void setSlotData (CArtPlace* artPlace, ArtifactPosition slotID);
	void updateWornSlots (bool redrawParent = true);

	void updateSlot(ArtifactPosition i);
	void eraseSlotData (CArtPlace* artPlace, ArtifactPosition slotID);

	CArtifactsOfHero(const Point& position, bool createCommonPart = false);
	//Alternative constructor, used if custom artifacts positioning required (Kingdom interface)
	CArtifactsOfHero(std::map<ArtifactPosition, CArtPlace *> ArtWorn, std::vector<CArtPlace *> Backpack,
		CButton *leftScroll, CButton *rightScroll, bool createCommonPart = false);
	~CArtifactsOfHero(); //d-tor
	void updateParentWindow();
	friend class CArtPlace;
};
