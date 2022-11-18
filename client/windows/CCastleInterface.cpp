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

#include "CAdvmapInterface.h"
#include "CHeroWindow.h"
#include "CTradeWindow.h"
#include "InfoWindows.h"
#include "GUIClasses.h"
#include "QuickRecruitmentWindow.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"

#include "../../CCallback.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CModHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CCampaignHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CBuildingRect::CBuildingRect(CCastleBuildings * Par, const CGTownInstance * Town, const CStructure * Str)
	: CShowableAnim(0, 0, Str->defName, CShowableAnim::BASE),
	parent(Par),
	town(Town),
	str(Str),
	stateCounter(80)
{
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += str->pos.x;
	pos.y += str->pos.y;

	if(!str->borderName.empty())
		border = BitmapHandler::loadBitmap(str->borderName, true);
	else
		border = nullptr;

	if(!str->areaName.empty())
		area = BitmapHandler::loadBitmap(str->areaName);
	else
		area = nullptr;
}

CBuildingRect::~CBuildingRect()
{
	if(border)
		SDL_FreeSurface(border);
	if(area)
		SDL_FreeSurface(area);
}

const CBuilding * CBuildingRect::getBuilding()
{
	if (!str->building)
		return nullptr;

	if (str->hiddenUpgrade) // hidden upgrades, e.g. hordes - return base (dwelling for hordes)
		return town->town->buildings.at(str->building->getBase());

	return str->building;
}

bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	return (str->pos.z) < (p2.str->pos.z);

}

void CBuildingRect::hover(bool on)
{
	if(on)
	{
		if(!(active & MOVE))
			addUsedEvents(MOVE);
	}
	else
	{
		if(active & MOVE)
			removeUsedEvents(MOVE);

		if(parent->selectedBuilding == this)
		{
			parent->selectedBuilding = nullptr;
			GH.statusbar->clear();
		}
	}
}

void CBuildingRect::clickLeft(tribool down, bool previousState)
{
	if(previousState && getBuilding() && area && !down && (parent->selectedBuilding==this))
	{
		if(!CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y)) //inside building image
		{
			auto building = getBuilding();
			parent->buildingClicked(building->bid, building->subId, building->upgrade);
		}
	}
}

void CBuildingRect::clickRight(tribool down, bool previousState)
{
	if((!area) || (!((bool)down)) || (this!=parent->selectedBuilding) || getBuilding() == nullptr)
		return;
	if( !CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) ) //inside building image
	{
		BuildingID bid = getBuilding()->bid;
		const CBuilding *bld = town->town->buildings.at(bid);
		if (bid < BuildingID::DWELL_FIRST)
		{
			CRClickPopup::createAndPush(CInfoWindow::genText(bld->Name(), bld->Description()),
				std::make_shared<CComponent>(CComponent::building, bld->town->faction->index, bld->bid));
		}
		else
		{
			int level = ( bid - BuildingID::DWELL_FIRST ) % GameConstants::CREATURES_PER_TOWN;
			GH.pushIntT<CDwellingInfoBox>(parent->pos.x+parent->pos.w/2, parent->pos.y+parent->pos.h/2, town, level);
		}
	}
}

SDL_Color multiplyColors(const SDL_Color & b, const SDL_Color & a, double f)
{
	SDL_Color ret;
	ret.r = static_cast<Uint8>(a.r * f + b.r * (1 - f));
	ret.g = static_cast<Uint8>(a.g * f + b.g * (1 - f));
	ret.b = static_cast<Uint8>(a.b * f + b.b * (1 - f));
	ret.a = static_cast<Uint8>(a.a * f + b.b * (1 - f));
	return ret;
}

void CBuildingRect::show(SDL_Surface * to)
{
	const ui32 stageDelay = 16;

	const ui32 S1_TRANSP  = 16; //0.5 sec building appear 0->100 transparency
	const ui32 S2_WHITE_B = 32; //0.5 sec border glows from white to yellow
	const ui32 S3_YELLOW_B= 48; //0.5 sec border glows from yellow to normal
	const ui32 BUILDED    = 80; //  1 sec delay, nothing happens

	if(stateCounter < S1_TRANSP)
	{
		setAlpha(255*stateCounter/stageDelay);
		CShowableAnim::show(to);
	}
	else
	{
		setAlpha(255);
		CShowableAnim::show(to);
	}

	if(border && stateCounter > S1_TRANSP)
	{
		if(stateCounter == BUILDED)
		{
			if(parent->selectedBuilding == this)
				blitAtLoc(border,0,0,to);
			return;
		}
		if(border->format->palette != nullptr)
		{
			// key colors in glowing border
			SDL_Color c1 = {200, 200, 200, 255};
			SDL_Color c2 = {120, 100,  60, 255};
			SDL_Color c3 = {200, 180, 110, 255};

			ui32 colorID = SDL_MapRGB(border->format, c3.r, c3.g, c3.b);
			SDL_Color oldColor = border->format->palette->colors[colorID];
			SDL_Color newColor;

			if (stateCounter < S2_WHITE_B)
				newColor = multiplyColors(c1, c2, static_cast<double>(stateCounter % stageDelay) / stageDelay);
			else
			if (stateCounter < S3_YELLOW_B)
				newColor = multiplyColors(c2, c3, static_cast<double>(stateCounter % stageDelay) / stageDelay);
			else
				newColor = oldColor;

			SDL_SetColors(border, &newColor, colorID, 1);
			blitAtLoc(border, 0, 0, to);
			SDL_SetColors(border, &oldColor, colorID, 1);
		}
	}
	if(stateCounter < BUILDED)
		stateCounter++;
}

void CBuildingRect::showAll(SDL_Surface * to)
{
	if (stateCounter == 0)
		return;

	CShowableAnim::showAll(to);
	if(!active && parent->selectedBuilding == this && border)
		blitAtLoc(border,0,0,to);
}

std::string CBuildingRect::getSubtitle()//hover text for building
{
	if (!getBuilding())
		return "";

	int bid = getBuilding()->bid;

	if (bid<30)//non-dwellings - only buiding name
		return town->town->buildings.at(getBuilding()->bid)->Name();
	else//dwellings - recruit %creature%
	{
		auto & availableCreatures = town->creatures[(bid-30)%GameConstants::CREATURES_PER_TOWN].second;
		if(availableCreatures.size())
		{
			int creaID = availableCreatures.back();//taking last of available creatures
			return CGI->generaltexth->allTexts[16] + " " + CGI->creh->objects.at(creaID)->namePl;
		}
		else
		{
			logGlobal->warn("Dwelling with id %d offers no creatures!", bid);
			return "#ERROR#";
		}
	}
}

void CBuildingRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if(area && isItIn(&pos,sEvent.x, sEvent.y))
	{
		if(CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y)) //hovered pixel is inside this building
		{
			if(parent->selectedBuilding == this)
			{
				parent->selectedBuilding = nullptr;
				GH.statusbar->clear();
			}
		}
		else //inside the area of this building
		{
			if(! parent->selectedBuilding //no building hovered
			  || (*parent->selectedBuilding)<(*this)) //or we are on top
			{
				parent->selectedBuilding = this;
				GH.statusbar->write(getSubtitle());
			}
		}
	}
}

CDwellingInfoBox::CDwellingInfoBox(int centerX, int centerY, const CGTownInstance * Town, int level)
	: CWindowObject(RCLICK_POPUP, "CRTOINFO", Point(centerX, centerY))
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background->colorize(Town->tempOwner);

	const CCreature * creature = CGI->creh->objects.at(Town->creatures.at(level).second.back());

	title = std::make_shared<CLabel>(80, 30, FONT_SMALL, CENTER, Colors::WHITE, creature->namePl);
	animation = std::make_shared<CCreaturePic>(30, 44, creature, true, true);

	std::string text = boost::lexical_cast<std::string>(Town->creatures.at(level).first);
	available = std::make_shared<CLabel>(80,190, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[217] + text);
	costPerTroop = std::make_shared<CLabel>(80, 227, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[346]);

	for(int i = 0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if(creature->cost[i])
		{
			resPicture.push_back(std::make_shared<CAnimImage>("RESOURCE", i, 0, 0, 0));
			resAmount.push_back(std::make_shared<CLabel>(0,0, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(creature->cost[i])));
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
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	owner = Owner;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;
	upg = updown;

	portrait = std::make_shared<CAnimImage>("PortraitsLarge", 0, 0, 0, 0);
	portrait->visible = false;

	flag = std::make_shared<CAnimImage>("CREST58", 0, 0, 0, 0);
	flag->visible = false;

	selection = std::make_shared<CAnimImage>("TWCRPORT", 1, 0);
	selection->visible = false;

	set(h);

	addUsedEvents(LCLICK | RCLICK | HOVER);
}

CHeroGSlot::~CHeroGSlot() = default;

void CHeroGSlot::hover(bool on)
{
	if(!on)
	{
		GH.statusbar->clear();
		return;
	}
	std::shared_ptr<CHeroGSlot> other = upg ? owner->garrisonedHero : owner->visitingHero;
	std::string temp;
	if(hero)
	{
		if(isSelected())//view NNN
		{
			temp = CGI->generaltexth->tcommands[4];
			boost::algorithm::replace_first(temp,"%s",hero->name);
		}
		else if(other->hero && other->isSelected())//exchange
		{
			temp = CGI->generaltexth->tcommands[7];
			boost::algorithm::replace_first(temp,"%s",hero->name);
			boost::algorithm::replace_first(temp,"%s",other->hero->name);
		}
		else// select NNN (in ZZZ)
		{
			if(upg)//down - visiting
			{
				temp = CGI->generaltexth->tcommands[32];
				boost::algorithm::replace_first(temp,"%s",hero->name);
			}
			else //up - garrison
			{
				temp = CGI->generaltexth->tcommands[12];
				boost::algorithm::replace_first(temp,"%s",hero->name);
			}
		}
	}
	else //we are empty slot
	{
		if(other->isSelected() && other->hero) //move NNNN
		{
			temp = CGI->generaltexth->tcommands[6];
			boost::algorithm::replace_first(temp,"%s",other->hero->name);
		}
		else //empty
		{
			temp = CGI->generaltexth->allTexts[507];
		}
	}
	if(temp.size())
		GH.statusbar->write(temp);
}

void CHeroGSlot::clickLeft(tribool down, bool previousState)
{
	std::shared_ptr<CHeroGSlot> other = upg ? owner->garrisonedHero : owner->visitingHero;
	if(!down)
	{
		owner->garr->setSplittingMode(false);
		owner->garr->selectSlot(nullptr);

		if(hero && isSelected())
		{
			setHighlight(false);
			LOCPLINT->openHeroWindow(hero);
		}
		else if(other->hero && other->isSelected())
		{
			bool allow = true;
			if(upg) //moving hero out of town - check if it is allowed
			{
				if(!hero && LOCPLINT->cb->howManyHeroes(false) >= VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)
				{
					std::string tmp = CGI->generaltexth->allTexts[18]; //You already have %d adventuring heroes under your command.
					boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes(false)));
					LOCPLINT->showInfoDialog(tmp, std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
					allow = false;
				}
				else if(!other->hero->stacksCount()) //hero has no creatures - strange, but if we have appropriate error message...
				{
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[19], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo); //This hero has no creatures.  A hero must have creatures before he can brave the dangers of the countryside.
					allow = false;
				}
			}

			setHighlight(false);
			other->setHighlight(false);

			if(allow)
			{
				owner->swapArmies();
				hero = other->hero;
			}
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
}

void CHeroGSlot::clickRight(tribool down, bool previousState)
{
	if(hero && down)
	{
		GH.pushIntT<CInfoBoxPopup>(Point(pos.x + 175, pos.y + 100), hero);
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
		for(auto & elem : owner->garr->splitButtons) //splitting enabled when slot higlighted
			elem->block(!on);
	}
}

void CHeroGSlot::set(const CGHeroInstance * newHero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	hero = newHero;

	selection->visible = false;
	portrait->visible = false;
	flag->visible = false;

	if(newHero)
	{
		portrait->visible = true;
		portrait->setFrame(newHero->portrait);
	}
	else if(!upg && owner->showEmpty) //up garrison
	{
		flag->visible = true;
		flag->setFrame(LOCPLINT->castleInt->town->getOwner().getNum());
	}
}

HeroSlots::HeroSlots(const CGTownInstance * Town, Point garrPos, Point visitPos, std::shared_ptr<CGarrisonInt> Garrison, bool ShowEmpty):
	showEmpty(ShowEmpty),
	town(Town),
	garr(Garrison)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	garrisonedHero = std::make_shared<CHeroGSlot>(garrPos.x, garrPos.y, 0, town->garrisonHero, this);
	visitingHero = std::make_shared<CHeroGSlot>(visitPos.x, visitPos.y, 1, town->visitingHero, this);
}

HeroSlots::~HeroSlots() = default;

void HeroSlots::update()
{
	garrisonedHero->set(town->garrisonHero);
	visitingHero->set(town->visitingHero);
}

void HeroSlots::splitClicked()
{
	if(!!town->visitingHero && town->garrisonHero && (visitingHero->isSelected() || garrisonedHero->isSelected()))
	{
		LOCPLINT->heroExchangeStarted(town->visitingHero->id, town->garrisonHero->id, QueryID(-1));
	}
}

void HeroSlots::swapArmies()
{
	if(!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{
		if(!town->visitingHero->canBeMergedWith(*town))
		{
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[275], std::vector<std::shared_ptr<CComponent>>(), soundBase::sound_todo);
			return;
		}
	}
	LOCPLINT->cb->swapGarrisonHero(town);
}


class SORTHELP
{
public:
	bool operator() (const CIntObject * a, const CIntObject * b)
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
	}
};

SORTHELP buildSorter;

CCastleBuildings::CCastleBuildings(const CGTownInstance* Town):
	town(Town),
	selectedBuilding(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	background = std::make_shared<CPicture>(town->town->clientInfo.townBackground);
	pos.w = background->pos.w;
	pos.h = background->pos.h;

	recreate();
}

CCastleBuildings::~CCastleBuildings() = default;

void CCastleBuildings::recreate()
{
	selectedBuilding = nullptr;
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	buildings.clear();
	groups.clear();

	//Generate buildings list

	auto buildingsCopy = town->builtBuildings;// a bit modified copy of built buildings

	if(vstd::contains(town->builtBuildings, BuildingID::SHIPYARD))
	{
		auto bayPos = town->bestLocation();
		if(!bayPos.valid())
			logGlobal->warn("Shipyard in non-coastal town!");
		std::vector <const CGObjectInstance *> vobjs = LOCPLINT->cb->getVisitableObjs(bayPos, false);
		//there is visitable obj at shipyard output tile and it's a boat or hero (on boat)
		if(!vobjs.empty() && (vobjs.front()->ID == Obj::BOAT || vobjs.front()->ID == Obj::HERO))
		{
			buildingsCopy.insert(BuildingID::SHIP);
		}
	}

	for(const CStructure * structure : town->town->clientInfo.structures)
	{
		if(!structure->building)
		{
			buildings.push_back(std::make_shared<CBuildingRect>(this, town, structure));
			continue;
		}
		if(vstd::contains(buildingsCopy, structure->building->bid))
		{
			groups[structure->building->getBase()].push_back(structure);
		}
	}

	for(auto & entry : groups)
	{
		const CBuilding * build = town->town->buildings.at(entry.first);

		const CStructure * toAdd = *boost::max_element(entry.second, [=](const CStructure * a, const CStructure * b)
		{
			return build->getDistance(a->building->bid) < build->getDistance(b->building->bid);
		});

		buildings.push_back(std::make_shared<CBuildingRect>(this, town, toAdd));
	}

	boost::sort(children, buildSorter); //TODO: create building in blit order
}

void CCastleBuildings::addBuilding(BuildingID building)
{
	//FIXME: implement faster method without complete recreation of town
	BuildingID base = town->town->buildings.at(building)->getBase();

	recreate();

	auto & structures = groups.at(base);

	for(auto buildingRect : buildings)
	{
		if(vstd::contains(structures, buildingRect->str))
		{
			//reset animation
			if(structures.size() == 1)
				buildingRect->stateCounter = 0; // transparency -> fully visible stage
			else
				buildingRect->stateCounter = 16; // already in fully visible stage
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
	if(town->visitingHero)
		return town->visitingHero;
	else
		return town->garrisonHero;
}

void CCastleBuildings::buildingClicked(BuildingID building, BuildingSubID::EBuildingSubID subID, BuildingID::EBuildingID upgrades)
{
	logGlobal->trace("You've clicked on %d", (int)building.toEnum());
	const CBuilding *b = town->town->buildings.find(building)->second;

	if(building >= BuildingID::DWELL_FIRST)
	{
		enterDwelling((building-BuildingID::DWELL_FIRST)%GameConstants::CREATURES_PER_TOWN);
	}
	else
	{
		switch(building)
		{
		case BuildingID::MAGES_GUILD_1:
		case BuildingID::MAGES_GUILD_2:
		case BuildingID::MAGES_GUILD_3:
		case BuildingID::MAGES_GUILD_4:
		case BuildingID::MAGES_GUILD_5:
				enterMagesGuild();
				break;

		case BuildingID::TAVERN:
				LOCPLINT->showTavernWindow(town);
				break;

		case BuildingID::SHIPYARD:
				if(town->shipyardStatus() == IBoatGenerator::GOOD)
					LOCPLINT->showShipyardDialog(town);
				else if(town->shipyardStatus() == IBoatGenerator::BOAT_ALREADY_BUILT)
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[51]);
				break;

		case BuildingID::FORT:
		case BuildingID::CITADEL:
		case BuildingID::CASTLE:
				GH.pushIntT<CFortScreen>(town);
				break;

		case BuildingID::VILLAGE_HALL:
		case BuildingID::CITY_HALL:
		case BuildingID::TOWN_HALL:
		case BuildingID::CAPITOL:
				enterTownHall();
				break;

		case BuildingID::MARKETPLACE:
				GH.pushIntT<CMarketplaceWindow>(town, town->visitingHero);
				break;

		case BuildingID::BLACKSMITH:
				enterBlacksmith(town->town->warMachine);
				break;

		case BuildingID::SHIP:
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[51]); //Cannot build another boat
			break;

		case BuildingID::SPECIAL_1:
		case BuildingID::SPECIAL_2:
		case BuildingID::SPECIAL_3:
				switch(subID)
				{
				case BuildingSubID::NONE:
						break;

				case BuildingSubID::MYSTIC_POND:
						enterFountain(building, subID, upgrades);
						break;

				case BuildingSubID::ARTIFACT_MERCHANT:
						if(town->visitingHero)
							GH.pushIntT<CMarketplaceWindow>(town, town->visitingHero, EMarketMode::RESOURCE_ARTIFACT);
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				case BuildingSubID::FOUNTAIN_OF_FORTUNE:
						enterFountain(building, subID, upgrades);
					break;

				case BuildingSubID::FREELANCERS_GUILD:
						if(getHero())
							GH.pushIntT<CMarketplaceWindow>(town, getHero(), EMarketMode::CREATURE_RESOURCE);
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				case BuildingSubID::MAGIC_UNIVERSITY:
						if (getHero())
							GH.pushIntT<CUniversityWindow>(getHero(), town);
						else
							enterBuilding(building);
						break;

				case BuildingSubID::BROTHERHOOD_OF_SWORD:
						if(upgrades == BuildingID::TAVERN)
							LOCPLINT->showTavernWindow(town);
						else
						enterBuilding(building);
						break;

				case BuildingSubID::CASTLE_GATE:
						enterCastleGate();
						break;

				case BuildingSubID::CREATURE_TRANSFORMER: //Skeleton Transformer
						GH.pushIntT<CTransformerWindow>(getHero(), town);
						break;

				case BuildingSubID::PORTAL_OF_SUMMONING:
						if (town->creatures[GameConstants::CREATURES_PER_TOWN].second.empty())//No creatures
							LOCPLINT->showInfoDialog(CGI->generaltexth->tcommands[30]);
						else
							enterDwelling(GameConstants::CREATURES_PER_TOWN);
						break;

				case BuildingSubID::BALLISTA_YARD:
						enterBlacksmith(ArtifactID::BALLISTA);
						break;

				default:
						enterBuilding(building);
						break;
				}
				break;

		default:
				enterBuilding(building);
				break;
		}
	}
}

void CCastleBuildings::enterBlacksmith(ArtifactID artifactID)
{
	const CGHeroInstance *hero = town->visitingHero;
	if(!hero)
	{
		LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % town->town->buildings.find(BuildingID::BLACKSMITH)->second->Name()));
		return;
	}
	auto art = artifactID.toArtifact(CGI->artifacts());

	int price = art->getPrice();
	bool possible = LOCPLINT->cb->getResourceAmount(Res::GOLD) >= price && !hero->hasArt(artifactID);
	CreatureID cre = art->getWarMachine();
	GH.pushIntT<CBlacksmithDialog>(possible, cre, artifactID, hero->id);
}

void CCastleBuildings::enterBuilding(BuildingID building)
{
	std::vector<std::shared_ptr<CComponent>> comps(1, std::make_shared<CComponent>(CComponent::building, town->subID, building));
	LOCPLINT->showInfoDialog( town->town->buildings.find(building)->second->Description(), comps);
}

void CCastleBuildings::enterCastleGate()
{
	if (!town->visitingHero)
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[126]);
		return;//only visiting hero can use castle gates
	}
	std::vector <int> availableTowns;
	std::vector <const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(true);
	for(auto & Town : Towns)
	{
		const CGTownInstance *t = Town;
		if (t->id != this->town->id && t->visitingHero == nullptr && //another town, empty and this is
			t->town->faction->index == town->town->faction->index && //the town of the same faction
			t->hasBuilt(BuildingSubID::CASTLE_GATE)) //and the town has a castle gate
		{
			availableTowns.push_back(t->id.getNum());//add to the list
		}
	}
	auto gateIcon = std::make_shared<CAnimImage>(town->town->clientInfo.buildingsIcons, BuildingID::CASTLE_GATE);//will be deleted by selection window
	GH.pushIntT<CObjectListWindow>(availableTowns, gateIcon, CGI->generaltexth->jktexts[40],
		CGI->generaltexth->jktexts[41], std::bind (&CCastleInterface::castleTeleport, LOCPLINT->castleInt, _1));
}

void CCastleBuildings::enterDwelling(int level)
{
	assert(level >= 0 && level < town->creatures.size());
	auto recruitCb = [=](CreatureID id, int count){ LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level); };
	GH.pushIntT<CRecruitmentWindow>(town, level, town, recruitCb, -87);
}

void CCastleBuildings::enterToTheQuickRecruitmentWindow()
{
	const auto beginIt = town->creatures.cbegin();
	const auto afterLastIt = town->creatures.size() > GameConstants::CREATURES_PER_TOWN
		? std::next(beginIt, GameConstants::CREATURES_PER_TOWN)
		: town->creatures.cend();
	const auto hasSomeoneToRecruit = std::any_of(beginIt, afterLastIt,
		[](const auto & creatureInfo) { return creatureInfo.first > 0; });
	if(hasSomeoneToRecruit)
		GH.pushIntT<QuickRecruitmentWindow>(town, pos);
	else
		CInfoWindow::showInfoDialog(CGI->generaltexth->localizedTexts["townHall"]["noCreaturesToRecruit"].String(), {});
}

void CCastleBuildings::enterFountain(const BuildingID & building, BuildingSubID::EBuildingSubID subID, BuildingID::EBuildingID upgrades)
{
	std::vector<std::shared_ptr<CComponent>> comps(1, std::make_shared<CComponent>(CComponent::building,town->subID,building));
	std::string descr = town->town->buildings.find(building)->second->Description();
	std::string hasNotProduced;
	std::string hasProduced;

	if(this->town->town->faction->index == (TFaction)ETownType::RAMPART)
	{
		hasNotProduced = CGI->generaltexth->allTexts[677];
		hasProduced = CGI->generaltexth->allTexts[678];
	}
	else
	{
		auto buildingName = town->town->getSpecialBuilding(subID)->Name();

		hasNotProduced = std::string(CGI->generaltexth->localizedTexts["townHall"]["hasNotProduced"].String());
		hasProduced = std::string(CGI->generaltexth->localizedTexts["townHall"]["hasProduced"].String());
		boost::algorithm::replace_first(hasNotProduced, "%s", buildingName);
		boost::algorithm::replace_first(hasProduced, "%s", buildingName);
	}

	bool isMysticPondOrItsUpgrade = subID == BuildingSubID::MYSTIC_POND
		|| (upgrades != BuildingID::NONE
			&& town->town->buildings.find(BuildingID(upgrades))->second->subId == BuildingSubID::MYSTIC_POND);

	if(upgrades != BuildingID::NONE)
		descr += "\n\n"+town->town->buildings.find(BuildingID(upgrades))->second->Description();

	if(isMysticPondOrItsUpgrade) //for vanila Rampart like towns
	{
		if(town->bonusValue.first == 0) //Mystic Pond produced nothing;
			descr += "\n\n" + hasNotProduced;
		else //Mystic Pond produced something;
		{
			descr += "\n\n" + hasProduced;
			boost::algorithm::replace_first(descr,"%s",CGI->generaltexth->restypes[town->bonusValue.first]);
			boost::algorithm::replace_first(descr,"%d",boost::lexical_cast<std::string>(town->bonusValue.second));
		}
	}
	LOCPLINT->showInfoDialog(descr, comps);
}

void CCastleBuildings::enterMagesGuild()
{
	const CGHeroInstance *hero = getHero();

	if(hero && !hero->hasSpellbook()) //hero doesn't have spellbok
	{
		const StartInfo *si = LOCPLINT->cb->getStartInfo();
		// it would be nice to find a way to move this hack to config/mapOverrides.json
		if(si && si->campState && si->campState->camp &&                // We're in campaign,
			(si->campState->camp->header.filename == "DATA/YOG.H3C") && // which is "Birth of a Barbarian",
			(hero->subID == 45))                                        // and the hero is Yog (based on Solmyr)
		{
			// "Yog has given up magic in all its forms..."
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[736]);
		}
		else if(LOCPLINT->cb->getResourceAmount(Res::GOLD) < 500) //not enough gold to buy spellbook
		{
			openMagesGuild();
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[213]);
		}
		else
		{
			CFunctionList<void()> onYes = [this](){ openMagesGuild(); };
			CFunctionList<void()> onNo = onYes;
			onYes += [hero](){ LOCPLINT->cb->buyArtifact(hero, ArtifactID::SPELLBOOK); };
			std::vector<std::shared_ptr<CComponent>> components(1, std::make_shared<CComponent>(CComponent::artifact,ArtifactID::SPELLBOOK,0));

			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[214], onYes, onNo, components);
		}
	}
	else
	{
		openMagesGuild();
	}
}

void CCastleBuildings::enterTownHall()
{
	if(town->visitingHero && town->visitingHero->hasArt(2) &&
		!vstd::contains(town->builtBuildings, BuildingID::GRAIL)) //hero has grail, but town does not have it
	{
		if(!vstd::contains(town->forbiddenBuildings, BuildingID::GRAIL))
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[597], //Do you wish this to be the permanent home of the Grail?
										[&](){ LOCPLINT->cb->buildBuilding(town, BuildingID::GRAIL); },
										[&](){ openTownHall(); });
		}
		else
		{
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[673]);
			dynamic_cast<CInfoWindow*>(GH.topInt().get())->buttons[0]->addCallback(std::bind(&CCastleBuildings::openTownHall, this));
		}
	}
	else
	{
		openTownHall();
	}
}

void CCastleBuildings::openMagesGuild()
{
	std::string mageGuildBackground;
	mageGuildBackground = LOCPLINT->castleInt->town->town->clientInfo.guildBackground;
	GH.pushIntT<CMageGuildScreen>(LOCPLINT->castleInt,mageGuildBackground);
}

void CCastleBuildings::openTownHall()
{
	GH.pushIntT<CHallInterface>(town);
}

CCreaInfo::CCreaInfo(Point position, const CGTownInstance * Town, int Level, bool compact, bool ShowAvailable):
	town(Town),
	level(Level),
	showAvailable(ShowAvailable)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos += position;

	if(town->creatures.size() <= level || town->creatures[level].second.empty())
	{
		level = -1;
		return;//No creature
	}
	addUsedEvents(LCLICK | RCLICK | HOVER);

	ui32 creatureID = town->creatures[level].second.back();
	creature = CGI->creh->objects[creatureID];

	picture = std::make_shared<CAnimImage>("CPRSMALL", creature->getIconIndex(), 0, 8, 0);

	std::string value;
	if(showAvailable)
		value = boost::lexical_cast<std::string>(town->creatures[level].first);
	else
		value = boost::lexical_cast<std::string>(town->creatureGrowth(level));

	if(compact)
	{
		label = std::make_shared<CLabel>(40, 32, FONT_TINY, BOTTOMRIGHT, Colors::WHITE, value);
		pos.x += 8;
		pos.w = 32;
		pos.h = 32;
	}
	else
	{
		label = std::make_shared<CLabel>(24, 40, FONT_SMALL, CENTER, Colors::WHITE, value);
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
			value = boost::lexical_cast<std::string>(town->creatures[level].first);
		else
			value = boost::lexical_cast<std::string>(town->creatureGrowth(level));

		if(value != label->text)
			label->setText(value);
	}
}

void CCreaInfo::hover(bool on)
{
	std::string message = CGI->generaltexth->allTexts[588];
	boost::algorithm::replace_first(message, "%s", creature->namePl);

	if(on)
	{
		GH.statusbar->write(message);
	}
	else
	{
		GH.statusbar->clearMatching(message);
	}
}

void CCreaInfo::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		int offset = LOCPLINT->castleInt? (-87) : 0;
		auto recruitCb = [=](CreatureID id, int count)
		{
			LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level);
		};
		GH.pushIntT<CRecruitmentWindow>(town, level, town, recruitCb, offset);
	}
}

std::string CCreaInfo::genGrowthText()
{
	GrowthInfo gi = town->getGrowthInfo(level);
	std::string descr = boost::str(boost::format(CGI->generaltexth->allTexts[589]) % creature->nameSing % gi.totalGrowth());

	for(const GrowthInfo::Entry & entry : gi.entries)
		descr +="\n" + entry.description;

	return descr;
}

void CCreaInfo::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if (showAvailable)
			GH.pushIntT<CDwellingInfoBox>(screen->w/2, screen->h/2, town, level);
		else
			CRClickPopup::createAndPush(genGrowthText(), std::make_shared<CComponent>(CComponent::creature, creature->idNumber));
	}
}

CTownInfo::CTownInfo(int posX, int posY, const CGTownInstance * Town, bool townHall)
	: town(Town),
	building(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	addUsedEvents(RCLICK | HOVER);
	pos.x += posX;
	pos.y += posY;
	int buildID;

	if(townHall)
	{
		buildID = 10 + town->hallLevel();
		picture = std::make_shared<CAnimImage>("ITMTL.DEF", town->hallLevel());
	}
	else
	{
		buildID = 6 + town->fortLevel();
		if(buildID == 6)
			return;//FIXME: suspicious statement, fix or comment
		picture = std::make_shared<CAnimImage>("ITMCL.DEF", town->fortLevel()-1);
	}
	building = town->town->buildings.at(BuildingID(buildID));
	pos = picture->pos;
}

void CTownInfo::hover(bool on)
{
	if(on)
	{
		if(building )
			GH.statusbar->write(building->Name());
	}
	else
	{
		GH.statusbar->clear();
	}
}

void CTownInfo::clickRight(tribool down, bool previousState)
{
	if(building && down)
	{
		auto c =  std::make_shared<CComponent>(CComponent::building, building->town->faction->index, building->bid);
		CRClickPopup::createAndPush(CInfoWindow::genText(building->Name(), building->Description()), c);
	}
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, const CGTownInstance * from):
	CStatusbarWindow(PLAYER_COLORED | BORDERED),
	town(Town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	LOCPLINT->castleInt = this;
	addUsedEvents(KEYBOARD);

	builds = std::make_shared<CCastleBuildings>(town);
	panel = std::make_shared<CPicture>("TOWNSCRN", 0, builds->pos.h);
	panel->colorize(LOCPLINT->playerID);
	pos.w = panel->pos.w;
	pos.h = builds->pos.h + panel->pos.h;
	center();
	updateShadow();

	garr = std::make_shared<CGarrisonInt>(305, 387, 4, Point(0,96), town->getUpperArmy(), town->visitingHero);
	garr->type |= REDRAW_PARENT;

	heroes = std::make_shared<HeroSlots>(town, Point(241, 387), Point(241, 483), garr, true);
	title = std::make_shared<CLabel>(85, 387, FONT_MEDIUM, TOPLEFT, Colors::WHITE, town->name);
	income = std::make_shared<CLabel>(195, 443, FONT_SMALL, CENTER);
	icon = std::make_shared<CAnimImage>("ITPT", 0, 0, 15, 387);

	exit = std::make_shared<CButton>(Point(744, 544), "TSBTNS", CButton::tooltip(CGI->generaltexth->tcommands[8]), [&](){close();}, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	exit->setImageOrder(4, 5, 6, 7);

	auto split = std::make_shared<CButton>(Point(744, 382), "TSBTNS", CButton::tooltip(CGI->generaltexth->tcommands[3]), [&]()
	{
		garr->splitClick();
		heroes->splitClicked();
	});
	garr->addSplitBtn(split);

	Rect barRect(9, 182, 732, 18);
	auto statusbarBackground = std::make_shared<CPicture>(*(panel.get()), barRect, 9, 555, false);
	statusbar = CGStatusBar::create(statusbarBackground);
	resdatabar = std::make_shared<CResDataBar>("ARESBAR", 3, 575, 32, 2, 85, 85);

	townlist = std::make_shared<CTownList>(3, Point(744, 414), "IAM014", "IAM015");
	if(from)
		townlist->select(from);

	townlist->select(town); //this will scroll list to select current town
	townlist->onSelect = std::bind(&CCastleInterface::townChange, this);

	recreateIcons();
	CCS->musich->playMusic(town->town->clientInfo.musicTheme, true, false);
}

CCastleInterface::~CCastleInterface()
{
	if(LOCPLINT->castleInt == this)
		LOCPLINT->castleInt = nullptr;
}

void CCastleInterface::updateGarrisons()
{
	garr->recreateSlots();
}

void CCastleInterface::close()
{
	if(town->tempOwner == LOCPLINT->playerID) //we may have opened window for an allied town
	{
		if(town->visitingHero && town->visitingHero->tempOwner == LOCPLINT->playerID)
			adventureInt->select(town->visitingHero);
		else
			adventureInt->select(town);
	}
	CWindowObject::close();
}

void CCastleInterface::castleTeleport(int where)
{
	const CGTownInstance * dest = LOCPLINT->cb->getTown(ObjectInstanceID(where));
	adventureInt->select(town->visitingHero);//according to assert(ho == adventureInt->selection) in the eraseCurrentPathOf
	LOCPLINT->cb->teleportHero(town->visitingHero, dest);
	LOCPLINT->eraseCurrentPathOf(town->visitingHero, false);
}

void CCastleInterface::townChange()
{
	//TODO: do not recreate window
	const CGTownInstance * dest = LOCPLINT->towns[townlist->getSelectedIndex()];
	const CGTownInstance * town = this->town;// "this" is going to be deleted
	if ( dest == town )
		return;
	close();
	GH.pushIntT<CCastleInterface>(dest, town);
}

void CCastleInterface::addBuilding(BuildingID bid)
{
	deactivate();
	builds->addBuilding(bid);
	recreateIcons();
	activate();
}

void CCastleInterface::removeBuilding(BuildingID bid)
{
	deactivate();
	builds->removeBuilding(bid);
	recreateIcons();
	activate();
}

void CCastleInterface::recreateIcons()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	icon->setFrame(iconIndex);
	TResources townIncome = town->dailyIncome();
	income->setText(boost::lexical_cast<std::string>(townIncome[Res::GOLD]));

	hall = std::make_shared<CTownInfo>(80, 413, town, true);
	fort = std::make_shared<CTownInfo>(122, 413, town, false);

	fastArmyPurchase = std::make_shared<CButton>(Point(122, 413), "itmcl.def", CButton::tooltip(), [&](){ builds->enterToTheQuickRecruitmentWindow(); });
	fastArmyPurchase->setImageOrder(town->fortLevel() - 1, town->fortLevel() - 1, town->fortLevel() - 1, town->fortLevel() - 1);
	fastArmyPurchase->setAnimateLonelyFrame(true);

	creainfo.clear();

	for(size_t i=0; i<4; i++)
		creainfo.push_back(std::make_shared<CCreaInfo>(Point(14+55*(int)i, 459), town, (int)i));

	for(size_t i=0; i<4; i++)
		creainfo.push_back(std::make_shared<CCreaInfo>(Point(14+55*(int)i, 507), town, (int)i+4));
}

void CCastleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.state != SDL_PRESSED) return;

	switch(key.keysym.sym)
	{
	case SDLK_UP:
		townlist->selectPrev();
		break;
	case SDLK_DOWN:
		townlist->selectNext();
		break;
	case SDLK_SPACE:
		heroes->swapArmies();
		break;
	case SDLK_t:
		if(town->hasBuilt(BuildingID::TAVERN))
			LOCPLINT->showTavernWindow(town);
		break;
	default:
		break;
	}
}

CHallInterface::CBuildingBox::CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building):
	town(Town),
	building(Building)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += x;
	pos.y += y;
	pos.w = 154;
	pos.h = 92;

	state = LOCPLINT->cb->canBuildStructure(town, building->bid);

	static int panelIndex[12] =
	{
		3, 3, 3, 0, 0, 2, 2, 1, 2, 2,  3,  3
	};
	static int iconIndex[12] =
	{
		-1, -1, -1, 0, 0, 1, 2, -1, 1, 1, -1, -1
	};

	icon = std::make_shared<CAnimImage>(town->town->clientInfo.buildingsIcons, building->bid, 0, 2, 2);
	header = std::make_shared<CAnimImage>("TPTHBAR", panelIndex[state], 0, 1, 73);
	if(iconIndex[state] >=0)
		mark = std::make_shared<CAnimImage>("TPTHCHK", iconIndex[state], 0, 136, 56);
	name = std::make_shared<CLabel>(75, 81, FONT_SMALL, CENTER, Colors::WHITE, building->Name());

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
			toPrint = CGI->generaltexth->hcommands[5];
		else if(state==EBuildingState::CANT_BUILD_TODAY)
			toPrint = CGI->generaltexth->allTexts[223];
		else
			toPrint = CGI->generaltexth->hcommands[state];
		boost::algorithm::replace_first(toPrint,"%s",building->Name());
		GH.statusbar->write(toPrint);
	}
	else
	{
		GH.statusbar->clear();
	}
}

void CHallInterface::CBuildingBox::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
		GH.pushIntT<CBuildWindow>(town,building,state,0);
}

void CHallInterface::CBuildingBox::clickRight(tribool down, bool previousState)
{
	if(down)
		GH.pushIntT<CBuildWindow>(town,building,state,1);
}

CHallInterface::CHallInterface(const CGTownInstance * Town):
	CStatusbarWindow(PLAYER_COLORED | BORDERED, Town->town->clientInfo.hallBackground),
	town(Town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);
	Rect barRect(5, 556, 740, 18);

	auto statusbarBackground = std::make_shared<CPicture>(*background, barRect, 5, 556, false);
	statusbar = CGStatusBar::create(statusbarBackground);

	title = std::make_shared<CLabel>(399, 12, FONT_MEDIUM, CENTER, Colors::WHITE, town->town->buildings.at(BuildingID(town->hallLevel()+BuildingID::VILLAGE_HALL))->Name());
	exit = std::make_shared<CButton>(Point(748, 556), "TPMAGE1.DEF", CButton::tooltip(CGI->generaltexth->hcommands[8]), [&](){close();}, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	auto & boxList = town->town->clientInfo.hallSlots;
	boxes.resize(boxList.size());
	for(size_t row=0; row<boxList.size(); row++) //for each row
	{
		for(size_t col=0; col<boxList[row].size(); col++) //for each box
		{
			const CBuilding * building = nullptr;
			for(auto & buildingID : boxList[row][col])//we are looking for the first not built structure
			{
				const CBuilding * current = town->town->buildings.at(buildingID);
				if(vstd::contains(town->builtBuildings, buildingID))
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
			int posX = pos.w/2 - (int)boxList[row].size()*154/2 - ((int)boxList[row].size()-1)*20 + 194*(int)col,
				posY = 35 + 104*(int)row;

			if(building)
				boxes[row].push_back(std::make_shared<CBuildingBox>(posX, posY, town, building));
		}
	}
}

CBuildWindow::CBuildWindow(const CGTownInstance *Town, const CBuilding * Building, int state, bool rightClick):
	CStatusbarWindow(PLAYER_COLORED | (rightClick ? RCLICK_POPUP : 0), "TPUBUILD"),
	town(Town),
	building(Building)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	icon = std::make_shared<CAnimImage>(town->town->clientInfo.buildingsIcons, building->bid, 0, 125, 50);
	auto statusbarBackground = std::make_shared<CPicture>(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26);
	statusbar = CGStatusBar::create(statusbarBackground);

	name = std::make_shared<CLabel>(197, 30, FONT_MEDIUM, CENTER, Colors::WHITE, boost::str(boost::format(CGI->generaltexth->hcommands[7]) % building->Name()));
	description = std::make_shared<CTextBox>(building->Description(), Rect(33, 135, 329, 67), 0, FONT_MEDIUM, CENTER);
	stateText = std::make_shared<CTextBox>(getTextForState(state), Rect(33, 216, 329, 67), 0, FONT_SMALL, CENTER);

	//Create components for all required resources
	std::vector<std::shared_ptr<CComponent>> components;

	for(int i = 0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if(building->resources[i])
			components.push_back(std::make_shared<CComponent>(CComponent::resource, i, building->resources[i], CComponent::small));
	}

	cost = std::make_shared<CComponentBox>(components, Rect(25, 300, pos.w - 50, 130));

	if(!rightClick)
	{	//normal window
		std::string tooltipYes = boost::str(boost::format(CGI->generaltexth->allTexts[595]) % building->Name());
		std::string tooltipNo  = boost::str(boost::format(CGI->generaltexth->allTexts[596]) % building->Name());

		buy = std::make_shared<CButton>(Point(45, 446), "IBUY30", CButton::tooltip(tooltipYes), [&](){ buyFunc(); }, SDLK_RETURN);
		buy->setBorderColor(Colors::METALLIC_GOLD);
		buy->block(state!=7 || LOCPLINT->playerID != town->tempOwner);

		cancel = std::make_shared<CButton>(Point(290, 445), "ICANCEL", CButton::tooltip(tooltipNo), [&](){ close();}, SDLK_ESCAPE);
		cancel->setBorderColor(Colors::METALLIC_GOLD);
	}
}

void CBuildWindow::buyFunc()
{
	LOCPLINT->cb->buildBuilding(town,building->bid);
	GH.popInts(2); //we - build window and hall screen
}

std::string CBuildWindow::getTextForState(int state)
{
	std::string ret;
	if(state < EBuildingState::ALLOWED)
		ret =  CGI->generaltexth->hcommands[state];
	switch (state)
	{
	case EBuildingState::ALREADY_PRESENT:
	case EBuildingState::CANT_BUILD_TODAY:
	case EBuildingState::NO_RESOURCES:
		ret.replace(ret.find_first_of("%s"), 2, building->Name());
		break;
	case EBuildingState::ALLOWED:
		return CGI->generaltexth->allTexts[219]; //all prereq. are met
	case EBuildingState::PREREQUIRES:
		{
			auto toStr = [&](const BuildingID build) -> std::string
			{
				return town->town->buildings.at(build)->Name();
			};

			ret = CGI->generaltexth->allTexts[52];
			ret += "\n" + town->genBuildingRequirements(building->bid).toString(toStr);
			break;
		}
	case EBuildingState::MISSING_BASE:
		{
			std::string msg = CGI->generaltexth->localizedTexts["townHall"]["missingBase"].String();
			ret = boost::str(boost::format(msg) % town->town->buildings.at(building->upgrade)->Name());
			break;
		}
	}
	return ret;
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int min, int max)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.x+=size.x;
	pos.y+=size.y;
	pos.w = size.w;
	pos.h = size.h;
	init(name, descr, min, max);
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int val)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
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
		valueText = boost::lexical_cast<std::string>(min);
		if(min != max)
			valueText += '-' + boost::lexical_cast<std::string>(max);
	}
	name = std::make_shared<CLabel>(3, 0, FONT_SMALL, TOPLEFT, Colors::WHITE, nameText);
	value = std::make_shared<CLabel>(pos.w-3, pos.h-2, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, valueText);
}

void LabeledValue::hover(bool on)
{
	if(on)
	{
		GH.statusbar->write(hoverText);
	}
	else
	{
		GH.statusbar->clear();
		parent->hovered = false;
	}
}

CFortScreen::CFortScreen(const CGTownInstance * town):
	CStatusbarWindow(PLAYER_COLORED | BORDERED, getBgName(town))
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	ui32 fortSize = static_cast<ui32>(town->creatures.size());
	if(fortSize > GameConstants::CREATURES_PER_TOWN && town->creatures.back().second.empty())
		fortSize--;

	const CBuilding * fortBuilding = town->town->buildings.at(BuildingID(town->fortLevel()+6));
	title = std::make_shared<CLabel>(400, 12, FONT_BIG, CENTER, Colors::WHITE, fortBuilding->Name());

	std::string text = boost::str(boost::format(CGI->generaltexth->fcommands[6]) % fortBuilding->Name());
	exit = std::make_shared<CButton>(Point(748, 556), "TPMAGE1", CButton::tooltip(text), [&](){ close(); }, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	std::vector<Point> positions =
	{
		Point(10,  22), Point(404, 22),
		Point(10, 155), Point(404,155),
		Point(10, 288), Point(404,288)
	};

	if(fortSize == GameConstants::CREATURES_PER_TOWN)
	{
		positions.push_back(Point(206,421));
	}
	else
	{
		positions.push_back(Point(10, 421));
		positions.push_back(Point(404,421));
	}

	for(ui32 i=0; i<fortSize; i++)
	{
		BuildingID buildingID;
		if(fortSize == GameConstants::CREATURES_PER_TOWN)
		{
			if(vstd::contains(town->builtBuildings, BuildingID::DWELL_UP_FIRST+i))
				buildingID = BuildingID(BuildingID::DWELL_UP_FIRST+i);
			else
				buildingID = BuildingID(BuildingID::DWELL_FIRST+i);
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

	auto statusbarBackground = std::make_shared<CPicture>(*background, barRect, 4, 554, false);
	statusbar = CGStatusBar::create(statusbarBackground);
}

std::string CFortScreen::getBgName(const CGTownInstance * town)
{
	ui32 fortSize = static_cast<ui32>(town->creatures.size());
	if(fortSize > GameConstants::CREATURES_PER_TOWN && town->creatures.back().second.empty())
		fortSize--;

	if(fortSize == GameConstants::CREATURES_PER_TOWN)
		return "TPCASTL7";
	else
		return "TPCASTL8";
}

void CFortScreen::creaturesChanged()
{
	for(auto & elem : recAreas)
		elem->creaturesChanged();
}

CFortScreen::RecruitArea::RecruitArea(int posX, int posY, const CGTownInstance * Town, int Level):
	town(Town),
	level(Level),
	availableCount(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.x +=posX;
	pos.y +=posY;
	pos.w = 386;
	pos.h = 126;

	if(!town->creatures[level].second.empty())
		addUsedEvents(LCLICK | RCLICK | HOVER);//Activate only if dwelling is present

	icons = std::make_shared<CPicture>("TPCAINFO", 261, 3);

	if(getMyBuilding() != nullptr)
	{
		buildingIcon = std::make_shared<CAnimImage>(town->town->clientInfo.buildingsIcons, getMyBuilding()->bid, 0, 4, 21);
		buildingName = std::make_shared<CLabel>(78, 101, FONT_SMALL, CENTER, Colors::WHITE, getMyBuilding()->Name());

		if(vstd::contains(town->builtBuildings, getMyBuilding()->bid))
		{
			ui32 available = town->creatures[level].first;
			std::string availableText = CGI->generaltexth->allTexts[217]+ boost::lexical_cast<std::string>(available);
			availableCount = std::make_shared<CLabel>(78, 119, FONT_SMALL, CENTER, Colors::WHITE, availableText);
		}
	}

	if(getMyCreature() != nullptr)
	{
		hoverText = boost::str(boost::format(CGI->generaltexth->tcommands[21]) % getMyCreature()->namePl);
		new CCreaturePic(159, 4, getMyCreature(), false);
		new CLabel(78,  11, FONT_SMALL, CENTER, Colors::WHITE, getMyCreature()->namePl);

		Rect sizes(287, 4, 96, 18);
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[190], CGI->generaltexth->fcommands[0], getMyCreature()->getAttack(false)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[191], CGI->generaltexth->fcommands[1], getMyCreature()->getDefense(false)));
		sizes.y+=21;
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[199], CGI->generaltexth->fcommands[2], getMyCreature()->getMinDamage(false), getMyCreature()->getMaxDamage(false)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[388], CGI->generaltexth->fcommands[3], getMyCreature()->MaxHealth()));
		sizes.y+=21;
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[193], CGI->generaltexth->fcommands[4], getMyCreature()->valOfBonuses(Bonus::STACKS_SPEED)));
		sizes.y+=20;
		values.push_back(std::make_shared<LabeledValue>(sizes, CGI->generaltexth->allTexts[194], CGI->generaltexth->fcommands[5], town->creatureGrowth(level)));
	}
}

const CCreature * CFortScreen::RecruitArea::getMyCreature()
{
	if(!town->creatures.at(level).second.empty()) // built
		return VLC->creh->objects[town->creatures.at(level).second.back()];
	if(!town->town->creatures.at(level).empty()) // there are creatures on this level
		return VLC->creh->objects[town->town->creatures.at(level).front()];
	return nullptr;
}

const CBuilding * CFortScreen::RecruitArea::getMyBuilding()
{
	BuildingID myID = BuildingID(BuildingID::DWELL_FIRST).advance(level);

	if (level == GameConstants::CREATURES_PER_TOWN)
		return town->town->getSpecialBuilding(BuildingSubID::PORTAL_OF_SUMMONING);

	if (!town->town->buildings.count(myID))
		return nullptr;

	const CBuilding * build = town->town->buildings.at(myID);
	while (town->town->buildings.count(myID))
	{
		if (town->hasBuilt(myID))
			build = town->town->buildings.at(myID);
		myID.advance(GameConstants::CREATURES_PER_TOWN);
	}
	return build;
}

void CFortScreen::RecruitArea::hover(bool on)
{
	if(on)
		GH.statusbar->write(hoverText);
	else
		GH.statusbar->clear();
}

void CFortScreen::RecruitArea::creaturesChanged()
{
	if(availableCount)
	{
		std::string availableText = CGI->generaltexth->allTexts[217] + boost::lexical_cast<std::string>(town->creatures[level].first);
		availableCount->setText(availableText);
	}
}

void CFortScreen::RecruitArea::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState)
		LOCPLINT->castleInt->builds->enterDwelling(level);
}

void CFortScreen::RecruitArea::clickRight(tribool down, bool previousState)
{
	clickLeft(down, false); //r-click does same as l-click - opens recr. window
}

CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner,std::string imagem)
	: CStatusbarWindow(BORDERED, imagem)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	window = std::make_shared<CPicture>(owner->town->town->clientInfo.guildWindow, 332, 76);

	resdatabar = std::make_shared<CMinorResDataBar>();
	resdatabar->moveBy(pos.topLeft(), true);

	Rect barRect(7, 556, 737, 18);

	auto statusbarBackground = std::make_shared<CPicture>(*background, barRect, 7, 556, false);
	statusbar = CGStatusBar::create(statusbarBackground);

	exit = std::make_shared<CButton>(Point(748, 556), "TPMAGE1.DEF", CButton::tooltip(CGI->generaltexth->allTexts[593]), [&](){ close(); }, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	static const std::vector<std::vector<Point> > positions =
	{
		{Point(222,445), Point(312,445), Point(402,445), Point(520,445), Point(610,445), Point(700,445)},
		{Point(48,53),   Point(48,147),  Point(48,241),  Point(48,335),  Point(48,429)},
		{Point(570,82),  Point(672,82),  Point(570,157), Point(672,157)},
		{Point(183,42),  Point(183,148), Point(183,253)},
		{Point(491,325), Point(591,325)}
	};

	for(size_t i=0; i<owner->town->town->mageLevel; i++)
	{
		size_t spellCount = owner->town->spellsAtLevel((int)i+1,false); //spell at level with -1 hmmm?
		for(size_t j=0; j<spellCount; j++)
		{
			if(i<owner->town->mageGuildLevel() && owner->town->spells[i].size()>j)
				spells.push_back(std::make_shared<Scroll>(positions[i][j], CGI->spellh->objects[owner->town->spells[i][j]]));
			else
				emptyScrolls.push_back(std::make_shared<CAnimImage>("TPMAGES.DEF", 1, 0, positions[i][j].x, positions[i][j].y));
		}
	}
}

CMageGuildScreen::Scroll::Scroll(Point position, const CSpell *Spell)
	: spell(Spell)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos += position;
	image = std::make_shared<CAnimImage>("SPELLSCR", spell->id);
	pos = image->pos;
}

void CMageGuildScreen::Scroll::clickLeft(tribool down, bool previousState)
{
	if(down)
		LOCPLINT->showInfoDialog(spell->getLevelDescription(0), std::make_shared<CComponent>(CComponent::spell, spell->id));
}

void CMageGuildScreen::Scroll::clickRight(tribool down, bool previousState)
{
	if(down)
		CRClickPopup::createAndPush(spell->getLevelDescription(0), std::make_shared<CComponent>(CComponent::spell, spell->id));
}

void CMageGuildScreen::Scroll::hover(bool on)
{
	if(on)
		GH.statusbar->write(spell->name);
	else
		GH.statusbar->clear();

}

CBlacksmithDialog::CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid):
	CStatusbarWindow(PLAYER_COLORED, "TPSMITH")
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	Rect barRect(8, pos.h - 26, pos.w - 16, 19);

	auto statusbarBackground = std::make_shared<CPicture>(*background, barRect, 8, pos.h - 26, false);
	statusbar = CGStatusBar::create(statusbarBackground);

	animBG = std::make_shared<CPicture>("TPSMITBK", 64, 50);
	animBG->needRefresh = true;

	const CCreature * creature = CGI->creh->objects[creMachineID];
	anim = std::make_shared<CCreatureAnim>(64, 50, creature->animDefName);
	anim->clipRect(113,125,200,150);

	title = std::make_shared<CLabel>(165, 28, FONT_BIG, CENTER, Colors::YELLOW,
	            boost::str(boost::format(CGI->generaltexth->allTexts[274]) % creature->nameSing));
	costText = std::make_shared<CLabel>(165, 218, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->jktexts[43]);
	costValue = std::make_shared<CLabel>(165, 290, FONT_MEDIUM, CENTER, Colors::WHITE,
	                boost::lexical_cast<std::string>(aid.toArtifact(CGI->artifacts())->getPrice()));

	std::string text = boost::str(boost::format(CGI->generaltexth->allTexts[595]) % creature->nameSing);
	buy = std::make_shared<CButton>(Point(42, 312), "IBUY30.DEF", CButton::tooltip(text), [&](){ close(); }, SDLK_RETURN);

	text = boost::str(boost::format(CGI->generaltexth->allTexts[596]) % creature->nameSing);
	cancel = std::make_shared<CButton>(Point(224, 312), "ICANCEL.DEF", CButton::tooltip(text), [&](){ close(); }, SDLK_ESCAPE);

	if(possible)
		buy->addCallback([=](){ LOCPLINT->cb->buyArtifact(LOCPLINT->cb->getHero(hid),aid); });
	else
		buy->block(true);

	costIcon = std::make_shared<CAnimImage>("RESOURCE", Res::GOLD, 0, 148, 244);
}
