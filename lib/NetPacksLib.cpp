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
	if(which <4)
	{
		if(abs)
			hero->primSkills[which] = val;
		else
			hero->primSkills[which] += val;
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
	if(hero->getSecSkillLevel(which) == 0)
	{
		hero->secSkills.push_back(std::pair<int,int>(which, val));
	}
	else
	{
		for(unsigned i=0;i<hero->secSkills.size();i++)
		{
			if(hero->secSkills[i].first == which)
			{
				if(abs)
					hero->secSkills[i].second = val;
				else
					hero->secSkills[i].second += val;

				if(hero->secSkills[i].second > 3) //workaround to avoid crashes when same sec skill is given more than once
				{
					tlog1 << "Warning: Skill " << which << " increased over limit! Decreasing to Expert.\n";
					hero->secSkills[i].second = 3;
				}
			}
		}
	}
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
	BOOST_FOREACH(int3 t, tiles)
		gs->getPlayer(player)->fogOfWarMap[t.x][t.y][t.z] = mode;
}

DLL_EXPORT void SetAvailableHeroes::applyGs( CGameState *gs )
{
	gs->getPlayer(player)->availableHeroes.clear();

	CGHeroInstance *h = (hid1>=0 ?  gs->hpool.heroesPool[hid1] : NULL);
	gs->getPlayer(player)->availableHeroes.push_back(h);
	if(h  &&  flags & 1)
	{
		h->army.slots.clear();
		h->army.slots[0] = std::pair<ui32,si32>(VLC->creh->nameToID[h->type->refTypeStack[0]],1);
	}

	h = (hid2>=0 ?  gs->hpool.heroesPool[hid2] : NULL);
	gs->getPlayer(player)->availableHeroes.push_back(h);
	if(flags & 2)
	{
		h->army.slots.clear();
		h->army.slots[0] = std::pair<ui32,si32>(VLC->creh->nameToID[h->type->refTypeStack[0]],1);
	}
}

DLL_EXPORT void GiveBonus::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(hid);
	h->bonuses.push_back(bonus);


	std::string &descr = h->bonuses.back().description;

	if(!bdescr.message.size() 
		&& bonus.source == HeroBonus::OBJECT 
		&& (bonus.type == HeroBonus::LUCK || bonus.type == HeroBonus::MORALE || bonus.type == HeroBonus::MORALE_AND_LUCK)
		&& gs->map->objects[bonus.id]->ID == 26) //it's morale/luck bonus from an event without description
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

DLL_EXPORT void RemoveObject::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->map->objects[id];
	if(obj->ID==HEROI_TYPE)
	{
		CGHeroInstance *h = static_cast<CGHeroInstance*>(obj);
		std::vector<CGHeroInstance*>::iterator nitr = std::find(gs->map->heroes.begin(), gs->map->heroes.end(),h);
		gs->map->heroes.erase(nitr);
		int player = h->tempOwner;
		nitr = std::find(gs->getPlayer(player)->heroes.begin(), gs->getPlayer(player)->heroes.end(), h);
		gs->getPlayer(player)->heroes.erase(nitr);
		h->tempOwner = 255; //no one owns beaten hero

		if(h->visitedTown)
		{
			if(h->inTownGarrison)
				h->visitedTown->garrisonHero = NULL;
			else
				h->visitedTown->visitingHero = NULL;
			h->visitedTown = NULL;
		}

		//TODO: add to the pool?
	}
	else if (obj->ID==CREI_TYPE) //only fixed monsters can be a part of quest
	{
		CGCreature *cre = static_cast<CGCreature*>(obj);
		gs->map->monsters[cre->identifier] = NULL;	
	}
	gs->map->objects[id] = NULL;	

	//unblock tiles
	if(obj->defInfo)
	{
		gs->map->removeBlockVisTiles(obj);
	}
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

	if((result == SUCCESS || result == BLOCKING_VISIT || result == EMBARK || result == DISEMBARK) && start != end)
		h->moveDir = getDir(start,end);

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
		h->boat->direction = h->moveDir;
		h->boat->pos = start;
		h->boat->hero = NULL;
		gs->map->addBlockVisTiles(h->boat);
		h->boat = NULL;
	}

	if(start!=end && (result == SUCCESS || result == TELEPORTATION || result == EMBARK || result == DISEMBARK))
	{
		gs->map->removeBlockVisTiles(h);
		h->pos = end;
		if(h->boat)
			h->boat->pos = end;
		gs->map->addBlockVisTiles(h);
	}

	BOOST_FOREACH(int3 t, fowRevealed)
		gs->getPlayer(h->getOwner())->fogOfWarMap[t.x][t.y][t.z] = 1;
}

DLL_EXPORT void SetGarrisons::applyGs( CGameState *gs )
{
	for(std::map<ui32,CCreatureSet>::iterator i = garrs.begin(); i!=garrs.end(); i++)
	{
		CArmedInstance *ai = static_cast<CArmedInstance*>(gs->map->objects[i->first]);
		ai->army = i->second;
		if(ai->ID==TOWNI_TYPE && (static_cast<CGTownInstance*>(ai))->garrisonHero) //if there is a hero in garrison then we must update also his army
			const_cast<CGHeroInstance*>((static_cast<CGTownInstance*>(ai))->garrisonHero)->army = i->second;
		else if(ai->ID==HEROI_TYPE)
		{
			CGHeroInstance *h =  static_cast<CGHeroInstance*>(ai);
			if(h->visitedTown && h->inTownGarrison)
				h->visitedTown->army = i->second;
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
	std::vector<ui32> equiped, unequiped;
	for(std::map<ui16,ui32>::const_iterator i = h->artifWorn.begin(); i != h->artifWorn.end(); i++)
		if(!vstd::contains(artifWorn,i->first)  ||  artifWorn[i->first] != i->second)
			unequiped.push_back(i->second);

	for(std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		if(!vstd::contains(h->artifWorn,i->first)  ||  h->artifWorn[i->first] != i->second)
			equiped.push_back(i->second);

	BOOST_FOREACH(ui32 id, equiped)
	{
		//if hero already had equipped at least one artifact of that type, don't give any new bonuses
		if(h->getArtPos(id) >= 0)
			continue;

		CArtifact &art = VLC->arth->artifacts[id];
		for(std::list<HeroBonus>::iterator i = art.bonuses.begin(); i != art.bonuses.end(); i++)
		{
			gained.push_back(&*i);
			h->bonuses.push_back(*i);
		}
	}

	//update hero data
	h->artifacts = artifacts;
	h->artifWorn = artifWorn;

	//remove bonus from unequipped artifact
	BOOST_FOREACH(ui32 id, unequiped)
	{
		//if hero still has equipped at least one artifact of that type, don't remove bonuses
		if(h->getArtPos(id) >= 0)
			continue;

		while(1)
		{
			std::list<HeroBonus>::iterator hlp = std::find_if(h->bonuses.begin(),h->bonuses.end(),boost::bind(HeroBonus::IsFrom,_1,HeroBonus::ARTIFACT,id));
			if(hlp != h->bonuses.end())
			{
				lost.push_back(&*hlp);
				h->bonuses.erase(hlp);
			}
			else
			{
				break;
			}
		}
	}
}

DLL_EXPORT void SetHeroArtifacts::setArtAtPos(ui16 pos, int art)
{
	if(art<0)
	{
		if(pos<19)
			artifWorn.erase(pos);
		else
			artifacts.erase(artifacts.begin() + (pos - 19));
	}
	else
	{
		if (pos < 19) {
			artifWorn[pos] = art;
		} else { // Goes into the backpack.
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
	t->visitingHero = h;
	h->visitedTown = t;
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

	gs->map->objects.push_back(o);
	gs->map->addBlockVisTiles(o);
	o->initObj();
	assert(o->defInfo);
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

	if(resetBuilded) //reset amount of structures set in this turn in towns
		BOOST_FOREACH(CGTownInstance* t, gs->map->towns)
		t->builded = 0;

	BOOST_FOREACH(CGHeroInstance *h, gs->map->heroes)
		h->bonuses.remove_if(HeroBonus::OneDay);

	if(gs->getDate(1) == 7) //new week
		BOOST_FOREACH(CGHeroInstance *h, gs->map->heroes)
			h->bonuses.remove_if(HeroBonus::OneWeek);

	//count days without town
	for( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		if(i->second.towns.size() || gs->day == 1)
			i->second.daysWithoutCastle = 0;
		else
			i->second.daysWithoutCastle++;
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

	if(what == 1)
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
	gs->getHero(heroid)->level = level;
}

DLL_EXPORT void BattleStart::applyGs( CGameState *gs )
{
	gs->curB = info;
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
		s->counterAttacks = 1 + s->valOfFeatures(StackFeature::ADDITIONAL_RETALIATION);

		//regeneration
		if( s->hasFeatureOfType(StackFeature::HP_REGENERATION) && s->alive() )
			s->firstHPleft = std::min<ui32>( s->MaxHealth(), s->valOfFeatures(StackFeature::HP_REGENERATION) );
		if( s->hasFeatureOfType(StackFeature::FULL_HP_REGENERATION) && s->alive() )
			s->firstHPleft = s->MaxHealth();

		//remove effects and restore only those with remaining turns in duration
		std::vector<CStack::StackEffect> tmpEffects = s->effects;
		s->effects.clear();
		for(int i=0; i < tmpEffects.size(); i++)
		{
			tmpEffects[i].turnsRemain--;
			if(tmpEffects[i].turnsRemain > 0)
				s->effects.push_back(tmpEffects[i]);
		}

		//the same as above for features
		std::vector<StackFeature> tmpFeatures = s->features;
		s->features.clear();
		for(int i=0; i < tmpFeatures.size(); i++)
		{
			if((tmpFeatures[i].duration & StackFeature::N_TURNS) != 0)
			{
				tmpFeatures[i].turnsRemain--;
				if(tmpFeatures[i].turnsRemain > 0)
					s->features.push_back(tmpFeatures[i]);
			}
			else
			{
				s->features.push_back(tmpFeatures[i]);
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
		h->bonuses.remove_if(HeroBonus::OneBattle);

	h = gs->curB->heroes[1];
	if(h) 
		h->bonuses.remove_if(HeroBonus::OneBattle);

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
	at->amount = newAmount;
	at->firstHPleft = newHP;
	if(killed())
		at->state -= ALIVE;
}

DLL_EXPORT void BattleAttack::applyGs( CGameState *gs )
{
	CStack *attacker = gs->curB->getStack(stackAttacking);
	if(counter())
		attacker->counterAttacks--;
	if(shot())
		attacker->shots--;
	BOOST_FOREACH(BattleStackAttacked stackAttacked, bsa)
		stackAttacked.applyGs(gs);

	for(int g=0; g<attacker->features.size(); ++g)
	{
		if((attacker->features[g].duration & StackFeature::UNTIL_ATTACK) != 0)
		{
			attacker->features.erase(attacker->features.begin() + g);
			g = 0;
		}
	}

	for(std::set<BattleStackAttacked>::const_iterator it = bsa.begin(); it != bsa.end(); ++it)
	{
		CStack * stack = gs->curB->getStack(it->stackAttacked, false);

		for(int g=0; g<stack->features.size(); ++g)
		{
			if((stack->features[g].duration & StackFeature::UNITL_BEING_ATTACKED) != 0)
			{
				stack->features.erase(stack->features.begin() + g);
				g = 0;
			}
		}
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
	case 2: case 6: case 7: case 9: case 10: case 11:
		st->state.insert(MOVED);
		break;
	}

	if(st)
		st->state -= WAITING; //if stack was waiting it has made move, so it won't be "waiting" anymore (if the action was WAIT, then we have returned)
}

DLL_EXPORT void SpellCast::applyGs( CGameState *gs )
{
	CGHeroInstance *h = (side) ? gs->curB->heroes[1] : gs->curB->heroes[0];
	if(h)
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
	if(side >= 0 && side < 2)
	{
		gs->curB->castSpells[side]++;
	}


	if(gs->curB && id == 35) //dispel
	{
		for(std::set<ui32>::const_iterator it = affectedCres.begin(); it != affectedCres.end(); ++it)
		{
			CStack *s = gs->curB->getStack(*it);
			if(s && !vstd::contains(resisted, s->ID)) //if stack exists and it didn't resist
			{
				s->effects.clear(); //removing all effects
				//removing all features from spells
				std::vector<StackFeature> tmpFeatures = s->features;
				s->features.clear();
				for(int i=0; i < tmpFeatures.size(); i++)
				{
					if(tmpFeatures[i].source != StackFeature::SPELL_EFFECT)
					{
						s->features.push_back(tmpFeatures[i]);
					}
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
		bool twoHex = vstd::contains(VLC->creh->creatures[creID].abilities, StackFeature::DOUBLE_WIDE);
		bool flying = vstd::contains(VLC->creh->creatures[creID].abilities, StackFeature::FLYING);
		gs->curB->getAccessibilityMap(ac, twoHex, !side, true, occupyable, flying);
		for(int g=0; g<BFIELD_SIZE; ++g)
		{
			if(g % BFIELD_WIDTH != 0 && g % BFIELD_WIDTH != BFIELD_WIDTH-1 && BattleInfo::isAccessible(g, ac, twoHex, !side, flying, true) )
			{
				pos = g;
				break;
			}
		}

		CStack * summonedStack = gs->curB->generateNewStack(h, creID, h->getPrimSkillLevel(2) * VLC->spellh->spells[id].powers[skill], gs->curB->stacks.size(), !side, 255, ter, pos);
		summonedStack->features.push_back( makeFeature(StackFeature::SUMMONED, StackFeature::WHOLE_BATTLE, 0, 0, StackFeature::BONUS_FROM_HERO) );

		gs->curB->stacks.push_back(summonedStack);
	}
}

static inline StackFeature featureGenerator(StackFeature::ECombatFeatures type, si16 subtype, si32 value, ui16 turnsRemain, si32 additionalInfo = 0)
{
	return makeFeature(type, StackFeature::N_TURNS, subtype, value, StackFeature::SPELL_EFFECT, turnsRemain, additionalInfo);
}

static std::vector<StackFeature> stackEffectToFeature(const CStack::StackEffect & sse)
{
	std::vector<StackFeature> sf;
	switch(sse.id)
	{
	case 27: //shield 
		sf.push_back(featureGenerator(StackFeature::GENERAL_DAMAGE_REDUCTION, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 28: //air shield
		sf.push_back(featureGenerator(StackFeature::GENERAL_DAMAGE_REDUCTION, 1, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 30: //protection from air
		sf.push_back(featureGenerator(StackFeature::SPELL_DAMAGE_REDUCTION, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 31: //protection from fire
		sf.push_back(featureGenerator(StackFeature::SPELL_DAMAGE_REDUCTION, 1, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 32: //protection from water
		sf.push_back(featureGenerator(StackFeature::SPELL_DAMAGE_REDUCTION, 2, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 33: //protection from earth
		sf.push_back(featureGenerator(StackFeature::SPELL_DAMAGE_REDUCTION, 3, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 34: //anti-magic
		sf.push_back(featureGenerator(StackFeature::LEVEL_SPELL_IMMUNITY, 0, VLC->spellh->spells[sse.id].powers[sse.level] - 1, sse.turnsRemain));
		break;
	case 41: //bless
		sf.push_back(featureGenerator(StackFeature::ALWAYS_MAXIMUM_DAMAGE, -1, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 42: //curse
		sf.push_back(featureGenerator(StackFeature::ALWAYS_MINIMUM_DAMAGE, -1, -1 * VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain, sse.level >= 2 ? 20 : 0));
		break;
	case 43: //bloodlust
		sf.push_back(featureGenerator(StackFeature::ATTACK_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 44: //precision
		sf.push_back(featureGenerator(StackFeature::ATTACK_BONUS, 1, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 45: //weakness
		sf.push_back(featureGenerator(StackFeature::ATTACK_BONUS, -1, -1 * VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 46: //stone skin
		sf.push_back(featureGenerator(StackFeature::DEFENCE_BONUS, -1, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 47: //disrupting ray
		sf.push_back(featureGenerator(StackFeature::DEFENCE_BONUS, -1, -1 * VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 48: //prayer
		sf.push_back(featureGenerator(StackFeature::ATTACK_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		sf.push_back(featureGenerator(StackFeature::DEFENCE_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		sf.push_back(featureGenerator(StackFeature::SPEED_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 49: //mirth
		sf.push_back(featureGenerator(StackFeature::MORALE_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 50: //sorrow
		sf.push_back(featureGenerator(StackFeature::MORALE_BONUS, 0, -1 * VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 51: //fortune
		sf.push_back(featureGenerator(StackFeature::LUCK_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 52: //misfortune
		sf.push_back(featureGenerator(StackFeature::LUCK_BONUS, 0, -1 * VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 53: //haste
		sf.push_back(featureGenerator(StackFeature::SPEED_BONUS, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 54: //slow
		sf.push_back(featureGenerator(StackFeature::SPEED_BONUS, 0, 0, sse.turnsRemain, -1 * ( 100 - VLC->spellh->spells[sse.id].powers[sse.level] ) ));
		break;
	case 55: //slayer
		sf.push_back(featureGenerator(StackFeature::SLAYER, 0, sse.level, sse.turnsRemain));
		break;
	case 56: //frenzy
		sf.push_back(featureGenerator(StackFeature::SLAYER, 0, sse.level, sse.turnsRemain));
		break;
	case 58: //counterstrike
		sf.push_back(featureGenerator(StackFeature::ADDITIONAL_RETALIATION, 0, VLC->spellh->spells[sse.id].powers[sse.level], sse.turnsRemain));
		break;
	case 59: //bersek
		sf.push_back(featureGenerator(StackFeature::ATTACKS_NEAREST_CREATURE, 0, sse.level, sse.turnsRemain));
		break;
	case 60: //hypnotize
		sf.push_back(featureGenerator(StackFeature::HYPNOTIZED, 0, sse.level, sse.turnsRemain));
		break;
	case 61: //forgetfulness
		sf.push_back(featureGenerator(StackFeature::SLAYER, 0, sse.level, sse.turnsRemain));
		break;
	case 62: //blind
		sf.push_back(makeFeature(StackFeature::NOT_ACTIVE, StackFeature::UNITL_BEING_ATTACKED | StackFeature::N_TURNS, 0, 0, StackFeature::SPELL_EFFECT, sse.turnsRemain, 0));
		sf.push_back(makeFeature(StackFeature::GENERAL_ATTACK_REDUCTION, StackFeature::UNTIL_ATTACK | StackFeature::N_TURNS, 0, VLC->spellh->spells[sse.id].powers[sse.level], StackFeature::SPELL_EFFECT, sse.turnsRemain, 0));
		break;
	}

	//setting positiveness
	for(int g=0; g<sf.size(); ++g)
	{
		sf[g].positiveness = VLC->spellh->spells[sse.id].positiveness;
	}

	return sf;
}

void actualizeEffect(CStack * s, CStack::StackEffect & ef)
{
	//actualizing effects vector
	for(int g=0; g<s->effects.size(); ++g)
	{
		if(s->effects[g].id == ef.id)
		{
			s->effects[g].turnsRemain = std::max(s->effects[g].turnsRemain, ef.turnsRemain);
		}
	}
	//actualizing features vector
	std::vector<StackFeature> sf = stackEffectToFeature(ef);
	for(int b=0; b<sf.size(); ++b)
	{
		for(int g=0; g<s->features.size(); ++g)
		{
			if(s->features[g].source == StackFeature::SPELL_EFFECT && s->features[g].type == sf[b].type && s->features[g].subtype == sf[b].subtype)
			{
				s->features[g].turnsRemain = std::max(s->features[g].turnsRemain, ef.turnsRemain);
			}
		}
	}

}

bool containsEff(const std::vector<CStack::StackEffect> & vec, int effectId)
{
	for(int g=0; g<vec.size(); ++g)
	{
		if(vec[g].id == effectId)
			return true;
	}
	return false;
}

DLL_EXPORT void SetStackEffect::applyGs( CGameState *gs )
{
	BOOST_FOREACH(ui32 id, stacks)
	{
		CStack *s = gs->curB->getStack(id);
		if(s)
		{
			if(effect.id == 42 || !containsEff(s->effects, effect.id))//disrupting ray or not on the list - just add
			{
				s->effects.push_back(effect);
				std::vector<StackFeature> sf = stackEffectToFeature(effect);
				for(int n=0; n<sf.size(); ++n)
				{
					s->features.push_back(sf[n]);
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
			changedStack->hasFeatureOfType(StackFeature::DOUBLE_WIDE), changedStack->attackerOwned,
			changedStack->hasFeatureOfType(StackFeature::FLYING), true))
			return; //position is already occupied

		//applying changes
		bool resurrected = !changedStack->alive(); //indicates if stack is resurrected or just healed
		if(!changedStack->alive())
		{
			changedStack->state.insert(ALIVE);
			if(healedStacks[g].lowLevelResurrection)
				changedStack->features.push_back( makeFeature(StackFeature::SUMMONED, StackFeature::WHOLE_BATTLE, 0, 0, StackFeature::BONUS_FROM_HERO) );
		}
		int missingHPfirst = changedStack->MaxHealth() - changedStack->firstHPleft;
		int res = std::min( healedStacks[g].healedHP / changedStack->MaxHealth() , changedStack->baseAmount - changedStack->amount );
		changedStack->amount += res;
		changedStack->firstHPleft += healedStacks[g].healedHP - res * changedStack->MaxHealth();
		if(changedStack->firstHPleft > changedStack->MaxHealth())
		{
			changedStack->firstHPleft -= changedStack->MaxHealth();
			if(changedStack->baseAmount > changedStack->amount)
			{
				changedStack->amount += 1;
			}
		}
		amin(changedStack->firstHPleft, changedStack->MaxHealth());
		//removal of negative effects
		if(resurrected)
		{
			for(int h=0; h<changedStack->effects.size(); ++h)
			{
				if(VLC->spellh->spells[changedStack->effects[h].id].positiveness < 0)
				{
					changedStack->effects.erase(changedStack->effects.begin() + h);
				}
			}
			
			//removing all features from negative spells
			std::vector<StackFeature> tmpFeatures = changedStack->features;
			changedStack->features.clear();
			for(int i=0; i < tmpFeatures.size(); i++)
			{
				if(tmpFeatures[i].source != StackFeature::SPELL_EFFECT || tmpFeatures[i].positiveness >= 0)
				{
					changedStack->features.push_back(tmpFeatures[i]);
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