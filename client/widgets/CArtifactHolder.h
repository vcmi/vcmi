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
public:
	CArtPlace(Point position, const CArtifactInstance * art = nullptr);
	const CArtifactInstance* getArt();
	void lockSlot(bool on);
	bool isLocked() const;
	void selectSlot(bool on);
	bool isSelected() const;
	void showAll(Canvas & to) override;
	void setArtifact(const CArtifactInstance * art);

protected:
	std::shared_ptr<CAnimImage> image;
	const CArtifactInstance * ourArt;
	int imageIndex;
	std::shared_ptr<CAnimImage> selection;
	bool locked;

	void setInternals(const CArtifactInstance * artInst);
};

class CCommanderArtPlace : public CArtPlace
{
protected:
	const CGHeroInstance * commanderOwner;
	ArtifactPosition commanderSlotID;

	void returnArtToHeroCallback();

public:
	CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * art = nullptr);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
};

class CHeroArtPlace: public CArtPlace
{
public:
	using ClickFunctor = std::function<void(CHeroArtPlace&)>;

	ArtifactPosition slot;
	ClickFunctor leftClickCallback;
	ClickFunctor showPopupCallback;

	CHeroArtPlace(Point position, const CArtifactInstance * art = nullptr);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void addCombinedArtInfo(std::map<const CArtifact*, int> & arts);
};

namespace ArtifactUtilsClient
{
	bool askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot);
	bool askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot);
}
