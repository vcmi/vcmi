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

VCMI_LIB_NAMESPACE_END

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
	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
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
	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
	void setArtifact(const CArtifactInstance * art) override;
	void addCombinedArtInfo(std::map<const CArtifact*, int> & arts);

protected:
	std::shared_ptr<CAnimImage> selection;
	bool locked;
	bool marked;

	void createImage() override;
};

namespace ArtifactUtilsClient
{
	bool askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot);
	bool askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot);
}
