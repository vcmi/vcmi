#include "StdInc.h"
#include "CCastleInterface.h"

#include "CAdvmapInterface.h"
#include "CHeroWindow.h"
#include "CTradeWindow.h"
#include "GUIClasses.h"

#include "../CBitmapHandler.h"
#include "../CDefHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../windows/InfoWindows.h"
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
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

/*
 * CCastleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const CBuilding * CBuildingRect::getBuilding()
{
	if (!str->building)
		return nullptr;

	if (str->hiddenUpgrade) // hidden upgrades, e.g. hordes - return base (dwelling for hordes)
		return town->town->buildings.at(str->building->getBase());

	return str->building;
}

CBuildingRect::CBuildingRect(CCastleBuildings * Par, const CGTownInstance *Town, const CStructure *Str)
	:CShowableAnim(0, 0, Str->defName, CShowableAnim::BASE | CShowableAnim::USE_RLE),
	parent(Par),
	town(Town),
	str(Str),
	stateCounter(80)
{
	recActions = ACTIVATE | DEACTIVATE | DISPOSE | SHARE_POS;
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += str->pos.x;
	pos.y += str->pos.y;

	if (!str->borderName.empty())
		border = BitmapHandler::loadBitmap(str->borderName, true);
	else
		border = nullptr;

	if (!str->areaName.empty())
		area = BitmapHandler::loadBitmap(str->areaName);
	else
		area = nullptr;
}

CBuildingRect::~CBuildingRect()
{
	SDL_FreeSurface(border);
	SDL_FreeSurface(area);
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
	if( previousState && !down && area && (parent->selectedBuilding==this) && getBuilding() )
		if (!CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) ) //inside building image
			parent->buildingClicked(getBuilding()->bid);
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
			                            new CComponent(CComponent::building, bld->town->faction->index, bld->bid));
		}
		else
		{
			int level = ( bid - BuildingID::DWELL_FIRST ) % GameConstants::CREATURES_PER_TOWN;
			GH.pushInt(new CDwellingInfoBox(parent->pos.x+parent->pos.w/2, parent->pos.y+parent->pos.h/2, town, level));
		}
	}
}

SDL_Color multiplyColors (const SDL_Color &b, const SDL_Color &a, double f)
{
	SDL_Color ret;
	ret.r = a.r*f + b.r*(1-f);
	ret.g = a.g*f + b.g*(1-f);
	ret.b = a.b*f + b.b*(1-f);
	return ret;
}

void CBuildingRect::show(SDL_Surface * to)
{
	const ui32 stageDelay = 16;

	const ui32 S1_TRANSP  = 16; //0.5 sec building appear 0->100 transparency
	const ui32 S2_WHITE_B = 32; //0.5 sec border glows from white to yellow
	const ui32 S3_YELLOW_B= 48; //0.5 sec border glows from yellow to normal
	const ui32 BUILDED    = 80; //  1 sec delay, nothing happens

	if (stateCounter < S1_TRANSP)
	{
		setAlpha(255*stateCounter/stageDelay);
		CShowableAnim::show(to);
	}
	else
	{
		setAlpha(255);
		CShowableAnim::show(to);
	}

	if (border && stateCounter > S1_TRANSP)
	{
		if (stateCounter == BUILDED)
		{
			if (parent->selectedBuilding == this)
				blitAtLoc(border,0,0,to);
			return;
		}
		if (border->format->palette != nullptr)
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
			blitAtLoc(border,0,0,to);
			SDL_SetColors(border, &oldColor, colorID, 1);
		}
	}
	if (stateCounter < BUILDED)
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
			return CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures.at(creaID)->namePl;
		}
		else
		{
			logGlobal->warnStream() << "Problem: dwelling with id " << bid << " offers no creatures!";
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
				GH.statusbar->setText(getSubtitle());
			}
		}
	}
}

CDwellingInfoBox::CDwellingInfoBox(int centerX, int centerY, const CGTownInstance *Town, int level):
CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "CRTOINFO", Point(centerX, centerY))
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	const CCreature * creature = CGI->creh->creatures.at(Town->creatures.at(level).second.back());

	title = new CLabel(80, 30, FONT_SMALL, CENTER, Colors::WHITE, creature->namePl);
	animation =  new CCreaturePic(30, 44, creature, true, true);

	std::string text = boost::lexical_cast<std::string>(Town->creatures.at(level).first);
	available = new CLabel(80,190, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[217] + text);
	costPerTroop = new CLabel(80, 227, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->allTexts[346]);

	for(int i = 0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if(creature->cost[i])
		{
			resPicture.push_back(new CAnimImage("RESOURCE", i, 0, 0, 0));
			resAmount.push_back(new CLabel(0,0, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(creature->cost[i])));
		}
	}

	int posY = 238;
	int posX = pos.w/2 - resAmount.size() * 25 + 5;
	for (size_t i=0; i<resAmount.size(); i++)
	{
		resPicture[i]->moveBy(Point(posX, posY));
		resAmount[i]->moveBy(Point(posX+16, posY+43));
		posX += 50;
	}
}

void CHeroGSlot::hover (bool on)
{
	if(!on)
	{
		GH.statusbar->clear();
		return;
	}
	CHeroGSlot *other = upg  ?  owner->garrisonedHero :  owner->visitingHero;
	std::string temp;
	if(hero)
	{
		if(selection)//view NNN
		{
			temp = CGI->generaltexth->tcommands[4];
			boost::algorithm::replace_first(temp,"%s",hero->name);
		}
		else if(other->hero && other->selection)//exchange
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
		if(other->selection && other->hero) //move NNNN
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
		GH.statusbar->setText(temp);
}

void CHeroGSlot::clickLeft(tribool down, bool previousState)
{
	CHeroGSlot *other = upg  ?  owner->garrisonedHero :  owner->visitingHero;
	if(!down)
	{
		owner->garr->setSplittingMode(false);
		owner->garr->selectSlot(nullptr);

		if(hero && selection)
		{
			setHighlight(false);
			LOCPLINT->openHeroWindow(hero);
		}
		else if(other->hero && other->selection)
		{
			bool allow = true;
			if(upg) //moving hero out of town - check if it is allowed
			{
				if(!hero && LOCPLINT->cb->howManyHeroes(false) >= VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)
				{
					std::string tmp = CGI->generaltexth->allTexts[18]; //You already have %d adventuring heroes under your command.
					boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes(false)));
					LOCPLINT->showInfoDialog(tmp,std::vector<CComponent*>(), soundBase::sound_todo);
					allow = false;
				}
				else if(!other->hero->stacksCount()) //hero has no creatures - strange, but if we have appropriate error message...
				{
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[19],std::vector<CComponent*>(), soundBase::sound_todo); //This hero has no creatures.  A hero must have creatures before he can brave the dangers of the countryside.
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
			showAll(screen2);
		}
		hover(false);hover(true); //refresh statusbar
	}
}

void CHeroGSlot::clickRight(tribool down, bool previousState)
{
	if(down && hero)
	{
		GH.pushInt(new CInfoBoxPopup(Point(pos.x + 175, pos.y + 100), hero));
	}
}

void CHeroGSlot::deactivate()
{
	vstd::clear_pointer(selection);
	CIntObject::deactivate();
}

CHeroGSlot::CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, HeroSlots * Owner)
{
	owner = Owner;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;
	upg = updown;
	selection = nullptr;
	image = nullptr;
	set(h);

	addUsedEvents(LCLICK | RCLICK | HOVER);
}

CHeroGSlot::~CHeroGSlot()
{
}

void CHeroGSlot::setHighlight( bool on )
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	vstd::clear_pointer(selection);
	if (on)
		selection = new CAnimImage("TWCRPORT", 1, 0);

	if(owner->garrisonedHero->hero && owner->visitingHero->hero) //two heroes in town
	{
		for(auto & elem : owner->garr->splitButtons) //splitting enabled when slot higlighted
			elem->block(!on);
	}
}

void CHeroGSlot::set(const CGHeroInstance *newHero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (image)
		delete image;
	hero = newHero;
	if (newHero)
		image = new CAnimImage("PortraitsLarge", newHero->portrait, 0, 0, 0);
	else if(!upg && owner->showEmpty) //up garrison
		image = new CAnimImage("CREST58", LOCPLINT->castleInt->town->getOwner().getNum(), 0, 0, 0);
	else
		image = nullptr;
}

template <class ptr>
class SORTHELP
{
public:
	bool operator ()
		(const ptr *a ,
		 const ptr *b)
	{
		return (*a)<(*b);
	}
};

SORTHELP<CBuildingRect> buildSorter;
SORTHELP<CStructure> structSorter;

CCastleBuildings::CCastleBuildings(const CGTownInstance* Town):
	town(Town),
	selectedBuilding(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	background = new CPicture(town->town->clientInfo.townBackground);
	pos.w = background->pos.w;
	pos.h = background->pos.h;

	recreate();
}

void CCastleBuildings::recreate()
{
	selectedBuilding = nullptr;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	//clear existing buildings
	for(auto build : buildings)
		delete build;
	buildings.clear();
	groups.clear();

	//Generate buildings list

	auto buildingsCopy = town->builtBuildings;// a bit modified copy of built buildings

	if(vstd::contains(town->builtBuildings, BuildingID::SHIPYARD))
	{
		auto bayPos = town->bestLocation();
		if(!bayPos.valid())
			logGlobal->warnStream() << "Shipyard in non-coastal town!";
		std::vector <const CGObjectInstance *> vobjs = LOCPLINT->cb->getVisitableObjs(bayPos, false);
		//there is visitable obj at shipyard output tile and it's a boat or hero (on boat)
		if(!vobjs.empty() && (vobjs.front()->ID == Obj::BOAT || vobjs.front()->ID == Obj::HERO))
		{
			buildingsCopy.insert(BuildingID::SHIP);
		}
	}

	for(const CStructure * structure : town->town->clientInfo.structures)
	{
		if (!structure->building)
		{
			buildings.push_back(new CBuildingRect(this, town, structure));
			continue;
		}
		if (vstd::contains(buildingsCopy, structure->building->bid))
		{
			groups[structure->building->getBase()].push_back(structure);
		}
	}

	for(auto & entry : groups)
	{
		const CBuilding * build = town->town->buildings.at(entry.first);

		const CStructure * toAdd = *boost::max_element(entry.second, [=](const CStructure * a, const CStructure * b)
		{
			return build->getDistance(a->building->bid)
			     < build->getDistance(b->building->bid);
		});

		buildings.push_back(new CBuildingRect(this, town, toAdd));
	}
	boost::sort(buildings, [] (const CBuildingRect * a, const CBuildingRect * b)
	{
		return *a < *b;
	});
}

CCastleBuildings::~CCastleBuildings()
{
}

void CCastleBuildings::addBuilding(BuildingID building)
{
	//FIXME: implement faster method without complete recreation of town
	BuildingID base = town->town->buildings.at(building)->getBase();

	recreate();

	auto & structures = groups.at(base);

	for(CBuildingRect * rect : buildings)
	{
		if (vstd::contains(structures, rect->str))
		{
			//reset animation
			if (structures.size() == 1)
				rect->stateCounter = 0; // transparency -> fully visible stage
			else
				rect->stateCounter = 16; // already in fully visible stage
			break;
		}
	}
}

void CCastleBuildings::removeBuilding(BuildingID building)
{
	//FIXME: implement faster method without complete recreation of town
	recreate();
}

void CCastleBuildings::show(SDL_Surface * to)
{
	CIntObject::show(to);
	for(CBuildingRect * str : buildings)
		str->show(to);
}

void CCastleBuildings::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	for(CBuildingRect * str : buildings)
		str->showAll(to);
}

const CGHeroInstance* CCastleBuildings::getHero()
{
	if (town->visitingHero)
		return town->visitingHero;
	if (town->garrisonHero)
		return town->garrisonHero;
	return nullptr;
}

void CCastleBuildings::buildingClicked(BuildingID building)
{
	logGlobal->traceStream()<<"You've clicked on "<<building;
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
				break;

		case BuildingID::FORT:
		case BuildingID::CITADEL:
		case BuildingID::CASTLE:
				GH.pushInt(new CFortScreen(town));
				break;

		case BuildingID::VILLAGE_HALL:
		case BuildingID::CITY_HALL:
		case BuildingID::TOWN_HALL:
		case BuildingID::CAPITOL:
				enterTownHall();
				break;

		case BuildingID::MARKETPLACE:
				GH.pushInt(new CMarketplaceWindow(town, town->visitingHero));
				break;

		case BuildingID::BLACKSMITH:
				enterBlacksmith(town->town->warMachine);
				break;

		case BuildingID::SPECIAL_1:
				switch(town->subID)
				{
				case ETownType::RAMPART://Mystic Pond
						enterFountain(building);
						break;

				case ETownType::TOWER:
				case ETownType::DUNGEON://Artifact Merchant
				case ETownType::CONFLUX:
						if(town->visitingHero)
							GH.pushInt(new CMarketplaceWindow(town, town->visitingHero, EMarketMode::RESOURCE_ARTIFACT));
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				default:
					enterBuilding(building);
					break;
				}
				break;

		case BuildingID::SHIP:
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[51]); //Cannot build another boat
				break;

		case BuildingID::SPECIAL_2:
				switch(town->subID)
				{
				case ETownType::RAMPART: //Fountain of Fortune
						enterFountain(building);
						break;

				case ETownType::STRONGHOLD: //Freelancer's Guild
						if(getHero())
							GH.pushInt(new CMarketplaceWindow(town, getHero(), EMarketMode::CREATURE_RESOURCE));
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				case ETownType::CONFLUX: //Magic University
						if (getHero())
							GH.pushInt(new CUniversityWindow(getHero(), town));
						else
							enterBuilding(building);
						break;

				default:
						enterBuilding(building);
						break;
				}
				break;

		case BuildingID::SPECIAL_3:
				switch(town->subID)
				{
				case ETownType::CASTLE: //Brotherhood of sword
						LOCPLINT->showTavernWindow(town);
						break;

				case ETownType::INFERNO: //Castle Gate
						enterCastleGate();
						break;

				case ETownType::NECROPOLIS: //Skeleton Transformer
						GH.pushInt( new CTransformerWindow(getHero(), town) );
						break;

				case ETownType::DUNGEON: //Portal of Summoning
						if (town->creatures[GameConstants::CREATURES_PER_TOWN].second.empty())//No creatures
							LOCPLINT->showInfoDialog(CGI->generaltexth->tcommands[30]);
						else
							enterDwelling(GameConstants::CREATURES_PER_TOWN);
						break;

				case ETownType::STRONGHOLD: //Ballista Yard
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
	int price = CGI->arth->artifacts[artifactID]->price;
	bool possible = LOCPLINT->cb->getResourceAmount(Res::GOLD) >= price && !hero->hasArt(artifactID);
	GH.pushInt(new CBlacksmithDialog(possible, CArtHandler::machineIDToCreature(artifactID), artifactID, hero->id));
}

void CCastleBuildings::enterBuilding(BuildingID building)
{
	std::vector<CComponent*> comps(1, new CComponent(CComponent::building, town->subID, building));

	LOCPLINT->showInfoDialog(
		town->town->buildings.find(building)->second->Description(),comps);
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
			t->hasBuilt(BuildingID::CASTLE_GATE, ETownType::INFERNO))
		{
			availableTowns.push_back(t->id.getNum());//add to the list
		}
	}
	auto  titlePic = new CPicture (LOCPLINT->castleInt->bicons->ourImages[BuildingID::CASTLE_GATE].bitmap, 0,0, false);//will be deleted by selection window
	GH.pushInt (new CObjectListWindow(availableTowns, titlePic, CGI->generaltexth->jktexts[40],
	    CGI->generaltexth->jktexts[41], std::bind (&CCastleInterface::castleTeleport, LOCPLINT->castleInt, _1)));
}

void CCastleBuildings::enterDwelling(int level)
{
	assert(level >= 0 && level < town->creatures.size());
	auto recruitCb = [=](CreatureID id, int count){ LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level); };
	GH.pushInt(new CRecruitmentWindow(town, level, town, recruitCb, -87));
}

void CCastleBuildings::enterFountain(BuildingID building)
{
	std::vector<CComponent*> comps(1, new CComponent(CComponent::building,town->subID,building));

	std::string descr = town->town->buildings.find(building)->second->Description();

	if ( building == BuildingID::FOUNTAIN_OF_FORTUNE)
		descr += "\n\n"+town->town->buildings.find(BuildingID::MYSTIC_POND)->second->Description();

	if (town->bonusValue.first == 0)//fountain was builded this week
		descr += "\n\n"+ CGI->generaltexth->allTexts[677];
	else//fountain produced something;
	{
		descr+= "\n\n"+ CGI->generaltexth->allTexts[678];
		boost::algorithm::replace_first(descr,"%s",CGI->generaltexth->restypes[town->bonusValue.first]);
		boost::algorithm::replace_first(descr,"%d",boost::lexical_cast<std::string>(town->bonusValue.second));
	}
	LOCPLINT->showInfoDialog(descr, comps);
}

void CCastleBuildings::enterMagesGuild()
{
	const CGHeroInstance *hero = getHero();

	if(hero && !hero->hasSpellbook()) //hero doesn't have spellbok
	{
		if(LOCPLINT->cb->getResourceAmount(Res::GOLD) < 500) //not enough gold to buy spellbook
		{
			openMagesGuild();
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[213]);
		}
		else
		{
			CFunctionList<void()> onYes = [this]{ openMagesGuild(); };
			CFunctionList<void()> onNo = onYes;
			onYes += [hero]{ LOCPLINT->cb->buyArtifact(hero, ArtifactID::SPELLBOOK); };
			std::vector<CComponent*> components(1, new CComponent(CComponent::artifact,ArtifactID::SPELLBOOK,0));

			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[214], onYes, onNo, true, components);
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
										[&]{ LOCPLINT->cb->buildBuilding(town, BuildingID::GRAIL); },
										[&]{ openTownHall(); },
										true);
		}
		else
		{
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[673]);
			dynamic_cast<CInfoWindow*>(GH.topInt())->buttons[0]->addCallback(std::bind(&CCastleBuildings::openTownHall, this));
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
	GH.pushInt(new CMageGuildScreen(LOCPLINT->castleInt,mageGuildBackground));
}

void CCastleBuildings::openTownHall()
{
	GH.pushInt(new CHallInterface(town));
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, const CGTownInstance * from):
	CWindowObject(PLAYER_COLORED | BORDERED),
	hall(nullptr),
	fort(nullptr),
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	LOCPLINT->castleInt = this;
	addUsedEvents(KEYBOARD);

	builds = new CCastleBuildings(town);
	panel = new CPicture("TOWNSCRN", 0, builds->pos.h);
	panel->colorize(LOCPLINT->playerID);
	pos.w = panel->pos.w;
	pos.h = builds->pos.h + panel->pos.h;
	center();
	updateShadow();

	garr = new CGarrisonInt(305, 387, 4, Point(0,96), panel->bg, Point(62,374), town->getUpperArmy(), town->visitingHero);
	heroes = new HeroSlots(town, Point(241, 387), Point(241, 483), garr, true);
	title = new CLabel(85, 387, FONT_MEDIUM, TOPLEFT, Colors::WHITE, town->name);
	income = new CLabel(195, 443, FONT_SMALL, CENTER);
	icon = new CAnimImage("ITPT", 0, 0, 15, 387);

	exit = new CButton(Point(744, 544), "TSBTNS", CButton::tooltip(CGI->generaltexth->tcommands[8]), [&]{close();}, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	exit->setImageOrder(4, 5, 6, 7);

	split = new CButton(Point(744, 382), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), [&]{garr->splitClick();});
	split->addCallback(std::bind(&HeroSlots::splitClicked, heroes));
	garr->addSplitBtn(split);

	Rect barRect(9, 182, 732, 18);
	statusbar = new CGStatusBar(new CPicture(*panel, barRect, 9, 555, false));
	resdatabar = new CResDataBar("ARESBAR", 3, 575, 32, 2, 85, 85);

	townlist = new CTownList(3, Point(744, 414), "IAM014", "IAM015");
	if (from)
		townlist->select(from);

	townlist->select(town); //this will scroll list to select current town
	townlist->onSelect = std::bind(&CCastleInterface::townChange, this);

	recreateIcons();
	CCS->musich->playMusic(town->town->clientInfo.musicTheme, true);

	bicons = CDefHandler::giveDefEss(town->town->clientInfo.buildingsIcons);
}

CCastleInterface::~CCastleInterface()
{
	LOCPLINT->castleInt = nullptr;
	delete bicons;
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
	LOCPLINT->cb->teleportHero(town->visitingHero, dest);
	LOCPLINT->eraseCurrentPathOf(town->visitingHero, false);
}

void CCastleInterface::townChange()
{
	const CGTownInstance * dest = LOCPLINT->towns[townlist->getSelectedIndex()];
	const CGTownInstance * town = this->town;// "this" is going to be deleted
	if ( dest == town )
		return;
	close();
	GH.pushInt(new CCastleInterface(dest, town));
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
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	delete fort;
	delete hall;

	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	icon->setFrame(iconIndex);
	TResources townIncome = town->dailyIncome();
	income->setText(boost::lexical_cast<std::string>(townIncome[Res::GOLD]));

	hall = new CTownInfo( 80, 413, town, true);
	fort = new CTownInfo(122, 413, town, false);

	for (auto & elem : creainfo)
		delete elem;
	creainfo.clear();

	for (size_t i=0; i<4; i++)
		creainfo.push_back(new CCreaInfo(Point(14+55*i, 459), town, i));

	for (size_t i=0; i<4; i++)
		creainfo.push_back(new CCreaInfo(Point(14+55*i, 507), town, i+4));
}

CCreaInfo::CCreaInfo(Point position, const CGTownInstance *Town, int Level, bool compact, bool ShowAvailable):
	town(Town),
	level(Level),
	showAvailable(ShowAvailable)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos += position;

	if ( town->creatures.size() <= level || town->creatures[level].second.empty())
	{
		level = -1;
		label = nullptr;
		picture = nullptr;
		return;//No creature
	}
	addUsedEvents(LCLICK | RCLICK | HOVER);

	ui32 creatureID = town->creatures[level].second.back();
	creature = CGI->creh->creatures[creatureID];

	picture = new CAnimImage("CPRSMALL", creature->iconIndex, 0, 8, 0);

	std::string value;
	if (showAvailable)
		value = boost::lexical_cast<std::string>(town->creatures[level].first);
	else
		value = boost::lexical_cast<std::string>(town->creatureGrowth(level));

	if (compact)
	{
		label = new CLabel(40, 32, FONT_TINY, BOTTOMRIGHT, Colors::WHITE, value);
		pos.x += 8;
		pos.w = 32;
		pos.h = 32;
	}
	else
	{
		label = new CLabel(24, 40, FONT_SMALL, CENTER, Colors::WHITE, value);
		pos.w = 48;
		pos.h = 48;
	}
}

void CCreaInfo::update()
{
	if (label)
	{
		std::string value;
		if (showAvailable)
			value = boost::lexical_cast<std::string>(town->creatures[level].first);
		else
			value = boost::lexical_cast<std::string>(town->creatureGrowth(level));

		if (value != label->text)
			label->setText(value);
	}
}

void CCreaInfo::hover(bool on)
{
	std::string message = CGI->generaltexth->allTexts[588];
	boost::algorithm::replace_first(message,"%s",creature->namePl);

	if(on)
	{
		GH.statusbar->setText(message);
	}
	else if (message == GH.statusbar->getText())
		GH.statusbar->clear();
}

void CCreaInfo::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		int offset = LOCPLINT->castleInt? (-87) : 0;
		auto recruitCb = [=](CreatureID id, int count) { LOCPLINT->cb->recruitCreatures(town, town->getUpperArmy(), id, count, level); };
		GH.pushInt(new CRecruitmentWindow(town, level, town, recruitCb, offset));
	}
}

int CCreaInfo::AddToString(std::string from, std::string & to, int numb)
{
	if (numb == 0)
		return 0;
	boost::algorithm::replace_first(from,"%+d", (numb > 0 ? "+" : "")+boost::lexical_cast<std::string>(numb)); //negative values don't need "+"
	to+="\n"+from;
	return numb;
}

std::string CCreaInfo::genGrowthText()
{
	GrowthInfo gi = town->getGrowthInfo(level);
	std::string descr = boost::str(boost::format(CGI->generaltexth->allTexts[589]) % creature->nameSing % gi.totalGrowth());

	for(const GrowthInfo::Entry &entry : gi.entries)
	{
		descr +="\n" + entry.description;
	}

	return descr;
}

void CCreaInfo::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if (showAvailable)
			GH.pushInt(new CDwellingInfoBox(screen->w/2, screen->h/2, town, level));
		else
			CRClickPopup::createAndPush(genGrowthText(), new CComponent(CComponent::creature, creature->idNumber));
	}
}

CTownInfo::CTownInfo(int posX, int posY, const CGTownInstance* Town, bool townHall):
	town(Town),
	building(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(RCLICK | HOVER);
	pos.x += posX;
	pos.y += posY;
	int buildID;

	if (townHall)
	{
		buildID = 10 + town->hallLevel();
		picture = new CAnimImage("ITMTL.DEF", town->hallLevel());
	}
	else
	{
		buildID = 6 + town->fortLevel();
		if (buildID == 6)
			return;
		picture = new CAnimImage("ITMCL.DEF", town->fortLevel()-1);
	}
	building = town->town->buildings.at(BuildingID(buildID));
	pos = picture->pos;
}

void CTownInfo::hover(bool on)
{
	if(on)
	{
		if ( building )
			GH.statusbar->setText(building->Name());
	}
	else
		GH.statusbar->clear();
}

void CTownInfo::clickRight(tribool down, bool previousState)
{
	if(down && building)
		CRClickPopup::createAndPush(CInfoWindow::genText(building->Name(), building->Description()),
		                            new CComponent(CComponent::building, building->town->faction->index, building->bid));

}

void CCastleInterface::keyPressed( const SDL_KeyboardEvent & key )
{
	if(key.state != SDL_PRESSED) return;

	switch(key.keysym.sym)
	{
#if 0 // code that can be used to fix blit order in towns using +/- keys. Quite ugly but works
	case SDLK_KP_PLUS :
		if (builds->selectedBuilding)
		{
			OBJ_CONSTRUCTION_CAPTURING_ALL;
			CStructure * str = const_cast<CStructure *>(builds->selectedBuilding->str);
			str->pos.z++;
			delete builds;
			builds = new CCastleBuildings(town);

			for(const CStructure * str : town->town->clientInfo.structures)
			{
				if (str->building)
					logGlobal->errorStream() << int(str->building->bid) << " -> " << int(str->pos.z);
			}
		}
		break;
	case SDLK_KP_MINUS:
		if (builds->selectedBuilding)
		{
			OBJ_CONSTRUCTION_CAPTURING_ALL;
			CStructure * str = const_cast<CStructure *>(builds->selectedBuilding->str);
			str->pos.z--;
			delete builds;
			builds = new CCastleBuildings(town);

			for(const CStructure * str : town->town->clientInfo.structures)
			{
				if (str->building)
					logGlobal->errorStream() << int(str->building->bid) << " -> " << int(str->pos.z);
			}

		}
		break;
#endif
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

HeroSlots::HeroSlots(const CGTownInstance * Town, Point garrPos, Point visitPos, CGarrisonInt *Garrison, bool ShowEmpty):
	showEmpty(ShowEmpty),
	town(Town),
	garr(Garrison)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	garrisonedHero = new CHeroGSlot(garrPos.x, garrPos.y, 0, town->garrisonHero, this);
	visitingHero = new CHeroGSlot(visitPos.x, visitPos.y, 1, town->visitingHero, this);
}

void HeroSlots::update()
{
	garrisonedHero->set(town->garrisonHero);
	visitingHero->set(town->visitingHero);
}

void HeroSlots::splitClicked()
{
	if(!!town->visitingHero && town->garrisonHero && (visitingHero->selection || garrisonedHero->selection))
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
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[275], std::vector<CComponent*>(), soundBase::sound_todo);
			return;
		}
	}
	LOCPLINT->cb->swapGarrisonHero(town);
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
		GH.statusbar->setText(toPrint);
	}
	else
		GH.statusbar->clear();
}

void CHallInterface::CBuildingBox::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
		GH.pushInt(new CBuildWindow(town,building,state,0));
}

void CHallInterface::CBuildingBox::clickRight(tribool down, bool previousState)
{
	if(down)
		GH.pushInt(new CBuildWindow(town,building,state,1));
}

CHallInterface::CBuildingBox::CBuildingBox(int x, int y, const CGTownInstance * Town, const CBuilding * Building):
	town(Town),
	building(Building)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos.x += x;
	pos.y += y;
	pos.w = 154;
	pos.h = 92;

	state = LOCPLINT->cb->canBuildStructure(town,building->bid);
	assert(state < EBuildingState::BUILDING_ERROR);
	static int panelIndex[10] = { 3,  3,  3, 0, 0, 2, 2,  1, 2, 2};
	static int  iconIndex[10] = {-1, -1, -1, 0, 0, 1, 2, -1, 1, 1};

	picture = new CAnimImage(town->town->clientInfo.buildingsIcons, building->bid, 0, 2, 2);
	panel = new CAnimImage("TPTHBAR", panelIndex[state], 0,   1, 73);
	if ( iconIndex[state] >=0 )
		icon  = new CAnimImage("TPTHCHK",  iconIndex[state], 0, 136, 56);
	label = new CLabel(75, 81, FONT_SMALL, CENTER, Colors::WHITE, building->Name());
}

CHallInterface::CHallInterface(const CGTownInstance *Town):
	CWindowObject(PLAYER_COLORED | BORDERED, Town->town->clientInfo.hallBackground),
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	Rect barRect(5, 556, 740, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 5, 556, false));

	title = new CLabel(399, 12, FONT_MEDIUM, CENTER, Colors::WHITE, town->town->buildings.at(BuildingID(town->hallLevel()+BuildingID::VILLAGE_HALL))->Name());
	exit = new CButton(Point(748, 556), "TPMAGE1.DEF", CButton::tooltip(CGI->generaltexth->hcommands[8]), [&]{close();}, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	auto & boxList = town->town->clientInfo.hallSlots;
	boxes.resize(boxList.size());
	for(size_t row=0; row<boxList.size(); row++) //for each row
	{
		for(size_t col=0; col<boxList[row].size(); col++) //for each box
		{
			const CBuilding *building = nullptr;
			for(auto & elem : boxList[row][col])//we are looking for the first not build structure
			{
				auto buildingID = elem;
				building = town->town->buildings.at(buildingID);

				if(!vstd::contains(town->builtBuildings,buildingID))
					break;
			}
			int posX = pos.w/2 - boxList[row].size()*154/2 - (boxList[row].size()-1)*20 + 194*col,
			    posY = 35 + 104*row;

			if (building)
				boxes[row].push_back(new CBuildingBox(posX, posY, town, building));
		}
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
			ret += "\n" + town->genBuildingRequirements(building->bid, false).toString(toStr);
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

CBuildWindow::CBuildWindow(const CGTownInstance *Town, const CBuilding * Building, int state, bool rightClick):
	CWindowObject(PLAYER_COLORED | (rightClick ? RCLICK_POPUP : 0), "TPUBUILD"),
	town(Town),
	building(Building)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	new CAnimImage(town->town->clientInfo.buildingsIcons, building->bid, 0, 125, 50);
	new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	new CLabel(197, 30, FONT_MEDIUM, CENTER, Colors::WHITE,
	            boost::str(boost::format(CGI->generaltexth->hcommands[7]) % building->Name()));
	new CTextBox(building->Description(), Rect(33, 135, 329, 67), 0, FONT_MEDIUM, CENTER);
	new CTextBox(getTextForState(state),  Rect(33, 216, 329, 67), 0, FONT_SMALL,  CENTER);

	//Create components for all required resources
	std::vector<CComponent *> components;

	for(int i = 0; i<GameConstants::RESOURCE_QUANTITY; i++)
	{
		if(building->resources[i])
		{
			components.push_back(new CComponent(CComponent::resource, i, building->resources[i], CComponent::small));
		}
	}

	new CComponentBox(components, Rect(25, 300, pos.w - 50, 130));

	if(!rightClick)
	{	//normal window
		std::string tooltipYes = boost::str(boost::format(CGI->generaltexth->allTexts[595]) % building->Name());
		std::string tooltipNo  = boost::str(boost::format(CGI->generaltexth->allTexts[596]) % building->Name());

		buy = new CButton(Point(45, 446), "IBUY30", CButton::tooltip(tooltipYes), [&]{ buyFunc(); }, SDLK_RETURN);
		buy->borderColor = Colors::METALLIC_GOLD;

		cancel = new CButton(Point(290, 445), "ICANCEL", CButton::tooltip(tooltipNo), [&] { close();}, SDLK_ESCAPE);
		cancel->borderColor = Colors::METALLIC_GOLD;
		buy->block(state!=7 || LOCPLINT->playerID != town->tempOwner);
	}
}


std::string CFortScreen::getBgName(const CGTownInstance *town)
{
	ui32 fortSize = town->creatures.size();
	if (fortSize > GameConstants::CREATURES_PER_TOWN && town->creatures.back().second.empty())
		fortSize--;

	if (fortSize == GameConstants::CREATURES_PER_TOWN)
		return "TPCASTL7";
	else
		return "TPCASTL8";
}

CFortScreen::CFortScreen(const CGTownInstance * town):
	CWindowObject(PLAYER_COLORED | BORDERED, getBgName(town))
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ui32 fortSize = town->creatures.size();
	if (fortSize > GameConstants::CREATURES_PER_TOWN && town->creatures.back().second.empty())
		fortSize--;

	const CBuilding *fortBuilding = town->town->buildings.at(BuildingID(town->fortLevel()+6));
	title = new CLabel(400, 12, FONT_BIG, CENTER, Colors::WHITE, fortBuilding->Name());

	std::string text = boost::str(boost::format(CGI->generaltexth->fcommands[6]) % fortBuilding->Name());
	exit = new CButton(Point(748, 556), "TPMAGE1", CButton::tooltip(text), [&]{ close(); }, SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	std::vector<Point> positions =
	{
		Point(10,  22), Point(404, 22),
		Point(10, 155), Point(404,155),
		Point(10, 288), Point(404,288)
	};

	if (fortSize == GameConstants::CREATURES_PER_TOWN)
		positions.push_back(Point(206,421));
	else
	{
		positions.push_back(Point(10, 421));
		positions.push_back(Point(404,421));
	}		

	for (ui32 i=0; i<fortSize; i++)
	{
		BuildingID buildingID;
		if (fortSize == GameConstants::CREATURES_PER_TOWN)
		{
			if (vstd::contains(town->builtBuildings, BuildingID::DWELL_UP_FIRST+i))
				buildingID = BuildingID(BuildingID::DWELL_UP_FIRST+i);
			else
				buildingID = BuildingID(BuildingID::DWELL_FIRST+i);
		}
		else
			buildingID = BuildingID::SPECIAL_3;
		recAreas.push_back(new RecruitArea(positions[i].x, positions[i].y, town, i));
	}

	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;

	Rect barRect(4, 554, 740, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 4, 554, false));
}

void CFortScreen::creaturesChanged()
{
	for (auto & elem : recAreas)
		elem->creaturesChanged();
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int min, int max)
{
	pos.x+=size.x;
	pos.y+=size.y;
	pos.w = size.w;
	pos.h = size.h;
	init(name, descr, min, max);
}

LabeledValue::LabeledValue(Rect size, std::string name, std::string descr, int val)
{
	pos.x+=size.x;
	pos.y+=size.y;
	pos.w = size.w;
	pos.h = size.h;
	init(name, descr, val, val);
}

void LabeledValue::init(std::string nameText, std::string descr, int min, int max)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(HOVER);
	hoverText = descr;
	std::string valueText;
	if (min && max)
	{
		valueText = boost::lexical_cast<std::string>(min);
		if (min != max)
			valueText += '-' + boost::lexical_cast<std::string>(max);
	}
	name =  new CLabel(3, 0, FONT_SMALL, TOPLEFT, Colors::WHITE, nameText);
	value = new CLabel(pos.w-3, pos.h-2, FONT_SMALL, BOTTOMRIGHT, Colors::WHITE, valueText);
}

void LabeledValue::hover(bool on)
{
	if(on)
		GH.statusbar->setText(hoverText);
	else
	{
		GH.statusbar->clear();
		parent->hovered = false;
	}
}

const CCreature * CFortScreen::RecruitArea::getMyCreature()
{
	if (!town->creatures.at(level).second.empty()) // built
		return VLC->creh->creatures[town->creatures.at(level).second.back()];
	if (!town->town->creatures.at(level).empty()) // there are creatures on this level
		return VLC->creh->creatures[town->town->creatures.at(level).front()];
	return nullptr;
}

const CBuilding * CFortScreen::RecruitArea::getMyBuilding()
{
	BuildingID myID = BuildingID(BuildingID::DWELL_FIRST).advance(level);

	if (level == GameConstants::CREATURES_PER_TOWN)
		return town->town->buildings.at(BuildingID::PORTAL_OF_SUMMON);

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

CFortScreen::RecruitArea::RecruitArea(int posX, int posY, const CGTownInstance *Town, int Level):
	town(Town),
	level(Level),
	availableCount(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x +=posX;
	pos.y +=posY;
	pos.w = 386;
	pos.h = 126;

	if (!town->creatures[level].second.empty())
		addUsedEvents(LCLICK | RCLICK | HOVER);//Activate only if dwelling is present

	icons = new CPicture("TPCAINFO", 261, 3);
	if (getMyBuilding() != nullptr)
	{
		buildingPic = new CAnimImage(town->town->clientInfo.buildingsIcons, getMyBuilding()->bid, 0, 4, 21);
		dwellingName = new CLabel(78, 101, FONT_SMALL, CENTER, Colors::WHITE, getMyBuilding()->Name());

		if (vstd::contains(town->builtBuildings, getMyBuilding()->bid))
		{
			ui32 available = town->creatures[level].first;
			std::string availableText = CGI->generaltexth->allTexts[217]+ boost::lexical_cast<std::string>(available);
			availableCount = new CLabel(78, 119, FONT_SMALL, CENTER, Colors::WHITE, availableText);
		}
	}

	if (getMyCreature() != nullptr)
	{
		hoverText = boost::str(boost::format(CGI->generaltexth->tcommands[21]) % getMyCreature()->namePl);
		creatureAnim = new CCreaturePic(159, 4, getMyCreature(), false);
		creatureName = new CLabel(78,  11, FONT_SMALL, CENTER, Colors::WHITE, getMyCreature()->namePl);

		Rect sizes(287, 4, 96, 18);
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[190], CGI->generaltexth->fcommands[0], getMyCreature()->Attack()));
		sizes.y+=20;
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[191], CGI->generaltexth->fcommands[1], getMyCreature()->Defense()));
		sizes.y+=21;
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[199], CGI->generaltexth->fcommands[2], getMyCreature()->getMinDamage(), getMyCreature()->getMaxDamage()));
		sizes.y+=20;
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[388], CGI->generaltexth->fcommands[3], getMyCreature()->MaxHealth()));
		sizes.y+=21;
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[193], CGI->generaltexth->fcommands[4], getMyCreature()->valOfBonuses(Bonus::STACKS_SPEED)));
		sizes.y+=20;
		values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[194], CGI->generaltexth->fcommands[5], town->creatureGrowth(level)));
	}
}

void CFortScreen::RecruitArea::hover(bool on)
{
	if(on)
		GH.statusbar->setText(hoverText);
	else
		GH.statusbar->clear();
}

void CFortScreen::RecruitArea::creaturesChanged()
{
	if (availableCount)
	{
		std::string availableText = CGI->generaltexth->allTexts[217] +
		            boost::lexical_cast<std::string>(town->creatures[level].first);
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




CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner,std::string imagem) :CWindowObject(BORDERED,imagem)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	window = new CPicture(owner->town->town->clientInfo.guildWindow , 332, 76);

	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	Rect barRect(7, 556, 737, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 7, 556, false));

	exit = new CButton(Point(748, 556), "TPMAGE1.DEF", CButton::tooltip(CGI->generaltexth->allTexts[593]), [&]{ close(); }, SDLK_RETURN);
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
		size_t spellCount = owner->town->spellsAtLevel(i+1,false); //spell at level with -1 hmmm?
		for(size_t j=0; j<spellCount; j++)
		{
			if(i<owner->town->mageGuildLevel() && owner->town->spells[i].size()>j)
				spells.push_back( new Scroll(positions[i][j], CGI->spellh->objects[owner->town->spells[i][j]]));
			else
				new CAnimImage("TPMAGES.DEF", 1, 0, positions[i][j].x, positions[i][j].y);//closed scroll
		}
	}
}

CMageGuildScreen::Scroll::Scroll(Point position, const CSpell *Spell)
	:spell(Spell)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(LCLICK | RCLICK | HOVER);
	pos += position;
	image = new CAnimImage("SPELLSCR", spell->id);
	pos = image->pos;
}

void CMageGuildScreen::Scroll::clickLeft(tribool down, bool previousState)
{
	if(down)
		LOCPLINT->showInfoDialog(spell->getLevelInfo(0).description, new CComponent(CComponent::spell,spell->id));
}

void CMageGuildScreen::Scroll::clickRight(tribool down, bool previousState)
{
	if(down)
		CRClickPopup::createAndPush(spell->getLevelInfo(0).description, new CComponent(CComponent::spell, spell->id));
}

void CMageGuildScreen::Scroll::hover(bool on)
{
	if(on)
		GH.statusbar->setText(spell->name);
	else
		GH.statusbar->clear();

}

CBlacksmithDialog::CBlacksmithDialog(bool possible, CreatureID creMachineID, ArtifactID aid, ObjectInstanceID hid):
	CWindowObject(PLAYER_COLORED, "TPSMITH")
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	statusBar = new CGStatusBar(new CPicture(*background, Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	animBG = new CPicture("TPSMITBK", 64, 50);
	animBG->needRefresh = true;

	const CCreature *creature = CGI->creh->creatures[creMachineID];
	anim = new CCreatureAnim(64, 50, creature->animDefName, Rect());
	anim->clipRect(113,125,200,150);

	title = new CLabel(165, 28, FONT_BIG, CENTER, Colors::YELLOW,
	            boost::str(boost::format(CGI->generaltexth->allTexts[274]) % creature->nameSing));
	costText = new CLabel(165, 218, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->jktexts[43]);
	costValue = new CLabel(165, 290, FONT_MEDIUM, CENTER, Colors::WHITE,
	                boost::lexical_cast<std::string>(CGI->arth->artifacts[aid]->price));

	std::string text = boost::str(boost::format(CGI->generaltexth->allTexts[595]) % creature->nameSing);
	buy = new CButton(Point(42, 312), "IBUY30.DEF", CButton::tooltip(text), [&]{ close(); }, SDLK_RETURN);

	text = boost::str(boost::format(CGI->generaltexth->allTexts[596]) % creature->nameSing);
	cancel = new CButton(Point(224, 312), "ICANCEL.DEF", CButton::tooltip(text), [&]{ close(); }, SDLK_ESCAPE);

	if(possible)
		buy->addCallback([=]{ LOCPLINT->cb->buyArtifact(LOCPLINT->cb->getHero(hid),aid); });
	else
		buy->block(true);

	new CAnimImage("RESOURCE", 6, 0, 148, 244);
}
