/*
 * TradePanels.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TradePanels.h"

#include "../../gui/CGuiHandler.h"
#include "../../render/Canvas.h"
#include "../../widgets/TextControls.h"
#include "../../windows/InfoWindows.h"

#include "../../CGameInfo.h"
#include "../../CPlayerInterface.h"

#include "../../../CCallback.h"

#include "../../../lib/CGeneralTextHandler.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

CTradeableItem::CTradeableItem(const Rect & area, EType Type, int ID, bool Left, int Serial)
	: SelectableSlot(area, Point(1, 1))
	, artInstance(nullptr)
	, type(EType(-1)) // set to invalid, will be corrected in setType
	, id(ID)
	, serial(Serial)
	, left(Left)
	, downSelection(false)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	addUsedEvents(LCLICK);
	addUsedEvents(HOVER);
	addUsedEvents(SHOW_POPUP);
	
	setType(Type);

	this->pos.w = area.w;
	this->pos.h = area.h;
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
	case EType::RESOURCE:
		return AnimationPath::builtin("RESOURCE");
	case EType::PLAYER:
		return AnimationPath::builtin("CREST58");
	case EType::ARTIFACT_TYPE:
	case EType::ARTIFACT_PLACEHOLDER:
	case EType::ARTIFACT_INSTANCE:
		return AnimationPath::builtin("artifact");
	case EType::CREATURE:
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
	case EType::RESOURCE:
	case EType::PLAYER:
		return id;
	case EType::ARTIFACT_TYPE:
	case EType::ARTIFACT_INSTANCE:
	case EType::ARTIFACT_PLACEHOLDER:
		return CGI->artifacts()->getByIndex(id)->getIconIndex();
	case EType::CREATURE:
		return CGI->creatures()->getByIndex(id)->getIconIndex();
	default:
		return -1;
	}
}

void CTradeableItem::showAll(Canvas & to)
{
	Point posToBitmap;
	Point posToSubCenter;

	switch(type)
	{
	case EType::RESOURCE:
		posToBitmap = Point(19, 9);
		posToSubCenter = Point(35, 57);
		break;
	case EType::CREATURE_PLACEHOLDER:
	case EType::CREATURE:
		posToSubCenter = Point(29, 77);
		break;
	case EType::PLAYER:
		posToSubCenter = Point(31, 77);
		break;
	case EType::ARTIFACT_PLACEHOLDER:
	case EType::ARTIFACT_INSTANCE:
		posToSubCenter = Point(22, 51);
		if (downSelection)
			posToSubCenter.y += 8;
		break;
	case EType::ARTIFACT_TYPE:
		posToSubCenter = Point(35, 57);
		posToBitmap = Point(13, 0);
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
	case EType::CREATURE:
	case EType::CREATURE_PLACEHOLDER:
		GH.statusbar()->write(boost::str(boost::format(CGI->generaltexth->allTexts[481]) % CGI->creh->objects[id]->getNamePluralTranslated()));
		break;
	case EType::ARTIFACT_PLACEHOLDER:
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
	case EType::CREATURE:
	case EType::CREATURE_PLACEHOLDER:
		break;
	case EType::ARTIFACT_TYPE:
	case EType::ARTIFACT_PLACEHOLDER:
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
	case EType::PLAYER:
		return CGI->generaltexth->capColors[id];
	case EType::RESOURCE:
		return CGI->generaltexth->restypes[id];
	case EType::CREATURE:
		if (number == 1)
			return CGI->creh->objects[id]->getNameSingularTranslated();
		else
			return CGI->creh->objects[id]->getNamePluralTranslated();
	case EType::ARTIFACT_TYPE:
	case EType::ARTIFACT_INSTANCE:
		return CGI->artifacts()->getByIndex(id)->getNameTranslated();
	}
	logGlobal->error("Invalid trade item type: %d", (int)type);
	return "";
}

const CArtifactInstance * CTradeableItem::getArtInstance() const
{
	switch(type)
	{
	case EType::ARTIFACT_PLACEHOLDER:
	case EType::ARTIFACT_INSTANCE:
		return artInstance;
	default:
		return nullptr;
	}
}

void CTradeableItem::setArtInstance(const CArtifactInstance * art)
{
	assert(type == EType::ARTIFACT_PLACEHOLDER || type == EType::ARTIFACT_INSTANCE);
	artInstance = art;
	if(art)
		setID(art->getTypeId());
	else
		setID(-1);
}

void TradePanelBase::updateSlots()
{
	if(updateSlotsCallback)
		updateSlotsCallback();
}

void TradePanelBase::deselect()
{
	for(const auto & slot : slots)
		slot->selectSlot(false);
}

void TradePanelBase::clearSubtitles()
{
	for(const auto & slot : slots)
		slot->subtitle.clear();
}

void TradePanelBase::updateOffer(CTradeableItem & slot, int cost, int qty)
{
	slot.subtitle = std::to_string(qty);
	if(cost != 1)
	{
		slot.subtitle.append("/");
		slot.subtitle.append(std::to_string(cost));
	}
}

void TradePanelBase::deleteSlots()
{
	if(deleteSlotsCheck)
		slots.erase(std::remove_if(slots.begin(), slots.end(), deleteSlotsCheck), slots.end());
}

ResourcesPanel::ResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles)
{
	assert(resourcesForTrade.size() == slotsPos.size());
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & res : resourcesForTrade)
	{
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(Rect(slotsPos[res.num], slotDimension),
			EType::RESOURCE, res.num, true, res.num));
		slot->clickPressedCallback = clickPressedCallback;
		slot->setSelectionWidth(selectionWidth);
	}
	updateSlotsCallback = updateSubtitles;
}

ArtifactsPanel::ArtifactsPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles,
	const std::vector<TradeItemBuy> & arts)
{
	assert(slotsForTrade == slotsPos.size());
	assert(slotsForTrade == arts.size());
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(auto slotIdx = 0; slotIdx < slotsForTrade; slotIdx++)
	{
		auto artType = arts[slotIdx].getNum();
		if(artType != ArtifactID::NONE)
		{
			auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(Rect(slotsPos[slotIdx], slotDimension),
				EType::ARTIFACT_TYPE, artType, false, slotIdx));
			slot->clickPressedCallback = clickPressedCallback;
			slot->setSelectionWidth(selectionWidth);
		}
	}
	updateSlotsCallback = updateSubtitles;
}

PlayersPanel::PlayersPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback)
{
	assert(PlayerColor::PLAYER_LIMIT_I <= slotsPos.size() + 1);
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	std::vector<PlayerColor> players;
	for(auto player = PlayerColor(0); player < PlayerColor::PLAYER_LIMIT_I; player++)
	{
		if(player != LOCPLINT->playerID && LOCPLINT->cb->getPlayerStatus(player) == EPlayerStatus::INGAME)
			players.emplace_back(player);
	}

	slots.resize(players.size());
	int slotNum = 0;
	for(auto & slot : slots)
	{
		slot = std::make_shared<CTradeableItem>(Rect(slotsPos[slotNum], slotDimension), EType::PLAYER, players[slotNum].num, false, slotNum);
		slot->clickPressedCallback = clickPressedCallback;
		slot->setSelectionWidth(selectionWidth);
		slot->subtitle = CGI->generaltexth->capColors[players[slotNum].num];
		slotNum++;
	}
}

CreaturesPanel::CreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, const slotsData & initialSlots)
{
	assert(initialSlots.size() <= GameConstants::ARMY_SIZE);
	assert(slotsPos.size() <= GameConstants::ARMY_SIZE);
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & [creatureId, slotId, creaturesNum] : initialSlots)
	{
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(Rect(slotsPos[slotId.num], slotDimension),
			creaturesNum == 0 ? EType::CREATURE_PLACEHOLDER : EType::CREATURE, creatureId.num, true, slotId));
		slot->clickPressedCallback = clickPressedCallback;
		if(creaturesNum != 0)
			slot->subtitle = std::to_string(creaturesNum);
		slot->setSelectionWidth(selectionWidth);
	}
}

CreaturesPanel::CreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback,
	const std::vector<std::shared_ptr<CTradeableItem>> & srcSlots, bool emptySlots)
{
	assert(slots.size() <= GameConstants::ARMY_SIZE);
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & srcSlot : srcSlots)
	{
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(Rect(slotsPos[srcSlot->serial], srcSlot->pos.dimensions()),
			emptySlots ? EType::CREATURE_PLACEHOLDER : EType::CREATURE, srcSlot->id, true, srcSlot->serial));
		slot->clickPressedCallback = clickPressedCallback;
		slot->subtitle = emptySlots ? "" : srcSlot->subtitle;
		slot->setSelectionWidth(selectionWidth);
	}
}
