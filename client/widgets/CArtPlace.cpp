/*
 * CArtPlace.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CArtPlace.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"

#include "CComponent.h"

#include "../windows/GUIClasses.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/IRenderHandler.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/ArtifactUtils.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CConfigHandler.h"

CArtPlace::CArtPlace(Point position, const ArtifactID & artId, const SpellID & spellId)
	: SelectableSlot(Rect(position, Point(44, 44)), Point(1, 1))
	, locked(false)
	, imageIndex(0)
{
	OBJECT_CONSTRUCTION;

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), 0);
	setArtifact(artId, spellId);
	moveSelectionForeground();
}

void CArtPlace::setArtifact(const SpellID & spellId)
{
	setArtifact(ArtifactID::SPELL_SCROLL, spellId);
}

void CArtPlace::setArtifact(const ArtifactID & artId, const SpellID & spellId)
{
	this->artId = artId;
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
		this->spellId = spellId;
		assert(spellId.num > 0);

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
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
	}
	else
	{
		ArtifactLocation src(commanderOwner->id, artifactPos);
		src.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		ArtifactLocation dst(commanderOwner->id, freeSlot);

		if(getArtifactId().toArtifact()->canBePutAt(commanderOwner, freeSlot, true))
		{
			LOCPLINT->cb->swapArtifacts(src, dst);
			setArtifact(ArtifactID(ArtifactID::NONE));
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickPressed(const Point & cursorPosition)
{
	if(getArtifactId() != ArtifactID::NONE && text.size())
		LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
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
		hoverText = CGI->generaltexth->allTexts[507];
	}
	else if(artId != ArtifactID::NONE)
	{
		image->setFrame(imageIndex);
		auto hoverText = MetaString::createFromRawString(CGI->generaltexth->heroscrn[1]);
		hoverText.replaceName(artId);
		this->hoverText = hoverText.toString();
	}
	else
	{
		hoverText = CGI->generaltexth->allTexts[507];
	}
}

bool CArtPlace::isLocked() const
{
	return locked;
}

void CArtPlace::clickPressed(const Point & cursorPosition)
{
	if(clickPressedCallback)
		clickPressedCallback(*this, cursorPosition);
}

void CArtPlace::showPopupWindow(const Point & cursorPosition)
{
	if(showPopupCallback)
		showPopupCallback(*this, cursorPosition);
}

void CArtPlace::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	if(gestureCallback)
		gestureCallback(*this, initialPosition);
}

void CArtPlace::setClickPressedCallback(const ClickFunctor & callback)
{
	clickPressedCallback = callback;
}

void CArtPlace::setShowPopupCallback(const ClickFunctor & callback)
{
	showPopupCallback = callback;
}

void CArtPlace::setGestureCallback(const ClickFunctor & callback)
{
	gestureCallback = callback;
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
