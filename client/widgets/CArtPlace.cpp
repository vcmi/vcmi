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

void CArtPlace::setInternals(const CArtifactInstance * artInst)
{
	ourArt = artInst;
	if(!artInst)
	{
		image->disable();
		text.clear();
		hoverText = CGI->generaltexth->allTexts[507];
		return;
	}

	imageIndex = artInst->artType->getIconIndex();
	if(artInst->getTypeId() == ArtifactID::SPELL_SCROLL)
	{
		auto spellID = artInst->getScrollSpellID();
		assert(spellID.num >= 0);

		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			imageIndex = spellID.num;
			if(component.type != ComponentType::SPELL_SCROLL)
			{
				image->setScale(Point(pos.w, 34));
				image->setAnimationPath(AnimationPath::builtin("spellscr"), imageIndex);
				image->moveTo(Point(pos.x, pos.y + 4));
			}
		}
		// Add spell component info (used to provide a pic in r-click popup)
		component.type = ComponentType::SPELL_SCROLL;
		component.subType = spellID;
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
		component.subType = artInst->getTypeId();
	}
	image->enable();
	text = artInst->getDescription();
}

CArtPlace::CArtPlace(Point position, const CArtifactInstance * art)
	: SelectableSlot(Rect(position, Point(44, 44)), Point(1, 1))
	, ourArt(art)
	, locked(false)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	imageIndex = 0;
	if(locked)
		imageIndex = ArtifactID::ART_LOCK;
	else if(ourArt)
		imageIndex = ourArt->artType->getIconIndex();

	image = std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), imageIndex);
	image->disable();
	moveSelectionForeground();
}

const CArtifactInstance * CArtPlace::getArt() const
{
	return ourArt;
}

CCommanderArtPlace::CCommanderArtPlace(Point position, const CGHeroInstance * commanderOwner, ArtifactPosition artSlot, const CArtifactInstance * art)
	: CArtPlace(position, art),
	commanderOwner(commanderOwner),
	commanderSlotID(artSlot.num)
{
	setArtifact(art);
}

void CCommanderArtPlace::returnArtToHeroCallback()
{
	ArtifactPosition artifactPos = commanderSlotID;
	ArtifactPosition freeSlot = ArtifactUtils::getArtBackpackPosition(commanderOwner, getArt()->getTypeId());
	if(freeSlot == ArtifactPosition::PRE_FIRST)
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("core.genrltxt.152"));
	}
	else
	{
		ArtifactLocation src(commanderOwner->id, artifactPos);
		src.creature = SlotID::COMMANDER_SLOT_PLACEHOLDER;
		ArtifactLocation dst(commanderOwner->id, freeSlot);

		if(getArt()->canBePutAt(commanderOwner, freeSlot, true))
		{
			LOCPLINT->cb->swapArtifacts(src, dst);
			setArtifact(nullptr);
			parent->redraw();
		}
	}
}

void CCommanderArtPlace::clickPressed(const Point & cursorPosition)
{
	if(getArt() && text.size())
		LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.commanderWindow.artifactMessage"), [this]() { returnArtToHeroCallback(); }, []() {});
}

void CCommanderArtPlace::showPopupWindow(const Point & cursorPosition)
{
	if(getArt() && text.size())
		CArtPlace::showPopupWindow(cursorPosition);
}

void CArtPlace::lockSlot(bool on)
{
	if(locked == on)
		return;

	locked = on;

	if(on)
		image->setFrame(ArtifactID::ART_LOCK);
	else if(ourArt)
		image->setFrame(imageIndex);
	else
		image->setFrame(0);
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

void CArtPlace::setArtifact(const CArtifactInstance * art)
{
	setInternals(art);
	if(art)
	{
		image->setFrame(locked ? static_cast<int>(ArtifactID::ART_LOCK) : imageIndex);

		if(locked) // Locks should appear as empty.
			hoverText = CGI->generaltexth->allTexts[507];
		else
			hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % ourArt->artType->getNameTranslated());
	}
	else
	{
		lockSlot(false);
	}
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
	for(const auto & availableArts : arts)
	{
		const auto combinedArt = availableArts.first.toArtifact();
		MetaString info;
		info.appendEOL();
		info.appendEOL();
		info.appendRawString("{");
		info.appendName(combinedArt->getId());
		info.appendRawString("}");
		info.appendRawString(" (%d/%d)");
		info.replaceNumber(availableArts.second.size());
		info.replaceNumber(combinedArt->getConstituents().size());
		for(const auto part : combinedArt->getConstituents())
		{
			info.appendEOL();
			if(vstd::contains(availableArts.second, part->getId()))
			{
				info.appendName(part->getId());
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
