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

CBuildingRect::CBuildingRect(CCastleBuildings * Par, const Structure *Str)
	:CShowableAnim(0, 0, Str->defName, CShowableAnim::BASE | CShowableAnim::USE_RLE),
	parent(Par), str(Str), stateCounter(80)
{
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

		std::vector<SComponent*> comps(1,
		new SComponent(SComponent::building, bld->tid, bld->bid,
		               LOCPLINT->castleInt->bicons->ourImages[bld->bid].bitmap, false));

		CRClickPopup::createAndPush(bld->Description(), comps);
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
	if (active)
		return;

	CShowableAnim::showAll(to);
	if(parent->selectedBuilding == this && border)
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

void CHeroGSlot::hover (bool on)
{
	if(!on)
	{
		GH.statusbar->clear();
		return;
	}
	CHeroGSlot *other = upg  ?  &owner->hslotup :  &owner->hslotdown;
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
	CHeroGSlot *other = upg  ?  &owner->hslotup :  &owner->hslotdown;
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
			show(screen2);
		}
		hover(false);hover(true); //refresh statusbar
	}
}

void CHeroGSlot::deactivate()
{
	highlight = false;
	CIntObject::deactivate();
}

void CHeroGSlot::show(SDL_Surface * to)
{
	if(hero) //there is hero
		blitAt(graphics->portraitLarge[hero->portrait],pos,to);
	else if(!upg) //up garrison
		blitAt(graphics->flags->ourImages[LOCPLINT->castleInt->town->getOwner()].bitmap,pos,to);
	if(highlight)
		blitAt(graphics->bigImgs[-1],pos,to);
}

CHeroGSlot::CHeroGSlot(int x, int y, int updown, const CGHeroInstance *h, CCastleInterface * Owner)
{
	used = LCLICK | HOVER;
	owner = Owner;
	pos.x = x;
	pos.y = y;
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
	if(owner->hslotup.hero && owner->hslotdown.hero) //two heroes in town
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
	pos = LOCPLINT->castleInt->pos;//FIXME: remove access to castleInt

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
				buildings.push_back(new CBuildingRect(this, structure->second));
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
		buildings.push_back(new CBuildingRect(this, group->second.back()));
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
			buildings.push_back(new CBuildingRect(this, structure->second));
		}
		else
		{
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
				buildings.push_back(new CBuildingRect(this, structure->second));
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
				buildings.push_back(new CBuildingRect(this, structure->second));
		}
	}
	std::sort(buildings.begin(),buildings.end(),buildSorter);
	checkRules();
}

void CCastleBuildings::show(SDL_Surface *to)
{
	//FIXME: object capturing, if possible
	background->show(to);
	for (std::vector< CBuildingRect* >::const_iterator it=buildings.begin() ; it !=buildings.end(); it++ )
		(*it)->show(to);
}

void CCastleBuildings::showAll(SDL_Surface *to)
{
	//FIXME: object capturing, if possible
	background->showAll(to);
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

	if(building >= 30)
	{
		enterDwelling((building-30)%CREATURES_PER_TOWN);
	}
	else
	{
		switch(building)
		{
		case 0: case 1: case 2: case 3: case 4: //mage guild
				enterMagesGuild();
				break;

		case 5: //tavern
				LOCPLINT->showTavernWindow(town);
				break;

		case 6: //shipyard
				LOCPLINT->showShipyardDialog(town);
				break;

		case 7: case 8: case 9: //fort/citadel/castle
				GH.pushInt(new CFortScreen(LOCPLINT->castleInt));
				break;

		case 10: case 11: case 12: case 13: //hall
				enterTownHall();
				break;

		case 14:  //marketplace
				GH.pushInt(new CMarketplaceWindow(town, town->visitingHero));
				break;

		case 16: //blacksmith
				enterBlacksmith(town->town->warMachine);
				break;

		case 17:
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

		case 20: //ship at shipyard
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[51]); //Cannot build another boat
				break;

		case 21: //special 2
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

		case 22: //special 3
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
		!vstd::contains(town->builtBuildings, 26)) //hero has grail, but town does not have it
	{
		if(!vstd::contains(town->forbiddenBuildings, 26))
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[597], //Do you wish this to be the permanent home of the Grail?
			                            std::vector<SComponent*>(),
			                            boost::bind(&CCallback::buildBuilding, LOCPLINT->cb, town, 26),
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
	GH.pushInt(new CHallInterface(LOCPLINT->castleInt));
}

CCastleInterface::CCastleInterface(const CGTownInstance * Town, int listPos)
:hslotup(241,387,0,Town->garrisonHero,this),hslotdown(241,483,1,Town->visitingHero,this)
{
	bars = CDefHandler::giveDefEss("TPTHBAR.DEF");
	status = CDefHandler::giveDefEss("TPTHCHK.DEF");
	LOCPLINT->castleInt = this;
	hall = NULL;
	fort = NULL;
	market = NULL;
	townInt = BitmapHandler::loadBitmap("TOWNSCRN.bmp");
	pos.x = screen->w/2 - 400;
	pos.y = screen->h/2 - 300;
	hslotup.pos.x += pos.x;
	hslotup.pos.y += pos.y;
	hslotdown.pos.x += pos.x;
	hslotdown.pos.y += pos.y;
	town = Town;
	winMode = 1;

	//garrison
	garr = new CGarrisonInt(pos.x+305,pos.y+387,4,Point(0,96),townInt,Point(62,374),town->getUpperArmy(),town->visitingHero);

	townlist = new CTownList(3,pos.x+744,pos.y+414,"IAM014.DEF","IAM015.DEF");//744,526);
	exit = new AdventureMapButton
		(CGI->generaltexth->tcommands[8],"",boost::bind(&CCastleInterface::close,this),pos.x+744,pos.y+544,"TSBTNS.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	split = new AdventureMapButton(CGI->generaltexth->tcommands[3],"",boost::bind(&CGarrisonInt::splitClick,garr),pos.x+744,pos.y+382,"TSBTNS.DEF");
	split->callback += boost::bind(&CCastleInterface::splitClicked,this);
	garr->addSplitBtn(split);
	statusbar = new CStatusBar(pos.x+7,pos.y+555,"TSTATBAR.bmp",732);
	resdatabar = new CResDataBar("ZRESBAR.bmp",pos.x+3,pos.y+575,32,2,85,85);

	townlist->fun = boost::bind(&CCastleInterface::townChange,this);
	//townlist->genList();
	townlist->selected = vstd::findPos(LOCPLINT->towns,Town);

	townlist->from = townlist->selected - listPos;
	amax(townlist->from, 0);
	amin(townlist->from, LOCPLINT->towns.size() - townlist->SIZE);

	graphics->blueToPlayersAdv(townInt,LOCPLINT->playerID);
	exit->setOffset(4);
	//growth icons and buildings
	builds = new CCastleBuildings(town);
	recreateIcons();
	bicons = CDefHandler::giveDefEss(graphics->buildingPics[town->subID]);
	CCS->musich->playMusic(CCS->musich->townMusics[town->subID], -1);
}

CCastleInterface::~CCastleInterface()
{
	delete bars;
	delete status;
	SDL_FreeSurface(townInt);
	delete exit;
	//delete split;
	delete hall;
	delete fort;
	delete market;
	delete garr;
	delete townlist;
	delete statusbar;
	delete resdatabar;
	delete builds;
	delete bicons;
	for(size_t i=0;i<creainfo.size();i++)
	{
		delete creainfo[i];
	}
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
	LOCPLINT->castleInt = NULL;
	GH.popIntTotally(this);
	CCS->musich->stopMusic(5000);
}

void CCastleInterface::castleTeleport(int where)
{
	const CGTownInstance * dest = LOCPLINT->cb->getTownInfo(where, 1);
	LOCPLINT->cb->teleportHero(town->visitingHero, dest);
}

void CCastleInterface::showAll( SDL_Surface * to/*=NULL*/)
{
	blitAt(townInt,pos.x,pos.y+374,to);
	adventureInt->resdatabar.draw(to);
	townlist->draw(to);
	statusbar->show(to);
	resdatabar->draw(to);

	garr->showAll(to);
	//draw creatures icons and their growths
	for(size_t i=0;i<creainfo.size();i++)
		creainfo[i]->show(to);

	//print name
	CSDL_Ext::printAt(town->name,pos.x+85,pos.y+387,FONT_MEDIUM,zwykly,to);
	//blit town icon
	int pom = town->subID*2;
	if (!town->hasFort())
		pom += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(graphics->bigTownPic->ourImages[pom].bitmap,pos.x+15,pos.y+387,to);

	hslotup.show(to);
	hslotdown.show(to);
	market->show(to);
	fort->show(to);
	hall->show(to);
	builds->showAll(to);
	statusbar->show(to);//refreshing statusbar
	if(screen->w != 800 || screen->h !=600)
		CMessage::drawBorder(LOCPLINT->playerID,to,828,628,pos.x-14,pos.y-15);
	exit->show(to);
	//split->show(to);
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

void CCastleInterface::show(SDL_Surface * to)
{
	//blit buildings
	builds->show(to);
	statusbar->show(to);//refreshing statusbar
}

void CCastleInterface::activate()
{
	townlist->activate();
	garr->activate();
	GH.statusbar = statusbar;
	exit->activate();
	fort->activate();
	hall->activate();
	market->activate();
	//split->activate();
	builds->activate();
	for(size_t i=0;i<creainfo.size();i++)
		creainfo[i]->activate();
	hslotdown.activate();
	hslotup.activate();
	activateKeys();
}

void CCastleInterface::deactivate()
{
	townlist->deactivate();
	garr->deactivate();
	exit->deactivate();
	fort->deactivate();
	hall->deactivate();
	market->deactivate();
	//split->deactivate();
	builds->deactivate();
	for(size_t i=0;i<creainfo.size();i++)
		creainfo[i]->deactivate();
	hslotdown.deactivate();
	hslotup.deactivate();
	deactivateKeys();
}

void CCastleInterface::addBuilding(int bid)
{
	if ( winMode == 2 )//we will only build buildings, no need to update interface - it will be closed in a moment
		return;
	deactivate();
	builds->addBuilding(bid);
	recreateIcons();
	activate();
}

void CCastleInterface::removeBuilding(int bid)
{
	builds->removeBuilding(bid);
	recreateIcons();
}

void CCastleInterface::recreateIcons()
{
	delete fort;
	delete hall;
	delete market;

	hall = new CTownInfo(0);
	fort = new CTownInfo(1);
	market = new CTownInfo(2);

	for(size_t i=0;i<creainfo.size();i++)
	{
		if(active)
			creainfo[i]->deactivate();
		delete creainfo[i];
	}
	creainfo.clear();
	for(size_t i=0;i<CREATURES_PER_TOWN;i++)
	{
		int crid = -1;
		int bid = 30+i;
		if (town->builtBuildings.find(bid)!=town->builtBuildings.end())
		{
			if (town->builtBuildings.find(bid+CREATURES_PER_TOWN)!=town->builtBuildings.end())
			{
				crid = town->town->upgradedCreatures[i];
				bid += CREATURES_PER_TOWN;
			}
			else
				crid = town->town->basicCreatures[i];
		}
		if (crid>=0)
			creainfo.push_back(new CCreaInfo(crid,(bid-30)%CREATURES_PER_TOWN));
	}
	if(town->subID == 5 && vstd::contains(town->builtBuildings, 22) && //we have Portal of Summoning
			!town->creatures[CREATURES_PER_TOWN].second.empty()) // with some creatures in it
		creainfo.push_back(new CCreaInfo(town->creatures[CREATURES_PER_TOWN].second[0], CREATURES_PER_TOWN));
}

CCastleInterface::CCreaInfo::~CCreaInfo()
{
}
CCastleInterface::CCreaInfo::CCreaInfo(int CRID, int LVL)
{
	used = LCLICK | RCLICK | HOVER;
	CCastleInterface * ci=LOCPLINT->castleInt;
	level = LVL;
	crid = CRID;
	pos.x = ci->pos.x+14+(55*(level%4));
	pos.y = (level>3)?(507+ci->pos.y):(459+ci->pos.y);
	pos.w = 48;
	pos.h = 48;
}

void CCastleInterface::CCreaInfo::hover(bool on)
{
	if(on)
	{
		std::string descr=CGI->generaltexth->allTexts[588];
		boost::algorithm::replace_first(descr,"%s",CGI->creh->creatures[crid]->namePl);
		GH.statusbar->print(descr);
	}
	else
		GH.statusbar->clear();
}

void CCastleInterface::CCreaInfo::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
		LOCPLINT->castleInt->builds->enterDwelling(level);
}

int CCastleInterface::CCreaInfo::AddToString(std::string from, std::string & to, int numb)
{
	if (!numb)
		return 0;//do not add string if 0
	boost::algorithm::replace_first(from,"%+d", (numb > 0 ? "+" : "")+boost::lexical_cast<std::string>(numb)); //negative values don't need "+"
	to+="\n"+from;
	return numb;
}

void CCastleInterface::CCreaInfo::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		CCastleInterface * ci=LOCPLINT->castleInt;
		std::set<si32> bld = ci->town->builtBuildings;
		int summ=0;
		std::string descr=CGI->generaltexth->allTexts[589];//Growth of creature is number
		boost::algorithm::replace_first(descr,"%s",CGI->creh->creatures[crid]->nameSing);
		boost::algorithm::replace_first(descr,"%d", boost::lexical_cast<std::string>(ci->town->creatureGrowth(level)));

		descr +="\n"+CGI->generaltexth->allTexts[590];
		summ = CGI->creh->creatures[crid]->growth;
		boost::algorithm::replace_first(descr,"%d", boost::lexical_cast<std::string>(summ));

		if ( level>=0 && level<CREATURES_PER_TOWN)
		{

			if ( bld.find(9)!=bld.end())//castle +100% to basic
				summ+=AddToString(CGI->buildh->buildings[ci->town->subID][9]->Name()+" %+d",descr,summ);
			else if ( bld.find(8)!=bld.end())//else if citadel+50% to basic
				summ+=AddToString(CGI->buildh->buildings[ci->town->subID][8]->Name()+" %+d",descr,summ/2);

			summ+=AddToString(CGI->generaltexth->allTexts[63] + " %+d",descr, //double growth or plague
				summ * CGI->creh->creatures[crid]->valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100);

			summ+=AddToString(CGI->generaltexth->artifNames[133] + " %+d",descr,
				summ * ci->town->valOfGlobalBonuses
				(Selector::type(Bonus::CREATURE_GROWTH_PERCENT) && Selector::sourceType(Bonus::ARTIFACT))/100); //Statue of Legion

			if(ci->town->town->hordeLvl[0]==level)//horde, x to summ
			if((bld.find(18)!=bld.end()) || (bld.find(19)!=bld.end()))
				summ+=AddToString(CGI->buildh->buildings[ci->town->subID][18]->Name()+" %+d",descr,
					CGI->creh->creatures[crid]->hordeGrowth);

			if(ci->town->town->hordeLvl[1]==level)//horde, x to summ
			if((bld.find(24)!=bld.end()) || (bld.find(25)!=bld.end()))
				summ+=AddToString(CGI->buildh->buildings[ci->town->subID][24]->Name()+" %+d",descr,
					CGI->creh->creatures[crid]->hordeGrowth);

			int cnt = 0;

			std::vector< const CGDwelling * > myDwellings = LOCPLINT->cb->getMyDwellings();
			for (std::vector<const CGDwelling*>::const_iterator it = myDwellings.begin(); it != myDwellings.end(); ++it)
				if (CGI->creh->creatures[ci->town->town->basicCreatures[level]]->idNumber == (*it)->creatures[0].second[0])
					cnt++;//external dwellings count to summ
			summ+=AddToString(CGI->generaltexth->allTexts[591],descr,cnt);

			const CGHeroInstance * ch = ci->town->garrisonHero;
			BonusList bl;
			for (cnt = 0; cnt<2; cnt++) // "loop" to avoid copy-pasting code
			{
				if(ch)
				{
					ch->getBonuses(bl, Selector::type(Bonus::CREATURE_GROWTH) && Selector::subtype(level) && Selector::sourceType(Bonus::ARTIFACT), ch);
				};
				ch = ci->town->visitingHero;
			};
			if (bl.size())
				summ+=AddToString (CGI->arth->artifacts[bl.front()->sid]->Name()+" %+d", descr, bl.totalValue());

			//TODO: player bonuses

			if(bld.find(26)!=bld.end()) //grail - +50% to ALL growth
				summ+=AddToString(CGI->buildh->buildings[ci->town->subID][26]->Name()+" %+d",descr,summ/2);

			summ+=AddToString(CGI->generaltexth->allTexts[63] + " %+d",descr, CGI->creh->creatures[crid]->valOfBonuses(Bonus::CREATURE_GROWTH));
		}

		CInfoPopup *mess = new CInfoPopup();//creating popup
		mess->free = true;
		mess->bitmap = CMessage::drawBoxTextBitmapSub
		(LOCPLINT->playerID, descr,graphics->bigImgs[crid],"");
		mess->pos.x = screen->w/2 - mess->bitmap->w/2;
		mess->pos.y = screen->h/2 - mess->bitmap->h/2;
		GH.pushInt(mess);
	}
}

void CCastleInterface::CCreaInfo::show(SDL_Surface * to)
{
	blitAt(graphics->smallImgs[crid],pos.x+8,pos.y,to);
	std::string str = "+" + boost::lexical_cast<std::string>(LOCPLINT->castleInt->town->creatureGrowth(level));
	CSDL_Ext::printAtMiddle(str, pos.x+24, pos.y+40, FONT_SMALL, zwykly, to);
}

CCastleInterface::CTownInfo::~CTownInfo()
{
	delete pic;
}

CCastleInterface::CTownInfo::CTownInfo(int BID)
{
	used = LCLICK | RCLICK | HOVER;
	CCastleInterface * ci=LOCPLINT->castleInt;
	switch (BID)
	{
		case 0:	//hall
			bid = 10 + ci->town->hallLevel();
			pos.x = ci->pos.x+80; pos.y = ci->pos.y+413; pos.w=40; pos.h=40;
			pic = CDefHandler::giveDef("ITMTL.DEF");
			break;
		case 1: //fort
			bid = 6 + ci->town->fortLevel();
			pos.x = ci->pos.x+122; pos.y = ci->pos.y+413; pos.w=40; pos.h=40;
			pic = CDefHandler::giveDef("ITMCL.DEF");
			break;
		case 2:	pos.x = ci->pos.x+164;pos.y = ci->pos.y+409; pos.w=64; pos.h=44;
			pic = NULL;
			bid = 14;
			break;
	}
}

void CCastleInterface::CTownInfo::hover(bool on)
{
	if(on)
	{
		std::string descr;
		if ( bid == 6 ) {} //empty "no fort" icon. no hover message
		else
		if ( bid == 14 ) //marketplace/income icon
			descr = CGI->generaltexth->allTexts[255];
		else
			descr = CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][bid]->Name();
		GH.statusbar->print(descr);
	}
	else
		GH.statusbar->clear();
}

void CCastleInterface::CTownInfo::clickLeft(tribool down, bool previousState)
{/*
	if(previousState && (!down))
		if (LOCPLINT->castleInt->town->builtBuildings.find(bid)!=LOCPLINT->castleInt->town->builtBuildings.end())
			LOCPLINT->castleInt->buildingClicked(bid);//activate building*/
}

void CCastleInterface::CTownInfo::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		if (( bid == 6 ) || ( bid == 14) )
			return;
		CInfoPopup *mess = new CInfoPopup();
		mess->free = true;
		CCastleInterface * ci=LOCPLINT->castleInt;
		const CBuilding *bld = CGI->buildh->buildings[ci->town->subID][bid];
		mess->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,bld->Description(),
			LOCPLINT->castleInt->bicons->ourImages[bid].bitmap,
			bld->Name());
		mess->pos.x = screen->w/2 - mess->bitmap->w/2;
		mess->pos.y = screen->h/2 - mess->bitmap->h/2;
		GH.pushInt(mess);
	}
}

void CCastleInterface::CTownInfo::show(SDL_Surface * to)
{
	if ( bid == 14 )//marketplace/income
	{
		std::string str = boost::lexical_cast<std::string>(LOCPLINT->castleInt->town->dailyIncome());
		CSDL_Ext::printAtMiddle(str, pos.x+33, pos.y+34, FONT_SMALL, zwykly, to);
	}
	else if ( bid == 6 )//no fort
		blitAt(pic->ourImages[3].bitmap,pos.x,pos.y,to);
	else if (bid < 10)//fort-castle
		blitAt(pic->ourImages[bid-7].bitmap,pos.x,pos.y,to);
	else//town halls
		blitAt(pic->ourImages[bid-10].bitmap,pos.x,pos.y,to);
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

void CCastleInterface::splitClicked()
{
	if(!!town->visitingHero && town->garrisonHero && (hslotdown.highlight || hslotup.highlight))
	{
		LOCPLINT->heroExchangeStarted(town->visitingHero->id, town->garrisonHero->id);
	}
}

void CHallInterface::CBuildingBox::hover(bool on)
{
	//Hoverable::hover(on);
	if(on)
	{
		std::string toPrint;
		if(state==8)
			toPrint = CGI->generaltexth->hcommands[5];
		else if(state==5)//"already builded today" message
			toPrint = CGI->generaltexth->allTexts[223];
		else
			toPrint = CGI->generaltexth->hcommands[state];
		std::vector<std::string> name;
		name.push_back(CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][BID]->Name());
		GH.statusbar->print(CSDL_Ext::processStr(toPrint,name));
	}
	else
		GH.statusbar->clear();
}
void CHallInterface::CBuildingBox::clickLeft(tribool down, bool previousState)
{
	if(previousState && (!down))
	{
		GH.pushInt(new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,0));
	}
	//ClickableL::clickLeft(down);
}
void CHallInterface::CBuildingBox::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		GH.pushInt(new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,1));
	}
	//ClickableR::clickRight(down);
}
void CHallInterface::CBuildingBox::show(SDL_Surface * to)
{
	CCastleInterface *ci = LOCPLINT->castleInt;
	if (( (BID == 18) && (vstd::contains(ci->town->builtBuildings,(ci->town->town->hordeLvl[0]+37))))
	||  ( (BID == 24) && (vstd::contains(ci->town->builtBuildings,(ci->town->town->hordeLvl[1]+37)))) )
		blitAt(ci->bicons->ourImages[BID+1].bitmap,pos.x,pos.y,to);
	else
		blitAt(ci->bicons->ourImages[BID].bitmap,pos.x,pos.y,to);
	int pom, pom2=-1;
	switch (state)
	{
	case 4:
		pom = 0;
		pom2 = 0;
		break;
	case 7:
		pom = 1;
		break;
	case 6:
		pom2 = 2;
		pom = 2;
		break;
	case 5: case 8:
		pom2 = 1;
		pom = 2;
		break;
	case 0: case 2: case 1: default:
		pom = 3;
		break;
	}
	blitAt(ci->bars->ourImages[pom].bitmap,pos.x-1,pos.y+71,to);
	if(pom2>=0)
		blitAt(ci->status->ourImages[pom2].bitmap,pos.x+135, pos.y+54,to);
	CSDL_Ext::printAtMiddle(CGI->buildh->buildings[ci->town->subID][BID]->Name(),pos.x-1+ci->bars->ourImages[0].bitmap->w/2,pos.y+71+ci->bars->ourImages[0].bitmap->h/2, FONT_SMALL,zwykly,to);
}
CHallInterface::CBuildingBox::~CBuildingBox()
{
}
CHallInterface::CBuildingBox::CBuildingBox(int id)
	:BID(id)
{
	used = LCLICK | RCLICK | HOVER;
	pos.w = 150;
	pos.h = 88;
}
CHallInterface::CBuildingBox::CBuildingBox(int id, int x, int y)
	:BID(id)
{
	used = LCLICK | RCLICK | HOVER;
	pos.x = x;
	pos.y = y;
	pos.w = 150;
	pos.h = 88;
}


CHallInterface::CHallInterface(CCastleInterface * owner)
{
	resdatabar = new CMinorResDataBar;
	pos = owner->pos;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	LOCPLINT->castleInt->statusbar->clear();
	bg = BitmapHandler::loadBitmap(CGI->buildh->hall[owner->town->subID].first);
	bid = owner->town->hallLevel()+10;
	graphics->blueToPlayersAdv(bg,LOCPLINT->playerID);
	exit = new AdventureMapButton
		(CGI->generaltexth->hcommands[8],"",boost::bind(&CHallInterface::close,this),pos.x+748,pos.y+556,"TPMAGE1.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);

	//preparing boxes with buildings//
	boxes.resize(5);
	for(size_t i=0;i<5;i++) //for each row
	{
		const std::vector< std::vector< std::vector<int> > > &boxList = CGI->buildh->hall[owner->town->subID].second;

		for(size_t j=0; j<boxList[i].size();j++) //for each box
		{
			size_t k=0;

			for(;k<boxList[i][j].size();k++)//we are looking for the first not build structure
			{
				int bid = boxList[i][j][k];
				//if building not build or this is unupgraded horde
				if(!vstd::contains(owner->town->builtBuildings,bid) || bid==18 || bid == 24)
				{
					int x = 34 + 194*j,
						y = 37 + 104*i,
						ID = bid;

					if (( bid == 18 && vstd::contains(owner->town->builtBuildings, owner->town->town->hordeLvl[0]+37))
					 || ( bid == 24 && vstd::contains(owner->town->builtBuildings, owner->town->town->hordeLvl[1]+37)))
						continue;//we have upgraded dwelling, horde description should be for upgraded creatures

					if(boxList[i].size() == 2) //only two boxes in this row
						x+=194;
					else if(boxList[i].size() == 3) //only three boxes in this row
						x+=97;
					boxes[i].push_back(new CBuildingBox(bid,pos.x+x,pos.y+y));
					boxes[i].back()->state = LOCPLINT->cb->canBuildStructure(owner->town,ID);
					break;
				}
			}
			if(k == boxList[i][j].size()) //all buildings built - let's take the last one
			{
				int x = 34 + 194*j,
					y = 37 + 104*i;
				if(boxList[i].size() == 2)
					x+=194;
				else if(boxList[i].size() == 3)
					x+=97;
				boxes[i].push_back(new CBuildingBox(boxList[i][j][k-1],pos.x+x,pos.y+y));
				boxes[i][boxes[i].size()-1]->state = 4; //already exists
			}
		}
	}
}
CHallInterface::~CHallInterface()
{
	SDL_FreeSurface(bg);
	for(size_t i=0;i<boxes.size();i++)
		for(size_t j=0;j<boxes[i].size();j++)
			delete boxes[i][j]; //TODO whats wrong with smartpointers?
	delete exit;
	delete resdatabar;
}
void CHallInterface::close()
{
	GH.popInts(LOCPLINT->castleInt->winMode == 2? 2 : 1 );
}
void CHallInterface::showAll(SDL_Surface * to)
{
	blitAt(bg,pos,to);
	LOCPLINT->castleInt->statusbar->show(to);
	printAtMiddleLoc(CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][bid]->Name(), 399, 12, FONT_MEDIUM,zwykly,to);
	resdatabar->show(to);
	exit->showAll(to);
	for(int i=0; i<5; i++)
	{
		for(size_t j=0;j<boxes[i].size(); ++j)
			boxes[i][j]->show(to);
	}
}
void CHallInterface::activate()
{
	for(int i=0;i<5;i++)
	{
		for(size_t j=0; j < boxes[i].size(); ++j)
		{
			boxes[i][j]->activate();
		}
	}
	exit->activate();
}
void CHallInterface::deactivate()
{
	for(int i=0;i<5;i++)
	{
		for(size_t j=0;j<boxes[i].size();++j)
		{
			boxes[i][j]->deactivate();
		}
	}
	exit->deactivate();
}

void CHallInterface::CBuildWindow::activate()
{
	activateRClick();
	if(mode)
		return;
	if(state==7)
		buy->activate();
	cancel->activate();
}

void CHallInterface::CBuildWindow::deactivate()
{
	deactivateRClick();
	if(mode)
		return;
	if(state==7)
		buy->deactivate();
	cancel->deactivate();
}

void CHallInterface::CBuildWindow::Buy()
{
	int building = bid;
	LOCPLINT->cb->buildBuilding(LOCPLINT->castleInt->town,building);
	GH.popInts(LOCPLINT->castleInt->winMode == 2? 3 : 2 ); //we - build window and hall screen
}

void CHallInterface::CBuildWindow::close()
{
	GH.popIntTotally(this);
}

void CHallInterface::CBuildWindow::clickRight(tribool down, bool previousState)
{
	if((!down || indeterminate(down)) && mode)
		close();
}

void CHallInterface::CBuildWindow::show(SDL_Surface * to)
{
	SDL_Rect pom = genRect(bitmap->h-1,bitmap->w-1,pos.x,pos.y);
	SDL_Rect poms = pom; poms.x=0;poms.y=0;
	SDL_BlitSurface(bitmap,&poms,to,&pom);
	if(!mode)
	{
		buy->showAll(to);
		cancel->showAll(to);
	}
}

std::string CHallInterface::CBuildWindow::getTextForState(int state)
{
	std::string ret;
	if(state<7)
		ret =  CGI->generaltexth->hcommands[state];
	switch (state)
	{
	case 4:	case 5: case 6:
		ret.replace(ret.find_first_of("%s"),2,CGI->buildh->buildings[tid][bid]->Name());
		break;
	case 7:
		return CGI->generaltexth->allTexts[219]; //all prereq. are met
	case 8:
		{
			ret = CGI->generaltexth->allTexts[52];
			std::set<int> reqs= LOCPLINT->cb->getBuildingRequiments(LOCPLINT->castleInt->town, bid);

			bool first=true;
			for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
			{
				if (vstd::contains(LOCPLINT->castleInt->town->builtBuildings, *i))
					continue;//skipping constructed buildings
				ret+=(((first)?(" "):(", ")) + CGI->buildh->buildings[tid][*i]->Name());
				first = false;//TODO - currently can return "Mage guild lvl 1, MG lvl 2..." - extra check needed
			}
		}
	}

	return ret;
}

CHallInterface::CBuildWindow::CBuildWindow(int Tid, int Bid, int State, bool Mode)
:tid(Tid), bid(Bid), state(State), mode(Mode)
{
	SDL_Surface *hhlp = BitmapHandler::loadBitmap("TPUBUILD.bmp");
	graphics->blueToPlayersAdv(hhlp,LOCPLINT->playerID);
	bitmap = SDL_ConvertSurface(hhlp,screen->format,0); //na 8bitowej mapie by sie psulo
	SDL_SetColorKey(hhlp,SDL_SRCCOLORKEY,SDL_MapRGB(hhlp->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = screen->w/2 - bitmap->w/2;
	pos.y = screen->h/2 - bitmap->h/2;
	blitAt(LOCPLINT->castleInt->bicons->ourImages[bid].bitmap,125,50,bitmap);
	std::vector<std::string> pom; pom.push_back(CGI->buildh->buildings[tid][bid]->Name());
	CSDL_Ext::printAtMiddleWB(CGI->buildh->buildings[tid][bid]->Description(),197,168,FONT_MEDIUM,43,zwykly,bitmap);
	CSDL_Ext::printAtMiddleWB(getTextForState(state),199,248,FONT_SMALL,50,zwykly,bitmap);
	CSDL_Ext::printAtMiddle(CSDL_Ext::processStr(CGI->generaltexth->hcommands[7],pom),197,30,FONT_BIG,tytulowy,bitmap);

	int resamount=0;
	for(int i=0;i<7;i++)
	{
		if(CGI->buildh->buildings[tid][bid]->resources[i])
		{
			resamount++;
		}
	}
	int ah = (resamount>4) ? 304 : 340;
	int cn=-1, it=0;
	int row1w = std::min(resamount,4) * 32 + (std::min(resamount,4)-1) * 45,
		row2w = (resamount-4) * 32 + (resamount-5) * 45;
	char buf[15];
	while(++cn<7)
	{
		if(!CGI->buildh->buildings[tid][bid]->resources[cn])
			continue;
		SDL_itoa(CGI->buildh->buildings[tid][bid]->resources[cn],buf,10);
		if(it<4)
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row1w/2)+77*it+16,ah+47,FONT_SMALL,zwykly,bitmap);
			blitAt(graphics->resources32->ourImages[cn].bitmap,(bitmap->w/2-row1w/2)+77*it++,ah,bitmap);
		}
		else
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row2w/2)+77*it+16-308,ah+47,FONT_SMALL,zwykly,bitmap);
			blitAt(graphics->resources32->ourImages[cn].bitmap,(bitmap->w/2-row2w/2)+77*it++ - 308,ah,bitmap);
		}
		if(it==4)
			ah+=75;
	}
	if(!mode)
	{
		buy = new AdventureMapButton
			("","",boost::bind(&CBuildWindow::Buy,this),pos.x+45,pos.y+446,"IBUY30.DEF",SDLK_RETURN);
		cancel = new AdventureMapButton
			("","",boost::bind(&CBuildWindow::close,this),pos.x+290,pos.y+445,"ICANCEL.DEF",SDLK_ESCAPE);
		if(state!=7)
			buy->setState(CButtonBase::BLOCKED);
	}
}

CHallInterface::CBuildWindow::~CBuildWindow()
{
	SDL_FreeSurface(bitmap);
	if(!mode)
	{
		delete buy;
		delete cancel;
	}
}

CFortScreen::~CFortScreen()
{
	for(size_t i=0;i<crePics.size();i++)
		delete crePics[i];
	for (size_t i=0;i<recAreas.size();i++)
		delete recAreas[i];
	SDL_FreeSurface(bg);
	delete exit;
	delete resdatabar;
}

void CFortScreen::show( SDL_Surface * to)
{
	for (int i=0; i<crePics.size(); i++)
	{
		crePics[i]->show(to);
	}
}

void CFortScreen::showAll( SDL_Surface * to)
{
	blitAt(bg,pos,to);
	for (int i=0; i<crePics.size(); i++)
	{
		crePics[i]->showAll(to);
	}
	exit->showAll(to);
	resdatabar->show(to);
	GH.statusbar->show(to);
	
}

void CFortScreen::activate()
{
	GH.statusbar = LOCPLINT->castleInt->statusbar;
	exit->activate();
	for (size_t i=0;i<recAreas.size(); ++i)
	{
		recAreas[i]->activate();
	}
}

void CFortScreen::deactivate()
{
	exit->deactivate();
	for (size_t i=0;i<recAreas.size();i++)
	{
		recAreas[i]->deactivate();
	}
}

void CFortScreen::close()
{
	GH.popInts(LOCPLINT->castleInt->winMode == 3? 2 : 1 );
}

CFortScreen::CFortScreen( CCastleInterface * owner )
{
	if (owner->town->creatures.size() > CREATURES_PER_TOWN
	        && owner->town->creatures[CREATURES_PER_TOWN].second.size() )//dungeon with active portal
		fortSize = CREATURES_PER_TOWN+1;
	else
		fortSize = CREATURES_PER_TOWN;
	resdatabar = new CMinorResDataBar;
	pos = owner->pos;
	bg = NULL;
	LOCPLINT->castleInt->statusbar->clear();
	std::string temp = CGI->generaltexth->fcommands[6];
	boost::algorithm::replace_first(temp,"%s",CGI->buildh->buildings[owner->town->subID][owner->town->fortLevel()+6]->Name());
	exit = new AdventureMapButton(temp,"",boost::bind(&CFortScreen::close,this),pos.x+748,pos.y+556,"TPMAGE1.DEF",SDLK_RETURN);
	positions += genRect(126,386,10,22),genRect(126,386,404,22),
		genRect(126,386,10,155),genRect(126,386,404,155),
		genRect(126,386,10,288),genRect(126,386,404,288);
	if (fortSize == CREATURES_PER_TOWN)
		positions += genRect(126,386,206,421);
	else
		positions += genRect(126,386,10,421),genRect(126,386,404,421);
	draw(owner,true);
	resdatabar->pos += pos;
}

void CFortScreen::draw( CCastleInterface * owner, bool first)
{
	if(bg)
		SDL_FreeSurface(bg);
	char buf[20];
	memset(buf,0,20);
	SDL_Surface *bg2;
	if (fortSize == CREATURES_PER_TOWN)
		bg2 = BitmapHandler::loadBitmap("TPCASTL7.bmp");
	else
		bg2 = BitmapHandler::loadBitmap("TPCASTL8.bmp");

	SDL_Surface *icons =  BitmapHandler::loadBitmap("ZPCAINFO.bmp");
	SDL_SetColorKey(icons,SDL_SRCCOLORKEY,SDL_MapRGB(icons->format,0,255,255));
	graphics->blueToPlayersAdv(bg2,LOCPLINT->playerID);
	bg = SDL_ConvertSurface(bg2,screen->format,0);
	SDL_FreeSurface(bg2);
	CSDL_Ext::printAtMiddle(CGI->buildh->buildings[owner->town->subID][owner->town->fortLevel()+6]->Name(),400,13,FONT_MEDIUM,zwykly,bg);
	for(int i=0;i<fortSize; i++)
	{
		int dwelling;// ID of buiding with this creature
		const CCreature *c;
		bool present = true;
		if ( i < CREATURES_PER_TOWN )
		{
			bool upgraded = owner->town->creatureDwelling(i,true);
			present = owner->town->creatureDwelling(i,false);
			c = CGI->creh->creatures[upgraded ? owner->town->town->upgradedCreatures[i] : owner->town->town->basicCreatures[i]];
			dwelling = 30+i+upgraded*7;
		}
		else
		{
			c = CGI->creh->creatures[owner->town->creatures[i].second[0]];
			dwelling = 22;//Portal of summon
		}

		CSDL_Ext::printAtMiddle(c->namePl,positions[i].x+79,positions[i].y+10,FONT_SMALL,zwykly,bg); //cr. name
		blitAt(owner->bicons->ourImages[dwelling].bitmap,positions[i].x+4,positions[i].y+21,bg); //dwelling pic
		CSDL_Ext::printAtMiddle(CGI->buildh->buildings[owner->town->subID][dwelling]->Name(),positions[i].x+79,positions[i].y+100,FONT_SMALL,zwykly,bg); //dwelling name
		if(present) //if creature is present print available quantity
		{
			SDL_itoa(owner->town->creatures[i].first,buf,10);
			CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[217] + buf,positions[i].x+79,positions[i].y+118,FONT_SMALL,zwykly,bg);
		}
		blitAt(icons,positions[i].x+261,positions[i].y+3,bg);

		//attack
		CSDL_Ext::printAt(CGI->generaltexth->allTexts[190],positions[i].x+288,positions[i].y+5,FONT_SMALL,zwykly,bg);
		SDL_itoa(c->Attack(),buf,10);
		CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+21,FONT_SMALL,zwykly,bg);

		//defense
		CSDL_Ext::printAt(CGI->generaltexth->allTexts[191],positions[i].x+288,positions[i].y+25,FONT_SMALL,zwykly,bg);
		SDL_itoa(c->Defense(),buf,10);
		CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+41,FONT_SMALL,zwykly,bg);

		//damage
		CSDL_Ext::printAt(CGI->generaltexth->allTexts[199],positions[i].x+288,positions[i].y+46,FONT_SMALL,zwykly,bg);
		SDL_itoa(c->damageMin,buf,10);
		int hlp;
		if(c->damageMin > 0)
			hlp = log10f(c->damageMin)+2;
		else
			hlp = 2;
		buf[hlp-1]=' '; buf[hlp]='-'; buf[hlp+1]=' ';
		SDL_itoa(c->damageMax,buf+hlp+2,10);
		CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+62,FONT_SMALL,zwykly,bg);

		//health
		CSDL_Ext::printAt(CGI->generaltexth->zelp[439].first,positions[i].x+288,positions[i].y+66,FONT_SMALL,zwykly,bg);
		SDL_itoa(c->MaxHealth(),buf,10);
		CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+82,FONT_SMALL,zwykly,bg);

		//speed
		CSDL_Ext::printAt(CGI->generaltexth->zelp[441].first,positions[i].x+288,positions[i].y+87,FONT_SMALL,zwykly,bg);
		SDL_itoa(c->valOfBonuses(Bonus::STACKS_SPEED), buf,10);
		CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+103,FONT_SMALL,zwykly,bg);

		if(present)//growth
		{
			CSDL_Ext::printAt(CGI->generaltexth->allTexts[194],positions[i].x+288,positions[i].y+107,FONT_SMALL,zwykly,bg);
			SDL_itoa(owner->town->creatureGrowth(i),buf,10);
			CSDL_Ext::printTo(buf,positions[i].x+381,positions[i].y+123,FONT_SMALL,zwykly,bg);
		}
		if(first)
		{
			crePics.push_back(new CCreaturePic( positions[i].x+pos.x+160, positions[i].y+pos.y+5, c,false));
			if(present)
			{
				recAreas.push_back(new RecArea(i));
				recAreas.back()->pos = positions[i] + pos;
			}
		}
	}
	SDL_FreeSurface(icons);
}
void CFortScreen::RecArea::clickLeft(tribool down, bool previousState)
{
	if(!down && previousState)
		LOCPLINT->castleInt->builds->enterDwelling(level);
}

void CFortScreen::RecArea::clickRight(tribool down, bool previousState)
{
	clickLeft(down, false); //r-click does same as l-click - opens recr. window
}
CMageGuildScreen::CMageGuildScreen(CCastleInterface * owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	
	bg = new CPicture("TPMAGE.bmp");
	pos = bg->center();
	resdatabar = new CMinorResDataBar;
	resdatabar->pos.x += pos.x;
	resdatabar->pos.y += pos.y;
	LOCPLINT->castleInt->statusbar->clear();
	
	exit = new AdventureMapButton(CGI->generaltexth->allTexts[593],"",boost::bind(&CMageGuildScreen::close,this), 748, 556,"TPMAGE1.DEF",SDLK_RETURN);
	exit->assignedKeys.insert(SDLK_ESCAPE);
	CAnimation scrolls("TPMAGES.DEF");
	scrolls.load();
	
	SDL_Surface *view = BitmapHandler::loadBitmap(graphics->guildBgs[owner->town->subID]);
	SDL_SetColorKey(view,SDL_SRCCOLORKEY,SDL_MapRGB(view->format,0,255,255));
	blitAt(view,332,76,*bg);
	SDL_FreeSurface(view);
	
	positions.resize(5);
	positions[0] += genRect(61,83,222,445), genRect(61,83,312,445), genRect(61,83,402,445), genRect(61,83,520,445), genRect(61,83,610,445), genRect(61,83,700,445);
	positions[1] += genRect(61,83,48,53), genRect(61,83,48,147), genRect(61,83,48,241), genRect(61,83,48,335), genRect(61,83,48,429);
	positions[2] += genRect(61,83,570,82), genRect(61,83,672,82), genRect(61,83,570,157), genRect(61,83,672,157);
	positions[3] += genRect(61,83,183,42), genRect(61,83,183,148), genRect(61,83,183,253);
	positions[4] += genRect(61,83,491,325), genRect(61,83,591,325);
	
	for(size_t i=0; i<owner->town->town->mageLevel; i++)
	{
		size_t sp = owner->town->spellsAtLevel(i+1,false); //spell at level with -1 hmmm?
		for(size_t j=0; j<sp; j++)
		{
			if(i<owner->town->mageGuildLevel() && owner->town->spells[i].size()>j)
			{
				spells.push_back( new Scroll(CGI->spellh->spells[owner->town->spells[i][j]]));
				spells[spells.size()-1]->pos = positions[i][j];
				blitAt(graphics->spellscr->ourImages[owner->town->spells[i][j]].bitmap,positions[i][j],*bg);
			}
			else
			{
				scrolls.getImage(0)->draw(*bg, positions[i][j].x, positions[i][j].y);
			}
		}
	}
	
	for(size_t i=0;i<spells.size();i++)
	{
		spells[i]->pos.x += pos.x;
		spells[i]->pos.y += pos.y;
	}
	scrolls.unload();
}

CMageGuildScreen::~CMageGuildScreen()
{
	
}

void CMageGuildScreen::close()
{
	GH.popIntTotally(this);
}

CMageGuildScreen::Scroll::Scroll(const CSpell *Spell)
	:spell(Spell)
{
	used = LCLICK | RCLICK | HOVER;
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
	bmp = new CPicture("TPSMITH");
	bmp->colorizeAndConvert(LOCPLINT->playerID);
	
	pos = bmp->center();
	
	animBG = new CPicture("TPSMITBK", 64, 50);
	animBG->needRefresh = true;

	anim = new CCreatureAnim(64, 50, CGI->creh->creatures[creMachineID]->animDefName, Rect());
	anim->clipRect(113,125,200,150);
	anim->startPreview();
	char pom[75];
	sprintf(pom,CGI->generaltexth->allTexts[274].c_str(),CGI->creh->creatures[creMachineID]->nameSing.c_str()); //build a new ...
	CSDL_Ext::printAtMiddle(pom,165,28,FONT_BIG,tytulowy,*bmp);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->jktexts[43],165,218,FONT_MEDIUM,zwykly,*bmp); //resource cost
	SDL_itoa(CGI->arth->artifacts[aid]->price,pom,10);
	CSDL_Ext::printAtMiddle(pom,165,290,FONT_MEDIUM,zwykly,*bmp);

	buy = new AdventureMapButton("","",boost::bind(&CBlacksmithDialog::close,this), 42, 312,"IBUY30.DEF",SDLK_RETURN);
	cancel = new AdventureMapButton("","",boost::bind(&CBlacksmithDialog::close,this), 224, 312,"ICANCEL.DEF",SDLK_ESCAPE);

	if(possible)
		buy->callback += boost::bind(&CCallback::buyArtifact,LOCPLINT->cb,LOCPLINT->cb->getHeroInfo(hid,2),aid);
	else
		buy->block(2);

	blitAt(graphics->resources32->ourImages[6].bitmap,148,244,*bmp);
}

CBlacksmithDialog::~CBlacksmithDialog()
{
	
}

void CBlacksmithDialog::close()
{
	GH.popIntTotally(this);
}
