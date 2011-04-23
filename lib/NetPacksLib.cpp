#define VCMI_DLL
#include "NetPacks.h"
#include "CGeneralTextHandler.h"
#include "CDefObjInfoHandler.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "VCMI_Lib.h"
#include "map.h"
#include "CSpellHandler.h"
#include "CCreatureHandler.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "CGameState.h"
#include "BattleState.h"

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
	hero->setSecSkillLevel(static_cast<CGHeroInstance::SecondarySkill>(which), val, abs);
}

DLL_EXPORT void HeroVisitCastle::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(hid);
	CGTownInstance *t = gs->getTown(tid);

	if(start())
		t->setVisitingHero(h);
	else
		t->setVisitingHero(NULL);
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
		boost::unordered_set<int3, ShashInt3> tilesRevealed;
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
		CGHeroInstance *h = (hid[i]>=0 ?  (CGHeroInstance*)gs->hpool.heroesPool[hid[i]] : NULL);
		if(h && army[i])
			h->setToArmy(army[i]);
		p->availableHeroes.push_back(h);
	}
}

DLL_EXPORT void GiveBonus::applyGs( CGameState *gs )
{
	CBonusSystemNode *cbsn = NULL;
	switch(who)
	{
	case HERO:
		cbsn = gs->getHero(id);
		break;
	case PLAYER:
		cbsn = gs->getPlayer(id);
		break;
	case TOWN:
		cbsn = gs->getTown(id);
		break;
	}

	assert(cbsn);

	Bonus *b = new Bonus(bonus);
	cbsn->addNewBonus(b);

	std::string &descr = b->description;

	if(!bdescr.message.size() 
		&& bonus.source == Bonus::OBJECT 
		&& (bonus.type == Bonus::LUCK || bonus.type == Bonus::MORALE)
		&& gs->map->objects[bonus.sid]->ID == EVENTI_TYPE) //it's morale/luck bonus from an event without description
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
	BonusList &bonuses = (who == HERO ? gs->getHero(whoID)->bonuses : gs->getPlayer(whoID)->bonuses);

	for(BonusList::iterator i = bonuses.begin(); i != bonuses.end(); i++)
	{
		if((*i)->source == source && (*i)->sid == id)
		{
			bonus = **i; //backup bonus (to show to interfaces later)
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
		PlayerState *p = gs->getPlayer(h->tempOwner);
		gs->map->heroes -= h;
		p->heroes -= h;
		h->detachFrom(h->whereShouldBeAttached(gs));
		h->tempOwner = 255; //no one owns beaten hero

		if(h->visitedTown)
		{
			if(h->inTownGarrison)
				h->visitedTown->garrisonHero = NULL;
			else
				h->visitedTown->visitingHero = NULL;
			h->visitedTown = NULL;
		}

		//return hero to the pool, so he may reappear in tavern
		gs->hpool.heroesPool[h->subID] = h;
		if(!vstd::contains(gs->hpool.pavailable, h->subID))
			gs->hpool.pavailable[h->subID] = 0xff;

		gs->map->objects[id] = NULL;
		

		return;
	}
// 	else if (obj->ID==CREI_TYPE  &&  gs->map->version > CMapHeader::RoE) //only fixed monsters can be a part of quest
// 	{
// 		CGCreature *cre = static_cast<CGCreature*>(obj);
// 		gs->map->monsters[cre->identifier]->pos = int3 (-1,-1,-1);	//use nonexistent monster for quest :>
// 	}
	gs->map->objects[id].dellNull();
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
		assert(tt.visitableObjects.size() >= 1  &&  tt.visitableObjects.back()->ID == 8); //the only vis obj at dest is Boat
		CGBoat *boat = static_cast<CGBoat*>(tt.visitableObjects.back());

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

DLL_EXPORT void NewStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	BOOST_FOREACH(si32 id,bid)
	{
		t->builtBuildings.insert(id);
	}
	t->builded = builded;
	t->recreateBuildingsBonuses();
}
DLL_EXPORT void RazeStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	BOOST_FOREACH(si32 id,bid)
	{
		t->builtBuildings.erase(id);
	}
	t->destroyed = destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

DLL_EXPORT void SetAvailableCreatures::applyGs( CGameState *gs )
{
	CGDwelling *dw = dynamic_cast<CGDwelling*>(gs->map->objects[tid].get());
	assert(dw);
	dw->creatures = creatures;
}

DLL_EXPORT void SetHeroesInTown::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);

	CGHeroInstance *v  = gs->getHero(visiting), 
		*g = gs->getHero(garrison);

	bool newVisitorComesFromGarrison = v && v == t->garrisonHero;
	bool newGarrisonComesFromVisiting = g && g == t->visitingHero;

	if(newVisitorComesFromGarrison)
		t->setGarrisonedHero(NULL);
	if(newGarrisonComesFromVisiting)
		t->setVisitingHero(NULL);
	if(!newGarrisonComesFromVisiting || v)
		t->setVisitingHero(v);
	if(!newVisitorComesFromGarrison || g)
		t->setGarrisonedHero(g);

	if(v)
	{
		gs->map->addBlockVisTiles(v);
	}
	if(g)
	{
		gs->map->removeBlockVisTiles(g);
	}
}

DLL_EXPORT void HeroRecruited::applyGs( CGameState *gs )
{
	assert(vstd::contains(gs->hpool.heroesPool, hid));
	CGHeroInstance *h = gs->hpool.heroesPool[hid];
	CGTownInstance *t = gs->getTown(tid);
	PlayerState *p = gs->getPlayer(player);
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
	p->heroes.push_back(h);
	h->attachTo(p);
	h->initObj();
	gs->map->addBlockVisTiles(h);

	if(t)
	{
		t->setVisitingHero(h);
	}
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
			//CStackInstance hlp;
			CGCreature *cre = static_cast<CGCreature*>(o);
			//cre->slots[0] = hlp;
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
DLL_EXPORT void NewArtifact::applyGs( CGameState *gs )
{
	assert(!vstd::contains(gs->map->artInstances, art));
	gs->map->addNewArtifactInstance(art);

	assert(!art->parents.size());
	art->setType(art->artType);
}

DLL_EXPORT const CStackInstance * StackLocation::getStack()
{
	if(!army->hasStackAtSlot(slot))
	{
		tlog2 << "Warning: " << army->nodeName() << " dont have a stack at slot " << slot << std::endl;
		return NULL;
	}
	return &army->getStack(slot);
}

DLL_EXPORT const CArtifactInstance *ArtifactLocation::getArt() const
{
	const ArtSlotInfo *s = getSlot();
	if(s && s->artifact)
	{
		if(!s->locked)
			return s->artifact;
		else
		{
			tlog3 << "ArtifactLocation::getArt: That location is locked!\n";
			return NULL;
		}
	}
	return NULL;
	return NULL;
}

DLL_EXPORT CArtifactInstance *ArtifactLocation::getArt()
{
	const ArtifactLocation *t = this;
	return const_cast<CArtifactInstance*>(t->getArt());
}

DLL_EXPORT const ArtSlotInfo *ArtifactLocation::getSlot() const
{
	return hero->getSlot(slot);
}

DLL_EXPORT void ChangeStackCount::applyGs( CGameState *gs )
{
	if(absoluteValue)
		sl.army->setStackCount(sl.slot, count);
	else
		sl.army->changeStackCount(sl.slot, count);
}

DLL_EXPORT void SetStackType::applyGs( CGameState *gs )
{
	sl.army->setStackType(sl.slot, type);
}

DLL_EXPORT void EraseStack::applyGs( CGameState *gs )
{
	sl.army->eraseStack(sl.slot);
}

DLL_EXPORT void SwapStacks::applyGs( CGameState *gs )
{
	CStackInstance *s1 = sl1.army->detachStack(sl1.slot),
		*s2 = sl2.army->detachStack(sl2.slot);

	sl2.army->putStack(sl2.slot, s1);
	sl1.army->putStack(sl1.slot, s2);
}

DLL_EXPORT void InsertNewStack::applyGs( CGameState *gs )
{
	CStackInstance *s = new CStackInstance(stack.type, stack.count);
	sl.army->putStack(sl.slot, s);
}

DLL_EXPORT void RebalanceStacks::applyGs( CGameState *gs )
{
	const CCreature *srcType = src.army->getCreature(src.slot);
	TQuantity srcCount = src.army->getStackCount(src.slot);

	if(srcCount == count) //moving whole stack
	{
		if(const CCreature *c = dst.army->getCreature(dst.slot)) //stack at dest -> merge
		{
			assert(c == srcType);
			if (STACK_EXP)
			{
				ui64 totalExp = srcCount * src.army->getStackExperience(src.slot) + dst.army->getStackCount(dst.slot) * dst.army->getStackExperience(dst.slot);
				src.army->eraseStack(src.slot);
				dst.army->changeStackCount(dst.slot, count);
				dst.army->setStackExp(dst.slot, totalExp /(dst.army->getStackCount(dst.slot))); //mean
			}
			else
			{
				src.army->eraseStack(src.slot);
				dst.army->changeStackCount(dst.slot, count);
			}
		}
		else //move stack to an empty slot, no exp change needed
		{
			CStackInstance *stackDetached = src.army->detachStack(src.slot);
			dst.army->putStack(dst.slot, stackDetached);
		}
	}
	else
	{
		if(const CCreature *c = dst.army->getCreature(dst.slot)) //stack at dest -> rebalance
		{
			assert(c == srcType);
			if (STACK_EXP)
			{
				ui64 totalExp = srcCount * src.army->getStackExperience(src.slot) + dst.army->getStackCount(dst.slot) * dst.army->getStackExperience(dst.slot);
				src.army->changeStackCount(src.slot, -count);
				dst.army->changeStackCount(dst.slot, count);
				dst.army->setStackExp(dst.slot, totalExp /(src.army->getStackCount(src.slot) + dst.army->getStackCount(dst.slot))); //mean
			}
			else
			{
				src.army->changeStackCount(src.slot, -count);
				dst.army->changeStackCount(dst.slot, count);
			}
		}
		else //split stack to an empty slot
		{
			src.army->changeStackCount(src.slot, -count);
			dst.army->addToSlot(dst.slot, srcType->idNumber, count, false);
			if (STACK_EXP)
				dst.army->setStackExp(dst.slot, src.army->getStackExperience(src.slot));
		}
	}
}

DLL_EXPORT void PutArtifact::applyGs( CGameState *gs )
{
	assert(art->canBePutAt(al));
	al.hero->putArtifact(al.slot, art);
}

DLL_EXPORT void EraseArtifact::applyGs( CGameState *gs )
{
	CArtifactInstance *a = al.getArt();
	assert(a);
	a->removeFrom(al.hero, al.slot);
}

DLL_EXPORT void MoveArtifact::applyGs( CGameState *gs )
{
	CArtifactInstance *a = src.getArt();
	if(dst.slot < Arts::BACKPACK_START)
		assert(!dst.getArt());

	a->move(src, dst);
}

DLL_EXPORT void AssembledArtifact::applyGs( CGameState *gs )
{
	CGHeroInstance *h = al.hero;
	const CArtifactInstance *transformedArt = al.getArt();
	assert(transformedArt);
	assert(vstd::contains(transformedArt->assemblyPossibilities(al.hero), builtArt));

	CCombinedArtifactInstance *combinedArt = new CCombinedArtifactInstance(builtArt);
	gs->map->addNewArtifactInstance(combinedArt);
	//retreive all constituents
	BOOST_FOREACH(si32 constituentID, *builtArt->constituents)
	{
		int pos = h->getArtPos(constituentID);
		assert(pos >= 0);
		CArtifactInstance *constituentInstance = h->getArt(pos);

		//move constituent from hero to be part of new, combined artifact
		constituentInstance->removeFrom(h, pos);
		combinedArt->addAsConstituent(constituentInstance, pos);
		if(!vstd::contains(combinedArt->artType->possibleSlots, al.slot) && vstd::contains(combinedArt->artType->possibleSlots, pos))
			al.slot = pos;
	}

	//put new combined artifacts
	combinedArt->putAt(h, al.slot);
}

DLL_EXPORT void DisassembledArtifact::applyGs( CGameState *gs )
{
	CGHeroInstance *h = al.hero;
	CCombinedArtifactInstance *disassembled = dynamic_cast<CCombinedArtifactInstance*>(al.getArt());
	assert(disassembled);

	std::vector<CCombinedArtifactInstance::ConstituentInfo> constituents = disassembled->constituentsInfo;
	disassembled->removeFrom(h, al.slot);
	BOOST_FOREACH(CCombinedArtifactInstance::ConstituentInfo &ci, constituents)
	{
		disassembled->detachFrom(ci.art);
		ci.art->putAt(h, ci.slot >= 0 ? ci.slot : al.slot); //-1 is slot of main constituent -> it'll replace combined artifact in its pos
	}

	gs->map->eraseArtifactInstance(disassembled);
}

DLL_EXPORT void SetAvailableArtifacts::applyGs( CGameState *gs )
{
	if(id >= 0)
	{
		if(CGBlackMarket *bm = dynamic_cast<CGBlackMarket*>(gs->map->objects[id].get()))
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
		//TODO? won't work for battles lasting several turns (not an issue now but...)
		gs->globalEffects.popBonuses(Bonus::OneDay); //works for children -> all game objs

		if(gs->getDate(1)) //new week, Monday that is
		{
			gs->globalEffects.popBonuses(Bonus::OneWeek); //works for children -> all game objs

			Bonus *b = new Bonus();
			b->duration = Bonus::ONE_WEEK;
			b->source = Bonus::SPECIAL_WEEK;
			b->effectRange = Bonus::NO_LIMIT;
			b->valType = Bonus::BASE_NUMBER; //certainly not intuitive
			switch (specialWeek)
			{
				case DOUBLE_GROWTH:
					b->val = 100;
					b->type = Bonus::CREATURE_GROWTH_PERCENT;
					b->limiter.reset(new CCreatureTypeLimiter(*VLC->creh->creatures[creatureid], false));
					break;
				case BONUS_GROWTH:
					b->val = 5;
					b->type = Bonus::CREATURE_GROWTH;
					b->limiter.reset(new CCreatureTypeLimiter(*VLC->creh->creatures[creatureid], false));
					break;
				case DEITYOFFIRE:
					b->val = 15;
					b->type = Bonus::CREATURE_GROWTH;
					b->limiter.reset(new CCreatureTypeLimiter(*VLC->creh->creatures[42], true));
					break;
				case PLAGUE:
					b->val = -100; //no basic creatures
					b->type = Bonus::CREATURE_GROWTH_PERCENT;
					break;
				default:
					b->val = 0;

			}
			if (b->val)
				gs->globalEffects.addNewBonus(b);
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

	CArmedInstance *cai = dynamic_cast<CArmedInstance *>(obj);
	if(what == ObjProperty::OWNER && cai)
	{
		if(obj->ID == TOWNI_TYPE)
		{
			CGTownInstance *t = static_cast<CGTownInstance*>(obj);
			if(t->tempOwner < PLAYER_LIMIT)
				gs->getPlayer(t->tempOwner)->towns -= t;
			if(val < PLAYER_LIMIT)
				gs->getPlayer(val)->towns.push_back(t);
		}

		CBonusSystemNode *nodeToMove = cai->whatShouldBeAttached();
		nodeToMove->detachFrom(cai->whereShouldBeAttached(gs));
		obj->setProperty(what,val);
		nodeToMove->attachTo(cai->whereShouldBeAttached(gs));
	}
	else //not an armed instance
	{
		obj->setProperty(what,val);
	}
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
	gs->curB->localInit();
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

		s->battleTurnPassed();
	}
}

DLL_EXPORT void BattleSetActiveStack::applyGs( CGameState *gs )
{
	gs->curB->activeStack = stack;
	CStack *st = gs->curB->getStack(stack);

	//remove bonuses that last until when stack gets new turn
	st->bonuses.remove_if(Bonus::UntilGetsTurn);

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

	if (STACK_EXP)
	{
		if (exp[0]) //checking local array is easier than dereferencing this crap twice
			gs->curB->belligerents[0]->giveStackExp(exp[0]);
		if (exp[1])
			gs->curB->belligerents[1]->giveStackExp(exp[1]);
	}

	gs->curB->belligerents[0]->battle = gs->curB->belligerents[1]->battle = NULL;
	gs->curB.dellNull();
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
			if(st->owner == attacker->owner && st->getCreature()->idNumber == 148 && st->alive())
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

	if(ba.actionType == BattleAction::END_TACTIC_PHASE)
	{
		gs->curB->tacticDistance = 0;
		return;
	}

	if(ba.actionType != BattleAction::HERO_SPELL) //don't check for stack if it's custom action by hero
	{
		assert(st);
	}
	else
	{
		gs->curB->usedSpellsHistory[ba.side].push_back(VLC->spellh->spells[ba.additionalInfo]);
	}

	switch(ba.actionType)
	{
	case BattleAction::DEFEND:
		st->state.insert(DEFENDING);
		break;
	case BattleAction::WAIT:
		st->state.insert(WAITING);
		return;
	case BattleAction::NO_ACTION: case BattleAction::WALK: case BattleAction::WALK_AND_ATTACK:
	case BattleAction::SHOOT: case BattleAction::CATAPULT: case BattleAction::MONSTER_SPELL:
	case BattleAction::BAD_MORALE: case BattleAction::STACK_HEAL:
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
			spellCost = gs->curB->getSpellCost(VLC->spellh->spells[id], h);
		}
		else
		{
			spellCost = VLC->spellh->spells[id]->costs[skill];
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
				if(onlyHelpful)
					s->popBonuses(Selector::positiveSpellEffects);
				else
					s->popBonuses(Selector::sourceType(Bonus::SPELL_EFFECT));
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
// 		const int3 & tile = gs->curB->tile;
// 		TerrainTile::EterrainType ter = gs->map->terrain[tile.x][tile.y][tile.z].tertype;

		int pos; //position of stack on the battlefield - to be calculated

		bool ac[BFIELD_SIZE];
		std::set<THex> occupyable;
		bool twoHex = VLC->creh->creatures[creID]->isDoubleWide();
		bool flying = VLC->creh->creatures[creID]->isFlying();// vstd::contains(VLC->creh->creatures[creID]->bonuses, Bonus::FLYING);
		gs->curB->getAccessibilityMap(ac, twoHex, !side, true, occupyable, flying);
		for(int g=0; g<BFIELD_SIZE; ++g)
		{
			if(g % BFIELD_WIDTH != 0 && g % BFIELD_WIDTH != BFIELD_WIDTH-1 && BattleInfo::isAccessible(g, ac, twoHex, !side, flying, true) )
			{
				pos = g;
				break;
			}
		}

		CStackInstance *csi = new CStackInstance(creID, h->getPrimSkillLevel(2) * VLC->spellh->spells[id]->powers[skill]); //deleted by d-tor of summoned stack
		csi->setArmyObj(h);
		CStack * summonedStack = gs->curB->generateNewStack(*csi, gs->curB->stacks.size(), !side, 255, pos);
		summonedStack->state.insert(SUMMONED);
		summonedStack->attachTo(csi);
		summonedStack->postInit();
		//summonedStack->addNewBonus( makeFeature(HeroBonus::SUMMONED, HeroBonus::ONE_BATTLE, 0, 0, HeroBonus::BONUS_FROM_HERO) );
		gs->curB->stacks.push_back(summonedStack);
	}
}

void actualizeEffect(CStack * s, const std::vector<Bonus> & ef)
{
	//actualizing features vector

	BOOST_FOREACH(const Bonus &fromEffect, ef)
	{
		BOOST_FOREACH(Bonus *stackBonus, s->bonuses) //TODO: optimize
		{
			if(stackBonus->source == Bonus::SPELL_EFFECT && stackBonus->type == fromEffect.type && stackBonus->subtype == fromEffect.subtype)
			{
				stackBonus->turnsRemain = std::max(stackBonus->turnsRemain, fromEffect.turnsRemain);
			}
		}
	}
}
void actualizeEffect(CStack * s, const Bonus & ef)
{
	BOOST_FOREACH(Bonus *stackBonus, s->bonuses) //TODO: optimize
	{
		if(stackBonus->source == Bonus::SPELL_EFFECT && stackBonus->type == ef.type && stackBonus->subtype == ef.subtype)
		{
			stackBonus->turnsRemain = std::max(stackBonus->turnsRemain, ef.turnsRemain);
		}
	}
}

DLL_EXPORT void SetStackEffect::applyGs( CGameState *gs )
{
	int id = effect.begin()->sid; //effects' source ID

	BOOST_FOREACH(ui32 id, stacks)
	{
		CStack *s = gs->curB->getStack(id);
		if(s)
		{
			if(id == 47 || !s->hasBonus(Selector::source(Bonus::SPELL_EFFECT, id)))//disrupting ray or not on the list - just add	
			{
				BOOST_FOREACH(Bonus &fromEffect, effect)
				{
					s->addNewBonus( new Bonus(fromEffect));
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
	typedef std::pair<ui32, Bonus> p;
	BOOST_FOREACH(p para, uniqueBonuses)
	{
		CStack *s = gs->curB->getStack(para.first);
		if (s)
		{
			if (!s->hasBonus(Selector::source(Bonus::SPELL_EFFECT, id) && Selector::typeSybtype(para.second.type, para.second.subtype)))
				s->addNewBonus(new Bonus(para.second));
			else
				actualizeEffect(s, effect);
		}
		else
			tlog1 << "Cannot find stack " << para.first << std::endl;
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
		std::vector<THex> access = gs->curB->getAccessibility(changedStack, true);
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
				//changedStack->addNewBonus( makeFeature(HeroBonus::SUMMONED, HeroBonus::ONE_BATTLE, 0, 0, HeroBonus::BONUS_FROM_HERO) );
		}
		//int missingHPfirst = changedStack->MaxHealth() - changedStack->firstHPleft;
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
				if(VLC->spellh->spells[(*it)->sid]->positiveness < 0)
				{
					changedStack->bonuses.erase(it);
				}
			}
			
			//removing all features from negative spells
			BonusList tmpFeatures = changedStack->bonuses;
			changedStack->bonuses.clear();

			BOOST_FOREACH(Bonus *b, tmpFeatures)
			{
				const CSpell *s = b->sourceSpell();
				if(s && s->positiveness >= 0)
				{
					changedStack->addNewBonus(b);
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

DLL_EXPORT Component::Component(const CStackBasicDescriptor &stack)
	:id(CREATURE), subtype(stack.type->idNumber), val(stack.count), when(0)
{

}