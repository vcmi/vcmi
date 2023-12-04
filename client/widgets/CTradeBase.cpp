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
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"

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
		posToSubCenter = Point(35, 57);
		break;
	case CREATURE_PLACEHOLDER:
	case CREATURE:
		posToSubCenter = Point(29, 77);
		break;
	case PLAYER:
		posToSubCenter = Point(31, 77);
		break;
	case ARTIFACT_PLACEHOLDER:
	case ARTIFACT_INSTANCE:
		posToSubCenter = Point(22, 51);
		if (downSelection)
			posToSubCenter.y += 8;
		break;
	case ARTIFACT_TYPE:
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

void STradePanel::updateSlots()
{
	if(updateSlotsCallback)
		updateSlotsCallback();
}

void STradePanel::deselect()
{
	for(auto & slot : slots)
		slot->selection->selectSlot(false);
}

void STradePanel::clearSubtitles()
{
	for(auto & slot : slots)
		slot->subtitle.clear();
}

void STradePanel::updateOffer(CTradeableItem & slot, int cost, int qty)
{
	slot.subtitle = std::to_string(qty);
	if(cost != 1)
		slot.subtitle += "/" + std::to_string(cost);
}

void STradePanel::deleteSlots()
{
	if(deleteSlotsCheck)
		slots.erase(std::remove_if(slots.begin(), slots.end(), deleteSlotsCheck), slots.end());
}

SResourcesPanel::SResourcesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles)
{
	assert(resourcesForTrade.size() == slotsPos.size());
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & res : resourcesForTrade)
	{
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(slotsPos[res.num], EType::RESOURCE, res.num, true, res.num));
		slot->clickPressedCallback = clickPressedCallback;
		slot->pos.w = 69; slots.back()->pos.h = 66;
		slot->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[res.num], slots.back()->pos.dimensions()), Point(1, 1), selectionWidth);
	}
	updateSlotsCallback = updateSubtitles;
}

SArtifactsPanel::SArtifactsPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, UpdateSlotsFunctor updateSubtitles,
	std::vector<TradeItemBuy> & arts)
{
	assert(slotsForTrade == slotsPos.size());
	assert(slotsForTrade == arts.size());
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(auto slotIdx = 0; slotIdx < slotsForTrade; slotIdx++)
	{
		auto artType = arts[slotIdx].getNum();
		if(artType != ArtifactID::NONE)
		{
			auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(slotsPos[slotIdx], EType::ARTIFACT_TYPE, artType, false, slotIdx));
			slot->clickPressedCallback = clickPressedCallback;
			slot->pos.w = 69; slot->pos.h = 66;
			slot->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[slotIdx], slot->pos.dimensions()), Point(1, 1), selectionWidth);
		}
	}
	updateSlotsCallback = updateSubtitles;
}

SPlayersPanel::SPlayersPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback)
{
	assert(PlayerColor::PLAYER_LIMIT_I <= slotsPos.size());
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
		slot = std::make_shared<CTradeableItem>(slotsPos[slotNum], EType::PLAYER, players[slotNum].num, false, slotNum);
		slot->clickPressedCallback = clickPressedCallback;
		slot->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[slotNum], slot->pos.dimensions()), Point(1, 1), selectionWidth);
		slot->subtitle = CGI->generaltexth->capColors[players[slotNum++].num];
	}
}

SCreaturesPanel::SCreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback, slotsData & initialSlots)
{
	assert(initialSlots.size() <= GameConstants::ARMY_SIZE);
	assert(slotsPos.size() <= GameConstants::ARMY_SIZE);
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & slotData : initialSlots)
	{
		auto slotId = std::get<1>(slotData);
		auto creaturesNum = std::get<2>(slotData);
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(slotsPos[slotId.num],
			creaturesNum == 0 ? EType::CREATURE_PLACEHOLDER : EType::CREATURE, std::get<0>(slotData).num, true, slotId));
		slot->clickPressedCallback = clickPressedCallback;
		if(creaturesNum != 0)
			slot->subtitle = std::to_string(std::get<2>(slotData));
		slot->pos.w = 58; slot->pos.h = 64;
		slot->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[slotId.num], slot->pos.dimensions()), Point(1, 1), selectionWidth);
	}
}

SCreaturesPanel::SCreaturesPanel(CTradeableItem::ClickPressedFunctor clickPressedCallback,
	std::vector<std::shared_ptr<CTradeableItem>> & stsSlots, bool emptySlots)
{
	assert(slots.size() <= GameConstants::ARMY_SIZE);
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	for(const auto & srcSlot : stsSlots)
	{
		auto slot = slots.emplace_back(std::make_shared<CTradeableItem>(slotsPos[srcSlot->serial],
			emptySlots ? EType::CREATURE_PLACEHOLDER : EType::CREATURE, srcSlot->id, true, srcSlot->serial));
		slot->clickPressedCallback = clickPressedCallback;
		slot->subtitle = emptySlots ? "" : srcSlot->subtitle;
		slot->pos.w = srcSlot->pos.w; slot->pos.h = srcSlot->pos.h;
		slot->selection = std::make_unique<SelectableSlot>(Rect(slotsPos[slot->serial], slot->pos.dimensions()), Point(1, 1), selectionWidth);
	}
}

CTradeBase::CTradeBase(const IMarket * market, const CGHeroInstance * hero)
	: market(market)
	, hero(hero)
{
	deal = std::make_shared<CButton>(Point(), AnimationPath::builtin("ALTSACR.DEF"),
		CGI->generaltexth->zelp[585], std::bind(&CTradeBase::makeDeal, this));
}

void CTradeBase::removeItems(const std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : toRemove)
		removeItem(item);
}

void CTradeBase::removeItem(std::shared_ptr<CTradeableItem> item)
{
	rightTradePanel->slots.erase(std::remove(rightTradePanel->slots.begin(), rightTradePanel->slots.end(), item));

	if(hRight == item)
		hRight.reset();
}

void CTradeBase::getEmptySlots(std::set<std::shared_ptr<CTradeableItem>> & toRemove)
{
	for(auto item : leftTradePanel->slots)
		if(!hero->getStackCount(SlotID(item->serial)))
			toRemove.insert(item);
}

void CTradeBase::deselect()
{
	if(hLeft)
		hLeft->selection->selectSlot(false);
	if(hRight)
		hRight->selection->selectSlot(false);
	hLeft = hRight = nullptr;
	deal->block(true);
}

void CTradeBase::onSlotClickPressed(std::shared_ptr<CTradeableItem> & newSlot, std::shared_ptr<CTradeableItem> & hCurSlot)
{
	if(newSlot == hCurSlot)
		return;

	if(hCurSlot)
		hCurSlot->selection->selectSlot(false);
	hCurSlot = newSlot;
	newSlot->selection->selectSlot(true);
}

CExpAltar::CExpAltar()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	// Experience needed to reach next level
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[475], Rect(15, 415, 125, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	// Total experience on the Altar
	texts.emplace_back(std::make_shared<CTextBox>(CGI->generaltexth->allTexts[476], Rect(15, 495, 125, 40), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW));
	deal->moveBy(dealButtonPos);
	expToLevel = std::make_shared<CLabel>(75, 477, FONT_SMALL, ETextAlignment::CENTER);
	expForHero = std::make_shared<CLabel>(75, 545, FONT_SMALL, ETextAlignment::CENTER);
}

CCreaturesSelling::CCreaturesSelling()
{
	assert(hero);
	SCreaturesPanel::slotsData slots;
	for(auto slotId = SlotID(0); slotId.num < GameConstants::ARMY_SIZE; slotId++)
	{
		if(const auto & creature = hero->getCreature(slotId))
			slots.emplace_back(std::make_tuple(creature->getId(), slotId, hero->getStackCount(slotId)));
	}
	leftTradePanel = std::make_shared<SCreaturesPanel>([this](std::shared_ptr<CTradeableItem> altarSlot) -> void
		{
			onSlotClickPressed(altarSlot, hLeft);
		}, slots);
}

bool CCreaturesSelling::slotDeletingCheck(std::shared_ptr<CTradeableItem> & slot)
{
	return hero->getStackCount(SlotID(slot->serial)) == 0 ? true : false;
}

void CCreaturesSelling::updateSubtitle()
{
	for(auto & heroSlot : leftTradePanel->slots)
		heroSlot->subtitle = std::to_string(this->hero->getStackCount(SlotID(heroSlot->serial)));
}
