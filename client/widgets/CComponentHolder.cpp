/*
 * CComponentHolder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CComponentHolder.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"

#include "CComponent.h"
#include "Images.h"

#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/IRenderHandler.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CComponentHolder::CComponentHolder(const Rect & area, const Point & selectionOversize)
	: SelectableSlot(area, selectionOversize)
{
	setClickPressedCallback([this](const CComponentHolder &, const Point & cursorPosition)
		{
			if(text.size())
				LRClickableAreaWTextComp::clickPressed(cursorPosition);
		});
	setShowPopupCallback([this](const CComponentHolder &, const Point & cursorPosition)
		{
			if(text.size())
				LRClickableAreaWTextComp::showPopupWindow(cursorPosition);
		});
}

void CComponentHolder::setClickPressedCallback(const ClickFunctor & callback)
{
	clickPressedCallback = callback;
}

void CComponentHolder::setShowPopupCallback(const ClickFunctor & callback)
{
	showPopupCallback = callback;
}

void CComponentHolder::setGestureCallback(const ClickFunctor & callback)
{
	gestureCallback = callback;
}

void CComponentHolder::clickPressed(const Point & cursorPosition)
{
	if(clickPressedCallback)
		clickPressedCallback(*this, cursorPosition);
}

void CComponentHolder::showPopupWindow(const Point & cursorPosition)
{
	if(showPopupCallback)
		showPopupCallback(*this, cursorPosition);
}

void CComponentHolder::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	if(gestureCallback)
		gestureCallback(*this, initialPosition);
}

CArtPlace::CArtPlace(Point position, const ArtifactID & artId, const SpellID & spellId)
	: CComponentHolder(Rect(position, Point(44, 44)), Point(1, 1))
	, locked(false)
	, imageIndex(0)
{
	OBJECT_CONSTRUCTION;

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), 0);
	setArtifact(artId, spellId);
	moveSelectionForeground();
}

void CArtPlace::setArtifact(const SpellID & newSpellId)
{
	setArtifact(ArtifactID::SPELL_SCROLL, newSpellId);
}

void CArtPlace::setArtifact(const ArtifactID & newArtId, const SpellID & newSpellId)
{
	artId = newArtId;
	if(artId == ArtifactID::NONE)
	{
		image->disable();
		text.clear();
		lockSlot(false);
		return;
	}

	const auto artType = artId.toArtifact();
	imageIndex = artType->getIconIndex();
	if(artId == ArtifactID::SPELL_SCROLL)
	{
		spellId = newSpellId;
		assert(spellId != SpellID::NONE);

		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			imageIndex = spellId.num;
			if(component.type != ComponentType::SPELL_SCROLL)
			{
				image->setScale(Point(pos.w, 34));
				image->setAnimationPath(AnimationPath::builtin("spellscr"), imageIndex);
				image->moveTo(Point(pos.x, pos.y + 4));
			}
		}
		// Add spell component info (used to provide a pic in r-click popup)
		component.type = ComponentType::SPELL_SCROLL;
		component.subType = spellId;
	}
	else
	{
		if(settings["general"]["enableUiEnhancements"].Bool() && component.type != ComponentType::ARTIFACT)
		{
			image->setScale(Point());
			image->setAnimationPath(AnimationPath::builtin("artifact"), imageIndex);
			image->moveTo(Point(pos.x, pos.y));
		}
		component.type = ComponentType::ARTIFACT;
		component.subType = artId;
	}
	image->enable();
	lockSlot(locked);

	text = artType->getDescriptionTranslated();
	if(artType->isScroll())
		ArtifactUtils::insertScrrollSpellName(text, spellId);
}

ArtifactID CArtPlace::getArtifactId() const
{
	return artId;
}

CCommanderArtPlace::CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot,
	const ArtifactID & artId, const SpellID & spellId)
	: CArtPlace(position, artId, spellId),
	commanderOwner(commanderOwner),
	commanderSlotID(artSlot.num)
{
}

void CCommanderArtPlace::returnArtToHeroCallback()
{
	ArtifactPosition artifactPos = commanderSlotID;
	ArtifactPosition freeSlot = ArtifactUtils::getArtBackpackPosition(commanderOwner, getArtifactId());
	if(freeSlot == ArtifactPosition::PRE_FIRST)
	{
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("core.genrltxt.152"));
	}
	else
	{
		ArtifactLocation src(commanderOwner->id, artifactPos);
		src.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		ArtifactLocation dst(commanderOwner->id, freeSlot);

		if(getArtifactId().toArtifact()->canBePutAt(commanderOwner, freeSlot, true))
		{
			GAME->interface()->cb->swapArtifacts(src, dst);
			setArtifact(ArtifactID(ArtifactID::NONE));
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickPressed(const Point & cursorPosition)
{
	if(getArtifactId() != ArtifactID::NONE && text.size())
		GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
}

void CCommanderArtPlace::showPopupWindow(const Point & cursorPosition)
{
	if(getArtifactId() != ArtifactID::NONE && text.size())
		CArtPlace::showPopupWindow(cursorPosition);
}

void CArtPlace::lockSlot(bool on)
{
	locked = on;
	if(on)
	{
		image->setFrame(ArtifactID::ART_LOCK);
		hoverText = LIBRARY->generaltexth->allTexts[507];
	}
	else if(artId != ArtifactID::NONE)
	{
		image->setFrame(imageIndex);
		auto hoverText = MetaString::createFromRawString(LIBRARY->generaltexth->heroscrn[1]);
		hoverText.replaceName(artId);
		this->hoverText = hoverText.toString();
	}
	else
	{
		hoverText = LIBRARY->generaltexth->allTexts[507];
	}
}

bool CArtPlace::isLocked() const
{
	return locked;
}

void CArtPlace::addCombinedArtInfo(const std::map<const ArtifactID, std::vector<ArtifactID>> & arts)
{
	for(auto [combinedId, availableArts] : arts)
	{
		const auto combinedArt = combinedId.toArtifact();
		MetaString info;
		info.appendEOL();
		info.appendEOL();
		info.appendRawString("{");
		info.appendName(combinedArt->getId());
		info.appendRawString("}");
		info.appendRawString(" (%d/%d)");
		info.replaceNumber(availableArts.size());
		info.replaceNumber(combinedArt->getConstituents().size());
		for(const auto part : combinedArt->getConstituents())
		{
			const auto found = std::find_if(availableArts.begin(), availableArts.end(), [part](const auto & availablePart) -> bool
				{
					return availablePart == part->getId() ? true : false;
				});

			info.appendEOL();
			if(found < availableArts.end())
			{
				info.appendName(part->getId());
				availableArts.erase(found);
			}
			else
			{
				info.appendRawString("{#A9A9A9|");
				info.appendName(part->getId());
				info.appendRawString("}");
			}
		}
		text += info.toString();
	}
}

CSecSkillPlace::CSecSkillPlace(const Point & position, const ImageSize & imageSize, const SecondarySkill & newSkillId, const uint8_t level)
	: CComponentHolder(Rect(position, Point()), Point())
{
	OBJECT_CONSTRUCTION;

	auto imagePath = AnimationPath::builtin("SECSK82");
	if(imageSize == ImageSize::MEDIUM)
		imagePath = AnimationPath::builtin("SECSKILL");
	if(imageSize == ImageSize::SMALL)
		imagePath = AnimationPath::builtin("SECSK32");

	image = std::make_shared<CAnimImage>(imagePath, 0);
	component.type = ComponentType::SEC_SKILL;
	pos.w = image->pos.w;
	pos.h = image->pos.h;
	setSkill(newSkillId, level);
}

void CSecSkillPlace::setSkill(const SecondarySkill & newSkillId, const uint8_t level)
{
	skillId = newSkillId;
	component.subType = newSkillId;
	setLevel(level);
}

void CSecSkillPlace::setLevel(const uint8_t level)
{
	// 0 - none
	// 1 - base
	// 2 - advanced
	// 3 - expert
	assert(level <= 3);
	if(skillId != SecondarySkill::NONE && level > 0)
	{
		const auto secSkill = skillId.toSkill();
		image->setFrame(secSkill->getIconIndex(level - 1));
		image->enable();
		auto hoverText = MetaString::createFromRawString(LIBRARY->generaltexth->heroscrn[21]);
		hoverText.replaceRawString(LIBRARY->generaltexth->levels[level - 1]);
		hoverText.replaceTextID(secSkill->getNameTextID());
		this->hoverText = hoverText.toString();
		component.value = level;
		text = secSkill->getDescriptionTranslated(level);
	}
	else
	{
		image->disable();
		hoverText.clear();
		text.clear();
	}
}
