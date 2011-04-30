#include "../stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "CCastleInterface.h"
#include "../CCallback.h"
#include "../lib/CArtHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "AdventureMapButton.h"
#include "CAdvmapInterface.h"
#include "CAnimation.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "CGameInfo.h"
#include "CHeroWindow.h"
#include "CMessage.h"
#include "CMusicHandler.h"
#include "CPlayerInterface.h"
#include "Graphics.h"
#include "SDL_Extensions.h"

using namespace boost::assign;

/*
 * CCastleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

int hordeToDwellingID(int bid)//helper, converts horde buiding ID into corresponding dwelling ID
{
	const CGTownInstance * t = LOCPLINT->castleInt->town;
	switch (bid)
	{
		case 18: return t->town->hordeLvl[0] + 30;
		case 19: return t->town->hordeLvl[0] + 37;
		case 24: return t->town->hordeLvl[1] + 30;
		case 25: return t->town->hordeLvl[1] + 37;
		default: return bid;
	}
}

CBuildingRect::CBuildingRect(CCastleBuildings * Par, const CGTownInstance *Town, const Structure *Str)
	:CShowableAnim(0, 0, Str->defName, CShowableAnim::BASE | CShowableAnim::USE_RLE),
	parent(Par),
	town(Town),
	str(Str),
	stateCounter(80)
{
	recActions = ACTIVATE | DEACTIVATE | DISPOSE | SHARE_POS;
	used |= LCLICK | RCLICK | HOVER;
	pos.x += str->pos.x;
	pos.y += str->pos.y;

	if (!str->borderName.empty())
		border = BitmapHandler::loadBitmap(str->borderName, true);
	else
		border = NULL;

	if (!str->areaName.empty())
		area = BitmapHandler::loadBitmap(str->areaName);
	else
		area = NULL;
}

CBuildingRect::~CBuildingRect()
{
	SDL_FreeSurface(border);
	SDL_FreeSurface(area);
}

bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	if(str->pos.z != p2.str->pos.z)
		return (str->pos.z) < (p2.str->pos.z);
	else
		return (str->ID) < (p2.str->ID);
}

void CBuildingRect::hover(bool on)
{
	if(on)
	{
		if(!(active & MOVE))
			changeUsedEvents(MOVE, true, true);
	}
	else
	{
		if(active & MOVE)
			changeUsedEvents(MOVE, false, true);

		if(parent->selectedBuilding == this)
		{
			parent->selectedBuilding = NULL;
			GH.statusbar->clear();
		}
	}
}

void CBuildingRect::clickLeft(tribool down, bool previousState)
{
	if( previousState && !down && area && (parent->selectedBuilding==this) )
		if (!CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) ) //inside building image
			parent->buildingClicked(str->ID);
}

void CBuildingRect::clickRight(tribool down, bool previousState)
{
	if((!area) || (!((bool)down)) || (this!=parent->selectedBuilding))
		return;
	if( !CSDL_Ext::isTransparent(area, GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) ) //inside building image
	{
		int bid = hordeToDwellingID(str->ID);
		const CBuilding *bld = CGI->buildh->buildings[str->townID].find(bid)->second;
		if (bid < Buildings::DWELL_FIRST)
		{
			std::vector<SComponent*> comps(1,
			new SComponent(SComponent::building, bld->tid, bld->bid,
			               LOCPLINT->castleInt->bicons->ourImages[bld->bid].bitmap, false));

			CRClickPopup::createAndPush(bld->Description(), comps);
		}
		else
		{
			int level = ( bid - Buildings::DWELL_FIRST ) % CREATURES_PER_TOWN;
			GH.pushInt(new CDwellingInfoBox(parent->pos.x+parent->pos.w/2, parent->pos.y+parent->pos.h/2, town, level));
		}
	}
}

SDL_Color multiplyColors (const SDL_Color &b, const SDL_Color &a, float f)
{
	SDL_Color ret;
	ret.r = a.r*f + b.r*(1-f);
	ret.g = a.g*f + b.g*(1-f);
	ret.b = a.b*f + b.b*(1-f);
	return ret;
}

void CBuildingRect::show(SDL_Surface *to)
{
	const unsigned int stageDelay = 16;

	const unsigned int S1_TRANSP  = 16; //0.5 sec building appear 0->100 transparency
	const unsigned int S2_WHITE_B = 32; //0.5 sec border glows from white to yellow
	const unsigned int S3_YELLOW_B= 48; //0.5 sec border glows from yellow to normal
	const unsigned int BUILDED    = 80; //  1 sec delay, nothing happens

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
		// key colors in glowing border
		SDL_Color c1 = {200, 200, 200, 255};
		SDL_Color c2 = {120, 100,  60, 255};
		SDL_Color c3 = {200, 180, 110, 255};

		unsigned int colorID = SDL_MapRGB(border->format, c3.r, c3.g, c3.b);
		SDL_Color oldColor = border->format->palette->colors[colorID];
		SDL_Color newColor;

		if (stateCounter < S2_WHITE_B)
			newColor = multiplyColors(c1, c2, float(stateCounter%stageDelay)/stageDelay);
		else
		if (stateCounter < S3_YELLOW_B)
			newColor = multiplyColors(c2, c3, float(stateCounter%stageDelay)/stageDelay);
		else
			newColor = oldColor;

		SDL_SetColors(border, &newColor, colorID, 1);
		blitAtLoc(border,0,0,to);
		SDL_SetColors(border, &oldColor, colorID, 1);

	}
	if (stateCounter < BUILDED)
		stateCounter++;
}

void CBuildingRect::showAll(SDL_Surface *to)
{
	if (stateCounter == 0)
		return;

	CShowableAnim::showAll(to);
	if(!active && parent->selectedBuilding == this && border)
		blitAtLoc(border,0,0,to);
}

std::string getBuildingSubtitle(int tid, int bid)//hover text for building
{
	const CGTownInstance * t = LOCPLINT->castleInt->town;
	bid = hordeToDwellingID(bid);

	if (bid<30)//non-dwellings - only buiding name
		return CGI->buildh->buildings[tid].find(bid)->second->Name();
	else//dwellings - recruit %creature%
	{
		int creaID = t->creatures[(bid-30)%CREATURES_PER_TOWN].second.back();//taking last of available creatures
		return CGI->generaltexth->allTexts[16] + " " + CGI->creh->creatures[creaID]->namePl;
	}
}

void CBuildingRect::mouseMoved (const SDL_MouseMotionEvent & sEvent)
{
	if(area && isItIn(&pos,sEvent.x, sEvent.y))
	{
		if(CSDL_Ext::SDL_GetPixel(area,sEvent.x-pos.x,sEvent.y-pos.y) == 0) //hovered pixel is inside this building
		{
			if(parent->selectedBuilding == this)
			{
				parent->selectedBuilding = NULL;
				GH.statusbar->clear();
			}
		}
		else //inside the area of this building
		{
			if(! parent->selectedBuilding //no building hovered
			  || (*parent->selectedBuilding)<(*this)) //or we are on top
			{
				parent->selectedBuilding = this;
				GH.statusbar->print(getBuildingSubtitle(str->townID, str->ID));
			}
		}
	}
}

CDwellingInfoBox::CDwellingInfoBox(int centerX, int centerY, const CGTownInstance *Town, int level)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	used |= RCLICK;
	background = new CPicture("CRTOINFO");
	background->colorize(LOCPLINT->playerID);
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	moveTo(Point(centerX - pos.w/2, centerY - pos.h/2));

	const CCreature * creature = CGI->creh->creatures[Town->creatures[level].second.back()];

	title = new CLabel(80, 30, FONT_SMALL, CENTER, zwykly, creature->namePl);
	animation =  new CCreaturePic(30, 44, creature, true, true);
	
	std::string text = boost::lexical_cast<std::string>(Town->creatures[level].first);
	available = new CLabel(80,190, FONT_SMALL, CENTER, zwykly, CGI->generaltexth->allTexts[217] + text);
	costPerTroop = new CLabel(80, 227, FONT_SMALL, CENTER, zwykly, CGI->generaltexth->allTexts[346]);
	
	for(int i = 0; i<RESOURCE_QUANTITY; i++)
	{
		if(creature->cost[i])
		{
			resPicture.push_back(new CPicture(graphics->resources32->ourImages[i].bitmap, 0, 0, false));
			resAmount.push_back(new CLabel(0,0, FONT_SMALL, CENTER, zwykly, boost::lexical_cast<std::string>(creature->cost[i])));
		}
	}

	int posY = 238;
	int posX = pos.w/2 - resAmount.size() * 40 + 24;
	for (size_t i=0; i<resAmount.size(); i++)
	{
		resPicture[i]->moveBy(Point(posX, posY));
		resAmount[i]->moveBy(Point(posX+16, posY+43));
		posX += 80;
	}
}

void CDwellingInfoBox::clickRight(tribool down, bool previousState)
{
	if((!down || indeterminate(down)))
		GH.popIntTotally(this);
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
		if(highlight)//view NNN
		{
			temp = CGI->generaltexth->tcommands[4];
			boost::algorithm::replace_first(temp,"%s",hero->name);
		}
		else if(other->hero && other->highlight)//exchange
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
		if(other->highlight && other->hero) //move NNNN
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
		GH.statusbar->print(temp);
}

void CHeroGSlot::clickLeft(tribool down, bool previousState)
{
	CHeroGSlot *other = upg  ?  owner->garrisonedHero :  owner->visitingHero;
	if(!down)
	{
		owner->garr->splitting = false;
		owner->garr->highlighted = NULL;

		if(hero && highlight)
		{
			setHighlight(false);
			LOCPLINT->openHeroWindow(hero);
		}
		else if(other->hero && other->highlight)
		{
			bool allow = true;
			if(upg) //moving hero out of town - check if it is allowed
			{
				if(!hero && LOCPLINT->cb->howManyHeroes(false) >= 8)
				{
					std::string tmp = CGI->generaltexth->allTexts[18]; //You already have %d adventuring heroes under your command.
					boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(LOCPLINT->cb->howManyHeroes(false)));
					LOCPLINT->showInfoDialog(tmp,std::vector<SComponent*>(), soundBase::sound_todo);
					allow = false;
				}
				else if(!other->hero->stacksCount()) //hero has no creatures - strange, but if we have appropriate error message...
				{
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[19],std::vector<SComponent*>(), soundBase::sound_todo); //This hero has no creatures.  A hero must have creatures before he can brave the dangers of the countryside.
					allow = false;
				}
			}

			setHighlight(false);
			other->setHighlight(false);

			if(allow)
				LOCPLINT->cb->swapGarrisonHero(owner->town);
		}
		else if(hero)
		{
			setHighlight(true);
			owner->garr->highlighted = NULL;
			showAll(screen2);
		}
		hover(false);hover(true); //refresh statusbar
	}
}

void CHeroGSlot::deactivate()
{
	highlight = false;
	CIntObject::deactivate();
}

void CHeroGSlot::showAll(SDL_Surface * to)
{
	if(hero) //there is hero
		blitAt(graphics->portraitLarge[hero->portrait],pos,to);
	else if(!upg && owner->showEmpty) //up garrison
		blitAt(graphics->flags->ourImages[LOCPLINT->castleInt->town->getOwner()].bitmap,pos,to);
	if(highlight)
		blitAt(graphics->bigImgs[-1],pos,to);
}

CHeroGSlot::CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, HeroSlots * Owner)
{
	used = LCLICK | HOVER;
	owner = Owner;
	pos.x += x;
	pos.y += y;
	pos.w = 58;
	pos.h = 64;
	hero = h;
	upg = updown;
	highlight = false;
}

CHeroGSlot::~CHeroGSlot()
{
}

void CHeroGSlot::setHighlight( bool on )
{
	highlight = on;
	if(owner->garrisonedHero->hero && owner->visitingHero->hero) //two heroes in town
	{
		for(size_t i = 0; i<owner->garr->splitButtons.size(); i++) //splitting enabled when slot higlighted
			owner->garr->splitButtons[i]->block(!on);
	}
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
SORTHELP<Structure> structSorter;

CCastleBuildings::CCastleBuildings(const CGTownInstance* Town):
	town(Town),
	selectedBuilding(NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	background = new CPicture(graphics->townBgs[town->subID]);
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	//Generate buildings list
	for (std::set<si32>::const_iterator building=town->builtBuildings.begin(); building!=town->builtBuildings.end(); building++)
	{
		std::map<int, Structure*>::iterator structure;
		structure = CGI->townh->structures[town->subID].find(*building);

		if(structure != CGI->townh->structures[town->subID].end())
		{
			if(structure->second->group<0) // no group - just add it
				buildings.push_back(new CBuildingRect(this, town, structure->second));
			else // generate list for each group
				groups[structure->second->group].push_back(structure->second);
		}
	}

	Structure * shipyard = CGI->townh->structures[town->subID][6];
	//ship in shipyard
	if(shipyard && vstd::contains(groups, shipyard->group))
	{
		std::vector <const CGObjectInstance *> vobjs = LOCPLINT->cb->getVisitableObjs(town->bestLocation());
		if(!vobjs.empty() && (vobjs.front()->ID == 8 || vobjs.front()->ID == HEROI_TYPE)) //there is visitable obj at shipyard output tile and it's a boat or hero (on boat)
		{
			groups[shipyard->group].push_back(CGI->townh->structures[town->subID][20]);
		}
	}
	//Create building for each group
	for (std::map< int, std::vector<const Structure*> >::iterator group = groups.begin(); group != groups.end(); group++)
	{
		std::sort(group->second.begin(), group->second.end(), structSorter);
		buildings.push_back(new CBuildingRect(this, town, group->second.back()));
	}
	std::sort(buildings.begin(),buildings.end(),buildSorter);
	checkRules();
}

CCastleBuildings::~CCastleBuildings()
{
}

void CCastleBuildings::checkRules()
{
	static const AnimRule animRule[2] = 
	{
		{5, 21, 4, 10, -1, 0, 9},//code for Mana Vortex
		{0,  6, 8,  1, -1, 0, 0},//code for the shipyard in the Castle
	};
	
	for (size_t i=0; i<2; i++)
	{
		if ( town->subID != animRule[i].townID ) //wrong town
			continue;
		
		int groupID = CGI->townh->structures[town->subID][animRule[i].buildID]->group;
		std::map< int, std::vector<const Structure*> >::const_iterator git= groups.find(groupID);
		if ( git == groups.end() || git->second.empty() ) //we have no buildings in this group
			continue;

		int buildID  = git->second.back()->ID;
		for (std::vector< CBuildingRect* >::const_iterator bit=buildings.begin() ; bit !=buildings.end(); bit++ )
		{
			if ( (*bit)->str->ID == buildID ) //last building in group
			{
				if (vstd::contains(town->builtBuildings, animRule[i].toCheck))
					(*bit)->set(0,animRule[i].firstA, animRule[i].lastA);
				else
					(*bit)->set(0,animRule[i].firstB, animRule[i].lastB);
			}
		}
	}
}

void CCastleBuildings::addBuilding(int building)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	std::map<int, Structure*>::const_iterator structure;
	structure = CGI->townh->structures[town->subID].find(building);

	if(structure != CGI->townh->structures[town->subID].end()) //we have info about that structure
	{
		if(structure->second->group<0) //no group - just add it
		{
			buildings.push_back(new CBuildingRect(this, town, structure->second));
			buildings.back()->stateCounter = 0;
		}
		else
		{
			//find last building in this group and replace it with new building if needed
			groups[structure->second->group].push_back(structure->second);
			int newBuilding = groups[structure->second->group].back()->ID;
			if (newBuilding == building)
			{
				for (std::vector< CBuildingRect* >::iterator it=buildings.begin() ; it !=buildings.end(); it++ )
				{
					if ((*it)->str->ID == newBuilding)
					{
						delChild(*it);
						buildings.erase(it);
						break;
					}
				}
				buildings.push_back(new CBuildingRect(this, town, structure->second));
				buildings.back()->stateCounter = 0;
			}
		}
	}
	std::sort(buildings.begin(),buildings.end(),buildSorter);
	checkRules();
}

void CCastleBuildings::removeBuilding(int building)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	std::map<int, Structure*>::const_iterator structure;
	structure = CGI->townh->structures[town->subID].find(building);

	if(structure != CGI->townh->structures[town->subID].end()) //we have info about that structure
	{
		if(structure->second->group<0) //no group - just add it
		{
			for (std::vector< CBuildingRect* >::iterator it=buildings.begin() ; it !=buildings.end(); it++ )
			{
				if ((*it)->str->ID == building)
				{
					delChild(*it);
					buildings.erase(it);
					break;
				}
			}
		}
		else
		{
			groups[structure->second->group].pop_back();
			delChild(buildings[building]);
			if (!groups[structure->second->group].empty())
				buildings.push_back(new CBuildingRect(this, town, structure->second));
		}
	}
	std::sort(buildings.begin(),buildings.end(),buildSorter);
	checkRules();
}

void CCastleBuildings::show(SDL_Surface *to)
{
	CIntObject::show(to);
	for (std::vector< CBuildingRect* >::const_iterator it=buildings.begin() ; it !=buildings.end(); it++ )
		(*it)->show(to);
}

void CCastleBuildings::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	for (std::vector< CBuildingRect* >::const_iterator it=buildings.begin() ; it !=buildings.end(); it++ )
		(*it)->showAll(to);
}

const CGHeroInstance* CCastleBuildings::getHero()
{
	if (town->visitingHero)
		return town->visitingHero;
	if (town->garrisonHero)
		return town->garrisonHero;
	return NULL;
}

void CCastleBuildings::buildingClicked(int building)
{
	tlog5<<"You've clicked on "<<building<<std::endl;
	building = hordeToDwellingID(building);
	const CBuilding *b = CGI->buildh->buildings[town->subID].find(building)->second;

	if(building >= Buildings::DWELL_FIRST)
	{
		enterDwelling((building-Buildings::DWELL_FIRST)%CREATURES_PER_TOWN);
	}
	else
	{
		switch(building)
		{
		case Buildings::MAGES_GUILD_1:
		case Buildings::MAGES_GUILD_2:
		case Buildings::MAGES_GUILD_3:
		case Buildings::MAGES_GUILD_4:
		case Buildings::MAGES_GUILD_5:
				enterMagesGuild();
				break;

		case Buildings::TAVERN:
				LOCPLINT->showTavernWindow(town);
				break;

		case Buildings::SHIPYARD:
				LOCPLINT->showShipyardDialog(town);
				break;

		case Buildings::FORT:
		case Buildings::CITADEL:
		case Buildings::CASTLE:
				GH.pushInt(new CFortScreen(town));
				break;

		case Buildings::VILLAGE_HALL:
		case Buildings::CITY_HALL:
		case Buildings::TOWN_HALL:
		case Buildings::CAPITOL:
				enterTownHall();
				break;

		case Buildings::MARKETPLACE:
				GH.pushInt(new CMarketplaceWindow(town, town->visitingHero));
				break;

		case Buildings::BLACKSMITH:
				enterBlacksmith(town->town->warMachine);
				break;

		case Buildings::SPECIAL_1:
				switch(town->subID)
				{
				case 1://Mystic Pond
						enterFountain(building);
						break;

				case 2: case 5: case 8://Artifact Merchant
						if(town->visitingHero)
							GH.pushInt(new CMarketplaceWindow(town, town->visitingHero, RESOURCE_ARTIFACT));
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				default:
					enterBuilding(building);
					break;
				}
				break;

		case Buildings::SHIP:
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[51]); //Cannot build another boat
				break;

		case Buildings::SPECIAL_2:
				switch(town->subID)
				{
				case 1: //Fountain of Fortune
						enterFountain(building);
						break;

				case 6: //Freelancer's Guild
						if(getHero())
							GH.pushInt(new CMarketplaceWindow(town, getHero(), CREATURE_RESOURCE));
						else
							LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % b->Name())); //Only visiting heroes may use the %s.
						break;

				case 8: //Magic University
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

		case Buildings::SPECIAL_3:
				switch(town->subID)
				{
				case 0: //Brotherhood of sword
						LOCPLINT->showTavernWindow(town);
						break;

				case 3: //Castle Gate
						enterCastleGate();
						break;

				case 4: //Skeleton Transformer
						GH.pushInt( new CTransformerWindow(getHero(), town) );
						break;

				case 5: //Portal of Summoning
						if (town->creatures[CREATURES_PER_TOWN].second.empty())//No creatures
							LOCPLINT->showInfoDialog(CGI->generaltexth->tcommands[30]);
						else
							enterDwelling(CREATURES_PER_TOWN);
						break;

				case 6: //Ballista Yard
						enterBlacksmith(4);
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

void CCastleBuildings::enterBlacksmith(int ArtifactID)
{
	const CGHeroInstance *hero = town->visitingHero;
	if(!hero)
	{
		LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[273]) % CGI->buildh->buildings[town->subID].find(16)->second->Name()));
		return;
	}
	int price = CGI->arth->artifacts[ArtifactID]->price;
	bool possible = LOCPLINT->cb->getResourceAmount(Res::GOLD) >= price && !hero->hasArt(ArtifactID+9);
	GH.pushInt(new CBlacksmithDialog(possible,CArtHandler::convertMachineID(ArtifactID,false),ArtifactID,hero->id));
}

void CCastleBuildings::enterBuilding(int building)
{
	std::vector<SComponent*> comps(1,
		new SComponent(SComponent::building, town->subID,building, 
		               LOCPLINT->castleInt->bicons->ourImages[building].bitmap, false));

	LOCPLINT->showInfoDialog(
		CGI->buildh->buildings[town->subID].find(building)->second->Description(),comps);
}

void CCastleBuildings::enterCastleGate()
{
	if (!town->visitingHero)
	{
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[126]);
		return;//only visiting hero can use castle gates
	}
	std::vector <int> availableTowns;
	std::vector <const CGTownInstance*> Towns = LOCPLINT->cb->getTownsInfo(false);
	for(size_t i=0;i<Towns.size();i++)
	{
		const CGTownInstance *t = Towns[i];
		if (t->id != this->town->id && t->visitingHero == NULL && //another town, empty and this is
			t->subID == 3 && vstd::contains(t->builtBuildings, 22))//inferno with castle gate
		{
			availableTowns.push_back(t->id);//add to the list
		}
	}
	CPicture *titlePic = new CPicture (LOCPLINT->castleInt->bicons->ourImages[22].bitmap, 0,0, false);//will be deleted by selection window
	GH.pushInt (new CObjectListWindow(availableTowns, titlePic, CGI->generaltexth->jktexts[40],
	    CGI->generaltexth->jktexts[41], boost::bind (&CCastleInterface::castleTeleport, LOCPLINT->castleInt, _1)));
}

void CCastleBuildings::enterDwelling(int level)
{
	assert(level >= 0 && level < town->creatures.size());
	GH.pushInt(new CRecruitmentWindow(town, level, town, boost::bind(&CCallback::recruitCreatures,LOCPLINT->cb,town,_1,_2,level), -87));
}

void CCastleBuildings::enterFountain(int building)
{
	std::vector<SComponent*> comps(1,
		new SComponent(SComponent::building,town->subID,building,
		               LOCPLINT->castleInt->bicons->ourImages[building].bitmap,false));

	std::string descr = CGI->buildh->buildings[town->subID].find(building)->second->Description();
	if ( building == 21)//we need description for mystic pond as well
		descr += "\n\n"+CGI->buildh->buildings[town->subID].find(17)->second->Description();
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
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[213]);
		}
		else
		{
			CFunctionList<void()> functionList = boost::bind(&CCallback::buyArtifact,LOCPLINT->cb, hero,0);
			functionList += boost::bind(&CCastleBuildings::openMagesGuild,this);
			std::vector<SComponent*> components(1, new SComponent(SComponent::artifact,0,0));

			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[214], components, functionList, 0, true);
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
		!vstd::contains(town->builtBuildings, Buildings::GRAIL)) //hero has grail, but town does not have it
	{
		if(!vstd::contains(town->forbiddenBuildings, Buildings::GRAIL))
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[597], //Do you wish this to be the permanent home of the Grail?
			                            std::vector<SComponent*>(),
			                            boost::bind(&CCallback::buildBuilding, LOCPLINT->cb, town, Buildings::GRAIL),
			                            boost::bind(&CCastleBuildings::openTownHall, this), true);
		}
		else
		{
			LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[673]);
			(dynamic_cast<CInfoWindow*>(GH.topInt()))->buttons[0]->callback += boost::bind(&CCastleBuildings::openTownHall, this);
		}
	}
	else
	{
		openTownHall();
	}
}

void CCastleBuildings::openMagesGuild()
{
	GH.pushInt(new CMageGuildScreen(LOCPLINT->castleInt));
}

void CCastleBuildings::openTownHall()
{
	GH.pushInt(new CHallInterface(town));
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, int listPos):
	hall(NULL),
	fort(NULL),
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	used |= KEYBOARD;

	builds = new CCastleBuildings(town);
	panel = new CPicture("TOWNSCRN", 0, builds->pos.h);
	panel->colorize(LOCPLINT->playerID);
	pos.w = panel->pos.w;
	pos.h = builds->pos.h + panel->pos.h;
	center();

	garr = new CGarrisonInt(305, 387, 4, Point(0,96), panel->bg, Point(62,374), town->getUpperArmy(), town->visitingHero);
	heroes = new HeroSlots(town, Point(241, 387), Point(241, 483), garr, true);
	title = new CLabel(85, 387, FONT_MEDIUM, TOPLEFT, zwykly, town->name);
	income = new CLabel(195, 443, FONT_SMALL, CENTER);
	icon = new CAnimImage("ITPT", 0, 0, 15, 387);

	exit = new AdventureMapButton(CGI->generaltexth->tcommands[8], "", boost::bind(&CCastleInterface::close,this), 744, 544, "TSBTNS", SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	exit->setOffset(4);

	split = new AdventureMapButton(CGI->generaltexth->tcommands[3], "", boost::bind(&CGarrisonInt::splitClick,garr), 744, 382, "TSBTNS.DEF");
	split->callback += boost::bind(&HeroSlots::splitClicked, heroes);
	removeChild(split);
	garr->addSplitBtn(split);

	Rect barRect(9, 182, 732, 18);
	statusbar = new CGStatusBar(new CPicture(*panel, barRect, 9, 555, false));
	resdatabar = new CResDataBar("ZRESBAR", 3, 575, 32, 2, 85, 85);

	townlist = new CTownList(3, 744, 414, "IAM014", "IAM015");
	townlist->fun = boost::bind(&CCastleInterface::townChange, this);
	townlist->selected = vstd::findPos(LOCPLINT->towns, Town);

	townlist->from = townlist->selected - listPos;
	amax(townlist->from, 0);
	amin(townlist->from, LOCPLINT->towns.size() - townlist->SIZE);

	LOCPLINT->castleInt = this;
	recreateIcons();
	CCS->musich->playMusic(CCS->musich->townMusics[town->subID], -1);
	
	bicons = CDefHandler::giveDefEss(graphics->buildingPics[town->subID]);
}

CCastleInterface::~CCastleInterface()
{
	LOCPLINT->castleInt = NULL;
	CCS->musich->stopMusic(5000);
	delete bicons;
}

void CCastleInterface::close()
{
	if(town->tempOwner == LOCPLINT->playerID) //we may have opened window for an allied town
	{
		if(town->visitingHero)
			adventureInt->select(town->visitingHero);
		else
			adventureInt->select(town);
	}
	GH.popIntTotally(this);
}

void CCastleInterface::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
}

void CCastleInterface::castleTeleport(int where)
{
	const CGTownInstance * dest = LOCPLINT->cb->getTownInfo(where, 1);
	LOCPLINT->cb->teleportHero(town->visitingHero, dest);
}

void CCastleInterface::townChange()
{
	const CGTownInstance * nt = LOCPLINT->towns[townlist->selected];
	int tpos = townlist->selected - townlist->from;
	if ( nt == town )
		return;
	GH.popIntTotally(this);
	GH.pushInt(new CCastleInterface(nt, tpos));
}

void CCastleInterface::addBuilding(int bid)
{
	deactivate();
	builds->addBuilding(bid);
	recreateIcons();
	activate();
}

void CCastleInterface::removeBuilding(int bid)
{
	deactivate();
	builds->removeBuilding(bid);
	recreateIcons();
	activate();
}

void CCastleInterface::recreateIcons()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	if (fort)
		delChild(fort);
	if (hall)
		delChild(hall);

	size_t iconIndex = town->subID*2;
	if (!town->hasFort())
		iconIndex += F_NUMBER*2;

	if(town->builded >= MAX_BUILDING_PER_TURN)
		iconIndex++;

	icon->setFrame(iconIndex);
	income->setTxt(boost::lexical_cast<std::string>(town->dailyIncome()));

	hall = new CTownInfo( 80, 413, town, true);
	fort = new CTownInfo(122, 413, town, false);

	for (size_t i=0; i<creainfo.size(); i++)
		delChild(creainfo[i]);
	creainfo.clear();

	for (size_t i=0; i<4; i++)
		creainfo.push_back(new CCreaInfo(14+55*i, 459, town, i));

	for (size_t i=0; i<4; i++)
		creainfo.push_back(new CCreaInfo(14+55*i, 507, town, i+4));
}

CCreaInfo::CCreaInfo(int posX, int posY, const CGTownInstance *Town, int Level):
	town(Town),
	level(Level)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x += posX;
	pos.y += posY;
	pos.w = 48;
	pos.h = 48;
	
	if ( town->creatures.size() <= level || town->creatures[level].second.empty())
	{
		level = -1;
		return;//No creature
	}
	used = LCLICK | RCLICK | HOVER;

	unsigned int creatureID = town->creatures[level].second.back();
	creature = CGI->creh->creatures[creatureID];

	label = new CLabel(24, 40, FONT_SMALL, CENTER, zwykly, boost::lexical_cast<std::string>(town->creatureGrowth(level)));
	picture = new CAnimImage("CPRSMALL", creatureID+2, 0, 8, 0);
}

void CCreaInfo::hover(bool on)
{
	std::string message = CGI->generaltexth->allTexts[588];
	boost::algorithm::replace_first(message,"%s",creature->namePl);

	if(on)
	{
		GH.statusbar->print(message);
	}
	else if (message == GH.statusbar->getCurrent())
		GH.statusbar->clear();
}

void CCreaInfo::clickLeft(tribool down, bool previousState)
{//FIXME: castleInt should be present - may be NULL if no castle window opened
	if(previousState && (!down) && LOCPLINT->castleInt)
		LOCPLINT->castleInt->builds->enterDwelling(level);
}

int CCreaInfo::AddToString(std::string from, std::string & to, int numb)
{
	if (numb == 0)
		return 0;
	boost::algorithm::replace_first(from,"%+d", (numb > 0 ? "+" : "")+boost::lexical_cast<std::string>(numb)); //negative values don't need "+"
	to+="\n"+from;
	return numb;
}

void CCreaInfo::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		int summ=0;
		std::string descr=CGI->generaltexth->allTexts[589];//Growth of creature is number
		boost::algorithm::replace_first(descr,"%s", creature->nameSing);
		boost::algorithm::replace_first(descr,"%d", boost::lexical_cast<std::string>(town->creatureGrowth(level)));

		descr +="\n"+CGI->generaltexth->allTexts[590];
		summ = creature->growth;
		boost::algorithm::replace_first(descr,"%d", boost::lexical_cast<std::string>(summ));

		if ( level>=0 && level<CREATURES_PER_TOWN)
		{

			if ( vstd::contains(town->builtBuildings, Buildings::CASTLE))
				summ+=AddToString(CGI->buildh->buildings[town->subID][Buildings::CASTLE]->Name()+" %+d",descr,summ);
			else if ( vstd::contains(town->builtBuildings, Buildings::CITADEL))
				summ+=AddToString(CGI->buildh->buildings[town->subID][Buildings::CITADEL]->Name()+" %+d",descr,summ/2);

			summ+=AddToString(CGI->generaltexth->allTexts[63] + " %+d",descr, //double growth or plague
				summ * creature->valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100);

			summ+=AddToString(CGI->generaltexth->artifNames[133] + " %+d",descr,
				summ * town->valOfGlobalBonuses
				(Selector::type(Bonus::CREATURE_GROWTH_PERCENT) && Selector::sourceType(Bonus::ARTIFACT))/100); //Statue of Legion

			if(town->town->hordeLvl[0]==level)//horde, x to summ
			if( vstd::contains(town->builtBuildings, Buildings::HORDE_1) || vstd::contains(town->builtBuildings, Buildings::HORDE_1_UPGR))
				summ+=AddToString(CGI->buildh->buildings[town->subID][Buildings::HORDE_1]->Name()+" %+d",descr,
					creature->hordeGrowth);

			if(town->town->hordeLvl[1]==level)//horde, x to summ
			if( vstd::contains(town->builtBuildings, Buildings::HORDE_2) || vstd::contains(town->builtBuildings, Buildings::HORDE_2_UPGR))
				summ+=AddToString(CGI->buildh->buildings[town->subID][Buildings::HORDE_2]->Name()+" %+d",descr,
					creature->hordeGrowth);

			int cnt = 0;

			std::vector< const CGDwelling * > myDwellings = LOCPLINT->cb->getMyDwellings();
			for (std::vector<const CGDwelling*>::const_iterator it = myDwellings.begin(); it != myDwellings.end(); ++it)
				if (CGI->creh->creatures[town->town->basicCreatures[level]]->idNumber == (*it)->creatures[0].second[0])
					cnt++;//external dwellings count to summ
			summ+=AddToString(CGI->generaltexth->allTexts[591],descr,cnt);

			BonusList bl;
			const CGHeroInstance *hero = town->garrisonHero;
			if (hero)
				hero->getBonuses(bl, Selector::type(Bonus::CREATURE_GROWTH) && Selector::subtype(level) 
			                      && Selector::sourceType(Bonus::ARTIFACT), hero);
			
			hero = town->visitingHero;
			if (hero)
				hero->getBonuses(bl, Selector::type(Bonus::CREATURE_GROWTH) && Selector::subtype(level) 
			                      && Selector::sourceType(Bonus::ARTIFACT), hero);
			
			if (bl.size())
				summ+=AddToString (CGI->arth->artifacts[bl.front()->sid]->Name()+" %+d", descr, bl.totalValue());

			//TODO: player bonuses

			if(vstd::contains(town->builtBuildings, Buildings::GRAIL)) //grail - +50% to ALL growth
				summ+=AddToString(CGI->buildh->buildings[town->subID][Buildings::GRAIL]->Name()+" %+d",descr,summ/2);

			summ+=AddToString(CGI->generaltexth->allTexts[63] + " %+d",descr, creature->valOfBonuses(Bonus::CREATURE_GROWTH));
		}

		CInfoPopup *mess = new CInfoPopup();//creating popup
		mess->free = true;
		mess->bitmap = CMessage::drawBoxTextBitmapSub
		(LOCPLINT->playerID, descr,graphics->bigImgs[creature->idNumber],"");
		mess->pos.x = screen->w/2 - mess->bitmap->w/2;
		mess->pos.y = screen->h/2 - mess->bitmap->h/2;
		GH.pushInt(mess);
	}
}

CTownInfo::CTownInfo(int posX, int posY, const CGTownInstance* Town, bool townHall):
	town(Town),
	building(NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	used = RCLICK | HOVER;
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
	building = CGI->buildh->buildings[town->subID][buildID];
	pos = picture->pos;
}

void CTownInfo::hover(bool on)
{
	if(on)
	{
		if ( building )
			GH.statusbar->print(building->Name());
	}
	else
		GH.statusbar->clear();
}

void CTownInfo::clickRight(tribool down, bool previousState)
{//FIXME: castleInt may be NULL
	if(down && building && LOCPLINT->castleInt)
	{
		CInfoPopup *mess = new CInfoPopup();
		mess->free = true;

		mess->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,
			 building->Description(),
			 LOCPLINT->castleInt->bicons->ourImages[building->bid].bitmap,
			 building->Name());
		mess->pos.x = screen->w/2 - mess->bitmap->w/2;
		mess->pos.y = screen->h/2 - mess->bitmap->h/2;
		GH.pushInt(mess);
	}
}

void CCastleInterface::keyPressed( const SDL_KeyboardEvent & key )
{
	if(key.state != SDL_PRESSED) return;

	switch(key.keysym.sym)
	{
	case SDLK_UP:
		if(townlist->selected)
		{
			townlist->selected--;
			townlist->from--;
			townChange();
		}
		break;
	case SDLK_DOWN:
		if(townlist->selected < LOCPLINT->towns.size() - 1)
		{
			townlist->selected++;
			townlist->from++;
			townChange();
		}
		break;
	case SDLK_SPACE:
		if(!!town->visitingHero && town->garrisonHero)
		{
			LOCPLINT->cb->swapGarrisonHero(town);
		}
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
	garrisonedHero->hero = town->garrisonHero;
	visitingHero->hero = town->visitingHero;
}

void HeroSlots::splitClicked()
{
	if(!!town->visitingHero && town->garrisonHero && (visitingHero->highlight || garrisonedHero->highlight))
	{
		LOCPLINT->heroExchangeStarted(town->visitingHero->id, town->garrisonHero->id);
	}
}

void CHallInterface::CBuildingBox::hover(bool on)
{
	if(on)
	{
		std::string toPrint;
		if(state==Buildings::PREREQUIRES)
			toPrint = CGI->generaltexth->hcommands[5];
		else if(state==Buildings::CANT_BUILD_TODAY)
			toPrint = CGI->generaltexth->allTexts[223];
		else
			toPrint = CGI->generaltexth->hcommands[state];
		boost::algorithm::replace_first(toPrint,"%s",building->Name());
		GH.statusbar->print(toPrint);
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
	used = LCLICK | RCLICK | HOVER;
	pos.x += x;
	pos.y += y;
	pos.w = 154;
	pos.h = 92;
	
	state = LOCPLINT->cb->canBuildStructure(town,building->bid);
	assert(state < Buildings::ERROR);
	static int panelIndex[9] = { 3,  3,  3, 0, 0, 2, 2,  1, 2};
	static int  iconIndex[9] = {-1, -1, -1, 0, 0, 1, 2, -1, 1};

	picture = new CAnimImage(graphics->buildingPics[town->subID], building->bid, 0, 2, 2);
	panel = new CAnimImage("TPTHBAR", panelIndex[state], 0,   1, 73);
	if ( iconIndex[state] >=0 )
		icon  = new CAnimImage("TPTHCHK",  iconIndex[state], 0, 136, 56);
	label = new CLabel(75, 81, FONT_SMALL, CENTER, zwykly, building->Name());
}

CHallInterface::CHallInterface(const CGTownInstance *Town):
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	background = new CPicture(CGI->buildh->hall[town->subID].first);
	background->colorize(LOCPLINT->playerID);
	pos = background->center();

	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	Rect barRect(5, 556, 740, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 5, 556, false));

	title = new CLabel(399, 12, FONT_MEDIUM, CENTER, zwykly, CGI->buildh->buildings[town->subID][town->hallLevel()+Buildings::VILLAGE_HALL]->Name());
	exit = new AdventureMapButton(CGI->generaltexth->hcommands[8], "", 
	           boost::bind(&CHallInterface::close,this), 748, 556, "TPMAGE1.DEF", SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	const std::vector< std::vector< std::vector<int> > > &boxList = CGI->buildh->hall[town->subID].second;
	boxes.resize(boxList.size());
	for(size_t row=0; row<boxList.size(); row++) //for each row
	{
		for(size_t col=0; col<boxList[row].size(); col++) //for each box
		{
			const CBuilding *building = NULL;
			for(size_t item=0; item<boxList[row][col].size(); item++)//we are looking for the first not build structure
			{
				int buildingID = boxList[row][col][item];
				building = CGI->buildh->buildings[town->subID][buildingID];

				if (buildingID == 18 && vstd::contains(town->builtBuildings, town->town->hordeLvl[0]+37))
					continue;
				if (buildingID == 24 && vstd::contains(town->builtBuildings, town->town->hordeLvl[1]+37))
					continue;
				if(vstd::contains(town->builtBuildings,buildingID))
					continue;
				break;
			}
			int posX = pos.w/2 - boxList[row].size()*154/2 - (boxList[row].size()-1)*20 + 194*col,
			    posY = 35 + 104*row;

			if (building)
				boxes[row].push_back(new CBuildingBox(posX, posY, town, building));
		}
	}
}

void CHallInterface::close()
{
	GH.popIntTotally(this);
}

void CBuildWindow::buyFunc()
{
	LOCPLINT->cb->buildBuilding(town,building->bid);
	GH.popInts(2); //we - build window and hall screen
}

void CBuildWindow::close()
{
	GH.popIntTotally(this);
}

void CBuildWindow::clickRight(tribool down, bool previousState)
{
	if((!down || indeterminate(down)) && mode)
		close();
}

std::string CBuildWindow::getTextForState(int state)
{
	std::string ret;
	if(state<7)
		ret =  CGI->generaltexth->hcommands[state];
	switch (state)
	{
	case 4:	case 5: case 6:
		ret.replace(ret.find_first_of("%s"),2,building->Name());
		break;
	case 7:
		return CGI->generaltexth->allTexts[219]; //all prereq. are met
	case 8:
		{
			ret = CGI->generaltexth->allTexts[52];
			std::set<int> reqs= LOCPLINT->cb->getBuildingRequiments(town, building->bid);

			for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
			{
				if (vstd::contains(town->builtBuildings, *i))
					continue;//skipping constructed buildings
				ret+= CGI->buildh->buildings[town->subID][*i]->Name() + ", ";
			}
			ret.erase(ret.size()-2);
		}
	}
	return ret;
}

CBuildWindow::CBuildWindow(const CGTownInstance *Town, const CBuilding * Building, int State, bool Mode)
:town(Town), building(Building), state(State), mode(Mode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	used |= RCLICK;
	background = new CPicture("TPUBUILD");
	pos = background->center();
	background->colorize(LOCPLINT->playerID);

	buildingPic = new CAnimImage(graphics->buildingPics[town->subID], building->bid, 0, 125, 50);
	Rect barRect(9, 494, 380, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 9, 494, false));

	title = new CLabel(197, 30, FONT_MEDIUM, CENTER, zwykly, 
	            boost::str(boost::format(CGI->generaltexth->hcommands[7]) % building->Name()));
	buildingDescr = new CTextBox(building->Description(), Rect(33, 135, 329, 67), 0, FONT_MEDIUM, CENTER);
	buildingState = new CTextBox(getTextForState(state),  Rect(33, 216, 329, 67), 0, FONT_SMALL,  CENTER);

	//Create objects for all required resources
	for(int i = 0; i<RESOURCE_QUANTITY; i++)
	{
		if(building->resources[i])
		{
			resPicture.push_back(new CPicture(graphics->resources32->ourImages[i].bitmap, 0, 0, false));
			resAmount.push_back(new CLabel(0,0, FONT_SMALL, CENTER, zwykly, 
			                        boost::lexical_cast<std::string>(building->resources[i])));
		}
	}

	unsigned int rowSize[2];
	int posY;
	if (resAmount.size() > 4)
	{//Resources will be placed in multiple rows
		rowSize[0] = (resAmount.size()+1)/2;
		posY = 303;
	}
	else
	{//one row
		rowSize[0] = resAmount.size();
		posY = 340;
	}
	rowSize[1] = resAmount.size() - rowSize[0];

	unsigned int index=0;
	for (size_t row=0; row<2; row++)
	{
		int posX = pos.w/2 - rowSize[row] * 40 + 24;
		for (size_t i=0; i<rowSize[row]; i++)
		{//Move current resource to correct position
			resPicture[index]->moveBy(Point(posX, posY));
			resAmount[index]->moveBy(Point(posX+16, posY+48));
			posX += 80;
			index++;
		}
		posY +=75;
	}

	if(!mode)
	{
		buy = new AdventureMapButton(boost::str(boost::format(CGI->generaltexth->allTexts[595]) % building->Name()),
		          "", boost::bind(&CBuildWindow::buyFunc,this), 45, 446,"IBUY30", SDLK_RETURN);
		buy->borderColor = int3(173, 142, 66);
		buy->borderEnabled = true;
		
		cancel = new AdventureMapButton(boost::str(boost::format(CGI->generaltexth->allTexts[596]) % building->Name()),
		             "", boost::bind(&CBuildWindow::close,this), 290, 445, "ICANCEL", SDLK_ESCAPE);
		cancel->borderColor = int3(173, 142, 66);
		cancel->borderEnabled = true;
		buy->block(state!=7);
	}
}

CBuildWindow::~CBuildWindow()
{}

void CFortScreen::close()
{
	GH.popIntTotally(this);
}

CFortScreen::CFortScreen(const CGTownInstance * town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	unsigned int fortSize = town->creatures.size();
	if (fortSize > CREATURES_PER_TOWN && town->creatures.back().second.empty())
		fortSize--;
	
	if (fortSize == CREATURES_PER_TOWN)
		background = new CPicture("TPCASTL7");
	else
		background = new CPicture("TPCASTL8");
	background->colorize(LOCPLINT->playerID);
	pos = background->center();

	const CBuilding *fortBuilding = CGI->buildh->buildings[town->subID][town->fortLevel()+6];
	title = new CLabel(400, 12, FONT_BIG, CENTER, zwykly, fortBuilding->Name());
	
	std::string text = boost::str(boost::format(CGI->generaltexth->fcommands[6]) % fortBuilding->Name());
	exit = new AdventureMapButton(text, "", boost::bind(&CFortScreen::close,this) ,748, 556, "TPMAGE1", SDLK_RETURN);

	std::vector<Point> positions;
	positions += Point(10,  22), Point(404, 22),
	             Point(10, 155), Point(404,155),
	             Point(10, 288), Point(404,288);

	if (fortSize == CREATURES_PER_TOWN)
		positions += Point(206,421);
	else
		positions += Point(10, 421), Point(404,421);
	
	for (unsigned int i=0; i<fortSize; i++)
	{
		int buildingID;
		if (fortSize == CREATURES_PER_TOWN)
		{
			if (vstd::contains(town->builtBuildings, Buildings::DWELL_UP_FIRST+i))
				buildingID = Buildings::DWELL_UP_FIRST+i;
			else
				buildingID = Buildings::DWELL_FIRST+i;
		}
		else
			buildingID = 22;
		recAreas.push_back(new RecruitArea(positions[i].x, positions[i].y, town, buildingID, i));
	}

	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;

	Rect barRect(4, 554, 740, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 4, 554, false));
}

void CFortScreen::creaturesChanged()
{
	for (size_t i=0; i<recAreas.size(); i++)
		recAreas[i]->creaturesChanged();
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
	used |= HOVER;
	hoverText = descr;
	std::string valueText;
	if (min && max)
	{
		valueText = boost::lexical_cast<std::string>(min);
		if (min != max)
			valueText += '-' + boost::lexical_cast<std::string>(max);
	}
	name =  new CLabel(3, 0, FONT_SMALL, TOPLEFT, zwykly, nameText);
	value = new CLabel(pos.w-3, pos.h-2, FONT_SMALL, BOTTOMRIGHT, zwykly, valueText);
}

void LabeledValue::hover(bool on)
{
	if(on)
		GH.statusbar->print(hoverText);
	else
	{
		GH.statusbar->clear();
		parent->hovered = false;
	}
}

CFortScreen::RecruitArea::RecruitArea(int posX, int posY, const CGTownInstance *Town, int buildingID, int Level):
	town(Town),
	level(Level),
	availableCount(NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x +=posX;
	pos.y +=posY;
	pos.w = 386;
	pos.h = 126;
	
	if (!town->creatures[level].second.empty())
		used |= LCLICK | RCLICK | HOVER;//Activate only if dwelling is present
	
	icons = new CPicture("ZPCAINFO", 261, 3);
	buildingPic = new CAnimImage(graphics->buildingPics[town->subID], buildingID, 0, 4, 21);

	const CCreature* creature = NULL;

	if (!town->creatures[level].second.empty())
		creature = CGI->creh->creatures[town->creatures[level].second.back()];
	else
		creature = CGI->creh->creatures[town->town->basicCreatures[level]];

	hoverText = boost::str(boost::format(CGI->generaltexth->tcommands[21]) % creature->namePl);
	creatureAnim = new CCreaturePic(159, 4, creature, false);

	Rect sizes(287, 4, 96, 18);
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[190], CGI->generaltexth->fcommands[0], creature->attack));
	sizes.y+=20;
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[191], CGI->generaltexth->fcommands[1], creature->defence));
	sizes.y+=21;
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[199], CGI->generaltexth->fcommands[2], creature->damageMin, creature->damageMax));
	sizes.y+=20;
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[388], CGI->generaltexth->fcommands[3], creature->hitPoints));
	sizes.y+=21;
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[193], CGI->generaltexth->fcommands[4], creature->speed));
	sizes.y+=20;
	values.push_back(new LabeledValue(sizes, CGI->generaltexth->allTexts[194], CGI->generaltexth->fcommands[5], town->creatureGrowth(level)));

	creatureName = new CLabel(78,  11, FONT_SMALL, CENTER, zwykly, creature->namePl);
	dwellingName = new CLabel(78, 101, FONT_SMALL, CENTER, zwykly, CGI->buildh->buildings[town->subID][buildingID]->Name());

	if (vstd::contains(town->builtBuildings, buildingID))
	{
		unsigned int available = town->creatures[level].first;
		std::string availableText = CGI->generaltexth->allTexts[217]+ boost::lexical_cast<std::string>(available);
		availableCount = new CLabel(78, 119, FONT_SMALL, CENTER, zwykly, availableText);
	}
}

void CFortScreen::RecruitArea::hover(bool on)
{
	if(on)
		GH.statusbar->print(hoverText);
	else
		GH.statusbar->clear();
}

void CFortScreen::RecruitArea::creaturesChanged()
{
	if (availableCount)
	{
		std::string availableText = CGI->generaltexth->allTexts[217] +
		            boost::lexical_cast<std::string>(town->creatures[level].first);
		availableCount->setTxt(availableText);
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

CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	
	background = new CPicture("TPMAGE.bmp");
	pos = background->center();
	window = new CPicture(graphics->guildBgs[owner->town->subID], 332, 76);
	
	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	Rect barRect(7, 556, 737, 18);
	statusBar = new CGStatusBar(new CPicture(*background, barRect, 7, 556, false));
	
	exit = new AdventureMapButton(CGI->generaltexth->allTexts[593],"",boost::bind(&CMageGuildScreen::close,this), 748, 556,"TPMAGE1.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	
	
	std::vector<std::vector<Point> > positions;

	positions.resize(5);
	positions[0] += Point(222,445), Point(312,445), Point(402,445), Point(520,445), Point(610,445), Point(700,445);
	positions[1] += Point(48,53),   Point(48,147),  Point(48,241),  Point(48,335),  Point(48,429);
	positions[2] += Point(570,82),  Point(672,82),  Point(570,157), Point(672,157);
	positions[3] += Point(183,42),  Point(183,148), Point(183,253);
	positions[4] += Point(491,325), Point(591,325);
	
	for(size_t i=0; i<owner->town->town->mageLevel; i++)
	{
		size_t spellCount = owner->town->spellsAtLevel(i+1,false); //spell at level with -1 hmmm?
		for(size_t j=0; j<spellCount; j++)
		{
			if(i<owner->town->mageGuildLevel() && owner->town->spells[i].size()>j)
				spells.push_back( new Scroll(positions[i][j], CGI->spellh->spells[owner->town->spells[i][j]]));
			else
				new CAnimImage("TPMAGES.DEF", 1, 0, positions[i][j].x, positions[i][j].y);//closed scroll
		}
	}
}

void CMageGuildScreen::close()
{
	GH.popIntTotally(this);
}

CMageGuildScreen::Scroll::Scroll(Point position, const CSpell *Spell)
	:spell(Spell)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	used = LCLICK | RCLICK | HOVER;
	pos += position;
	image = new CAnimImage("SPELLSCR", spell->id);
	pos = image->pos;
}

void CMageGuildScreen::Scroll::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		std::vector<SComponent*> comps(1,
			new SComponent(SComponent::spell,spell->id,0)
		);
		LOCPLINT->showInfoDialog(spell->descriptions[0],comps, soundBase::sound_todo);
	}
}

void CMageGuildScreen::Scroll::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CInfoPopup *vinya = new CInfoPopup();
		vinya->free = true;
		vinya->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,
			spell->descriptions[0],graphics->spellscr->ourImages[spell->id].bitmap,
			spell->name,30,30);
		vinya->pos.x = screen->w/2 - vinya->bitmap->w/2;
		vinya->pos.y = screen->h/2 - vinya->bitmap->h/2;
		GH.pushInt(vinya);
	}
}

void CMageGuildScreen::Scroll::hover(bool on)
{
	if(on)
		GH.statusbar->print(spell->name);
	else
		GH.statusbar->clear();

}

CBlacksmithDialog::CBlacksmithDialog(bool possible, int creMachineID, int aid, int hid)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	background = new CPicture("TPSMITH");
	pos = background->center();
	background->colorize(LOCPLINT->playerID);
	statusBar = new CGStatusBar(164, 370);
	
	animBG = new CPicture("TPSMITBK", 64, 50);
	animBG->needRefresh = true;

	const CCreature *creature = CGI->creh->creatures[creMachineID];
	anim = new CCreatureAnim(64, 50, creature->animDefName, Rect());
	anim->clipRect(113,125,200,150);
	
	title = new CLabel(165, 28, FONT_BIG, CENTER, tytulowy, 
	            boost::str(boost::format(CGI->generaltexth->allTexts[274]) % creature->nameSing));
	costText = new CLabel(165, 218, FONT_MEDIUM, CENTER, zwykly, CGI->generaltexth->jktexts[43]);
	costValue = new CLabel(165, 290, FONT_MEDIUM, CENTER, zwykly,
	                boost::lexical_cast<std::string>(CGI->arth->artifacts[aid]->price));

	std::string text = boost::str(boost::format(CGI->generaltexth->allTexts[595]) % creature->nameSing);
	buy = new AdventureMapButton(text,"",boost::bind(&CBlacksmithDialog::close, this), 42, 312,"IBUY30.DEF",SDLK_RETURN);
	
	text = boost::str(boost::format(CGI->generaltexth->allTexts[596]) % creature->nameSing);
	cancel = new AdventureMapButton(text,"",boost::bind(&CBlacksmithDialog::close, this), 224, 312,"ICANCEL.DEF",SDLK_ESCAPE);

	if(possible)
		buy->callback += boost::bind(&CCallback::buyArtifact,LOCPLINT->cb,LOCPLINT->cb->getHeroInfo(hid,2),aid);
	else
		buy->block(true);

	gold = new CPicture(graphics->resources32->ourImages[6].bitmap,148,244, false);
}

void CBlacksmithDialog::close()
{
	GH.popIntTotally(this);
}
