/*
 * CCastleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCastleInterface.h"

#include "CHeroWindow.h"
#include "CMarketWindow.h"
#include "InfoWindows.h"
#include "GUIClasses.h"
#include "QuickRecruitmentWindow.h"
#include "CCreatureWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../PlayerLocalState.h"

#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../eventsSDL/InputHandler.h"
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../widgets/RadialMenu.h"
#include "../widgets/CExchangeController.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/ColorFilter.h"
#include "../render/IFont.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../adventureMap/CList.h"
#include "../adventureMap/CResDataBar.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CSoundBase.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/gameState/UpgradeInfo.h"
#include "../../lib/StartInfo.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/TownBuildingInstance.h"


static bool useCompactCreatureBox()
{
	return settings["gameTweaks"]["compactTownCreatureInfo"].Bool();
}

static bool useAvailableAmountAsCreatureLabel()
{
	return settings["gameTweaks"]["availableCreaturesAsDwellingLabel"].Bool();
}

CBuildingRect::CBuildingRect(CCastleBuildings * Par, const CGTownInstance * Town, const CStructure * Str)
	: CShowableAnim(0, 0, Str->defName, CShowableAnim::BASE, BUILDING_FRAME_TIME),
	  parent(Par),
	  town(Town),
	  str(Str),
	  border(nullptr),
	  area(nullptr),
	  stateTimeCounter(BUILD_ANIMATION_FINISHED_TIMEPOINT)
{
	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | HOVER | TIME);
	pos.x += str->pos.x;
	pos.y += str->pos.y;

	// special animation frame manipulation for castle shipyard with and without ship
	// done due to .def used in special way, not to animate building - first image is for shipyard without citadel moat, 2nd image is for including moat
	if(Town->getFactionID() == FactionID::CASTLE && Str->building &&
		(Str->building->bid == BuildingID::SHIPYARD || Str->building->bid == BuildingID::SHIP))
	{
		if(Town->hasBuilt(BuildingID::CITADEL))
		{
			this->first = 1;
			this->frame = 1;
		}
		else
			this->last = 0;
	}

	if(!str->borderName.empty())
		border = ENGINE->renderHandler().loadImage(str->borderName, EImageBlitMode::COLORKEY);

	if(!str->areaName.empty())
		area = ENGINE->renderHandler().loadImage(str->areaName, EImageBlitMode::SIMPLE);
}

const CBuilding * CBuildingRect::getBuilding()
{
	if (!str->building)
		return nullptr;

	if (str->hiddenUpgrade) // hidden upgrades, e.g. hordes - return base (dwelling for hordes)
		return town->getTown()->buildings.at(str->building->getBase()).get();

	return str->building;
}

bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	return (str->pos.z) < (p2.str->pos.z);
}

void CBuildingRect::hover(bool on)
{
	if (!area)
		return;

	if(on)
	{
		if(! parent->selectedBuilding //no building hovered
		  || (*parent->selectedBuilding)<(*this)) //or we are on top
		{
			parent->selectedBuilding = this;
			ENGINE->statusbar()->write(getSubtitle());
		}
	}
	else
	{
		if(parent->selectedBuilding == this)
		{
			parent->selectedBuilding = nullptr;
			ENGINE->statusbar()->clear();
		}
	}
}

void CBuildingRect::clickPressed(const Point & cursorPosition)
{
	if(getBuilding() && area && (parent->selectedBuilding==this))
	{
		auto building = getBuilding();
		parent->buildingClicked(building->bid);
	}
}

void CBuildingRect::showPopupWindow(const Point & cursorPosition)
{
	if((!area) || (this!=parent->selectedBuilding) || getBuilding() == nullptr)
		return;

	BuildingID bid = getBuilding()->bid;
	const CBuilding *bld = town->getTown()->buildings.at(bid).get();
	if (!bid.isDwelling())
	{
		CRClickPopup::createAndPush(CInfoWindow::genText(bld->getNameTranslated(), bld->getDescriptionTranslated()),
									std::make_shared<CComponent>(ComponentType::BUILDING, BuildingTypeUniqueID(bld->town->faction->getId(), bld->bid)));
	}
	else
	{
		int level = BuildingID::getLevelFromDwelling(bid);
		ENGINE->windows().createAndPushWindow<CDwellingInfoBox>(parent->pos.x+parent->pos.w / 2, parent->pos.y+parent->pos.h  /2, town, level);
	}
}

void CBuildingRect::show(Canvas & to)
{
	uint32_t stageDelay = BUILDING_APPEAR_TIMEPOINT;

	bool showTextOverlay = (ENGINE->isKeyboardAltDown() || ENGINE->input().getNumTouchFingers() == 2) && settings["general"]["enableOverlay"].Bool();

	if(stateTimeCounter < BUILDING_APPEAR_TIMEPOINT)
	{
		setAlpha(255 * stateTimeCounter / stageDelay);
		CShowableAnim::show(to);
	}
	else
	{
		setAlpha(255);
		CShowableAnim::show(to);
	}

	if(border && stateTimeCounter > BUILDING_APPEAR_TIMEPOINT)
	{
		if(stateTimeCounter >= BUILD_ANIMATION_FINISHED_TIMEPOINT)
		{
			if(parent->selectedBuilding == this || showTextOverlay)
				to.draw(border, pos.topLeft());
			return;
		}

		auto darkBorder = ColorFilter::genRangeShifter(0.f, 0.f, 0.f, 0.5f, 0.5f, 0.5f );
		auto lightBorder = ColorFilter::genRangeShifter(0.f, 0.f, 0.f, 2.0f, 2.0f, 2.0f );
		auto baseBorder = ColorFilter::genEmptyShifter();

		float progress = float(stateTimeCounter % stageDelay) / stageDelay;

		if (stateTimeCounter < BUILDING_WHITE_BORDER_TIMEPOINT)
			border->adjustPalette(ColorFilter::genInterpolated(lightBorder, darkBorder, progress), 0);
		else
		if (stateTimeCounter < BUILDING_YELLOW_BORDER_TIMEPOINT)
			border->adjustPalette(ColorFilter::genInterpolated(darkBorder, baseBorder, progress), 0);
		else
			border->adjustPalette(baseBorder, 0);

		to.draw(border, pos.topLeft());
	}
}

void CBuildingRect::tick(uint32_t msPassed)
{
	CShowableAnim::tick(msPassed);
	stateTimeCounter += msPassed;
}

void CBuildingRect::showAll(Canvas & to)
{
	if (stateTimeCounter == 0)
		return;

	CShowableAnim::showAll(to);
	if(!isActive() && parent->selectedBuilding == this && border)
		to.draw(border, pos.topLeft());
}

std::string CBuildingRect::getSubtitle()//hover text for building
{
	if (!getBuilding())
		return "";

	auto bid = getBuilding()->bid;

	if (!bid.isDwelling())//non-dwellings - only building name
		return town->getTown()->buildings.at(getBuilding()->bid)->getNameTranslated();
	else//dwellings - recruit %creature%
	{
		int level = BuildingID::getLevelFromDwelling(getBuilding()->bid);
		auto & availableCreatures = town->creatures[level].second;
		if(availableCreatures.size())
		{
			int creaID = availableCreatures.back();//taking last of available creatures
			return LIBRARY->generaltexth->allTexts[16] + " " + LIBRARY->creh->objects.at(creaID)->getNamePluralTranslated();
		}
		else
		{
			logGlobal->warn("Dwelling with id %d offers no creatures!", bid);
			return "#ERROR#";
		}
	}
}

void CBuildingRect::mouseMoved (const Point & cursorPosition, const Point & lastUpdateDistance)
{
	hover(true);
}

bool CBuildingRect::receiveEvent(const Point & position, int eventType) const
{
	if (!pos.isInside(position.x, position.y))
		return false;

	if(area && area->isTransparent(position - pos.topLeft()))
		return false;

	return CIntObject::receiveEvent(position, eventType);
}

CDwellingInfoBox::CDwellingInfoBox(int centerX, int centerY, const CGTownInstance * Town, int level)
	: CWindowObject(RCLICK_POPUP, ImagePath::builtin("CRTOINFO"), Point(centerX, centerY))
{
	OBJECT_CONSTRUCTION;
	background->setPlayerColor(Town->tempOwner);

	const CCreature * creature = Town->creatures.at(level).second.back().toCreature();

	title = std::make_shared<CLabel>(80, 30, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, creature->getNamePluralTranslated());
	animation = std::make_shared<CCreaturePic>(30, 44, creature, true, true);

	std::string text = std::to_string(Town->creatures.at(level).first);
	available = std::make_shared<CLabel>(80,190, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[217] + text);
	costPerTroop = std::make_shared<CLabel>(80, 227, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[346]);

	for(int i = 0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		auto res = static_cast<EGameResID>(i);
		if(creature->getRecruitCost(res))
		{
			resPicture.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), i, 0, 0, 0));
			resAmount.push_back(std::make_shared<CLabel>(0,0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(creature->getRecruitCost(res))));
		}
	}

	int posY = 238;
	int posX = pos.w/2 - (int)resAmount.size() * 25 + 5;
	for (size_t i=0; i<resAmount.size(); i++)
	{
		resPicture[i]->moveBy(Point(posX, posY));
		resAmount[i]->moveBy(Point(posX+16, posY+43));
		posX += 50;
	}
}

CDwellingInfoBox::~CDwellingInfoBox() = default;

CHeroGSlot::CHeroGSlot(int x, int y, int updown, const CGHeroInstance * h, HeroSlots * Owner)
{
	OBJECT_CONSTRUCTION;

	owner = Owner;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;
	upg = updown;

	portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), 0, 0, 0, 0);
	portrait->visible = false;

	flag = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), 0, 0, 0, 0);
	flag->visible = false;

	selection = std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), 1, 0);
	selection->visible = false;

	set(h);

	addUsedEvents(LCLICK | SHOW_POPUP | GESTURE | HOVER);
}

CHeroGSlot::~CHeroGSlot() = default;

auto CHeroGSlot::getUpgradableSlots(const CArmedInstance *obj) const
{
	struct result { bool isCreatureUpgradePossible; bool canAffordAny; bool canAffordAll; TResources totalCosts; std::vector<std::pair<SlotID, UpgradeInfo>> upgradeInfos; };

	std::vector<std::pair<SlotID, UpgradeInfo>> upgradeInfos;
	for(const auto & slot : obj->Slots())
	{
		auto upgradeInfo = std::make_pair(slot.first, UpgradeInfo(slot.second->getCreatureID()));
		GAME->interface()->cb->fillUpgradeInfo(slot.second->getArmy(), slot.first, upgradeInfo.second);
		bool canUpgrade = obj->tempOwner == GAME->interface()->playerID && upgradeInfo.second.canUpgrade();
		if(canUpgrade)
			upgradeInfos.push_back(upgradeInfo);
	}

	std::sort(upgradeInfos.begin(), upgradeInfos.end(), [&](const std::pair<SlotID, UpgradeInfo> & lhs, const std::pair<SlotID, UpgradeInfo> & rhs) {
		return lhs.second.oldID.toCreature()->getLevel() > rhs.second.oldID.toCreature()->getLevel();
	});
	bool hasCreaturesToUpgrade = !upgradeInfos.empty();

	TResources costs;
	std::vector<SlotID> slotInfosToDelete;
	for(const auto & upgradeInfo : upgradeInfos)
	{
		TResources upgradeCosts = upgradeInfo.second.getUpgradeCosts() * obj->Slots().at(upgradeInfo.first)->getCount();
		if(GAME->interface()->cb->getResourceAmount().canAfford(costs + upgradeCosts))
			costs += upgradeCosts;
		else
			slotInfosToDelete.push_back(upgradeInfo.first);
	}
	upgradeInfos.erase(std::remove_if(upgradeInfos.begin(), upgradeInfos.end(), [&slotInfosToDelete](const auto& item) {
		return std::count(slotInfosToDelete.begin(), slotInfosToDelete.end(), item.first);
	}), upgradeInfos.end());

	return result { hasCreaturesToUpgrade, !upgradeInfos.empty(), slotInfosToDelete.empty(), costs, upgradeInfos };
}

void CHeroGSlot::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(!on)
		return;

	const CArmedInstance *obj = hero;
	if(upg == 0 && !obj)
		obj = owner->town->getUpperArmy();
	if(!obj)
		return;

	auto upgradableSlots = getUpgradableSlots(obj);
	auto upgradeAll = [upgradableSlots, obj](){
		if(!upgradableSlots.canAffordAny)
		{
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.townWindow.upgradeAll.notUpgradable"));
			return;
		}

		std::vector<std::shared_ptr<CComponent>> resComps;
		for(TResources::nziterator i(upgradableSlots.totalCosts); i.valid(); i++)
			resComps.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, i->resType, i->resVal));
		if(resComps.empty())
			resComps.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, static_cast<GameResID>(GameResID::GOLD), 0)); // add at least gold, when there are no costs
		resComps.back()->newLine = true;
		for(auto & upgradeInfo : upgradableSlots.upgradeInfos)
			resComps.push_back(std::make_shared<CComponent>(ComponentType::CREATURE, upgradeInfo.second.getUpgrade(), obj->Slots().at(upgradeInfo.first)->getCount()));
			
		std::string textID = upgradableSlots.canAffordAll ? "core.genrltxt.207" : "vcmi.townWindow.upgradeAll.notAllUpgradable";

		GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->translate(textID), [upgradableSlots, obj](){
			for(auto & upgradeInfo : upgradableSlots.upgradeInfos)
				GAME->interface()->cb->upgradeCreature(obj, upgradeInfo.first, upgradeInfo.second.getUpgrade());
		}, nullptr, resComps);
	};

	if (!settings["input"]["radialWheelGarrisonSwipe"].Bool())
		return;

	if(!hero)
	{
		if(upgradableSlots.isCreatureUpgradePossible)
		{
			std::vector<RadialMenuConfig> menuElements = {
				{ RadialMenuConfig::ITEM_WW, true, "upgradeCreatures", "vcmi.radialWheel.upgradeCreatures", [upgradeAll](){ upgradeAll(); } },
			};
			ENGINE->windows().createAndPushWindow<RadialMenu>(pos.center(), menuElements);
		}
		return;
	}

	std::shared_ptr<CHeroGSlot> other = upg ? owner->garrisonedHero : owner->visitingHero;

	bool twoHeroes = hero && other->hero;

	ObjectInstanceID heroId = hero->id;
	ObjectInstanceID heroOtherId = twoHeroes ? other->hero->id : ObjectInstanceID::NONE;

	std::vector<RadialMenuConfig> menuElements = {
		{ RadialMenuConfig::ITEM_NW, twoHeroes, "moveTroops", "vcmi.radialWheel.heroGetArmy", [heroId, heroOtherId](){CExchangeController(heroId, heroOtherId).moveArmy(false, std::nullopt);} },
		{ RadialMenuConfig::ITEM_NE, twoHeroes, "stackSplitDialog", "vcmi.radialWheel.heroSwapArmy", [heroId, heroOtherId](){CExchangeController(heroId, heroOtherId).swapArmy();} },
		{ RadialMenuConfig::ITEM_EE, twoHeroes, "tradeHeroes", "vcmi.radialWheel.heroExchange", [heroId, heroOtherId](){GAME->interface()->showHeroExchange(heroId, heroOtherId);} },
		{ RadialMenuConfig::ITEM_SW, twoHeroes, "moveArtifacts", "vcmi.radialWheel.heroGetArtifacts", [heroId, heroOtherId](){CExchangeController(heroId, heroOtherId).moveArtifacts(false, true, true);} },
		{ RadialMenuConfig::ITEM_SE, twoHeroes, "swapArtifacts", "vcmi.radialWheel.heroSwapArtifacts", [heroId, heroOtherId](){CExchangeController(heroId, heroOtherId).swapArtifacts(true, true);} }
	};
	RadialMenuConfig upgradeSlot = { RadialMenuConfig::ITEM_WW, true, "upgradeCreatures", "vcmi.radialWheel.upgradeCreatures", [upgradeAll](){ upgradeAll(); } };
	RadialMenuConfig dismissSlot = { RadialMenuConfig::ITEM_WW, true, "dismissHero", "vcmi.radialWheel.heroDismiss", [this](){ GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[22], [this](){ GAME->interface()->cb->dismissHero(hero); }, nullptr); } };

	if(upgradableSlots.isCreatureUpgradePossible)
		menuElements.push_back(upgradeSlot);
	else
		menuElements.push_back(dismissSlot);

	ENGINE->windows().createAndPushWindow<RadialMenu>(pos.center(), menuElements);
}

void CHeroGSlot::hover(bool on)
{
	if(!on)
	{
		ENGINE->statusbar()->clear();
		return;
	}
	std::shared_ptr<CHeroGSlot> other = upg ? owner->garrisonedHero : owner->visitingHero;
	std::string temp;
	if(hero)
	{
		if(isSelected())//view NNN
		{
			temp = LIBRARY->generaltexth->tcommands[4];
			boost::algorithm::replace_first(temp,"%s",hero->getNameTranslated());
		}
		else if(other->hero && other->isSelected())//exchange
		{
			temp = LIBRARY->generaltexth->tcommands[7];
			boost::algorithm::replace_first(temp,"%s",hero->getNameTranslated());
			boost::algorithm::replace_first(temp,"%s",other->hero->getNameTranslated());
		}
		else// select NNN (in ZZZ)
		{
			if(upg)//down - visiting
			{
				temp = LIBRARY->generaltexth->tcommands[32];
				boost::algorithm::replace_first(temp,"%s",hero->getNameTranslated());
			}
			else //up - garrison
			{
				temp = LIBRARY->generaltexth->tcommands[12];
				boost::algorithm::replace_first(temp,"%s",hero->getNameTranslated());
			}
		}
	}
	else //we are empty slot
	{
		if(other->isSelected() && other->hero) //move NNNN
		{
			temp = LIBRARY->generaltexth->tcommands[6];
			boost::algorithm::replace_first(temp,"%s",other->hero->getNameTranslated());
		}
		else //empty
		{
			temp = LIBRARY->generaltexth->allTexts[507];
		}
	}
	if(temp.size())
		ENGINE->statusbar()->write(temp);
}

void CHeroGSlot::clickPressed(const Point & cursorPosition)
{
	std::shared_ptr<CHeroGSlot> other = upg ? owner->garrisonedHero : owner->visitingHero;

	owner->garr->setSplittingMode(false);
	owner->garr->selectSlot(nullptr);

	if(hero && isSelected())
	{
		setHighlight(false);

		if(other->hero && !ENGINE->isKeyboardShiftDown())
			GAME->interface()->showHeroExchange(hero->id, other->hero->id);
		else
			GAME->interface()->openHeroWindow(hero);
	}
	else if(other->hero && other->isSelected())
	{
		owner->swapArmies();
	}
	else if(hero)
	{
		setHighlight(true);
		owner->garr->selectSlot(nullptr);
		redraw();
	}

	//refresh statusbar
	hover(false);
	hover(true);
}

void CHeroGSlot::showPopupWindow(const Point & cursorPosition)
{
	if(hero)
	{
		ENGINE->windows().createAndPushWindow<CInfoBoxPopup>(pos.center(), hero);
	}
}

void CHeroGSlot::deactivate()
{
	selection->visible = false;
	CIntObject::deactivate();
}

bool CHeroGSlot::isSelected() const
{
	return selection->visible;
}

void CHeroGSlot::setHighlight(bool on)
{
	selection->visible = on;

	if(owner->garrisonedHero->hero && owner->visitingHero->hero) //two heroes in town
	{
		for(auto & elem : owner->garr->splitButtons) //splitting enabled when slot highlighted
			elem->block(!on);
	}
}

void CHeroGSlot::set(const CGHeroInstance * newHero)
{
	OBJECT_CONSTRUCTION;

	hero = newHero;

	selection->visible = false;
	portrait->visible = false;
	flag->visible = false;

	if(newHero)
	{
		portrait->visible = true;
		portrait->setFrame(newHero->getIconIndex());
	}
	else if(!upg && owner->showEmpty) //up garrison
	{
		flag->visible = true;
		flag->setFrame(GAME->interface()->castleInt->town->getOwner().getNum());
	}
}

HeroSlots::HeroSlots(const CGTownInstance * Town, Point garrPos, Point visitPos, std::shared_ptr<CGarrisonInt> Garrison, bool ShowEmpty):
	showEmpty(ShowEmpty),
	town(Town),
	garr(Garrison)
{
	OBJECT_CONSTRUCTION;
	garrisonedHero = std::make_shared<CHeroGSlot>(garrPos.x, garrPos.y, 0, town->getGarrisonHero(), this);
	visitingHero = std::make_shared<CHeroGSlot>(visitPos.x, visitPos.y, 1, town->getVisitingHero(), this);
}

HeroSlots::~HeroSlots() = default;

void HeroSlots::update()
{
	garrisonedHero->set(town->getGarrisonHero());
	visitingHero->set(town->getVisitingHero());
}

void HeroSlots::swapArmies()
{
	bool allow = true;

	//moving hero out of town - check if it is allowed
	if (town->getGarrisonHero())
	{
		if (!town->getVisitingHero() && GAME->interface()->cb->howManyHeroes(false) >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))
		{
			std::string text = LIBRARY->generaltexth->translate("core.genrltxt.18"); //You already have %d adventuring heroes under your command.
			boost::algorithm::replace_first(text,"%d",std::to_string(GAME->interface()->cb->howManyHeroes(false)));

			GAME->interface()->showInfoDialog(text, std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			allow = false;
		}
		else if (town->getGarrisonHero()->stacksCount() == 0)
		{
			//This hero has no creatures.  A hero must have creatures before he can brave the dangers of the countryside.
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("core.genrltxt.19"), {}, soundBase::sound_todo);
			allow = false;
		}
	}

	if(!town->getGarrisonHero() && town->getVisitingHero()) //visiting => garrison, merge armies: town army => hero army
	{
		if(!town->getVisitingHero()->canBeMergedWith(*town))
		{
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[275], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			allow = false;
		}
	}

	garrisonedHero->setHighlight(false);
	visitingHero->setHighlight(false);

	if (allow)
		GAME->interface()->cb->swapGarrisonHero(town);
}

CCastleBuildings::CCastleBuildings(const CGTownInstance* Town):
	town(Town),
	selectedBuilding(nullptr)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(town->getTown()->clientInfo.townBackground, Point(0,0), EImageBlitMode::OPAQUE);
	background->needRefresh = true;
	pos.w = background->pos.w;
	pos.h = background->pos.h;

	recreate();
}

CCastleBuildings::~CCastleBuildings() = default;

void CCastleBuildings::recreate()
{
	selectedBuilding = nullptr;
	OBJECT_CONSTRUCTION;

	buildings.clear();
	groups.clear();

	//Generate buildings list

	auto buildingsCopy = town->getBuildings();// a bit modified copy of built buildings

	if(town->hasBuilt(BuildingID::SHIPYARD))
	{
		auto bayPos = town->bestLocation();
		if(!bayPos.isValid())
			logGlobal->warn("Shipyard in non-coastal town!");
		std::vector <const CGObjectInstance *> vobjs = GAME->interface()->cb->getVisitableObjs(bayPos, false);
		//there is visitable obj at shipyard output tile and it's a boat or hero (on boat)
		if(!vobjs.empty() && (vobjs.front()->ID == Obj::BOAT || vobjs.front()->ID == Obj::HERO))
		{
			buildingsCopy.insert(BuildingID::SHIP);
		}
	}

	for(const auto & structure : town->getTown()->clientInfo.structures)
	{
		if(!structure->building)
		{
			buildings.push_back(std::make_shared<CBuildingRect>(this, town, structure.get()));
			continue;
		}
		if(vstd::contains(buildingsCopy, structure->building->bid))
		{
			groups[structure->building->getBase()].push_back(structure.get());
		}
	}

	for(auto & entry : groups)
	{
		const CBuilding * build = town->getTown()->buildings.at(entry.first).get();

		const CStructure * toAdd = *boost::max_element(entry.second, [=](const CStructure * a, const CStructure * b)
		{
			return build->getDistance(a->building->bid) < build->getDistance(b->building->bid);
		});

		buildings.push_back(std::make_shared<CBuildingRect>(this, town, toAdd));
	}

	const auto & buildSorter = [](const CIntObject * a, const CIntObject * b)
	{
		auto b1 = dynamic_cast<const CBuildingRect *>(a);
		auto b2 = dynamic_cast<const CBuildingRect *>(b);

		if(!b1 && !b2)
			return intptr_t(a) < intptr_t(b);
		if(b1 && !b2)
			return false;
		if(!b1 && b2)
			return true;

		return (*b1)<(*b2);
	};

	boost::sort(children, buildSorter); //TODO: create building in blit order
}

void CCastleBuildings::drawOverlays(Canvas & to, std::vector<std::shared_ptr<CBuildingRect>> buildingRects)
{
	std::vector<Rect> textRects;
	for(auto & buildingRect : buildingRects)
	{
		if(!buildingRect->border)
			continue;

		auto building = buildingRect->getBuilding();
		if (!building)
			continue;
		auto bid = building->bid;

		auto overlay = town->getTown()->buildings.at(bid)->getNameTranslated();
		const auto & font = ENGINE->renderHandler().loadFont(FONT_TINY);

		auto backColor = Colors::GREEN; // Other
		if(bid.isDwelling())
			backColor = Colors::PURPLE; // dwelling

		auto contentRect = buildingRect->border->contentRect();
		auto center = Rect(buildingRect->pos.x + contentRect.x, buildingRect->pos.y + contentRect.y, contentRect.w, contentRect.h).center();
		Point dimensions(font->getStringWidth(overlay), font->getLineHeight());
		Rect textRect = Rect(center - dimensions / 2, dimensions).resize(2);

		while(!pos.resize(-5).isInside(Point(textRect.topLeft().x, textRect.center().y)))
			textRect.x++;
		while(!pos.resize(-5).isInside(Point(textRect.topRight().x, textRect.center().y)))
			textRect.x--;
		while(std::any_of(textRects.begin(), textRects.end(), [textRect](Rect existingTextRect) { return existingTextRect.resize(3).intersectionTest(textRect); }))
			textRect.y++;
		textRects.push_back(textRect);

		to.drawColor(textRect, backColor);
		to.drawBorder(textRect, Colors::BRIGHT_YELLOW);
		to.drawText(textRect.center(), EFonts::FONT_TINY, Colors::BLACK, ETextAlignment::CENTER, overlay);
	}
}

void CCastleBuildings::show(Canvas & to)
{
	CIntObject::show(to);

	bool showTextOverlay = (ENGINE->isKeyboardAltDown() || ENGINE->input().getNumTouchFingers() == 2) && settings["general"]["enableOverlay"].Bool();
	if(showTextOverlay)
		drawOverlays(to, buildings);
}

void CCastleBuildings::addBuilding(BuildingID building)
{
	//FIXME: implement faster method without complete recreation of town
	BuildingID base = town->getTown()->buildings.at(building)->getBase();

	recreate();

	auto & structures = groups.at(base);

	for(auto buildingRect : buildings)
	{
		if(vstd::contains(structures, buildingRect->str))
		{
			//reset animation
			if(structures.size() == 1)
				buildingRect->stateTimeCounter = 0; // transparency -> fully visible stage
			else
				buildingRect->stateTimeCounter = CBuildingRect::BUILDING_APPEAR_TIMEPOINT; // already in fully visible stage
			break;
		}
	}
}

void CCastleBuildings::removeBuilding(BuildingID building)
{
	//FIXME: implement faster method without complete recreation of town
	recreate();
}

const CGHeroInstance * CCastleBuildings::getHero()
{
	if(town->getVisitingHero())
		return town->getVisitingHero();
	else
		return town->getGarrisonHero();
}

void CCastleBuildings::buildingClicked(BuildingID building)
{
	std::vector<BuildingID> buildingsToTest;

	for(BuildingID buildingToEnter = building;;)
	{
		const auto &b = town->getTown()->buildings.find(buildingToEnter)->second;

		buildingsToTest.push_back(buildingToEnter);
		if (!b->upgrade.hasValue())
			break;

		buildingToEnter = b->upgrade;
	}

	for(BuildingID buildingToEnter : boost::adaptors::reverse(buildingsToTest))
	{
		if (buildingTryActivateCustomUI(buildingToEnter, building))
			return;
	}

	enterBuilding(building);
}

bool CCastleBuildings::buildingTryActivateCustomUI(BuildingID buildingToTest, BuildingID buildingTarget)
{
	logGlobal->trace("You've clicked on %d", (int)buildingToTest.toEnum());
	const auto & b = town->getTown()->buildings.at(buildingToTest);

	if (town->getWarMachineInBuilding(buildingToTest).hasValue())
	{
		enterBlacksmith(buildingTarget, town->getWarMachineInBuilding(buildingToTest));
		return true;
	}

	if (!b->marketModes.empty())
	{
		switch (*b->marketModes.begin())
		{
			case EMarketMode::CREATURE_UNDEAD:
				ENGINE->windows().createAndPushWindow<CTransformerWindow>(town, getHero(), nullptr);
				return true;

			case EMarketMode::RESOURCE_SKILL:
				if (getHero())
					ENGINE->windows().createAndPushWindow<CUniversityWindow>(getHero(), buildingTarget, town, nullptr);
				return true;

			case EMarketMode::RESOURCE_RESOURCE:
				// can't use allied marketplace
				if (town->getOwner() == GAME->interface()->playerID)
				{
					ENGINE->windows().createAndPushWindow<CMarketWindow>(town, getHero(), nullptr, *b->marketModes.begin());
					return true;
				}
				else
					return false;
			default:
				if(getHero())
					ENGINE->windows().createAndPushWindow<CMarketWindow>(town, getHero(), nullptr, *b->marketModes.begin());
				else
					GAME->interface()->showInfoDialog(boost::str(boost::format(LIBRARY->generaltexth->allTexts[273]) % b->getNameTranslated())); //Only visiting heroes may use the %s.
				return true;
		}
	}

	if (town->rewardableBuildings.count(buildingToTest) && town->getTown()->buildings.at(buildingToTest)->manualHeroVisit)
	{
		enterRewardable(buildingToTest);
		return true;
	}

	if (buildingToTest.isDwelling())
	{
		enterDwelling((BuildingID::getLevelFromDwelling(buildingToTest)));
		return true;
	}
	else
	{
		switch(buildingToTest.toEnum())
		{
		case BuildingID::MAGES_GUILD_1:
		case BuildingID::MAGES_GUILD_2:
		case BuildingID::MAGES_GUILD_3:
		case BuildingID::MAGES_GUILD_4:
		case BuildingID::MAGES_GUILD_5:
				enterMagesGuild();
				return true;

		case BuildingID::TAVERN:
				GAME->interface()->showTavernWindow(town, nullptr, QueryID::NONE);
				return true;

		case BuildingID::SHIPYARD:
				if(town->shipyardStatus() == IBoatGenerator::GOOD)
				{
					GAME->interface()->showShipyardDialog(town);
					return true;
				}
				else if(town->shipyardStatus() == IBoatGenerator::BOAT_ALREADY_BUILT)
				{
					GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[51]);
					return true;
				}
				return false;

		case BuildingID::FORT:
		case BuildingID::CITADEL:
		case BuildingID::CASTLE:
				ENGINE->windows().createAndPushWindow<CFortScreen>(town);
				return true;

		case BuildingID::VILLAGE_HALL:
		case BuildingID::CITY_HALL:
		case BuildingID::TOWN_HALL:
		case BuildingID::CAPITOL:
				enterTownHall();
				return true;

		case BuildingID::SHIP:
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[51]); //Cannot build another boat
			return true;

		case BuildingID::SPECIAL_1:
		case BuildingID::SPECIAL_2:
		case BuildingID::SPECIAL_3:
		case BuildingID::SPECIAL_4:
				switch (b->subId)
				{
				case BuildingSubID::MYSTIC_POND:
						enterFountain(buildingToTest, b->subId, buildingTarget);
						return true;

				case BuildingSubID::CASTLE_GATE:
						if (GAME->interface()->makingTurn)
						{
							enterCastleGate(buildingToTest);
							return true;
						}
						return false;

				case BuildingSubID::PORTAL_OF_SUMMONING:
						if (town->creatures[town->getTown()->creatures.size()].second.empty())//No creatures
							GAME->interface()->showInfoDialog(LIBRARY->generaltexth->tcommands[30]);
						else
							enterDwelling(town->getTown()->creatures.size());
						return true;

				case BuildingSubID::BANK:
						enterBank(buildingTarget);
						return true;
				}
		}
	}

	for (auto const & bonus : b->buildingBonuses)
	{
		if (bonus->type == BonusType::THIEVES_GUILD_ACCESS)
		{
			enterAnyThievesGuild();
			return true;
		}
	}
	return false;
}

void CCastleBuildings::enterRewardable(BuildingID building)
{
	if (town->getVisitingHero() == nullptr)
	{
		MetaString message;
		message.appendTextID("core.genrltxt.273"); // only visiting heroes may visit %s
		message.replaceTextID(town->getTown()->buildings.at(building)->getNameTextID());

		GAME->interface()->showInfoDialog(message.toString());
	}
	else
	{
		if (town->rewardableBuildings.at(building)->wasVisited(town->getVisitingHero()))
			enterBuilding(building);
		else
			GAME->interface()->cb->visitTownBuilding(town, building);
	}
}

void CCastleBuildings::enterBlacksmith(BuildingID building, ArtifactID artifactID)
{
	const CGHeroInstance *hero = town->getVisitingHero();
	if(!hero)
	{
		GAME->interface()->showInfoDialog(boost::str(boost::format(LIBRARY->generaltexth->allTexts[273]) % town->getTown()->buildings.find(building)->second->getNameTranslated()));
		return;
	}
	auto art = artifactID.toArtifact();

	int price = art->getPrice();
	bool possible = GAME->interface()->cb->getResourceAmount(EGameResID::GOLD) >= price;
	if(possible)
	{
		for(auto slot : art->getPossibleSlots().at(ArtBearer::HERO))
		{
			if(hero->getArt(slot) == nullptr || hero->getArt(slot)->getTypeId() != artifactID)
			{
				possible = true;
				break;
			}
			else
			{
				possible = false;
			}
		}
	}

	CreatureID creatureID = artifactID.toArtifact()->getWarMachine();
	ENGINE->windows().createAndPushWindow<CBlacksmithDialog>(possible, creatureID, artifactID, hero->id);
}

void CCastleBuildings::enterBuilding(BuildingID building)
{
	std::vector<std::shared_ptr<CComponent>> comps(1, std::make_shared<CComponent>(ComponentType::BUILDING, BuildingTypeUniqueID(town->getFactionID(), building)));
	GAME->interface()->showInfoDialog( town->getTown()->buildings.find(building)->second->getDescriptionTranslated(), comps);
}

void CCastleBuildings::enterCastleGate(BuildingID building)
{
	if (!town->getVisitingHero())
	{
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[126]);
		return;//only visiting hero can use castle gates
	}
	std::vector <int> availableTowns;
	std::vector<std::shared_ptr<IImage>> images;
	std::vector <const CGTownInstance*> Towns = GAME->interface()->localState->getOwnedTowns();
	for(auto & Town : Towns)
	{
		const CGTownInstance *t = Town;
		if (t->id != this->town->id && t->getVisitingHero() == nullptr && //another town, empty and this is
			t->getFactionID() == town->getFactionID() && //the town of the same faction
			t->hasBuilt(BuildingSubID::CASTLE_GATE)) //and the town has a castle gate
		{
			availableTowns.push_back(t->id.getNum());//add to the list
			if(settings["general"]["enableUiEnhancements"].Bool())
			{
				auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("ITPA"), t->getTown()->clientInfo.icons[t->hasFort()][false] + 2, 0, EImageBlitMode::OPAQUE);
				image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
				images.push_back(image);
			}
		}
	}

	auto gateIcon = std::make_shared<CAnimImage>(town->getTown()->clientInfo.buildingsIcons, building.getNum());//will be deleted by selection window
	auto wnd = std::make_shared<CObjectListWindow>(availableTowns, gateIcon, LIBRARY->generaltexth->jktexts[40],
		LIBRARY->generaltexth->jktexts[41], std::bind (&CCastleInterface::castleTeleport, GAME->interface()->castleInt, _1), 0, images);
	wnd->onPopup = [availableTowns](int index) { CRClickPopup::createAndPush(GAME->interface()->cb->getObjInstance(ObjectInstanceID(availableTowns[index])), ENGINE->getCursorPosition()); };
	ENGINE->windows().pushWindow(wnd);
}

void CCastleBuildings::enterDwelling(int level)
{
	if (level < 0 || level >= town->creatures.size() || town->creatures[level].second.empty())
	{
		assert(0);
		logGlobal->error("Attempt to enter into invalid dwelling of level %d in town %s (%s)", level, town->getNameTranslated(), town->getFaction()->getNameTranslated());
		return;
	}

	auto recruitCb = [this, level](CreatureID id, int count)
	{
		GAME->interface()->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level);
	};
	ENGINE->windows().createAndPushWindow<CRecruitmentWindow>(town, level, town->getUpperArmy(), recruitCb, nullptr, -87);
}

void CCastleBuildings::enterToTheQuickRecruitmentWindow()
{
	const auto beginIt = town->creatures.cbegin();
	const auto afterLastIt = town->creatures.size() > town->getTown()->creatures.size()
		? std::next(beginIt, town->getTown()->creatures.size())
		: town->creatures.cend();
	const auto hasSomeoneToRecruit = std::any_of(beginIt, afterLastIt,
		[](const auto & creatureInfo) { return creatureInfo.first > 0; });
	if(hasSomeoneToRecruit)
		ENGINE->windows().createAndPushWindow<QuickRecruitmentWindow>(town, pos);
	else
		CInfoWindow::showInfoDialog(LIBRARY->generaltexth->translate("vcmi.townHall.noCreaturesToRecruit"), {});
}

void CCastleBuildings::enterFountain(const BuildingID & building, BuildingSubID::EBuildingSubID subID, BuildingID upgrades)
{
	std::vector<std::shared_ptr<CComponent>> comps(1, std::make_shared<CComponent>(ComponentType::BUILDING, BuildingTypeUniqueID(town->getFactionID(), building)));
	std::string descr = town->getTown()->buildings.find(building)->second->getDescriptionTranslated();
	std::string hasNotProduced;
	std::string hasProduced;

	hasNotProduced = LIBRARY->generaltexth->allTexts[677];
	hasProduced = LIBRARY->generaltexth->allTexts[678];

	bool isMysticPondOrItsUpgrade = subID == BuildingSubID::MYSTIC_POND
		|| (upgrades != BuildingID::NONE
			&& town->getTown()->buildings.find(BuildingID(upgrades))->second->subId == BuildingSubID::MYSTIC_POND);

	if(upgrades != BuildingID::NONE)
		descr += "\n\n"+town->getTown()->buildings.find(BuildingID(upgrades))->second->getDescriptionTranslated();

	if(isMysticPondOrItsUpgrade) //for vanila Rampart like towns
	{
		if(town->bonusValue.first == 0) //Mystic Pond produced nothing;
			descr += "\n\n" + hasNotProduced;
		else //Mystic Pond produced something;
		{
			descr += "\n\n" + hasProduced;
			boost::algorithm::replace_first(descr,"%s",LIBRARY->generaltexth->restypes[town->bonusValue.first]);
			boost::algorithm::replace_first(descr,"%d",std::to_string(town->bonusValue.second));
		}
	}
	GAME->interface()->showInfoDialog(descr, comps);
}

void CCastleBuildings::enterMagesGuild()
{
	const CGHeroInstance *hero = getHero();

	// hero doesn't have spellbok
	// or it is not our turn and we can't make actions
	if(hero && !hero->hasSpellbook() && GAME->interface()->makingTurn)
	{
		if(hero->isCampaignYog())
		{
			// "Yog has given up magic in all its forms..."
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[736]);
		}
		else if(GAME->interface()->cb->getResourceAmount(EGameResID::GOLD) < 500) //not enough gold to buy spellbook
		{
			openMagesGuild();
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[213]);
		}
		else
		{
			CFunctionList<void()> onYes = [this](){ openMagesGuild(); };
			CFunctionList<void()> onNo = onYes;
			onYes += [hero](){ GAME->interface()->cb->buyArtifact(hero, ArtifactID::SPELLBOOK); };
			std::vector<std::shared_ptr<CComponent>> components(1, std::make_shared<CComponent>(ComponentType::ARTIFACT, ArtifactID(ArtifactID::SPELLBOOK)));

			GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[214], onYes, onNo, components);
		}
	}
	else
	{
		openMagesGuild();
	}
}

void CCastleBuildings::enterTownHall()
{
	if(town->getVisitingHero() && town->getVisitingHero()->hasArt(ArtifactID::GRAIL) &&
		!town->hasBuilt(BuildingID::GRAIL)) //hero has grail, but town does not have it
	{
		if(!vstd::contains(town->forbiddenBuildings, BuildingID::GRAIL))
		{
			GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[597], //Do you wish this to be the permanent home of the Grail?
										[&](){ GAME->interface()->cb->buildBuilding(town, BuildingID::GRAIL); },
										[&](){ openTownHall(); });
		}
		else
		{
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[673]);
			assert(ENGINE->windows().topWindow<CInfoWindow>() != nullptr);
			ENGINE->windows().topWindow<CInfoWindow>()->buttons[0]->addCallback(std::bind(&CCastleBuildings::openTownHall, this));
		}
	}
	else
	{
		openTownHall();
	}
}

void CCastleBuildings::openMagesGuild()
{
	auto mageGuildBackground = GAME->interface()->castleInt->town->getTown()->clientInfo.guildBackground;
	ENGINE->windows().createAndPushWindow<CMageGuildScreen>(GAME->interface()->castleInt, mageGuildBackground);
}

void CCastleBuildings::openTownHall()
{
	ENGINE->windows().createAndPushWindow<CHallInterface>(town);
}

void CCastleBuildings::enterAnyThievesGuild()
{
	std::vector<const CGTownInstance*> towns = GAME->interface()->cb->getTownsInfo(true);
	for(auto & ownedTown : towns)
	{
		if(ownedTown->hasBuilt(BuildingID::TAVERN))
		{
			GAME->interface()->showThievesGuildWindow(ownedTown);
			return;
		}
	}
	GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.noTownWithTavern"));
}

void CCastleBuildings::enterBank(BuildingID building)
{
	std::vector<std::shared_ptr<CComponent>> components;
	if(town->bonusValue.second > 0)
	{
		components.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE_PER_DAY, GameResID(GameResID::GOLD), -500));
		GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.townStructure.bank.payBack"), components);
	}
	else{
	
		components.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, GameResID(GameResID::GOLD), 2500));
		GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.townStructure.bank.borrow"), [this, building](){ GAME->interface()->cb->visitTownBuilding(town, building); }, nullptr, components);
	}
}

void CCastleBuildings::enterAnyMarket()
{
	if(town->hasBuilt(BuildingID::MARKETPLACE))
	{
		ENGINE->windows().createAndPushWindow<CMarketWindow>(town, nullptr, nullptr, EMarketMode::RESOURCE_RESOURCE);
		return;
	}

	std::vector<const CGTownInstance*> towns = GAME->interface()->cb->getTownsInfo(true);
	for(auto & town : towns)
	{
		if(town->hasBuilt(BuildingID::MARKETPLACE))
		{
			ENGINE->windows().createAndPushWindow<CMarketWindow>(town, nullptr, nullptr, EMarketMode::RESOURCE_RESOURCE);
			return;
		}
	}
	GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.noTownWithMarket"));
}

CCreaInfo::CCreaInfo(Point position, const CGTownInstance * Town, int Level, bool compact, bool _showAvailable):
	town(Town),
	level(Level),
	showAvailable(_showAvailable)
{
	OBJECT_CONSTRUCTION;
	pos += position;

	if(town->creatures.size() <= level || town->creatures[level].second.empty())
	{
		level = -1;
		return;//No creature
	}
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER);

	creature = town->creatures[level].second.back();

	picture = std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), creature.toEntity(LIBRARY)->getIconIndex(), 0, 8, 0);

	std::string value;
	if(showAvailable)
		value = std::to_string(town->creatures[level].first);
	else
		value = std::string("+") + std::to_string(town->creatureGrowth(level));

	if(compact)
	{
		label = std::make_shared<CLabel>(40, 32, FONT_TINY, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, value);
		pos.x += 8;
		pos.w = 32;
		pos.h = 32;
	}
	else
	{
		label = std::make_shared<CLabel>(24, 40, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, value);
		pos.w = 48;
		pos.h = 48;
	}
}

void CCreaInfo::update()
{
	if(label)
	{
		std::string value;
		if(showAvailable)
			value = std::to_string(town->creatures[level].first);
		else
			value = std::string("+") + std::to_string(town->creatureGrowth(level));

		if(value != label->getText())
			label->setText(value);
	}
}

void CCreaInfo::hover(bool on)
{
	MetaString message;
	message.appendTextID("core.genrltxt.588");
	message.replaceNameSingular(creature);

	if(on)
	{
		ENGINE->statusbar()->write(message.toString());
	}
	else
	{
		ENGINE->statusbar()->clearIfMatching(message.toString());
	}
}

void CCreaInfo::clickPressed(const Point & cursorPosition)
{
	int offset = GAME->interface()->castleInt? (-87) : 0;
	auto recruitCb = [this](CreatureID id, int count)
	{
		GAME->interface()->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level);
	};
	ENGINE->windows().createAndPushWindow<CRecruitmentWindow>(town, level, town->getUpperArmy(), recruitCb, nullptr, offset);
}

std::string CCreaInfo::genGrowthText()
{
	GrowthInfo gi = town->getGrowthInfo(level);
	MetaString descr;
	descr.appendTextID("core.genrltxt.589");
	descr.replaceNameSingular(creature);
	descr.replaceNumber(gi.totalGrowth());

	for(const GrowthInfo::Entry & entry : gi.entries)
	{
		descr.appendEOL();
		descr.appendRawString(entry.description);
	}

	return descr.toString();
}

void CCreaInfo::showPopupWindow(const Point & cursorPosition)
{
	if (showAvailable)
		ENGINE->windows().createAndPushWindow<CDwellingInfoBox>(ENGINE->screenDimensions().x / 2, ENGINE->screenDimensions().y / 2, town, level);
	else
		CRClickPopup::createAndPush(genGrowthText(), std::make_shared<CComponent>(ComponentType::CREATURE, creature));
}

bool CCreaInfo::getShowAvailable()
{
	return showAvailable;
}

CTownInfo::CTownInfo(int posX, int posY, const CGTownInstance * Town, bool townHall)
	: town(Town),
	building(nullptr)
{
	OBJECT_CONSTRUCTION;
	addUsedEvents(SHOW_POPUP | HOVER);
	pos.x += posX;
	pos.y += posY;
	int buildID;

	if(townHall)
	{
		buildID = 10 + town->hallLevel();
		picture = std::make_shared<CAnimImage>(AnimationPath::builtin("ITMTL.DEF"), town->hallLevel());
	}
	else
	{
		buildID = 6 + town->fortLevel();
		if(buildID == 6)
			return;//FIXME: suspicious statement, fix or comment
		picture = std::make_shared<CAnimImage>(AnimationPath::builtin("ITMCL.DEF"), town->fortLevel()-1);
	}
	building = town->getTown()->buildings.at(BuildingID(buildID)).get();
	pos = picture->pos;
}

void CTownInfo::hover(bool on)
{
	if(on)
	{
		if(building )
			ENGINE->statusbar()->write(building->getNameTranslated());
	}
	else
	{
		ENGINE->statusbar()->clear();
	}
}

void CTownInfo::showPopupWindow(const Point & cursorPosition)
{
	if(building)
	{
		auto c =  std::make_shared<CComponent>(ComponentType::BUILDING, BuildingTypeUniqueID(building->town->faction->getId(), building->bid));
		CRClickPopup::createAndPush(CInfoWindow::genText(building->getNameTranslated(), building->getDescriptionTranslated()), c);
	}
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, const CGTownInstance * from):
	CWindowObject(PLAYER_COLORED | BORDERED),
	town(Town)
{
	OBJECT_CONSTRUCTION;

	GAME->interface()->castleInt = this;
	addUsedEvents(KEYBOARD);

	builds = std::make_shared<CCastleBuildings>(town);
	panel = std::make_shared<CPicture>(ImagePath::builtin("TOWNSCRN"), 0, builds->pos.h);
	panel->setPlayerColor(GAME->interface()->playerID);
	pos.w = panel->pos.w;
	pos.h = builds->pos.h + panel->pos.h;
	center();
	updateShadow();

	garr = std::make_shared<CGarrisonInt>(Point(305, 387), 4, Point(0,96), town->getUpperArmy(), town->getVisitingHero());
	garr->setRedrawParent(true);

	heroes = std::make_shared<HeroSlots>(town, Point(241, 387), Point(241, 483), garr, true);
	title = std::make_shared<CLabel>(85, 387, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, town->getNameTranslated());
	income = std::make_shared<CLabel>(195, 443, FONT_SMALL, ETextAlignment::CENTER);
	icon = std::make_shared<CAnimImage>(AnimationPath::builtin("ITPT"), 0, 0, 15, 387);

	exit = std::make_shared<CButton>(Point(744, 544), AnimationPath::builtin("TSBTNS"), CButton::tooltip(LIBRARY->generaltexth->tcommands[8]), [&](){close();}, EShortcut::GLOBAL_RETURN);
	exit->setImageOrder(4, 5, 6, 7);

	auto split = std::make_shared<CButton>(Point(744, 382), AnimationPath::builtin("TSBTNS"), CButton::tooltip(LIBRARY->generaltexth->tcommands[3]), [this]() { garr->splitClick(); }, EShortcut::HERO_ARMY_SPLIT);
	garr->addSplitBtn(split);

	Rect barRect(9, 182, 732, 18);
	auto statusbarBackground = std::make_shared<CPicture>(panel->getSurface(), barRect, 9, 555);
	statusbar = CGStatusBar::create(statusbarBackground);
	resdatabar = std::make_shared<CResDataBar>(ImagePath::builtin("ARESBAR"), 3, 575, 37, 3, 84, 78);

	townlist = std::make_shared<CTownList>(3, Rect(Point(743, 414), Point(48, 128)), Point(1,16), Point(0, 32), GAME->interface()->localState->getOwnedTowns().size() );
	townlist->setScrollUpButton( std::make_shared<CButton>( Point(744, 414), AnimationPath::builtin("IAM014"), CButton::tooltipLocalized("core.help.306"), 0));
	townlist->setScrollDownButton( std::make_shared<CButton>( Point(744, 526), AnimationPath::builtin("IAM015"), CButton::tooltipLocalized("core.help.307"), 0));

	if(from)
		townlist->select(from);

	townlist->select(town); //this will scroll list to select current town
	townlist->onSelect = std::bind(&CCastleInterface::townChange, this);

	recreateIcons();
	if (!from)
		adventureInt->onAudioPaused();
	ENGINE->music().playMusicFromSet("faction", town->getFaction()->getJsonKey(), true, false);
}

CCastleInterface::~CCastleInterface()
{
	// resume map audio if:
	// adventureInt exists (may happen on exiting client with open castle interface)
	// castleInt has not been replaced (happens on switching between towns inside castle interface)
	if (adventureInt && GAME->interface()->castleInt == this)
		adventureInt->onAudioResumed();
	if(GAME->interface()->castleInt == this)
		GAME->interface()->castleInt = nullptr;
}

void CCastleInterface::updateGarrisons()
{
	garr->setArmy(town->getUpperArmy(), EGarrisonType::UPPER);
	garr->setArmy(town->getVisitingHero(), EGarrisonType::LOWER);
	garr->recreateSlots();
	heroes->update();

	redraw();
}

bool CCastleInterface::holdsGarrison(const CArmedInstance * army)
{
	return army == town || army == town->getUpperArmy() || army == town->getVisitingHero();
}

void CCastleInterface::close()
{
	if(town->tempOwner == GAME->interface()->playerID) //we may have opened window for an allied town
	{
		if(town->getVisitingHero() && town->getVisitingHero()->tempOwner == GAME->interface()->playerID)
			GAME->interface()->localState->setSelection(town->getVisitingHero());
		else
			GAME->interface()->localState->setSelection(town);
	}
	CWindowObject::close();
}

void CCastleInterface::castleTeleport(int where)
{
	const CGTownInstance * dest = GAME->interface()->cb->getTown(ObjectInstanceID(where));
	GAME->interface()->localState->setSelection(town->getVisitingHero());//according to assert(ho == adventureInt->selection) in the eraseCurrentPathOf
	GAME->interface()->cb->teleportHero(town->getVisitingHero(), dest);
	GAME->interface()->localState->erasePath(town->getVisitingHero());
}

void CCastleInterface::townChange()
{
	//TODO: do not recreate window
	const CGTownInstance * dest = GAME->interface()->localState->getOwnedTown(townlist->getSelectedIndex());
	const CGTownInstance * town = this->town;// "this" is going to be deleted
	if ( dest == town )
		return;
	close();
	ENGINE->windows().createAndPushWindow<CCastleInterface>(dest, town);
}

void CCastleInterface::addBuilding(BuildingID bid)
{
	if (town->getTown()->buildings.at(bid)->mode != CBuilding::BUILD_AUTO)
		ENGINE->sound().playSound(soundBase::newBuilding);

	deactivate();
	builds->addBuilding(bid);
	recreateIcons();
	activate();
	redraw();
}

void CCastleInterface::removeBuilding(BuildingID bid)
{
	deactivate();
	builds->removeBuilding(bid);
	recreateIcons();
	activate();
	redraw();
}

void CCastleInterface::recreateIcons()
{
	OBJECT_CONSTRUCTION;
	size_t iconIndex = town->getTown()->clientInfo.icons[town->hasFort()][town->built >= GAME->interface()->cb->getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP)];

	icon->setFrame(iconIndex);
	TResources townIncome = town->dailyIncome();
	income->setText(std::to_string(townIncome[EGameResID::GOLD]));

	hall = std::make_shared<CTownInfo>(80, 413, town, true);
	fort = std::make_shared<CTownInfo>(122, 413, town, false);

	fastTownHall = std::make_shared<CButton>(Point(80, 413), AnimationPath::builtin("castleInterfaceQuickAccess"), CButton::tooltip(), [this](){ builds->enterTownHall(); }, EShortcut::TOWN_OPEN_HALL);
	fastTownHall->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("ITMTL"), town->hallLevel()));

	int imageIndex = town->fortLevel() == CGTownInstance::EFortLevel::NONE ? 3 : town->fortLevel() - 1;
	fastArmyPurchase = std::make_shared<CButton>(Point(122, 413), AnimationPath::builtin("castleInterfaceQuickAccess"), CButton::tooltip(), [this](){ builds->enterToTheQuickRecruitmentWindow(); }, EShortcut::TOWN_OPEN_RECRUITMENT);
	fastArmyPurchase->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin("itmcl"), imageIndex));

	fastMarket = std::make_shared<LRClickableArea>(Rect(163, 410, 64, 42), [this]() { builds->enterAnyMarket(); });
	fastTavern = std::make_shared<LRClickableArea>(Rect(15, 387, 58, 64), [&]()
	{
		if(town->hasBuilt(BuildingID::TAVERN))
			GAME->interface()->showTavernWindow(town, nullptr, QueryID::NONE);
	}, [this]{
		if(!town->getFaction()->getDescriptionTranslated().empty())
			CRClickPopup::createAndPush(town->getFaction()->getDescriptionTranslated());
	});

	creainfo.clear();

	bool compactCreatureInfo = useCompactCreatureBox();
	bool useAvailableCreaturesForLabel = useAvailableAmountAsCreatureLabel();

	for(size_t i=0; i<4; i++)
		creainfo.push_back(std::make_shared<CCreaInfo>(Point(14 + 55 * (int)i, 459), town, (int)i, compactCreatureInfo, useAvailableCreaturesForLabel));


	for(size_t i=0; i<4; i++)
		creainfo.push_back(std::make_shared<CCreaInfo>(Point(14 + 55 * (int)i, 507), town, (int)i + 4, compactCreatureInfo, useAvailableCreaturesForLabel));
}

void CCastleInterface::keyPressed(EShortcut key)
{
	switch(key)
	{
	case EShortcut::MOVE_UP:
		townlist->selectPrev();
		break;
	case EShortcut::MOVE_DOWN:
		townlist->selectNext();
		break;
	case EShortcut::TOWN_OPEN_FORT:
		ENGINE->windows().createAndPushWindow<CFortScreen>(town);
		break;
	case EShortcut::TOWN_OPEN_MARKET:
		builds->enterAnyMarket();
		break;
	case EShortcut::TOWN_OPEN_MAGE_GUILD:
		if(town->hasBuilt(BuildingID::MAGES_GUILD_1))
			builds->enterMagesGuild();
		break;
	case EShortcut::TOWN_OPEN_THIEVES_GUILD:
		break;
	case EShortcut::TOWN_OPEN_HERO_EXCHANGE:
		if (town->getVisitingHero() && town->getGarrisonHero())
			GAME->interface()->showHeroExchange(town->getVisitingHero()->id, town->getGarrisonHero()->id);
		break;
	case EShortcut::TOWN_OPEN_HERO:
		if (town->getVisitingHero())
			GAME->interface()->openHeroWindow(town->getVisitingHero());
		else if (town->getGarrisonHero())
			GAME->interface()->openHeroWindow(town->getGarrisonHero());
		break;
	case EShortcut::TOWN_OPEN_VISITING_HERO:
		if (town->getVisitingHero())
			GAME->interface()->openHeroWindow(town->getVisitingHero());
		break;
	case EShortcut::TOWN_OPEN_GARRISONED_HERO:
		if (town->getGarrisonHero())
			GAME->interface()->openHeroWindow(town->getGarrisonHero());
		break;
	case EShortcut::TOWN_SWAP_ARMIES:
		heroes->swapArmies();
		break;
	case EShortcut::TOWN_OPEN_TAVERN:
		if(town->hasBuilt(BuildingID::TAVERN))
			GAME->interface()->showTavernWindow(town, nullptr, QueryID::NONE);
		break;
	default:
		break;
	}
}

void CCastleInterface::creaturesChangedEventHandler()
{
	for(auto creatureInfoBox : creainfo)
	{
		if(creatureInfoBox->getShowAvailable())
		{
			creatureInfoBox->update();
		}
	}
}

CHallInterface::CBuildingBox::CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building):
	town(Town),
	building(Building)
{
	OBJECT_CONSTRUCTION;
	addUsedEvents(LCLICK | SHOW_POPUP | HOVER);
	pos.x += x;
	pos.y += y;
	pos.w = 154;
	pos.h = 92;

	state = GAME->interface()->cb->canBuildStructure(town, building->bid);

	constexpr std::array panelIndex =
	{
		3, 3, 3, 0, 0, 2, 2, 1, 2, 2,  3,  3
	};
	constexpr std::array iconIndex =
	{
		-1, -1, -1, 0, 0, 1, 2, -1, 1, 1, -1, -1
	};

	icon = std::make_shared<CAnimImage>(town->getTown()->clientInfo.buildingsIcons, building->bid.getNum(), 0, 2, 2);
	header = std::make_shared<CAnimImage>(AnimationPath::builtin("TPTHBAR"), panelIndex[static_cast<int>(state)], 0, 1, 73);
	if(iconIndex[static_cast<int>(state)] >=0)
		mark = std::make_shared<CAnimImage>(AnimationPath::builtin("TPTHCHK"), iconIndex[static_cast<int>(state)], 0, 136, 56);
	name = std::make_shared<CLabel>(78, 81, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, building->getNameTranslated(), 150);

	//todo: add support for all possible states
	if(state >= EBuildingState::BUILDING_ERROR)
		state = EBuildingState::FORBIDDEN;
}

void CHallInterface::CBuildingBox::hover(bool on)
{
	if(on)
	{
		std::string toPrint;
		if(state==EBuildingState::PREREQUIRES || state == EBuildingState::MISSING_BASE)
			toPrint = LIBRARY->generaltexth->hcommands[5];
		else if(state==EBuildingState::CANT_BUILD_TODAY)
			toPrint = LIBRARY->generaltexth->allTexts[223];
		else
			toPrint = LIBRARY->generaltexth->hcommands[static_cast<int>(state)];
		boost::algorithm::replace_first(toPrint,"%s",building->getNameTranslated());
		ENGINE->statusbar()->write(toPrint);
	}
	else
	{
		ENGINE->statusbar()->clear();
	}
}

void CHallInterface::CBuildingBox::clickPressed(const Point & cursorPosition)
{
	ENGINE->windows().createAndPushWindow<CBuildWindow>(town,building,state,0);
}

void CHallInterface::CBuildingBox::showPopupWindow(const Point & cursorPosition)
{
	ENGINE->windows().createAndPushWindow<CBuildWindow>(town,building,state,1);
}

CHallInterface::CHallInterface(const CGTownInstance * Town):
	CWindowObject(PLAYER_COLORED | BORDERED, Town->getTown()->clientInfo.hallBackground),
	town(Town)
{
	OBJECT_CONSTRUCTION;

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);
	Rect barRect(5, 556, 740, 18);

	auto statusbarBackground = std::make_shared<CPicture>(background->getSurface(), barRect, 5, 556);
	statusbar = CGStatusBar::create(statusbarBackground);

	title = std::make_shared<CLabel>(399, 12, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, town->getTown()->buildings.at(BuildingID(town->hallLevel()+BuildingID::VILLAGE_HALL))->getNameTranslated());
	exit = std::make_shared<CButton>(Point(748, 556), AnimationPath::builtin("TPMAGE1.DEF"), CButton::tooltip(LIBRARY->generaltexth->hcommands[8]), [&](){close();}, EShortcut::GLOBAL_RETURN);

	auto & boxList = town->getTown()->clientInfo.hallSlots;
	boxes.resize(boxList.size());
	for(size_t row=0; row<boxList.size(); row++) //for each row
	{
		for(size_t col=0; col<boxList[row].size(); col++) //for each box
		{
			const CBuilding * building = nullptr;
			for(auto & buildingID : boxList[row][col])//we are looking for the first not built structure
			{
				if (!buildingID.hasValue())
				{
					logMod->warn("Invalid building ID found in hallSlots of town '%s'", town->getFaction()->getJsonKey() );
					continue;
				}

				const CBuilding * current = town->getTown()->buildings.at(buildingID).get();
				if(town->hasBuilt(buildingID))
				{
					building = current;
				}
				else
				{
					if(current->mode == CBuilding::BUILD_NORMAL)
					{
						building = current;
						break;
					}
				}
			}
			int posX = pos.w/2 - (int)boxList[row].size()*154/2 - ((int)boxList[row].size()-1)*20 + 194*(int)col;
			int posY = 35 + 104*(int)row;

			if(building)
				boxes[row].push_back(std::make_shared<CBuildingBox>(posX, posY, town, building));
		}
	}
}

CBuildWindow::CBuildWindow(const CGTownInstance *Town, const CBuilding * Building, EBuildingState state, bool rightClick):
	CWindowObject(PLAYER_COLORED | (rightClick ? RCLICK_POPUP : 0), ImagePath::builtin("TPUBUILD")),
	town(Town),
	building(Building)
{
	OBJECT_CONSTRUCTION;

	icon = std::make_shared<CAnimImage>(town->getTown()->clientInfo.buildingsIcons, building->bid.getNum(), 0, 125, 50);
	auto statusbarBackground = std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26);
	statusbar = CGStatusBar::create(statusbarBackground);

	MetaString nameString;
	nameString.appendTextID("core.hallinfo.7");
	nameString.replaceTextID(building->getNameTextID());

	name = std::make_shared<CLabel>(197, 30, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, nameString.toString());
	description = std::make_shared<CTextBox>(building->getDescriptionTranslated(), Rect(33, 135, 329, 67), 0, FONT_MEDIUM, ETextAlignment::CENTER);
	stateText = std::make_shared<CTextBox>(getTextForState(state), Rect(33, 216, 329, 67), 0, FONT_SMALL, ETextAlignment::CENTER);

	//Create components for all required resources
	std::vector<std::shared_ptr<CComponent>> components;

	for(GameResID i : GameResID::ALL_RESOURCES())
	{
		if(building->resources[i])
		{
			MetaString message;
			int resourceAmount = GAME->interface()->cb->getResourceAmount(i);
			bool canAfford = resourceAmount >= building->resources[i];

			if(!canAfford && state != EBuildingState::ALREADY_PRESENT && settings["general"]["enableUiEnhancements"].Bool())
			{
				message.appendRawString("{H3Red|%d}/%d");
				message.replaceNumber(resourceAmount);
			}
			else
				message.appendRawString("%d");

			message.replaceNumber(building->resources[i]);
			components.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, i, message.toString(), CComponent::small));
		}
	}

	cost = std::make_shared<CComponentBox>(components, Rect(25, 300, pos.w - 50, 130));

	if(!rightClick)
	{	//normal window

		MetaString tooltipYes;
		tooltipYes.appendTextID("core.genrltxt.595");
		tooltipYes.replaceTextID(building->getNameTextID());

		MetaString tooltipNo;
		tooltipNo.appendTextID("core.genrltxt.596");
		tooltipNo.replaceTextID(building->getNameTextID());

		buy = std::make_shared<CButton>(Point(45, 446), AnimationPath::builtin("IBUY30"), CButton::tooltip(tooltipYes.toString()), [&](){ buyFunc(); }, EShortcut::GLOBAL_ACCEPT);
		buy->setBorderColor(Colors::METALLIC_GOLD);
		buy->block(state != EBuildingState::ALLOWED || GAME->interface()->playerID != town->tempOwner || !GAME->interface()->makingTurn);

		cancel = std::make_shared<CButton>(Point(290, 445), AnimationPath::builtin("ICANCEL"), CButton::tooltip(tooltipNo.toString()), [&](){ close();}, EShortcut::GLOBAL_CANCEL);
		cancel->setBorderColor(Colors::METALLIC_GOLD);
	}
}

void CBuildWindow::buyFunc()
{
	GAME->interface()->cb->buildBuilding(town,building->bid);
	ENGINE->windows().popWindows(2); //we - build window and hall screen
}

std::string CBuildWindow::getTextForState(EBuildingState state)
{
	std::string ret;
	if(state < EBuildingState::ALLOWED)
		ret =  LIBRARY->generaltexth->hcommands[static_cast<int>(state)];
	switch (state)
	{
	case EBuildingState::ALREADY_PRESENT:
	case EBuildingState::CANT_BUILD_TODAY:
	case EBuildingState::NO_RESOURCES:
		ret.replace(ret.find_first_of("%s"), 2, building->getNameTranslated());
		break;
	case EBuildingState::ALLOWED:
		return LIBRARY->generaltexth->allTexts[219]; //all prereq. are met
	case EBuildingState::PREREQUIRES:
		{
			auto toStr = [&](const BuildingID build) -> std::string
			{
				return town->getTown()->buildings.at(build)->getNameTranslated();
			};

			ret = LIBRARY->generaltexth->allTexts[52];
			ret += "\n" + town->genBuildingRequirements(building->bid).toString(toStr);
			break;
		}
	case EBuildingState::MISSING_BASE:
		{
			std::string msg = LIBRARY->generaltexth->translate("vcmi.townHall.missingBase");
			ret = boost::str(boost::format(msg) % town->getTown()->buildings.at(building->upgrade)->getNameTranslated());
			break;
		}
	}
	return ret;
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int min, int max)
{
	OBJECT_CONSTRUCTION;
	pos.x+=size.x;
	pos.y+=size.y;
	pos.w = size.w;
	pos.h = size.h;
	init(name, descr, min, max);
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int val)
{
	OBJECT_CONSTRUCTION;
	pos.x+=size.x;
	pos.y+=size.y;
	pos.w = size.w;
	pos.h = size.h;
	init(name, descr, val, val);
}

void LabeledValue::init(std::string nameText, std::string descr, int min, int max)
{
	addUsedEvents(HOVER);
	hoverText = descr;
	std::string valueText;
	if(min && max)
	{
		valueText = std::to_string(min);
		if(min != max)
			valueText += '-' + std::to_string(max);
	}
	name = std::make_shared<CLabel>(3, 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, nameText);
	value = std::make_shared<CLabel>(pos.w-3, pos.h-2, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, valueText);
}

void LabeledValue::hover(bool on)
{
	if(on)
	{
		ENGINE->statusbar()->write(hoverText);
	}
	else
	{
		ENGINE->statusbar()->clear();
	}
}

CFortScreen::CFortScreen(const CGTownInstance * town):
	CWindowObject(PLAYER_COLORED | BORDERED, getBgName(town))
{
	OBJECT_CONSTRUCTION;
	ui32 fortSize = static_cast<ui32>(town->creatures.size());
	if(fortSize > town->getTown()->creatures.size() && town->creatures.back().second.empty())
		fortSize--;
	fortSize = std::min(fortSize, static_cast<ui32>(GameConstants::CREATURES_PER_TOWN)); // for 8 creatures + portal of summoning

	const auto & fortBuilding = town->getTown()->buildings.at(BuildingID(town->fortLevel()+6));
	title = std::make_shared<CLabel>(400, 12, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE, fortBuilding->getNameTranslated());

	std::string text = boost::str(boost::format(LIBRARY->generaltexth->fcommands[6]) % fortBuilding->getNameTranslated());
	exit = std::make_shared<CButton>(Point(748, 556), AnimationPath::builtin("TPMAGE1"), CButton::tooltip(text), [&](){ close(); }, EShortcut::GLOBAL_RETURN);

	std::vector<Point> positions =
	{
		Point(10,  22), Point(404, 22),
		Point(10, 155), Point(404,155),
		Point(10, 288), Point(404,288)
	};

	if(fortSize == GameConstants::CREATURES_PER_TOWN)
	{
		positions.push_back(Point(10, 421));
		positions.push_back(Point(404,421));
	}
	else
	{
		positions.push_back(Point(206,421));
	}

	for(ui32 i=0; i<fortSize; i++)
	{
		BuildingID buildingID;
		if(fortSize == town->getTown()->creatures.size())
		{
			BuildingID buildID = BuildingID(BuildingID::getDwellingFromLevel(i, 0));

			for(; town->getBuildings().count(buildID); BuildingID::advanceDwelling(buildID))
			{
				if(town->hasBuilt(buildID))
					buildingID = buildID;
			}
		}
		else
		{
			buildingID = BuildingID::SPECIAL_3;
		}

		recAreas.push_back(std::make_shared<RecruitArea>(positions[i].x, positions[i].y, town, i));
	}

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);

	Rect barRect(4, 554, 740, 18);

	auto statusbarBackground = std::make_shared<CPicture>(background->getSurface(), barRect, 4, 554);
	statusbar = CGStatusBar::create(statusbarBackground);
}

ImagePath CFortScreen::getBgName(const CGTownInstance * town)
{
	ui32 fortSize = static_cast<ui32>(town->creatures.size());
	if(fortSize > town->getTown()->creatures.size() && town->creatures.back().second.empty())
		fortSize--;
	fortSize = std::min(fortSize, static_cast<ui32>(GameConstants::CREATURES_PER_TOWN)); // for 8 creatures + portal of summoning

	if(fortSize == GameConstants::CREATURES_PER_TOWN)
		return ImagePath::builtin("TPCASTL8");
	else
		return ImagePath::builtin("TPCASTL7");
}

void CFortScreen::creaturesChangedEventHandler()
{
	for(auto & elem : recAreas)
		elem->creaturesChangedEventHandler();

	GAME->interface()->castleInt->creaturesChangedEventHandler();
}

CFortScreen::RecruitArea::RecruitArea(int posX, int posY, const CGTownInstance * Town, int Level):
	town(Town),
	level(Level),
	availableCount(nullptr)
{
	OBJECT_CONSTRUCTION;
	pos.x +=posX;
	pos.y +=posY;
	pos.w = 386;
	pos.h = 126;

	if(!town->creatures[level].second.empty())
		addUsedEvents(LCLICK | HOVER);//Activate only if dwelling is present

	addUsedEvents(SHOW_POPUP);

	icons = std::make_shared<CPicture>(ImagePath::builtin("TPCAINFO"), 261, 3);

	if(getMyBuilding() != nullptr)
	{
		buildingIcon = std::make_shared<CAnimImage>(town->getTown()->clientInfo.buildingsIcons, getMyBuilding()->bid, 0, 4, 21);
		buildingName = std::make_shared<CLabel>(78, 101, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, getMyBuilding()->getNameTranslated(), 152);

		if(town->hasBuilt(getMyBuilding()->bid))
		{
			ui32 available = town->creatures[level].first;
			std::string availableText = LIBRARY->generaltexth->allTexts[217]+ std::to_string(available);
			availableCount = std::make_shared<CLabel>(78, 119, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, availableText);
		}
	}

	if(getMyCreature() != nullptr)
	{
		hoverText = boost::str(boost::format(LIBRARY->generaltexth->tcommands[21]) % getMyCreature()->getNamePluralTranslated());
		new CCreaturePic(159, 4, getMyCreature(), false);
		new CLabel(78,  11, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, getMyCreature()->getNamePluralTranslated(), 152);

		Rect sizes(287, 4, 96, 18);
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[190], LIBRARY->generaltexth->fcommands[0], getMyCreature()->getAttack(false)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[191], LIBRARY->generaltexth->fcommands[1], getMyCreature()->getDefense(false)));
		sizes.y+=21;
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[199], LIBRARY->generaltexth->fcommands[2], getMyCreature()->getMinDamage(false), getMyCreature()->getMaxDamage(false)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[388], LIBRARY->generaltexth->fcommands[3], getMyCreature()->getMaxHealth()));
		sizes.y+=21;
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[193], LIBRARY->generaltexth->fcommands[4], getMyCreature()->valOfBonuses(BonusType::STACKS_SPEED)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, LIBRARY->generaltexth->allTexts[194], LIBRARY->generaltexth->fcommands[5], town->creatureGrowth(level)));
	}
}

const CCreature * CFortScreen::RecruitArea::getMyCreature()
{
	if(!town->creatures.at(level).second.empty()) // built
		return town->creatures.at(level).second.back().toCreature();
	if(!town->getTown()->creatures.at(level).empty()) // there are creatures on this level
		return town->getTown()->creatures.at(level).front().toCreature();
	return nullptr;
}

const CBuilding * CFortScreen::RecruitArea::getMyBuilding()
{
	BuildingID myID = BuildingID(BuildingID::getDwellingFromLevel(level, 0));

	if (level == town->getTown()->creatures.size())
		return town->getTown()->getSpecialBuilding(BuildingSubID::PORTAL_OF_SUMMONING);

	if (!town->getTown()->buildings.count(myID))
		return nullptr;

	const CBuilding * build = town->getTown()->buildings.at(myID).get();
	while (town->getTown()->buildings.count(myID))
	{
		if (town->hasBuilt(myID))
			build = town->getTown()->buildings.at(myID).get();
		BuildingID::advanceDwelling(myID);
	}

	return build;
}

void CFortScreen::RecruitArea::hover(bool on)
{
	if(on)
		ENGINE->statusbar()->write(hoverText);
	else
		ENGINE->statusbar()->clear();
}

void CFortScreen::RecruitArea::creaturesChangedEventHandler()
{
	if(availableCount)
	{
		std::string availableText = LIBRARY->generaltexth->allTexts[217] + std::to_string(town->creatures[level].first);
		availableCount->setText(availableText);
	}
}

void CFortScreen::RecruitArea::clickPressed(const Point & cursorPosition)
{
	GAME->interface()->castleInt->builds->enterDwelling(level);
}

void CFortScreen::RecruitArea::showPopupWindow(const Point & cursorPosition)
{
	if (getMyCreature() != nullptr)
		ENGINE->windows().createAndPushWindow<CStackWindow>(getMyCreature(), true);
}

CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner, const ImagePath & imagename)
	: CWindowObject(BORDERED, imagename), townId(owner->town->id)
{
	OBJECT_CONSTRUCTION;

	window = std::make_shared<CPicture>(owner->town->getTown()->clientInfo.guildWindow, 332, 76);

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);

	Rect barRect(7, 556, 737, 18);

	auto statusbarBackground = std::make_shared<CPicture>(background->getSurface(), barRect, 7, 556);
	statusbar = CGStatusBar::create(statusbarBackground);

	exit = std::make_shared<CButton>(Point(748, 556), AnimationPath::builtin("TPMAGE1.DEF"), CButton::tooltip(LIBRARY->generaltexth->allTexts[593]), [&](){ close(); }, EShortcut::GLOBAL_RETURN);

	updateSpells(townId);
}

void CMageGuildScreen::updateSpells(ObjectInstanceID tID)
{
	if(tID != townId)
		return;

	OBJECT_CONSTRUCTION;
	static const std::vector<std::vector<Point> > positions =
	{
		{Point(222,445), Point(312,445), Point(402,445), Point(520,445), Point(610,445), Point(700,445)},
		{Point(48,53),   Point(48,147),  Point(48,241),  Point(48,335),  Point(48,429)},
		{Point(570,82),  Point(672,82),  Point(570,157), Point(672,157)},
		{Point(183,42),  Point(183,148), Point(183,253)},
		{Point(491,325), Point(591,325)}
	};

	spells.clear();
	emptyScrolls.clear();

	const CGTownInstance * town = GAME->interface()->cb->getTown(townId);

	for(uint32_t i=0; i<town->getTown()->mageLevel; i++)
	{
		uint32_t spellCount = town->spellsAtLevel(i+1,false); //spell at level with -1 hmmm?
		for(uint32_t j=0; j<spellCount; j++)
		{
			if(i<town->mageGuildLevel() && town->spells[i].size()>j)
				spells.push_back(std::make_shared<Scroll>(positions[i][j], town->spells[i][j].toSpell(), townId));
			else
				emptyScrolls.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TPMAGES.DEF"), 1, 0, positions[i][j].x, positions[i][j].y));
		}
	}

	redraw();
}

CMageGuildScreen::Scroll::Scroll(Point position, const CSpell *Spell, ObjectInstanceID townId)
	: spell(Spell), townId(townId)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | SHOW_POPUP | HOVER);
	pos += position;
	image = std::make_shared<CAnimImage>(AnimationPath::builtin("SPELLSCR"), spell->id.getNum());
	pos = image->pos;
}

void CMageGuildScreen::Scroll::clickPressed(const Point & cursorPosition)
{
	const CGTownInstance * town = GAME->interface()->cb->getTown(townId);
	if(GAME->interface()->cb->getSettings().getBoolean(EGameSettings::TOWNS_SPELL_RESEARCH) && town->spellResearchAllowed)
	{
		int level = -1;
		for(int i = 0; i < town->spells.size(); i++)
			if(vstd::find_pos(town->spells[i], spell->id) != -1)
				level = i;
				
		if(town->spellResearchCounterDay >= GAME->interface()->cb->getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_PER_DAY).Vector()[level].Float())
		{
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.spellResearch.comeAgain"));
			return;
		}

		auto costBase = TResources(GAME->interface()->cb->getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_COST).Vector()[level]);
		auto costExponent = GAME->interface()->cb->getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_COST_EXPONENT_PER_RESEARCH).Vector()[level].Float();
		auto cost = costBase * std::pow(town->spellResearchAcceptedCounter + 1, costExponent);

		std::vector<std::shared_ptr<CComponent>> resComps;

		int index = town->spellsAtLevel(level, false);
		if (index >= town->spells[level].size())
		{
			GAME->interface()->showInfoDialog(LIBRARY->generaltexth->translate("vcmi.spellResearch.noMoreSpells"));
			return;
		}
		auto newSpell = town->spells[level].at(index);
		resComps.push_back(std::make_shared<CComponent>(ComponentType::SPELL, spell->id));
		resComps.push_back(std::make_shared<CComponent>(ComponentType::SPELL, newSpell));
		resComps.back()->newLine = true;
		for(TResources::nziterator i(cost); i.valid(); i++)
		{
			resComps.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, i->resType, i->resVal, CComponent::ESize::medium));
		}

		std::vector<std::pair<AnimationPath, CFunctionList<void()>>> pom;
		for(int i = 0; i < 3; i++)
			pom.emplace_back(AnimationPath::builtin("settingsWindow/button80"), nullptr);

		auto text = LIBRARY->generaltexth->translate(GAME->interface()->cb->getResourceAmount().canAfford(cost) ? "vcmi.spellResearch.pay" : "vcmi.spellResearch.canNotAfford");
		boost::replace_first(text, "%SPELL1", spell->id.toSpell()->getNameTranslated());
		boost::replace_first(text, "%SPELL2", newSpell.toSpell()->getNameTranslated());
		auto temp = std::make_shared<CInfoWindow>(text, GAME->interface()->playerID, resComps, pom);

		temp->buttons[0]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("spellResearch/accept")));
		temp->buttons[0]->addCallback([this, town](){ GAME->interface()->cb->spellResearch(town, spell->id, true); });
		temp->buttons[0]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.spellResearch.research")); });
		temp->buttons[0]->setEnabled(GAME->interface()->cb->getResourceAmount().canAfford(cost));
		temp->buttons[1]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("spellResearch/reroll")));
		temp->buttons[1]->addCallback([this, town](){ GAME->interface()->cb->spellResearch(town, spell->id, false); });
		temp->buttons[1]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.spellResearch.skip")); });
		temp->buttons[2]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("spellResearch/close")));
		temp->buttons[2]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.spellResearch.abort")); });

		ENGINE->windows().pushWindow(temp);
	}
	else
		GAME->interface()->showInfoDialog(spell->getDescriptionTranslated(0), std::make_shared<CComponent>(ComponentType::SPELL, spell->id));
}

void CMageGuildScreen::Scroll::showPopupWindow(const Point & cursorPosition)
{
	CRClickPopup::createAndPush(spell->getDescriptionTranslated(0), std::make_shared<CComponent>(ComponentType::SPELL, spell->id));
}

void CMageGuildScreen::Scroll::hover(bool on)
{
	if(on)
		ENGINE->statusbar()->write(spell->getNameTranslated());
	else
		ENGINE->statusbar()->clear();

}

CBlacksmithDialog::CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid):
	CWindowObject(PLAYER_COLORED, ImagePath::builtin("TPSMITH"))
{
	OBJECT_CONSTRUCTION;

	Rect barRect(8, pos.h - 26, pos.w - 16, 19);

	auto statusbarBackground = std::make_shared<CPicture>(background->getSurface(), barRect, 8, pos.h - 26);
	statusbar = CGStatusBar::create(statusbarBackground);

	animBG = std::make_shared<CPicture>(ImagePath::builtin("TPSMITBK"), 64, 50);
	animBG->needRefresh = true;

	const CCreature * creature = creMachineID.toCreature();
	anim = std::make_shared<CCreatureAnim>(64, 50, creature->animDefName);
	anim->clipRect(113,125,200,150);

	MetaString titleString;
	titleString.appendTextID("core.genrltxt.274");
	titleString.replaceTextID(creature->getNameSingularTextID());

	MetaString buyText;
	buyText.appendTextID("core.genrltxt.595");
	buyText.replaceTextID(creature->getNameSingularTextID());

	MetaString cancelText;
	cancelText.appendTextID("core.genrltxt.596");
	cancelText.replaceTextID(creature->getNameSingularTextID());

	std::string costString = std::to_string(aid.toEntity(LIBRARY)->getPrice());

	title = std::make_shared<CLabel>(165, 28, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, titleString.toString());
	costText = std::make_shared<CLabel>(165, 218, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->jktexts[43]);
	costValue = std::make_shared<CLabel>(165, 292, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, costString);
	buy = std::make_shared<CButton>(Point(42, 312), AnimationPath::builtin("IBUY30.DEF"), CButton::tooltip(buyText.toString()), [&](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	cancel = std::make_shared<CButton>(Point(224, 312), AnimationPath::builtin("ICANCEL.DEF"), CButton::tooltip(cancelText.toString()), [&](){ close(); }, EShortcut::GLOBAL_CANCEL);

	if(possible)
		buy->addCallback([=](){ GAME->interface()->cb->buyArtifact(GAME->interface()->cb->getHero(hid),aid); });
	else
		buy->block(true);

	costIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), GameResID(EGameResID::GOLD).getNum(), 0, 148, 244);
}
