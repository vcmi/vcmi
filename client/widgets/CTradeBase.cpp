/*
 * CTradeBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTradeBase.h"
#include "MiscWidgets.h"

#include "../gui/CGuiHandler.h"
#include "../render/Canvas.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

#include "../CGameInfo.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CTradeableItem::CTradeableItem(Point pos, EType Type, int ID, bool Left, int Serial)
	: CIntObject(LCLICK | HOVER | SHOW_POPUP, pos)
	, type(EType(-1)) // set to invalid, will be corrected in setType
	, id(ID)
	, serial(Serial)
	, left(Left)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	downSelection = false;
	hlp = nullptr;
	setType(Type);
	if(image)
	{
		this->pos.w = image->pos.w;
		this->pos.h = image->pos.h;
	}
}

void CTradeableItem::setType(EType newType)
{
	if(type != newType)
	{
		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);
		type = newType;

		if(getIndex() < 0)
		{
			image = std::make_shared<CAnimImage>(getFilename(), 0);
			image->disable();
		}
		else
		{
			image = std::make_shared<CAnimImage>(getFilename(), getIndex());
		}
	}
}

void CTradeableItem::setID(int newID)
{
	if(id != newID)
	{
		id = newID;
		if(image)
		{
			int index = getIndex();
			if(index < 0)
				image->disable();
			else
			{
				image->enable();
				image->setFrame(index);
			}
		}
	}
}

AnimationPath CTradeableItem::getFilename()
{
	switch(type)
	{
	case RESOURCE:
		return AnimationPath::builtin("RESOURCE");
	case PLAYER:
		return AnimationPath::builtin("CREST58");
	case ARTIFACT_TYPE:
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return AnimationPath::builtin("artifact");
	case CREATURE:
		return AnimationPath::builtin("TWCRPORT");
	default:
		return {};
	}
}

int CTradeableItem::getIndex()
{
	if(id < 0)
		return -1;

	switch(type)
	{
	case RESOURCE:
	case PLAYER:
		return id;
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
	case ARTIFACT_PLACEHOLDER:
		return CGI->artifacts()->getByIndex(id)->getIconIndex();
	case CREATURE:
		return CGI->creatures()->getByIndex(id)->getIconIndex();
	default:
		return -1;
	}
}

void CTradeableItem::showAll(Canvas & to)
{
	Point posToBitmap;
	Point posToSubCenter;

	switch (type)
	{
	case RESOURCE:
		posToBitmap = Point(19, 9);
		posToSubCenter = Point(36, 59);
		break;
	case CREATURE_PLACEHOLDER:
	case CREATURE:
		posToSubCenter = Point(29, 77);
		break;
	case PLAYER:
		posToSubCenter = Point(31, 76);
		break;
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		posToSubCenter = Point(19, 54);
		if (downSelection)
			posToSubCenter.y += 8;
		break;
	case ARTIFACT_TYPE:
		posToSubCenter = Point(19, 58);
		break;
	}

	if(image)
	{
		image->moveTo(pos.topLeft() + posToBitmap);
		CIntObject::showAll(to);
	}

	to.drawText(pos.topLeft() + posToSubCenter, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, subtitle);
}

void CTradeableItem::clickPressed(const Point & cursorPosition)
{
	if(clickPressedCallback)
		clickPressedCallback(shared_from_this());
}

void CTradeableItem::showAllAt(const Point & dstPos, const std::string & customSub, Canvas & to)
{
	Rect oldPos = pos;
	std::string oldSub = subtitle;
	downSelection = true;

	moveTo(dstPos);
	subtitle = customSub;
	showAll(to);

	downSelection = false;
	moveTo(oldPos.topLeft());
	subtitle = oldSub;
}

void CTradeableItem::hover(bool on)
{
	if(!on)
	{
		GH.statusbar()->clear();
		return;
	}

	switch(type)
	{
	case CREATURE:
	case CREATURE_PLACEHOLDER:
		GH.statusbar()->write(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->objects[id]->getNamePluralTranslated()));
		break;
	case ARTIFACT_PLACEHOLDER:
		if(id < 0)
			GH.statusbar()->write(CGI->generaltexth->zelp[582].first);
		else
			GH.statusbar()->write(CGI->artifacts()->getByIndex(id)->getNameTranslated());
		break;
	}
}

void CTradeableItem::showPopupWindow(const Point & cursorPosition)
{
	switch(type)
	{
	case CREATURE:
	case CREATURE_PLACEHOLDER:
		break;
	case ARTIFACT_TYPE:
	case ARTIFACT_PLACEHOLDER:
		//TODO: it's would be better for market to contain actual CArtifactInstance and not just ids of certain artifact type so we can use getEffectiveDescription.
		if (id >= 0)
			CRClickPopup::createAndPush(CGI->artifacts()->getByIndex(id)->getDescriptionTranslated());
		break;
	}
}

std::string CTradeableItem::getName(int number) const
{
	switch(type)
	{
	case PLAYER:
		return CGI->generaltexth->capColors[id];
	case RESOURCE:
		return CGI->generaltexth->restypes[id];
	case CREATURE:
		if (number == 1)
			return CGI->creh->objects[id]->getNameSingularTranslated();
		else
			return CGI->creh->objects[id]->getNamePluralTranslated();
	case ARTIFACT_TYPE:
	case ARTIFACT_INSTANCE:
		return CGI->artifacts()->getByIndex(id)->getNameTranslated();
	}
	logGlobal->error("Invalid trade item type: %d", (int)type);
	return "";
}

const CArtifactInstance * CTradeableItem::getArtInstance() const
{
	switch(type)
	{
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		return hlp;
	default:
		return nullptr;
	}
}

void CTradeableItem::setArtInstance(const CArtifactInstance * art)
{
	assert(type == ARTIFACT_PLACEHOLDER || type == ARTIFACT_INSTANCE);
	hlp = art;
	if(art)
		setID(art->artType->getId());
	else
		setID(-1);
}

SResourcesPanel::SResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, updatePanelFunctor updateSubtitles)
	: updateSubtitles(updateSubtitles)
{
	assert(resourcesForTrade.size() == slotsPos.size());
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & res : resourcesForTrade)
	{
		slots.emplace_back(std::make_shared<CTradeableItem>(slotsPos[res.num], EType::RESOURCE, res.num, true, res.num));
		slots.back()->clickPressedCallback = clickPressedCallback;
		slots.back()->pos.w = 69; slots.back()->pos.h = 66;
		slots.back()->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[res.num], slots.back()->pos.dimensions()));
	}
}

void SResourcesPanel::updateSlots()
{
	if(updateSubtitles)
		updateSubtitles();
}

void SResourcesPanel::deselect()
{
	for(auto & slot : slots)
		slot->selection->selectSlot(false);
}

CTradeBase::CTradeBase(const IMarket * market, const CGHeroInstance * hero)
	: market(market)
	, hero(hero)
{
}

void CTradeBase::removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : toRemove)
		removeItem(item);
}

void CTradeBase::removeItem(std::shared_ptr<CTradeableItem> item)
{
	items[item->left] -= item;

	if(hRight == item)
		hRight.reset();
}

void CTradeBase::getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : items[1])
		if(!hero->getStackCount(SlotID(item->serial)))
			toRemove.insert(item);
}
