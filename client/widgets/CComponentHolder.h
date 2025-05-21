/*
 * CComponentHolder.h, part of VCMI engine
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

class CComponentHolder : public SelectableSlot
{
public:
	using ClickFunctor = std::function<void(CComponentHolder&, const Point&)>;

	ClickFunctor clickPressedCallback;
	ClickFunctor showPopupCallback;
	ClickFunctor gestureCallback;
	std::shared_ptr<CAnimImage> image;

	CComponentHolder(const Rect & area, const Point & selectionOversize);
	void setClickPressedCallback(const ClickFunctor & callback);
	void setShowPopupCallback(const ClickFunctor & callback);
	void setGestureCallback(const ClickFunctor & callback);
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
};

class CArtPlace : public CComponentHolder
{
public:
	ArtifactPosition slot;
	
	CArtPlace(Point position, const ArtifactID & newArtId = ArtifactID::NONE, const SpellID & newSpellId = SpellID::NONE);
	void setArtifact(const SpellID & newSpellId);
	void setArtifact(const ArtifactID & newArtId, const SpellID & newSpellId = SpellID::NONE);
	ArtifactID getArtifactId() const;
	void lockSlot(bool on);
	bool isLocked() const;
	void addCombinedArtInfo(const std::map<const ArtifactID, std::vector<ArtifactID>> & arts);
	void addChargedArtInfo(const uint16_t charges);

private:
	ArtifactID artId;
	SpellID spellId;
	bool locked;
	int32_t imageIndex;
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

class CSecSkillPlace : public CComponentHolder
{
public:
	enum class ImageSize
	{
		LARGE,
		MEDIUM,
		SMALL
	};

	CSecSkillPlace(const Point & position, const ImageSize & imageSize, const SecondarySkill & skillId = SecondarySkill::NONE, const uint8_t level = 0);
	void setSkill(const SecondarySkill & newSkillId, const uint8_t level = 0);
	void setLevel(const uint8_t level);

private:
	SecondarySkill skillId;
};
