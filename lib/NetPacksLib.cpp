#define VCMI_DLL
#include "NetPacks.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CArtHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "VCMI_Lib.h"
#include "map.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CCreatureHandler.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#undef min
#undef max

/*
 * NetPacksLib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

DLL_EXPORT void SetResource::applyGs( CGameState *gs )
{
	assert(player < PLAYER_LIMIT);
	amax(val, 0); //new value must be >= 0
	gs->getPlayer(player)->resources[resid] = val;
}

 DLL_EXPORT void SetResources::applyGs( CGameState *gs )
 {
 	assert(player < PLAYER_LIMIT);
 	for(int i=0;i<res.size();i++)
 		gs->getPlayer(player)->resources[i] = res[i];
 }

DLL_EXPORT void SetPrimSkill::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(id);
	assert(hero);

	if(which <4)
	{
		Bonus *skill = hero->getBonus(Selector::type(Bonus::PRIMARY_SKILL) && Selector::subtype(which) && Selector::sourceType(Bonus::HERO_BASE_SKILL));
		assert(skill);
		
		if(abs)
			skill->val = val;
		else
			skill->val += val;
	}
	else if(which == 4) //XP
	{
		if(abs)
			hero->exp = val;
		else
			hero->exp += val;
	}
}

DLL_EXPORT void SetSecSkill::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(id);
	hero->setSecSkillLevel(which, val, abs);
}

DLL_EXPORT void HeroVisitCastle::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(hid);
	CGTownInstance *t = gs->getTown(tid);
	if(start())
	{
		if(garrison())
		{
			t->garrisonHero = h;
			h->visitedTown = t;
			h->inTownGarrison = true;
		}
		else
		{
			t->visitingHero = h;
			h->visitedTown = t;
			h->inTownGarrison = false;
		}
	}
	else
	{
		if(garrison())
		{
			t->garrisonHero = NULL;
			h->visitedTown = NULL;
			h->inTownGarrison = false;
		}
		else
		{
			t->visitingHero = NULL;
			h->visitedTown = NULL;
			h->inTownGarrison = false;
		}
	}
}

DLL_EXPORT void ChangeSpells::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);

	if(learn)
		BOOST_FOREACH(ui32 sid, spells)
		hero->spells.insert(sid);
	else
		BOOST_FOREACH(ui32 sid, spells)
		hero->spells.erase(sid);
}

DLL_EXPORT void SetMana::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);
	amax(val, 0); //not less than 0
	hero->mana = val;
}

DLL_EXPORT void SetMovePoints::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);
	hero->movement = val;
}

DLL_EXPORT void FoWChange::applyGs( CGameState *gs )
{
	TeamState * team = gs->getPlayerTeam(player);
	BOOST_FOREACH(int3 t, tiles)
		team->fogOfWarMap[t.x][t.y][t.z] = mode;
	if (mode == 0) //do not hide too much
	{
		std::set<int3> tilesRevealed;
		for (size_t i = 0; i < gs->map->objects.size(); i++)
		{
			if (gs->map->objects[i])
			{
				switch(gs->map->objects[i]->ID)
				{
				case 34://hero
				case 53://mine
				case 98://town
				case 220:
					if(vstd::contains(team->players, gs->map->objects[i]->tempOwner)) //check owned observators
						gs->map->objects[i]->getSightTiles(tilesRevealed);
					break;
				}
			}
		}
		BOOST_FOREACH(int3 t, tilesRevealed) //probably not the most optimal solution ever
			team->fogOfWarMap[t.x][t.y][t.z] = 1;
	}
}
DLL_EXPORT void SetAvailableHeroes::applyGs( CGameState *gs )
{
	PlayerState *p = gs->getPlayer(player);
	p->availableHeroes.clear();

	for (int i = 0; i < AVAILABLE_HEROES_PER_PLAYER; i++)
	{
		CGHeroInstance *h = (hid[i]>=0 ?  gs->hpool.heroesPool[hid[i]] : NULL);
		if(h && army[i])
			h->setArmy(*army[i]);
		p->availableHeroes.push_back(h);
	}
}

DLL_EXPORT void GiveBonus::applyGs( CGameState *gs )
{
	BonusList *bonuses = NULL;
	switch(who)
	{
	case HERO:
		{
			CGHeroInstance *h = gs->getHero(id);
			assert(h);
			bonuses = &h->bonuses;
		}
		break;
	case PLAYER:
		{
			PlayerState *p = gs->getPlayer(id);
			assert(p);
			bonuses = &p->bonuses;
		}
		break;
	case TOWN:
		{
			CGTownInstance *t = gs->getTown(id);
			assert(t);
			bonuses = &t->bonuses;
		}
	}

	bonuses->push_back(bonus);
	std::string &descr = bonuses->back().description;

	if(!bdescr.message.size() 
		&& bonus.source == Bonus::OBJECT 
		&& (bonus.type == Bonus::LUCK || bonus.type == Bonus::MORALE)
		&& gs->map->objects[bonus.id]->ID == EVENTI_TYPE) //it's morale/luck bonus from an event without description
	{
		descr = VLC->generaltexth->arraytxt[bonus.val > 0 ? 110 : 109]; //+/-%d Temporary until next battle"
		boost::replace_first(descr,"%d",boost::lexical_cast<std::string>(std::abs(bonus.val)));
	}
	else
	{
		bdescr.toString(descr);
	}
}

DLL_EXPORT void ChangeObjPos::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->map->objects[objid];
	if(!obj)
	{
		tlog1 << "Wrong ChangeObjPos: object " << objid << " doesn't exist!\n";
		return;
	}
	gs->map->removeBlockVisTiles(obj);
	obj->pos = nPos;
	gs->map->addBlockVisTiles(obj);
}

DLL_EXPORT void PlayerEndsGame::applyGs( CGameState *gs )
{
	PlayerState *p = gs->getPlayer(player);
	p->status = victory ? 2 : 1;
}

DLL_EXPORT void RemoveBonus::applyGs( CGameState *gs )
{
	std::list<Bonus> &bonuses = (who == HERO ? gs->getHero(whoID)->bonuses : gs->getPlayer(whoID)->bonuses);

	for(std::list<Bonus>::iterator i = bonuses.begin(); i != bonuses.end(); i++)
	{
		if(i->source == source && i->id == id)
		{
			bonus = *i; //backup bonus (to show to interfaces later)
			bonuses.erase(i);
			break;
		}
	}
}

DLL_EXPORT void RemoveObject::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->map->objects[id];
	//unblock tiles
	if(obj->defInfo)
	{
		gs->map->removeBlockVisTiles(obj);
	}

	if(obj->ID==HEROI_TYPE)
	{
		CGHeroInstance *h = static_cast<CGHeroInstance*>(obj);
		std::vector<CGHeroInstance*>::iterator nitr = std::find(gs->map->heroes.begin(), gs->map->heroes.end(),h);
		gs->map->heroes.erase(nitr);
		int player = h->tempOwner;
		nitr = std::find(gs->getPlayer(player)->heroes.begin(), gs->getPlayer(player)->heroes.end(), h);
		gs->getPlayer(player)->heroes.erase(nitr);
		h->tempOwner = 255; //no one owns beaten hero

		if(CGTownInstance *t = const_cast<CGTownInstance *>(h->visitedTown))
		{
			if(h->inTownGarrison)
				t->garrisonHero = NULL;
			else
				t->visitingHero = NULL;
			h->visitedTown = NULL;
		}

		//return hero to the pool, so he may reappear in tavern
		gs->hpool.heroesPool[h->subID] = h;
		if(!vstd::contains(gs->hpool.pavailable, h->subID))
			gs->hpool.pavailable[h->subID] = 0xff;
	}
	else if (obj->ID==CREI_TYPE  &&  gs->map->version > CMapHeader::RoE) //only fixed monsters can be a part of quest
	{
		CGCreature *cre = static_cast<CGCreature*>(obj);
		gs->map->monsters[cre->identifier]->pos = int3 (-1,-1,-1);	//use nonexistent monster for quest :>
	}
	gs->map->objects[id] = NULL;
}

static int getDir(int3 src, int3 dst)
{
	int ret = -1;
	if(dst.x+1 == src.x && dst.y+1 == src.y) //tl
	{
		ret = 1;
	}
	else if(dst.x == src.x && dst.y+1 == src.y) //t
	{
		ret = 2;
	}
	else if(dst.x-1 == src.x && dst.y+1 == src.y) //tr
	{
		ret = 3;
	}
	else if(dst.x-1 == src.x && dst.y == src.y) //r
	{
		ret = 4;
	}
	else if(dst.x-1 == src.x && dst.y-1 == src.y) //br
	{
		ret = 5;
	}
	else if(dst.x == src.x && dst.y-1 == src.y) //b
	{
		ret = 6;
	}
	else if(dst.x+1 == src.x && dst.y-1 == src.y) //bl
	{
		ret = 7;
	}
	else if(dst.x+1 == src.x && dst.y == src.y) //l
	{
		ret = 8;
	}
	return ret;
}

void TryMoveHero::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(id);
	h->movement = movePoints;

	if((result == SUCCESS || result == BLOCKING_VISIT || result == EMBARK || result == DISEMBARK) && start != end) {
		h->moveDir = getDir(start,end);
	}

	if(result == EMBARK) //hero enters boat at dest tile
	{
		const TerrainTile &tt = gs->map->getTile(CGHeroInstance::convertPosition(end, false));
		assert(tt.visitableObjects.size() == 1  &&  tt.visitableObjects.front()->ID == 8); //the only vis obj at dest is Boat
		CGBoat *boat = static_cast<CGBoat*>(tt.visitableObjects.front());

		gs->map->removeBlockVisTiles(boat); //hero blockvis mask will be used, we don't need to duplicate it with boat
		h->boat = boat;
		boat->hero = h;
	}
	else if(result == DISEMBARK) //hero leaves boat to dest tile
	{
		CGBoat *b = const_cast<CGBoat *>(h->boat);
		b->direction = h->moveDir;
		b->pos = start;
		b->hero = NULL;
		gs->map->addBlockVisTiles(b);
		h->boat = NULL;
	}

	if(start!=end && (result == SUCCESS || result == TELEPORTATION || result == EMBARK || result == DISEMBARK))
	{
		gs->map->removeBlockVisTiles(h);
		h->pos = end;
		if(CGBoat *b = const_cast<CGBoat *>(h->boat))
			b->pos = end;
		gs->map->addBlockVisTiles(h);
	}

	BOOST_FOREACH(int3 t, fowRevealed)
		gs->getPlayerTeam(h->getOwner())->fogOfWarMap[t.x][t.y][t.z] = 1;
}

DLL_EXPORT void SetGarrisons::applyGs( CGameState *gs )
{
	for(std::map<ui32,CCreatureSet>::iterator i = garrs.begin(); i!=garrs.end(); i++)
	{
		CArmedInstance *ai = static_cast<CArmedInstance*>(gs->map->objects[i->first]);
		ai->setArmy(i->second);
		if(ai->ID==TOWNI_TYPE && (static_cast<CGTownInstance*>(ai))->garrisonHero) //if there is a hero in garrison then we must update also his army
			const_cast<CGHeroInstance*>((static_cast<CGTownInstance*>(ai))->garrisonHero)->setArmy(i->second);
		else if(ai->ID==HEROI_TYPE)
		{
			CGHeroInstance *h =  static_cast<CGHeroInstance*>(ai);
			CGTownInstance *t = const_cast<CGTownInstance *>(h->visitedTown);
			if(t && h->inTownGarrison)
				t->setArmy(i->second);
		}
	}
}

DLL_EXPORT void NewStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	BOOST_FOREACH(si32 id,bid)
	{
		t->builtBuildings.insert(id);
	}
	t->builded = builded;
}
DLL_EXPORT void RazeStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	BOOST_FOREACH(si32 id,bid)
	{
		t->builtBuildings.erase(id);
	}
	t->destroyed = destroyed; //yeaha
}
DLL_EXPORT void SetAvailableCreatures::applyGs( CGameState *gs )
{
	CGDwelling *dw = dynamic_cast<CGDwelling*>(gs->map->objects[tid]);
	assert(dw);
	dw->creatures = creatures;
}

DLL_EXPORT void SetHeroesInTown::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);

	CGHeroInstance *v  = gs->getHero(visiting), 
		*g = gs->getHero(garrison);

	t->visitingHero = v;
	t->garrisonHero = g;
	if(v)
	{
		v->visitedTown = t;
		v->inTownGarrison = false;
		gs->map->addBlockVisTiles(v);
	}
	if(g)
	{
		g->visitedTown = t;
		g->inTownGarrison = true;
		gs->map->removeBlockVisTiles(g);
	}
}

DLL_EXPORT void SetHeroArtifacts::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(hid);
	for(std::map<ui16,ui32>::const_iterator i = h->artifWorn.begin(); i != h->artifWorn.end(); i++)
		if(!vstd::contains(artifWorn,i->first)  ||  artifWorn[i->first] != i->second)
			unequiped.push_back(i->second);

	for(std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		if(!vstd::contains(h->artifWorn,i->first)  ||  h->artifWorn[i->first] != i->second)
			equiped.push_back(i->second);

	//update hero data
	h->artifacts = artifacts;
	h->artifWorn = artifWorn;
}

DLL_EXPORT void SetHeroArtifacts::setArtAtPos(ui16 pos, int art)
{
	if(art < 0)
	{
		if(pos<19)
			VLC->arth->unequipArtifact(artifWorn, pos);
		else if (pos - 19 < artifacts.size())
			artifacts.erase(artifacts.begin() + (pos - 19));
	}
	else
	{
		if (pos < 19) 
		{
			VLC->arth->equipArtifact(artifWorn, pos, (ui32) art);
		} 
		else // Goes into the backpack.
		{ 
			if(pos - 19 < artifacts.size())
				artifacts.insert(artifacts.begin() + (pos - 19), art);
			else
				artifacts.push_back(art);
		}
	}
}


DLL_EXPORT void HeroRecruited::applyGs( CGameState *gs )
{
	assert(vstd::contains(gs->hpool.heroesPool, hid));
	CGHeroInstance *h = gs->hpool.heroesPool[hid];
	CGTownInstance *t = gs->getTown(tid);
	h->setOwner(player);
	h->pos = tile;
	h->movement =  h->maxMovePoints(true);

	gs->hpool.heroesPool.erase(hid);
	if(h->id < 0)
	{
		h->id = gs->map->objects.size();
		gs->map->objects.push_back(h);
	}
	else
		gs->map->objects[h->id] = h;

	h->initHeroDefInfo();
	gs->map->heroes.push_back(h);
	gs->getPlayer(h->getOwner())->heroes.push_back(h);
	h->initObj();
	gs->map->addBlockVisTiles(h);

	if(t)
	{
		t->visitingHero = h;
		h->visitedTown = t;
	}
	h->inTownGarrison = false;
}

DLL_EXPORT void GiveHero::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(id);
	gs->map->removeBlockVisTiles(h,true);
	h->setOwner(player);
	h->movement =  h->maxMovePoints(true);
	h->initHeroDefInfo();
	gs->map->heroes.push_back(h);
	gs->getPlayer(h->getOwner())->heroes.push_back(h);
	gs->map->addBlockVisTiles(h);
	h->inTownGarrison = false;
}

DLL_EXPORT void NewObject::applyGs( CGameState *gs )
{
	
	CGObjectInstance *o = NULL;
	switch(ID)
	{
	case 8:
		o = new CGBoat();
		break;
	case 54: //probably more options will be needed
		o = new CGCreature();
		{
			CStackInstance hlp;
			CGCreature *cre = static_cast<CGCreature*>(o);
			cre->slots[0] = hlp;
			cre->notGrowingTeam = cre->neverFlees = 0;
			cre->character = 2;
			cre->gainedArtifact = -1;
		}
		break;
	default:
		o = new CGObjectInstance();
		break;
	}
	o->ID = ID;
	o->subID = subID;
	o->pos = pos;
	o->defInfo = VLC->dobjinfo->gobjs[ID][subID];
	id = o->id = gs->map->objects.size();
	o->hoverName = VLC->generaltexth->names[ID];

	switch(ID)
	{
		case 54: //cfreature
			o->defInfo = VLC->dobjinfo->gobjs[ID][subID];
			assert(o->defInfo);
			break;
		case 124://hole
			const TerrainTile &t = gs->map->getTile(pos);
			o->defInfo = VLC->dobjinfo->gobjs[ID][t.tertype];
			assert(o->defInfo);
		break;
	}

	gs->map->objects.push_back(o);
	gs->map->addBlockVisTiles(o);
	o->initObj();
	assert(o->defInfo);
}

DLL_EXPORT void SetAvailableArtifacts::applyGs( CGameState *gs )
{
	if(id >= 0)
	{
		if(CGBlackMarket *bm = dynamic_cast<CGBlackMarket*>(gs->map->objects[id]))
		{
			bm->artifacts = arts;
		}
		else
		{
			tlog1 << "Wrong black market id!" << std::endl;
		}
	}
	else
	{
		CGTownInstance::merchantArtifacts = arts;
	}
}

DLL_EXPORT void NewTurn::applyGs( CGameState *gs )
{
	gs->day = day;
	BOOST_FOREACH(NewTurn::Hero h, heroes) //give mana/movement point
	{
		CGHeroInstance *hero = gs->getHero(h.id);
		hero->movement = h.move;
		hero->mana = h.mana;
	}

	for(std::map<ui8, std::vector<si32> >::iterator i = res.begin(); i != res.end(); i++)
	{
		assert(i->first < PLAYER_LIMIT);
		std::vector<si32> &playerRes = gs->getPlayer(i->first)->resources;
		for(int j = 0;  j < i->second.size();  j++)
			playerRes[j] = i->second[j];
	}

	BOOST_FOREACH(SetAvailableCreatures h, cres) //set available creatures in towns
		h.applyGs(gs);

	if (specialWeek != NO_ACTION) //first pack applied, reset all effects and aplly new
	{
		BOOST_FOREACH(CGHeroInstance *h, gs->map->heroes)
			h->bonuses.remove_if(Bonus::OneDay);

		if(gs->getDate(1)) //new week, Monday that is
		{
			for( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
				i->second.bonuses.remove_if(Bonus::OneWeek);
			BOOST_FOREACH(CGHeroInstance *h, gs->map->heroes)
				h->bonuses.remove_if(Bonus::OneWeek);

			gs->globalEffects.bonuses.remove_if(Bonus::OneWeek);

			Bonus b;
			b.duration = Bonus::ONE_WEEK;
			b.source = Bonus::SPECIAL_WEEK;
			b.effectRange = Bonus::NO_LIMIT;
			b.valType = Bonus::BASE_NUMBER; //certainly not intuitive
			switch (specialWeek)
			{
				case DOUBLE_GROWTH:
					b.val = 100;
					b.type = Bonus::CREATURE_GROWTH_PERCENT;
					b.limiter = new CCreatureTypeLimiter(*VLC->creh->creatures[creatureid], false);
					break;
				case BONUS_GROWTH:
					b.val = 5;
					b.type = Bonus::CREATURE_GROWTH;
					b.limiter = new CCreatureTypeLimiter(*VLC->creh->creatures[creatureid], false);
					break;
				case DEITYOFFIRE:
					b.val = 15;
					b.type = Bonus::CREATURE_GROWTH;
					b.limiter = new CCreatureTypeLimiter(*VLC->creh->creatures[42], true);
					break;
				case PLAGUE:
					b.val = -100; //no basic creatures
					b.type = Bonus::CREATURE_GROWTH_PERCENT;
					break;
				default:
					b.val = 0;

			}
			if (b.val)
				gs->globalEffects.bonuses.push_back(b);
		}
	}
	else //second pack is applied
	{
		//count days without town
		for( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
		{
			if(i->second.towns.size() || gs->day == 1)
				i->second.daysWithoutCastle = 0;
			else
				i->second.daysWithoutCastle++;

			i->second.bonuses.remove_if(Bonus::OneDay);
		}
		if(resetBuilded) //reset amount of structures set in this turn in towns
		{
			BOOST_FOREACH(CGTownInstance* t, gs->map->towns)
				t->builded = 0;
		}
	}
}

DLL_EXPORT void SetObjectProperty::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->map->objects[id];
	if(!obj)
	{
		tlog1 << "Wrong object ID - property cannot be set!\n";
		return;
	}

	if(what == ObjProperty::OWNER)
	{
		if(obj->ID == TOWNI_TYPE)
		{
			CGTownInstance *t = static_cast<CGTownInstance*>(obj);
			if(t->tempOwner < PLAYER_LIMIT)
				gs->getPlayer(t->tempOwner)->towns -= t;

			if(val < PLAYER_LIMIT)
				gs->getPlayer(val)->towns.push_back(t);
		}
	}
	
	obj->setProperty(what,val);
}

DLL_EXPORT void SetHoverName::applyGs( CGameState *gs )
{
	name.toString(gs->map->objects[id]->hoverName);
}

DLL_EXPORT void HeroLevelUp::applyGs( CGameState *gs )
{
	CGHeroInstance* h = gs->getHero(heroid);
	h->level = level;
	//speciality
	h->UpdateSpeciality();
}

DLL_EXPORT void BattleStart::applyGs( CGameState *gs )
{
	gs->curB = info;
	info->belligerents[0]->battle = info->belligerents[1]->battle = info;
}

DLL_EXPORT void BattleNextRound::applyGs( CGameState *gs )
{
	gs->curB->castSpells[0] = gs->curB->castSpells[1] = 0;
	gs->curB->round = round;

	BOOST_FOREACH(CStack *s, gs->curB->stacks)
	{
		s->state -= DEFENDING;
		s->state -= WAITING;
		s->state -= MOVED;
		s->state -= HAD_MORALE;
		s->counterAttacks = 1 + s->valOfBonuses(Bonus::ADDITIONAL_RETALIATION);

		//regeneration
		if( s->hasBonusOfType(Bonus::HP_REGENERATION) && s->alive() )
			s->firstHPleft = std::min<ui32>( s->MaxHealth(), s->valOfBonuses(Bonus::HP_REGENERATION) );
		if( s->hasBonusOfType(Bonus::FULL_HP_REGENERATION) && s->alive() )
			s->firstHPleft = s->MaxHealth();

		//remove effects and restore only those with remaining turns in duration
		BonusList tmpEffects = s->bonuses;
		s->bonuses.removeSpells(Bonus::CASTED_SPELL);
		for (BonusList::iterator it = tmpEffects.begin(); it != tmpEffects.end(); it++)
		{
			it->turnsRemain--;
			if(it->turnsRemain > 0)
				s->bonuses.push_back(*it);
		}

		//the same as above for features
		BonusList tmpFeatures = s->bonuses;
		s->bonuses.clear();

		BOOST_FOREACH(Bonus &b, tmpFeatures)
		{
			if((b.duration & Bonus::N_TURNS) != 0)
			{
				b.turnsRemain--;
				if(b.turnsRemain > 0)
					s->bonuses.push_back(b);
			}
			else
			{
				s->bonuses.push_back(b);
			}
		}
	}
}

DLL_EXPORT void BattleSetActiveStack::applyGs( CGameState *gs )
{
	gs->curB->activeStack = stack;
	CStack *st = gs->curB->getStack(stack);
	if(vstd::contains(st->state,MOVED)) //if stack is moving second time this turn it must had a high morale bonus
		st->state.insert(HAD_MORALE);
}

void BattleResult::applyGs( CGameState *gs )
{
	for(unsigned i=0;i<gs->curB->stacks.size();i++)
		delete gs->curB->stacks[i];

	//remove any "until next battle" bonuses
	CGHeroInstance *h;
	h = gs->curB->heroes[0];
	if(h)
		h->bonuses.remove_if(Bonus::OneBattle);

	h = gs->curB->heroes[1];
	if(h) 
		h->bonuses.remove_if(Bonus::OneBattle);

	gs->curB->belligerents[0]->battle = gs->curB->belligerents[1]->battle = NULL;
	delete gs->curB;
	gs->curB = NULL;
}

void BattleStackMoved::applyGs( CGameState *gs )
{
	gs->curB->getStack(stack)->position = tile;
}

DLL_EXPORT void BattleStackAttacked::applyGs( CGameState *gs )
{
	CStack * at = gs->curB->getStack(stackAttacked);
	at->count = newAmount;
	at->firstHPleft = newHP;
	if(killed())
		at->state -= ALIVE;

	//life drain handling
	for (int g=0; g<healedStacks.size(); ++g)
	{
		healedStacks[g].applyGs(gs);
	}
}

DLL_EXPORT void BattleAttack::applyGs( CGameState *gs )
{
	CStack *attacker = gs->curB->getStack(stackAttacking);
	if(counter())
		attacker->counterAttacks--;
	
	if(shot())
	{
		//don't remove ammo if we have a working ammo cart
		bool hasAmmoCart = false;
		BOOST_FOREACH(const CStack * st, gs->curB->stacks)
		{
			if(st->owner == attacker->owner && st->type->idNumber == 148 && st->alive())
			{
				hasAmmoCart = true;
				break;
			}
		}

		if (!hasAmmoCart)
		{
			attacker->shots--;
		}
	}
	BOOST_FOREACH(BattleStackAttacked stackAttacked, bsa)
		stackAttacked.applyGs(gs);

	attacker->bonuses.remove_if(Bonus::UntilAttack);

	for(std::vector<BattleStackAttacked>::const_iterator it = bsa.begin(); it != bsa.end(); ++it)
	{
		CStack * stack = gs->curB->getStack(it->stackAttacked, false);
		stack->bonuses.remove_if(Bonus::UntilBeingAttacked);
	}
}

DLL_EXPORT void StartAction::applyGs( CGameState *gs )
{
	CStack *st = gs->curB->getStack(ba.stackNumber);

	if(ba.actionType != 1) //don't check for stack if it's custom action by hero
		assert(st);

	switch(ba.actionType)
	{
	case 3:
		st->state.insert(DEFENDING);
		break;
	case 8:
		st->state.insert(WAITING);
		return;
	case 0: case 2: case 6: case 7: case 9: case 10: case 11: case 12:
		st->state.insert(MOVED);
		break;
	}

	if(st)
		st->state -= WAITING; //if stack was waiting it has made move, so it won't be "waiting" anymore (if the action was WAIT, then we have returned)
}

DLL_EXPORT void BattleSpellCast::applyGs( CGameState *gs )
{
	assert(gs->curB);
	CGHeroInstance *h = (side) ? gs->curB->heroes[1] : gs->curB->heroes[0];
	if(h && castedByHero)
	{
		int spellCost = 0;
		if(gs->curB)
		{
			spellCost = gs->curB->getSpellCost(&VLC->spellh->spells[id], h);
		}
		else
		{
			spellCost = VLC->spellh->spells[id].costs[skill];
		}
		h->mana -= spellCost;
		if(h->mana < 0) h->mana = 0;
	}
	if(side >= 0 && side < 2 && castedByHero)
	{
		gs->curB->castSpells[side]++;
	}


	if(id == 35 || id == 78) //dispel and dispel helpful spells
	{
		bool onlyHelpful = id == 78;
		for(std::set<ui32>::const_iterator it = affectedCres.begin(); it != affectedCres.end(); ++it)
		{
			CStack *s = gs->curB->getStack(*it);
			if(s && !vstd::contains(resisted, s->ID)) //if stack exists and it didn't resist
			{
				BonusList remainingEff;
				for (BonusList::iterator it = remainingEff.begin(); it != remainingEff.end(); it++)
				{
					if (onlyHelpful && VLC->spellh->spells[ it->id ].positiveness != 1)
					{
						remainingEff.push_back(*it);
					}
					
				}
				s->bonuses.removeSpells(Bonus::CASTED_SPELL); //removing all effects
				s->bonuses = remainingEff; //assigning effects that should remain

				//removing all features from spells
				BonusList tmpFeatures = s->bonuses;
				s->bonuses.clear();
				BOOST_FOREACH(Bonus &b, tmpFeatures)
				{
					const CSpell *sp = b.sourceSpell();
					if(sp && sp->positiveness != 1) //if(b.source != HeroBonus::SPELL_EFFECT || b.positiveness != 1)
						s->bonuses.push_back(b);
				}
			}
		}
	}

	//elemental summoning
	if(id >= 66 && id <= 69)
	{
		int creID;
		switch(id)
		{
		case 66:
			creID = 114; //fire elemental
			break;
		case 67:
			creID = 113; //earth elemental
			break;
		case 68:
			creID = 115; //water elemental
			break;
		case 69:
			creID = 112; //air elemental
			break;
		}
		const int3 & tile = gs->curB->tile;
		TerrainTile::EterrainType ter = gs->map->terrain[tile.x][tile.y][tile.z].tertype;

		int pos; //position of stack on the battlefield - to be calculated

		bool ac[BFIELD_SIZE];
		std::set<int> occupyable;
		bool twoHex = VLC->creh->creatures[creID]->isDoubleWide();
		bool flying = vstd::contains(VLC->creh->creatures[creID]->bonuses, Bonus::FLYING);
		gs->curB->getAccessibilityMap(ac, twoHex, !side, true, occupyable, flying);
		for(int g=0; g<BFIELD_SIZE; ++g)
		{
			if(g % BFIELD_WIDTH != 0 && g % BFIELD_WIDTH != BFIELD_WIDTH-1 && BattleInfo::isAccessible(g, ac, twoHex, !side, flying, true) )
			{
				pos = g;
				break;
			}
		}

		CStack * summonedStack = gs->curB->generateNewStack(CStackInstance(creID, h->getPrimSkillLevel(2) * VLC->spellh->spells[id].powers[skill], h), gs->curB->stacks.size(), !side, 255, ter, pos);
		summonedStack->state.insert(SUMMONED);
		//summonedStack->bonuses.push_back( makeFeature(HeroBonus::SUMMONED, HeroBonus::ONE_BATTLE, 0, 0, HeroBonus::BONUS_FROM_HERO) );

		gs->curB->stacks.push_back(summonedStack);
	}
}

void actualizeEffect(CStack * s, Bonus & ef)
{
	//actualizing effects vector
	for (BonusList::iterator it = s->bonuses.begin(); it != s->bonuses.end(); it++)
	{
		if(it->id == ef.id)
		{
			it->turnsRemain = std::max(it->turnsRemain, ef.turnsRemain);
		}
	}
	//actualizing features vector
	BonusList sf;
	s->stackEffectToFeature(sf, ef);

	BOOST_FOREACH(const Bonus &fromEffect, sf)
	{
		BOOST_FOREACH(Bonus &stackBonus, s->bonuses) //TODO: optimize
		{
			if(stackBonus.source == Bonus::SPELL_EFFECT && stackBonus.type == fromEffect.type && stackBonus.subtype == fromEffect.subtype)
			{
				stackBonus.turnsRemain = std::max(stackBonus.turnsRemain, ef.turnsRemain);
			}
		}
	}

}

DLL_EXPORT void SetStackEffect::applyGs( CGameState *gs )
{
	BOOST_FOREACH(ui32 id, stacks)
	{
		CStack *s = gs->curB->getStack(id);
		if(s)
		{
			if(effect.id == 42 || !s->hasBonus(Selector::source(Bonus::CASTED_SPELL, effect.id)))//disrupting ray or not on the list - just add	
			{
				s->bonuses.push_back(effect);
				BonusList sf;
				s->stackEffectToFeature(sf, effect);
				BOOST_FOREACH(const Bonus &fromEffect, sf)
				{
					s->bonuses.push_back(fromEffect);
				}
			}
			else //just actualize
			{
				actualizeEffect(s, effect);
			}
		}
		else
			tlog1 << "Cannot find stack " << id << std::endl;
	}
}

DLL_EXPORT void StacksInjured::applyGs( CGameState *gs )
{
	BOOST_FOREACH(BattleStackAttacked stackAttacked, stacks)
		stackAttacked.applyGs(gs);
}

DLL_EXPORT void StacksHealedOrResurrected::applyGs( CGameState *gs )
{
	for(int g=0; g<healedStacks.size(); ++g)
	{
		CStack * changedStack = gs->curB->getStack(healedStacks[g].stackID, false);

		//checking if we resurrect a stack that is under a living stack
		std::vector<int> access = gs->curB->getAccessibility(changedStack->ID, true);
		bool acc[BFIELD_SIZE];
		for(int h=0; h<BFIELD_SIZE; ++h)
			acc[h] = false;
		for(int h=0; h<access.size(); ++h)
			acc[access[h]] = true;
		if(!changedStack->alive() && !gs->curB->isAccessible(changedStack->position, acc,
			changedStack->doubleWide(), changedStack->attackerOwned,
			changedStack->hasBonusOfType(Bonus::FLYING), true))
			return; //position is already occupied

		//applying changes
		bool resurrected = !changedStack->alive(); //indicates if stack is resurrected or just healed
		if(resurrected)
		{
			changedStack->state.insert(ALIVE);
			if(healedStacks[g].lowLevelResurrection)
				changedStack->state.insert(SUMMONED);
				//changedStack->bonuses.push_back( makeFeature(HeroBonus::SUMMONED, HeroBonus::ONE_BATTLE, 0, 0, HeroBonus::BONUS_FROM_HERO) );
		}
		int missingHPfirst = changedStack->MaxHealth() - changedStack->firstHPleft;
		int res = std::min( healedStacks[g].healedHP / changedStack->MaxHealth() , changedStack->baseAmount - changedStack->count );
		changedStack->count += res;
		changedStack->firstHPleft += healedStacks[g].healedHP - res * changedStack->MaxHealth();
		if(changedStack->firstHPleft > changedStack->MaxHealth())
		{
			changedStack->firstHPleft -= changedStack->MaxHealth();
			if(changedStack->baseAmount > changedStack->count)
			{
				changedStack->count += 1;
			}
		}
		amin(changedStack->firstHPleft, changedStack->MaxHealth());
		//removal of negative effects
		if(resurrected)
		{
			for (BonusList::iterator it = changedStack->bonuses.begin(); it != changedStack->bonuses.end(); it++)
			{
				if(VLC->spellh->spells[it->id].positiveness < 0)
				{
					changedStack->bonuses.erase(it);
				}
			}
			
			//removing all features from negative spells
			BonusList tmpFeatures = changedStack->bonuses;
			changedStack->bonuses.clear();

			BOOST_FOREACH(Bonus &b, tmpFeatures)
			{
				const CSpell *s = b.sourceSpell();
				if(s && s->positiveness >= 0)
				{
					changedStack->bonuses.push_back(b);
				}
			}
		}
	}
}

DLL_EXPORT void ObstaclesRemoved::applyGs( CGameState *gs )
{
	if(gs->curB) //if there is a battle
	{
		for(std::set<si32>::const_iterator it = obstacles.begin(); it != obstacles.end(); ++it)
		{
			for(int i=0; i<gs->curB->obstacles.size(); ++i)
			{
				if(gs->curB->obstacles[i].uniqueID == *it) //remove this obstacle
				{
					gs->curB->obstacles.erase(gs->curB->obstacles.begin() + i);
					break;
				}
			}
		}
	}
}

DLL_EXPORT void CatapultAttack::applyGs( CGameState *gs )
{
	if(gs->curB && gs->curB->siege != 0) //if there is a battle and it's a siege
	{
		for(std::set< std::pair< std::pair< ui8, si16 >, ui8> >::const_iterator it = attackedParts.begin(); it != attackedParts.end(); ++it)
		{
			gs->curB->si.wallState[it->first.first] = 
				std::min( gs->curB->si.wallState[it->first.first] + it->second, 3 );
		}
	}
}

DLL_EXPORT void BattleStacksRemoved::applyGs( CGameState *gs )
{
	if(!gs->curB)
		return;

	for(std::set<ui32>::const_iterator it = stackIDs.begin(); it != stackIDs.end(); ++it) //for each removed stack
	{
		for(int b=0; b<gs->curB->stacks.size(); ++b) //find it in vector of stacks
		{
			if(gs->curB->stacks[b]->ID == *it) //if found
			{
				gs->curB->stacks.erase(gs->curB->stacks.begin() + b); //remove
				break;
			}
		}
	}
}

DLL_EXPORT void YourTurn::applyGs( CGameState *gs )
{
	gs->currentPlayer = player;
}

DLL_EXPORT void SetSelection::applyGs( CGameState *gs )
{
	gs->getPlayer(player)->currentSelection = id;
}

DLL_EXPORT Component::Component(const CStackInstance &stack)
	:id(CREATURE), subtype(stack.type->idNumber), val(stack.count), when(0)
{

}