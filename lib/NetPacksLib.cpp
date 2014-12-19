#include "StdInc.h"
#include "NetPacks.h"

#include "CGeneralTextHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "CModHandler.h"
#include "VCMI_Lib.h"
#include "mapping/CMap.h"
#include "CSpellHandler.h"
#include "CCreatureHandler.h"
#include "CGameState.h"
#include "BattleState.h"
#include "CTownHandler.h"
#include "mapping/CMapInfo.h"
#include "StartInfo.h"

/*
 * NetPacksLib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#undef min
#undef max


std::ostream & operator<<(std::ostream & out, const CPack * pack)
{
	return out << pack->toString();
}

DLL_LINKAGE void SetResource::applyGs( CGameState *gs )
{
	assert(player < PlayerColor::PLAYER_LIMIT);
	vstd::amax(val, 0); //new value must be >= 0
	gs->getPlayer(player)->resources[resid] = val;
}

DLL_LINKAGE void SetResources::applyGs( CGameState *gs )
{
	assert(player < PlayerColor::PLAYER_LIMIT);
	gs->getPlayer(player)->resources = res;
}

DLL_LINKAGE void SetPrimSkill::applyGs( CGameState *gs )
{
	CGHeroInstance * hero = gs->getHero(id);
	assert(hero);
	hero->setPrimarySkill(which, val, abs);
}

DLL_LINKAGE void SetSecSkill::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(id);
	hero->setSecSkillLevel(which, val, abs);
}


DLL_LINKAGE SelectMap::SelectMap(const CMapInfo &src)
{
	mapInfo = &src;
	free = false;
}
DLL_LINKAGE SelectMap::SelectMap()
{
	mapInfo = nullptr;
	free = true;
}

DLL_LINKAGE SelectMap::~SelectMap()
{
	if(free)
		delete mapInfo;
}

DLL_LINKAGE  UpdateStartOptions::UpdateStartOptions(StartInfo &src)
{
	options = &src;
	free = false;
}
DLL_LINKAGE  UpdateStartOptions::UpdateStartOptions()
{
	options = nullptr;
	free = true;
}

DLL_LINKAGE UpdateStartOptions::~UpdateStartOptions()
{
	if(free)
		delete options;
}

DLL_LINKAGE void SetCommanderProperty::applyGs(CGameState *gs)
{
	CCommanderInstance * commander = gs->getHero(heroid)->commander;
	assert (commander);

	switch (which)
	{
		case BONUS:
			commander->accumulateBonus (accumulatedBonus);
			break;
		case SPECIAL_SKILL:
			commander->accumulateBonus (accumulatedBonus);
			commander->specialSKills.insert (additionalInfo);
			break;
		case SECONDARY_SKILL:
			commander->secondarySkills[additionalInfo] = amount;
			break;
		case ALIVE:
			if (amount)
				commander->setAlive(true);
			else
				commander->setAlive(false);
			break;
		case EXPERIENCE:
			commander->giveStackExp (amount); //TODO: allow setting exp for stacks via netpacks
			break;
	}
}

DLL_LINKAGE void AddQuest::applyGs(CGameState *gs)
{
	assert (vstd::contains(gs->players, player));
	auto vec = &gs->players[player].quests;
	if (!vstd::contains(*vec, quest))
		vec->push_back (quest);
	else
        logNetwork->warnStream() << "Warning! Attempt to add duplicated quest";
}

DLL_LINKAGE void UpdateArtHandlerLists::applyGs(CGameState *gs)
{
	VLC->arth->minors = minors;
	VLC->arth->majors = majors;
	VLC->arth->treasures = treasures;
	VLC->arth->relics = relics;
}

DLL_LINKAGE void UpdateMapEvents::applyGs(CGameState *gs)
{
	gs->map->events = events;
}


DLL_LINKAGE void UpdateCastleEvents::applyGs(CGameState *gs)
{
	auto t = gs->getTown(town);
	t->events = events;
}

DLL_LINKAGE void HeroVisitCastle::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(hid);
	CGTownInstance *t = gs->getTown(tid);

	assert(h);
	assert(t);

	if(start())
		t->setVisitingHero(h);
	else
		t->setVisitingHero(nullptr);
}

DLL_LINKAGE void ChangeSpells::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);

	if(learn)
		for(auto sid : spells)
			hero->spells.insert(sid);
	else
		for(auto sid : spells)
			hero->spells.erase(sid);
}

DLL_LINKAGE void SetMana::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);
	vstd::amax(val, 0); //not less than 0
	hero->mana = val;
}

DLL_LINKAGE void SetMovePoints::applyGs( CGameState *gs )
{
	CGHeroInstance *hero = gs->getHero(hid);
	hero->movement = val;
}

DLL_LINKAGE void FoWChange::applyGs( CGameState *gs )
{
	TeamState * team = gs->getPlayerTeam(player);
	for(int3 t : tiles)
		team->fogOfWarMap[t.x][t.y][t.z] = mode;
	if (mode == 0) //do not hide too much
	{
		std::unordered_set<int3, ShashInt3> tilesRevealed;
		for (auto & elem : gs->map->objects)
		{
			const CGObjectInstance *o = elem;
			if (o)
			{
				switch(o->ID)
				{
				case Obj::HERO:
				case Obj::MINE:
				case Obj::TOWN:
				case Obj::ABANDONED_MINE:
					if(vstd::contains(team->players, o->tempOwner)) //check owned observators
						gs->getTilesInRange(tiles, o->getSightCenter(), o->getSightRadious(), o->tempOwner, 1);
					break;
				}
			}
		}
		for(int3 t : tilesRevealed) //probably not the most optimal solution ever
			team->fogOfWarMap[t.x][t.y][t.z] = 1;
	}
}
DLL_LINKAGE void SetAvailableHeroes::applyGs( CGameState *gs )
{
	PlayerState *p = gs->getPlayer(player);
	p->availableHeroes.clear();

	for (int i = 0; i < GameConstants::AVAILABLE_HEROES_PER_PLAYER; i++)
	{
		CGHeroInstance *h = (hid[i]>=0 ?  gs->hpool.heroesPool[hid[i]].get() : nullptr);
		if(h && army[i])
			h->setToArmy(army[i]);
		p->availableHeroes.push_back(h);
	}
}

DLL_LINKAGE void GiveBonus::applyGs( CGameState *gs )
{
	CBonusSystemNode *cbsn = nullptr;
	switch(who)
	{
	case HERO:
		cbsn = gs->getHero(ObjectInstanceID(id));
		break;
	case PLAYER:
		cbsn = gs->getPlayer(PlayerColor(id));
		break;
	case TOWN:
		cbsn = gs->getTown(ObjectInstanceID(id));
		break;
	}

	assert(cbsn);

	auto b = new Bonus(bonus);
	cbsn->addNewBonus(b);

	std::string &descr = b->description;

	if(!bdescr.message.size()
		&& bonus.source == Bonus::OBJECT
		&& (bonus.type == Bonus::LUCK || bonus.type == Bonus::MORALE))
	{
		descr = VLC->generaltexth->arraytxt[bonus.val > 0 ? 110 : 109]; //+/-%d Temporary until next battle"
	}
	else
	{
		bdescr.toString(descr);
	}
	// Some of(?) versions of H3 use %s here instead of %d. Try to replace both of them
	boost::replace_first(descr,"%d",boost::lexical_cast<std::string>(std::abs(bonus.val)));
	boost::replace_first(descr,"%s",boost::lexical_cast<std::string>(std::abs(bonus.val)));
}

DLL_LINKAGE void ChangeObjPos::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->getObjInstance(objid);
	if(!obj)
	{
        logNetwork->errorStream() << "Wrong ChangeObjPos: object " << objid.getNum() << " doesn't exist!";
		return;
	}
	gs->map->removeBlockVisTiles(obj);
	obj->pos = nPos;
	gs->map->addBlockVisTiles(obj);
}

DLL_LINKAGE void ChangeObjectVisitors::applyGs( CGameState *gs )
{
	switch (mode) {
		case VISITOR_ADD:
			gs->getHero(hero)->visitedObjects.insert(object);
			gs->getPlayer(gs->getHero(hero)->tempOwner)->visitedObjects.insert(object);
			break;
		case VISITOR_CLEAR:
			for (CGHeroInstance * hero : gs->map->allHeroes)
				hero->visitedObjects.erase(object); // remove visit info from all heroes, including those that are not present on map
			break;
		case VISITOR_REMOVE:
			gs->getHero(hero)->visitedObjects.erase(object);
			break;
	}
}

DLL_LINKAGE void PlayerEndsGame::applyGs( CGameState *gs )
{
	PlayerState *p = gs->getPlayer(player);
	if(victoryLossCheckResult.victory()) p->status = EPlayerStatus::WINNER;
	else p->status = EPlayerStatus::LOSER;
}

DLL_LINKAGE void RemoveBonus::applyGs( CGameState *gs )
{
	CBonusSystemNode *node;
	if (who == HERO)
		node = gs->getHero(ObjectInstanceID(whoID));
	else
		node = gs->getPlayer(PlayerColor(whoID));

	BonusList &bonuses = node->getBonusList();

	for (int i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];
		if(b->source == source && b->sid == id)
		{
			bonus = *b; //backup bonus (to show to interfaces later)
			bonuses.erase(i);
			break;
		}
	}
}

DLL_LINKAGE void RemoveObject::applyGs( CGameState *gs )
{

	CGObjectInstance *obj = gs->getObjInstance(id);
	logGlobal->debugStream() << "removing object id=" << id << "; address=" << (intptr_t)obj << "; name=" << obj->getObjectName();
	//unblock tiles
	gs->map->removeBlockVisTiles(obj);

	if(obj->ID==Obj::HERO)
	{
		CGHeroInstance *h = static_cast<CGHeroInstance*>(obj);
		PlayerState *p = gs->getPlayer(h->tempOwner);
		gs->map->heroesOnMap -= h;
		p->heroes -= h;
		h->detachFrom(h->whereShouldBeAttached(gs));
		h->tempOwner = PlayerColor::NEUTRAL; //no one owns beaten hero

		if(h->visitedTown)
		{
			if(h->inTownGarrison)
				h->visitedTown->garrisonHero = nullptr;
			else
				h->visitedTown->visitingHero = nullptr;
			h->visitedTown = nullptr;
		}

		//return hero to the pool, so he may reappear in tavern
		gs->hpool.heroesPool[h->subID] = h;
		if(!vstd::contains(gs->hpool.pavailable, h->subID))
			gs->hpool.pavailable[h->subID] = 0xff;

		gs->map->objects[id.getNum()] = nullptr;

		//If hero on Boat is removed, the Boat disappears
		if(h->boat)
		{
			gs->map->objects[h->boat->id.getNum()].dellNull();
			h->boat = nullptr;
		}

		return;
	}

	auto quest = dynamic_cast<const IQuestObject *>(obj);
	if (quest)
	{
		gs->map->quests[quest->quest->qid] = nullptr;
		for (auto &player : gs->players)
		{
			for (auto &q : player.second.quests)
			{
				if (q.obj == obj)
				{
					q.obj = nullptr;
				}
			}
		}
	}

	for (TriggeredEvent & event : gs->map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if (cond.object == obj)
			{
				if (cond.condition == EventCondition::DESTROY)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 1; // destroyed object, from now on always fulfilled
				}
				if (cond.condition == EventCondition::CONTROL)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 0; // destroyed object, from now on can not be fulfilled
				}
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}

	gs->map->objects[id.getNum()].dellNull();
	gs->map->calculateGuardingGreaturePositions();
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
	{
		auto dir = getDir(start,end);
		if(dir > 0  &&  dir <= 8)
			h->moveDir = dir;
		//else don`t change move direction - hero might have traversed the subterranean gate, direction should be kept
	}

	if(result == EMBARK) //hero enters boat at destination tile
	{
		const TerrainTile &tt = gs->map->getTile(CGHeroInstance::convertPosition(end, false));
		assert(tt.visitableObjects.size() >= 1  &&  tt.visitableObjects.back()->ID == Obj::BOAT); //the only visitable object at destination is Boat
		CGBoat *boat = static_cast<CGBoat*>(tt.visitableObjects.back());

		gs->map->removeBlockVisTiles(boat); //hero blockvis mask will be used, we don't need to duplicate it with boat
		h->boat = boat;
		boat->hero = h;
	}
	else if(result == DISEMBARK) //hero leaves boat to destination tile
	{
		CGBoat *b = const_cast<CGBoat *>(h->boat);
		b->direction = h->moveDir;
		b->pos = start;
		b->hero = nullptr;
		gs->map->addBlockVisTiles(b);
		h->boat = nullptr;
	}

	if(start!=end && (result == SUCCESS || result == TELEPORTATION || result == EMBARK || result == DISEMBARK))
	{
		gs->map->removeBlockVisTiles(h);
		h->pos = end;
		if(CGBoat *b = const_cast<CGBoat *>(h->boat))
			b->pos = end;
		gs->map->addBlockVisTiles(h);
	}

	for(int3 t : fowRevealed)
		gs->getPlayerTeam(h->getOwner())->fogOfWarMap[t.x][t.y][t.z] = 1;
}

DLL_LINKAGE void NewStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	for(const auto & id : bid)
	{
		assert(t->town->buildings.at(id) != nullptr);
		t->builtBuildings.insert(id);
	}
	t->builded = builded;
	t->recreateBuildingsBonuses();
}
DLL_LINKAGE void RazeStructures::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);
	for(const auto & id : bid)
	{
		t->builtBuildings.erase(id);
	}
	t->destroyed = destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

DLL_LINKAGE void SetAvailableCreatures::applyGs( CGameState *gs )
{
	CGDwelling *dw = dynamic_cast<CGDwelling*>(gs->getObjInstance(tid));
	assert(dw);
	dw->creatures = creatures;
}

DLL_LINKAGE void SetHeroesInTown::applyGs( CGameState *gs )
{
	CGTownInstance *t = gs->getTown(tid);

	CGHeroInstance *v  = gs->getHero(visiting),
		*g = gs->getHero(garrison);

	bool newVisitorComesFromGarrison = v && v == t->garrisonHero;
	bool newGarrisonComesFromVisiting = g && g == t->visitingHero;

	if(newVisitorComesFromGarrison)
		t->setGarrisonedHero(nullptr);
	if(newGarrisonComesFromVisiting)
		t->setVisitingHero(nullptr);
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

DLL_LINKAGE void HeroRecruited::applyGs( CGameState *gs )
{
	assert(vstd::contains(gs->hpool.heroesPool, hid));
	CGHeroInstance *h = gs->hpool.heroesPool[hid];
	CGTownInstance *t = gs->getTown(tid);
	PlayerState *p = gs->getPlayer(player);

	assert(!h->boat);

	h->setOwner(player);
	h->pos = tile;
	h->movement =  h->maxMovePoints(true);

	gs->hpool.heroesPool.erase(hid);
	if(h->id == ObjectInstanceID())
	{
		h->id = ObjectInstanceID(gs->map->objects.size());
		gs->map->objects.push_back(h);
	}
	else
		gs->map->objects[h->id.getNum()] = h;

	gs->map->heroesOnMap.push_back(h);
	p->heroes.push_back(h);
	h->attachTo(p);
	h->initObj();
	gs->map->addBlockVisTiles(h);

	if(t)
	{
		t->setVisitingHero(h);
	}
}

DLL_LINKAGE void GiveHero::applyGs( CGameState *gs )
{
	CGHeroInstance *h = gs->getHero(id);

	//bonus system
	h->detachFrom(&gs->globalEffects);
	h->attachTo(gs->getPlayer(player));
	h->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, h->type->heroClass->id)->getTemplates().front();

	gs->map->removeBlockVisTiles(h,true);
	h->setOwner(player);
	h->movement =  h->maxMovePoints(true);
	gs->map->heroesOnMap.push_back(h);
	gs->getPlayer(h->getOwner())->heroes.push_back(h);
	gs->map->addBlockVisTiles(h);
	h->inTownGarrison = false;
}

DLL_LINKAGE void NewObject::applyGs( CGameState *gs )
{
	const TerrainTile &t = gs->map->getTile(pos);
	ETerrainType terrainType = t.terType;

	CGObjectInstance *o = nullptr;
	switch(ID)
	{
	case Obj::BOAT:
		o = new CGBoat();
		terrainType = ETerrainType::WATER; //TODO: either boat should only spawn on water, or all water objects should be handled this way
		break;
	case Obj::MONSTER: //probably more options will be needed
		o = new CGCreature();
		{
			//CStackInstance hlp;
			CGCreature *cre = static_cast<CGCreature*>(o);
			//cre->slots[0] = hlp;
			cre->notGrowingTeam = cre->neverFlees = 0;
			cre->character = 2;
			cre->gainedArtifact = ArtifactID::NONE;
			cre->identifier = -1;
			cre->addToSlot(SlotID(0), new CStackInstance(CreatureID(subID), -1)); //add placeholder stack
		}
		break;
	default:
		o = new CGObjectInstance();
		break;
	}
	o->ID = ID;
	o->subID = subID;
	o->pos = pos;
	o->appearance = VLC->objtypeh->getHandlerFor(o->ID, o->subID)->getTemplates(terrainType).front();
	id = o->id = ObjectInstanceID(gs->map->objects.size());

	gs->map->objects.push_back(o);
	gs->map->addBlockVisTiles(o);
	o->initObj();
	gs->map->calculateGuardingGreaturePositions();

	logGlobal->debugStream() << "added object id=" << id << "; address=" << (intptr_t)o << "; name=" << o->getObjectName();
}

DLL_LINKAGE void NewArtifact::applyGs( CGameState *gs )
{
	assert(!vstd::contains(gs->map->artInstances, art));
	gs->map->addNewArtifactInstance(art);

	assert(!art->getParentNodes().size());
	art->setType(art->artType);
	if (CCombinedArtifactInstance* cart = dynamic_cast<CCombinedArtifactInstance*>(art.get()))
		cart->createConstituents();
}

DLL_LINKAGE const CStackInstance * StackLocation::getStack()
{
	if(!army->hasStackAtSlot(slot))
	{
		logNetwork->warnStream() << "Warning: " << army->nodeName() << " don't have a stack at slot " << slot;
		return nullptr;
	}
	return &army->getStack(slot);
}

struct ObjectRetriever : boost::static_visitor<const CArmedInstance *>
{
	const CArmedInstance * operator()(const ConstTransitivePtr<CGHeroInstance> &h) const
	{
		return h;
	}
	const CArmedInstance * operator()(const ConstTransitivePtr<CStackInstance> &s) const
	{
		return s->armyObj;
	}
};
template <typename T>
struct GetBase : boost::static_visitor<T*>
{
	template <typename TArg>
	T * operator()(TArg &arg) const
	{
		return arg;
	}
};


DLL_LINKAGE void ArtifactLocation::removeArtifact()
{
	CArtifactInstance *a = getArt();
	assert(a);
	a->removeFrom(*this);
}

DLL_LINKAGE const CArmedInstance * ArtifactLocation::relatedObj() const
{
	return boost::apply_visitor(ObjectRetriever(), artHolder);
}

DLL_LINKAGE PlayerColor ArtifactLocation::owningPlayer() const
{
	auto obj = relatedObj();
	return obj ? obj->tempOwner : PlayerColor::NEUTRAL;
}

DLL_LINKAGE CArtifactSet *ArtifactLocation::getHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), artHolder);
}

DLL_LINKAGE CBonusSystemNode *ArtifactLocation::getHolderNode()
{
	return boost::apply_visitor(GetBase<CBonusSystemNode>(), artHolder);
}

DLL_LINKAGE const CArtifactInstance *ArtifactLocation::getArt() const
{
	const ArtSlotInfo *s = getSlot();
	if(s && s->artifact)
	{
		if(!s->locked)
			return s->artifact;
		else
		{
            logNetwork->warnStream() << "ArtifactLocation::getArt: That location is locked!";
			return nullptr;
		}
	}
	return nullptr;
}

DLL_LINKAGE const CArtifactSet * ArtifactLocation::getHolderArtSet() const
{
	ArtifactLocation *t = const_cast<ArtifactLocation*>(this);
	return t->getHolderArtSet();
}

DLL_LINKAGE const CBonusSystemNode * ArtifactLocation::getHolderNode() const
{
	ArtifactLocation *t = const_cast<ArtifactLocation*>(this);
	return t->getHolderNode();
}

DLL_LINKAGE CArtifactInstance *ArtifactLocation::getArt()
{
	const ArtifactLocation *t = this;
	return const_cast<CArtifactInstance*>(t->getArt());
}

DLL_LINKAGE const ArtSlotInfo *ArtifactLocation::getSlot() const
{
	return getHolderArtSet()->getSlot(slot);
}

DLL_LINKAGE void ChangeStackCount::applyGs( CGameState *gs )
{
	if(absoluteValue)
		sl.army->setStackCount(sl.slot, count);
	else
		sl.army->changeStackCount(sl.slot, count);
}

DLL_LINKAGE void SetStackType::applyGs( CGameState *gs )
{
	sl.army->setStackType(sl.slot, type);
}

DLL_LINKAGE void EraseStack::applyGs( CGameState *gs )
{
	sl.army->eraseStack(sl.slot);
}

DLL_LINKAGE void SwapStacks::applyGs( CGameState *gs )
{
	CStackInstance *s1 = sl1.army->detachStack(sl1.slot),
		*s2 = sl2.army->detachStack(sl2.slot);

	sl2.army->putStack(sl2.slot, s1);
	sl1.army->putStack(sl1.slot, s2);
}

DLL_LINKAGE void InsertNewStack::applyGs( CGameState *gs )
{
	auto s = new CStackInstance(stack.type, stack.count);
	sl.army->putStack(sl.slot, s);
}

DLL_LINKAGE void RebalanceStacks::applyGs( CGameState *gs )
{
	const CCreature *srcType = src.army->getCreature(src.slot);
	TQuantity srcCount = src.army->getStackCount(src.slot);
	bool stackExp = VLC->modh->modules.STACK_EXP;

	if(srcCount == count) //moving whole stack
	{
		if(const CCreature *c = dst.army->getCreature(dst.slot)) //stack at dest -> merge
		{
			assert(c == srcType);
			UNUSED(c);
			auto alHere = ArtifactLocation (src.getStack(), ArtifactPosition::CREATURE_SLOT);
			auto alDest = ArtifactLocation (dst.getStack(), ArtifactPosition::CREATURE_SLOT);
			auto artHere = alHere.getArt();
			auto artDest = alDest.getArt();
			if (artHere)
			{
				if (alDest.getArt())
				{
					auto hero = dynamic_cast <CGHeroInstance *>(src.army.get());
					if (hero)
					{
						artDest->move (alDest, ArtifactLocation (hero, alDest.getArt()->firstBackpackSlot (hero)));
					}
					//else - artifact cna be lost :/
					else
					{
                        logNetwork->warnStream() << "Artifact is present at destination slot!";
					}
					artHere->move (alHere, alDest);
					//TODO: choose from dialog
				}
				else //just move to the other slot before stack gets erased
				{
					artHere->move (alHere, alDest);
				}
			}
			if (stackExp)
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
			UNUSED(c);
			if (stackExp)
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
			if (stackExp)
				dst.army->setStackExp(dst.slot, src.army->getStackExperience(src.slot));
		}
	}

	CBonusSystemNode::treeHasChanged();
}

DLL_LINKAGE void PutArtifact::applyGs( CGameState *gs )
{
	assert(art->canBePutAt(al));
	art->putAt(al);
	//al.hero->putArtifact(al.slot, art);
}

DLL_LINKAGE void EraseArtifact::applyGs( CGameState *gs )
{
	al.removeArtifact();
}

DLL_LINKAGE void MoveArtifact::applyGs( CGameState *gs )
{
	CArtifactInstance *a = src.getArt();
	if(dst.slot < GameConstants::BACKPACK_START)
		assert(!dst.getArt());

	a->move(src, dst);

	//TODO what'll happen if Titan's thunder is equipped by pickin git up or the start of game?
	if (a->artType->id == 135 && dst.slot == ArtifactPosition::RIGHT_HAND) //Titan's Thunder creates new spellbook on equip
	{
		auto hPtr = boost::get<ConstTransitivePtr<CGHeroInstance> >(&dst.artHolder);
		if(hPtr)
		{
			CGHeroInstance *h = *hPtr;
			if(h && !h->hasSpellbook())
				gs->giveHeroArtifact(h, ArtifactID::SPELLBOOK);
		}
	}
}

DLL_LINKAGE void AssembledArtifact::applyGs( CGameState *gs )
{
	CArtifactSet *artSet = al.getHolderArtSet();
	const CArtifactInstance *transformedArt = al.getArt();
	assert(transformedArt);
	assert(vstd::contains(transformedArt->assemblyPossibilities(artSet), builtArt));
	UNUSED(transformedArt);

	auto combinedArt = new CCombinedArtifactInstance(builtArt);
	gs->map->addNewArtifactInstance(combinedArt);
	//retrieve all constituents
	for(const CArtifact * constituent : *builtArt->constituents)
	{
		ArtifactPosition pos = artSet->getArtPos(constituent->id);
		assert(pos >= 0);
		CArtifactInstance *constituentInstance = artSet->getArt(pos);

		//move constituent from hero to be part of new, combined artifact
		constituentInstance->removeFrom(ArtifactLocation(al.artHolder, pos));
		combinedArt->addAsConstituent(constituentInstance, pos);
		if(!vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], al.slot) && vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], pos))
			al.slot = pos;
	}

	//put new combined artifacts
	combinedArt->putAt(al);
}

DLL_LINKAGE void DisassembledArtifact::applyGs( CGameState *gs )
{
	CCombinedArtifactInstance *disassembled = dynamic_cast<CCombinedArtifactInstance*>(al.getArt());
	assert(disassembled);

	std::vector<CCombinedArtifactInstance::ConstituentInfo> constituents = disassembled->constituentsInfo;
	disassembled->removeFrom(al);
	for(CCombinedArtifactInstance::ConstituentInfo &ci : constituents)
	{
		ArtifactLocation constituentLoc = al;
		constituentLoc.slot = (ci.slot >= 0 ? ci.slot : al.slot); //-1 is slot of main constituent -> it'll replace combined artifact in its pos
		disassembled->detachFrom(ci.art);
		ci.art->putAt(constituentLoc);
	}

	gs->map->eraseArtifactInstance(disassembled);
}

DLL_LINKAGE void HeroVisit::applyGs( CGameState *gs )
{
}

DLL_LINKAGE void SetAvailableArtifacts::applyGs( CGameState *gs )
{
	if(id >= 0)
	{
		if(CGBlackMarket *bm = dynamic_cast<CGBlackMarket*>(gs->map->objects[id].get()))
		{
			bm->artifacts = arts;
		}
		else
		{
            logNetwork->errorStream() << "Wrong black market id!";
		}
	}
	else
	{
		CGTownInstance::merchantArtifacts = arts;
	}
}

DLL_LINKAGE void NewTurn::applyGs( CGameState *gs )
{
	gs->day = day;
	for(NewTurn::Hero h : heroes) //give mana/movement point
	{
		CGHeroInstance *hero = gs->getHero(h.id);
		hero->movement = h.move;
		hero->mana = h.mana;
	}

	for(auto i = res.cbegin(); i != res.cend(); i++)
	{
		assert(i->first < PlayerColor::PLAYER_LIMIT);
		gs->getPlayer(i->first)->resources = i->second;
	}

	for(auto creatureSet : cres) //set available creatures in towns
		creatureSet.second.applyGs(gs);

	gs->globalEffects.popBonuses(Bonus::OneDay); //works for children -> all game objs
	if(gs->getDate(Date::DAY_OF_WEEK) == 1) //new week
		gs->globalEffects.popBonuses(Bonus::OneWeek); //works for children -> all game objs

	//TODO not really a single root hierarchy, what about bonuses placed elsewhere? [not an issue with H3 mechanics but in the future...]

	for(CGTownInstance* t : gs->map->towns)
		t->builded = 0;
}

DLL_LINKAGE void SetObjectProperty::applyGs( CGameState *gs )
{
	CGObjectInstance *obj = gs->getObjInstance(id);
	if(!obj)
	{
        logNetwork->errorStream() << "Wrong object ID - property cannot be set!";
		return;
	}

	CArmedInstance *cai = dynamic_cast<CArmedInstance *>(obj);
	if(what == ObjProperty::OWNER && cai)
	{
		if(obj->ID == Obj::TOWN)
		{
			CGTownInstance *t = static_cast<CGTownInstance*>(obj);
			if(t->tempOwner < PlayerColor::PLAYER_LIMIT)
				gs->getPlayer(t->tempOwner)->towns -= t;
			if(val < PlayerColor::PLAYER_LIMIT_I)
				gs->getPlayer(PlayerColor(val))->towns.push_back(t);
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

DLL_LINKAGE void HeroLevelUp::applyGs( CGameState *gs )
{
	CGHeroInstance * h = gs->getHero(hero->id);
	h->levelUp(skills);
}

DLL_LINKAGE void CommanderLevelUp::applyGs (CGameState *gs)
{
	CCommanderInstance * commander = gs->getHero(hero->id)->commander;
	assert (commander);
	commander->levelUp();
}

DLL_LINKAGE void BattleStart::applyGs( CGameState *gs )
{
	gs->curB = info;
	gs->curB->localInit();
}

DLL_LINKAGE void BattleNextRound::applyGs( CGameState *gs )
{
	for (int i = 0; i < 2; ++i)
	{
		gs->curB->sides[i].castSpellsCount = 0;
		vstd::amax(--gs->curB->sides[i].enchanterCounter, 0);
	}

	gs->curB->round = round;

	for(CStack *s : gs->curB->stacks)
	{
		s->state -= EBattleStackState::DEFENDING;
		s->state -= EBattleStackState::WAITING;
		s->state -= EBattleStackState::MOVED;
		s->state -= EBattleStackState::HAD_MORALE;
		s->state -= EBattleStackState::FEAR;
		s->state -= EBattleStackState::DRAINED_MANA;
		s->counterAttacks = 1 + s->valOfBonuses(Bonus::ADDITIONAL_RETALIATION);
		// new turn effects
		s->battleTurnPassed();
	}

	for(auto &obst : gs->curB->obstacles)
		obst->battleTurnPassed();
}

DLL_LINKAGE void BattleSetActiveStack::applyGs( CGameState *gs )
{
	gs->curB->activeStack = stack;
	CStack *st = gs->curB->getStack(stack);

	//remove bonuses that last until when stack gets new turn
	st->getBonusList().remove_if(Bonus::UntilGetsTurn);

	if(vstd::contains(st->state,EBattleStackState::MOVED)) //if stack is moving second time this turn it must had a high morale bonus
		st->state.insert(EBattleStackState::HAD_MORALE);
}

DLL_LINKAGE void BattleTriggerEffect::applyGs( CGameState *gs )
{
	CStack *st = gs->curB->getStack(stackID);
	switch (effect)
	{
		case Bonus::HP_REGENERATION:
			st->firstHPleft += val;
			vstd::amin (st->firstHPleft, (ui32)st->MaxHealth());
			break;
		case Bonus::MANA_DRAIN:
		{
			CGHeroInstance * h = gs->getHero(ObjectInstanceID(additionalInfo));
			st->state.insert (EBattleStackState::DRAINED_MANA);
			h->mana -= val;
			vstd::amax(h->mana, 0);
			break;
		}
		case Bonus::POISON:
		{
			Bonus * b = st->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT, 71)
											.And(Selector::type(Bonus::STACK_HEALTH)));
			if (b)
				b->val = val;
			break;
		}
		case Bonus::ENCHANTER:
			break;
		case Bonus::FEAR:
			st->state.insert(EBattleStackState::FEAR);
			break;
		default:
            logNetwork->warnStream() << "Unrecognized trigger effect type "<< type;
	}
}

DLL_LINKAGE void BattleObstaclePlaced::applyGs( CGameState *gs )
{
	gs->curB->obstacles.push_back(obstacle);
}

void BattleResult::applyGs( CGameState *gs )
{
	for (CStack *s : gs->curB->stacks)
	{
		if (s->base && s->base->armyObj && vstd::contains(s->state, EBattleStackState::SUMMONED))
		{
			//stack with SUMMONED flag but coming from garrison -> most likely resurrected, needs to be removed
			assert(&s->base->armyObj->getStack(s->slot) == s->base);
			const_cast<CArmedInstance*>(s->base->armyObj)->eraseStack(s->slot);
		}
	}
	for (auto & elem : gs->curB->stacks)
		delete elem;


	for(int i = 0; i < 2; ++i)
	{
		if(auto h = gs->curB->battleGetFightingHero(i))
		{
			h->getBonusList().remove_if(Bonus::OneBattle); 	//remove any "until next battle" bonuses
			if (h->commander && h->commander->alive)
			{
				for (auto art : h->commander->artifactsWorn) //increment bonuses for commander artifacts
				{
					art.second.artifact->artType->levelUpArtifact (art.second.artifact);
				}
			}
		}
	}

	if(VLC->modh->modules.STACK_EXP)
	{
		for(int i = 0; i < 2; i++)
			if(exp[i])
				gs->curB->battleGetArmyObject(i)->giveStackExp(exp[i]);

		CBonusSystemNode::treeHasChanged();
	}

	for(int i = 0; i < 2; i++)
		gs->curB->battleGetArmyObject(i)->battle = nullptr;

	gs->curB.dellNull();
}

void BattleStackMoved::applyGs( CGameState *gs )
{
	CStack *s = gs->curB->getStack(stack);
	assert(s);
	BattleHex dest = tilesToMove.back();

	//if unit ended movement on quicksands that were created by enemy, that quicksand patch becomes visible for owner
	for(auto &oi : gs->curB->obstacles)
	{
		if(oi->obstacleType == CObstacleInstance::QUICKSAND
		&& vstd::contains(oi->getAffectedTiles(), tilesToMove.back()))
		{
			SpellCreatedObstacle *sands = dynamic_cast<SpellCreatedObstacle*>(oi.get());
			assert(sands);
			if(sands->casterSide != !s->attackerOwned)
				sands->visibleForAnotherSide = true;
		}
	}
	s->position = dest;
}

DLL_LINKAGE void BattleStackAttacked::applyGs( CGameState *gs )
{
	CStack * at = gs->curB->getStack(stackAttacked);
	assert(at);
	at->count = newAmount;
	at->firstHPleft = newHP;

	if(killed())
	{
		at->state -= EBattleStackState::ALIVE;
	}
	//life drain handling
	for (auto & elem : healedStacks)
	{
		elem.applyGs(gs);
	}
	if (willRebirth())
	{
		at->casts--;
		at->state.insert(EBattleStackState::ALIVE); //hmm?
	}
	if (cloneKilled())
	{
		//"hide" killed creatures instead so we keep info about it
		at->state.insert(EBattleStackState::DEAD_CLONE);

	}
}

DLL_LINKAGE void BattleAttack::applyGs( CGameState *gs )
{
	CStack *attacker = gs->curB->getStack(stackAttacking);
	if(counter())
		attacker->counterAttacks--;

	if(shot())
	{
		//don't remove ammo if we have a working ammo cart
		bool hasAmmoCart = false;
		for(const CStack * st : gs->curB->stacks)
		{
			if(st->owner == attacker->owner && st->getCreature()->idNumber == CreatureID::AMMO_CART && st->alive())
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
	for(BattleStackAttacked stackAttacked : bsa)
		stackAttacked.applyGs(gs);

	attacker->getBonusList().remove_if(Bonus::UntilAttack);

	for(auto & elem : bsa)
	{
		CStack * stack = gs->curB->getStack(elem.stackAttacked, false);
		if (stack) //cloned stack is already gone
			stack->getBonusList().remove_if(Bonus::UntilBeingAttacked);
	}
}

DLL_LINKAGE void StartAction::applyGs( CGameState *gs )
{
	CStack *st = gs->curB->getStack(ba.stackNumber);

	if(ba.actionType == Battle::END_TACTIC_PHASE)
	{
		gs->curB->tacticDistance = 0;
		return;
	}

	if(gs->curB->tacticDistance)
	{
		// moves in tactics phase do not affect creature status
		// (tactics stack queue is managed by client)
		return;
	}

	if(ba.actionType != Battle::HERO_SPELL) //don't check for stack if it's custom action by hero
	{
		assert(st);
	}
	else
	{
		gs->curB->sides[ba.side].usedSpellsHistory.push_back(SpellID(ba.additionalInfo).toSpell());
	}

	switch(ba.actionType)
	{
	case Battle::DEFEND:
		st->state.insert(EBattleStackState::DEFENDING);
		break;
	case Battle::WAIT:
		st->state.insert(EBattleStackState::WAITING);
		return;
	case Battle::HERO_SPELL: //no change in current stack state
		return;
	default: //any active stack action - attack, catapult, heal, spell...
		st->state.insert(EBattleStackState::MOVED);
		break;
	}

	if(st)
		st->state -= EBattleStackState::WAITING; //if stack was waiting it has made move, so it won't be "waiting" anymore (if the action was WAIT, then we have returned)
}

DLL_LINKAGE void BattleSpellCast::applyGs( CGameState *gs )
{
	assert(gs->curB);
	if (castedByHero)
	{
		CGHeroInstance * h = gs->curB->battleGetFightingHero(side);
		CGHeroInstance * enemy = gs->curB->battleGetFightingHero(!side);

		h->mana -= spellCost;
			vstd::amax(h->mana, 0);
		if (enemy && manaGained)
			enemy->mana += manaGained;
		if (side < 2)
		{
			gs->curB->sides[side].castSpellsCount++;
		}
	}

	//Handle spells removing effects from stacks
	const CSpell *spell = SpellID(id).toSpell();
	const bool removeAllSpells = id == SpellID::DISPEL;
	const bool removeHelpful = id == SpellID::DISPEL_HELPFUL_SPELLS;

	for(auto stackID : affectedCres)
	{
		if(vstd::contains(resisted, stackID))
			continue;

		CStack *s = gs->curB->getStack(stackID);
		s->popBonuses([&](const Bonus *b) -> bool
		{
			//check for each bonus if it should be removed
			const bool isSpellEffect = Selector::sourceType(Bonus::SPELL_EFFECT)(b);
			const bool isPositiveSpell = Selector::positiveSpellEffects(b);
			const int spellID = isSpellEffect ? b->sid : -1;

			return (removeHelpful && isPositiveSpell)
				|| (removeAllSpells && isSpellEffect)
				|| vstd::contains(spell->counteredSpells, spellID);
		});
	}
}

void actualizeEffect(CStack * s, const std::vector<Bonus> & ef)
{
	//actualizing features vector

	for(const Bonus &fromEffect : ef)
	{
		for(Bonus *stackBonus : s->getBonusList()) //TODO: optimize
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
	for(Bonus *stackBonus : s->getBonusList()) //TODO: optimize
	{
		if(stackBonus->source == Bonus::SPELL_EFFECT && stackBonus->type == ef.type && stackBonus->subtype == ef.subtype)
		{
			stackBonus->turnsRemain = std::max(stackBonus->turnsRemain, ef.turnsRemain);
		}
	}
}

DLL_LINKAGE void SetStackEffect::applyGs( CGameState *gs )
{
    if (effect.empty())
    {
        logGlobal->errorStream() << "Trying to apply SetStackEffect with no effects";
        return;
    }

	int spellid = effect.begin()->sid; //effects' source ID

	for(ui32 id : stacks)
	{
		CStack *s = gs->curB->getStack(id);
		if(s)
		{
			if(spellid == SpellID::DISRUPTING_RAY || spellid == SpellID::ACID_BREATH_DEFENSE || !s->hasBonus(Selector::source(Bonus::SPELL_EFFECT, spellid)))//disrupting ray or acid breath or not on the list - just add
			{
				for(Bonus &fromEffect : effect)
				{
					logBonus->traceStream() << s->nodeName() << " receives a new bonus: " << fromEffect.Description();
					s->addNewBonus( new Bonus(fromEffect));
				}
			}
			else //just actualize
			{
				actualizeEffect(s, effect);
			}
		}
		else
            logNetwork->errorStream() << "Cannot find stack " << id;
	}
	typedef std::pair<ui32, Bonus> p;
	for(p para : uniqueBonuses)
	{
		CStack *s = gs->curB->getStack(para.first);
		if (s)
		{
			if (!s->hasBonus(Selector::source(Bonus::SPELL_EFFECT, spellid).And(Selector::typeSubtype(para.second.type, para.second.subtype))))
				s->addNewBonus(new Bonus(para.second));
			else
				actualizeEffect(s, effect);
		}
		else
            logNetwork->errorStream() << "Cannot find stack " << para.first;
	}
}

DLL_LINKAGE void StacksInjured::applyGs( CGameState *gs )
{
	for(BattleStackAttacked stackAttacked : stacks)
		stackAttacked.applyGs(gs);
}

DLL_LINKAGE void StacksHealedOrResurrected::applyGs( CGameState *gs )
{
	for(auto & elem : healedStacks)
	{
		CStack * changedStack = gs->curB->getStack(elem.stackID, false);

		//checking if we resurrect a stack that is under a living stack
		auto accessibility = gs->curB->getAccesibility();

		if(!changedStack->alive() && !accessibility.accessible(changedStack->position, changedStack))
		{
            logNetwork->errorStream() << "Cannot resurrect " << changedStack->nodeName() << " because hex " << changedStack->position << " is occupied!";
			return; //position is already occupied
		}

		//applying changes
		bool resurrected = !changedStack->alive(); //indicates if stack is resurrected or just healed
		if(resurrected)
		{
			changedStack->state.insert(EBattleStackState::ALIVE);
		}
		//int missingHPfirst = changedStack->MaxHealth() - changedStack->firstHPleft;
		int res = std::min( elem.healedHP / changedStack->MaxHealth() , changedStack->baseAmount - changedStack->count );
		changedStack->count += res;
		if(elem.lowLevelResurrection)
			changedStack->resurrected += res;
		changedStack->firstHPleft += elem.healedHP - res * changedStack->MaxHealth();
		if(changedStack->firstHPleft > changedStack->MaxHealth())
		{
			changedStack->firstHPleft -= changedStack->MaxHealth();
			if(changedStack->baseAmount > changedStack->count)
			{
				changedStack->count += 1;
			}
		}
		vstd::amin(changedStack->firstHPleft, changedStack->MaxHealth());
		//removal of negative effects
		if(resurrected)
		{

// 			for (BonusList::iterator it = changedStack->bonuses.begin(); it != changedStack->bonuses.end(); it++)
// 			{
// 				if(VLC->spellh->spells[(*it)->sid]->positiveness < 0)
// 				{
// 					changedStack->bonuses.erase(it);
// 				}
// 			}

			//removing all features from negative spells
			const BonusList tmpFeatures = changedStack->getBonusList();
			//changedStack->bonuses.clear();

			for(Bonus *b : tmpFeatures)
			{
				const CSpell *s = b->sourceSpell();
				if(s && s->isNegative())
				{
					changedStack->removeBonus(b);
				}
			}
		}
	}
}

DLL_LINKAGE void ObstaclesRemoved::applyGs( CGameState *gs )
{
	if(gs->curB) //if there is a battle
	{
		for(const si32 rem_obst :obstacles)
		{
			for(int i=0; i<gs->curB->obstacles.size(); ++i)
			{
				if(gs->curB->obstacles[i]->uniqueID == rem_obst) //remove this obstacle
				{
					gs->curB->obstacles.erase(gs->curB->obstacles.begin() + i);
					break;
				}
			}
		}
	}
}


DLL_LINKAGE CatapultAttack::CatapultAttack()
{
	type = 3015;
}

DLL_LINKAGE CatapultAttack::~CatapultAttack()
{
}

DLL_LINKAGE void CatapultAttack::applyGs( CGameState *gs )
{
	if(gs->curB && gs->curB->town && gs->curB->town->fortLevel() != CGTownInstance::NONE) //if there is a battle and it's a siege
	{
		for(const auto &it :attackedParts)
		{
			gs->curB->si.wallState[it.attackedPart] =
			        SiegeInfo::applyDamage(EWallState::EWallState(gs->curB->si.wallState[it.attackedPart]), it.damageDealt);
		}
	}
}

DLL_LINKAGE std::string CatapultAttack::AttackInfo::toString() const
{
	return boost::str(boost::format("{AttackInfo: destinationTile '%d', attackedPart '%d', damageDealt '%d'}")
					  % destinationTile % static_cast<int>(attackedPart) % static_cast<int>(damageDealt));
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CatapultAttack::AttackInfo & attackInfo)
{
	return out << attackInfo.toString();
}

DLL_LINKAGE std::string CatapultAttack::toString() const
{
	return boost::str(boost::format("{CatapultAttack: attackedParts '%s', attacker '%d'}") % attackedParts % attacker);
}

DLL_LINKAGE void BattleStacksRemoved::applyGs( CGameState *gs )
{
	if(!gs->curB)
		return;
	for(ui32 rem_stack : stackIDs)
	{
		for(int b=0; b<gs->curB->stacks.size(); ++b) //find it in vector of stacks
		{
			if(gs->curB->stacks[b]->ID == rem_stack) //if found
			{
				CStack *toRemove = gs->curB->stacks[b];
				gs->curB->stacks.erase(gs->curB->stacks.begin() + b); //remove

				toRemove->detachFromAll();
				delete toRemove;
				break;
			}
		}
	}
}

DLL_LINKAGE void BattleStackAdded::applyGs(CGameState *gs)
{
	if (!BattleHex(pos).isValid())
	{
        logNetwork->warnStream() << "No place found for new stack!";
		return;
	}

	CStackBasicDescriptor csbd(creID, amount);
	CStack * addedStack = gs->curB->generateNewStack(csbd, attacker, SlotID(255), pos); //TODO: netpacks?
	if (summoned)
		addedStack->state.insert(EBattleStackState::SUMMONED);

	gs->curB->localInitStack(addedStack);
	gs->curB->stacks.push_back(addedStack); //the stack is not "SUMMONED", it is permanent
}

DLL_LINKAGE void BattleSetStackProperty::applyGs(CGameState *gs)
{
	CStack * stack = gs->curB->getStack(stackID);
	switch (which)
	{
		case CASTS:
		{
			if (absolute)
				stack->casts = val;
			else
				stack->casts += val;
			vstd::amax(stack->casts, 0);
			break;
		}
		case ENCHANTER_COUNTER:
		{
			auto & counter = gs->curB->sides[gs->curB->whatSide(stack->owner)].enchanterCounter;
			if (absolute)
				counter = val;
			else
				counter += val;
			vstd::amax(counter, 0);
			break;
		}
		case UNBIND:
		{
			stack->popBonuses(Selector::type(Bonus::BIND_EFFECT));
			break;
		}
		case CLONED:
		{
			stack->state.insert(EBattleStackState::CLONED);
			break;
		}
	}
}

DLL_LINKAGE void YourTurn::applyGs( CGameState *gs )
{
	gs->currentPlayer = player;

	//count days without town
	auto & playerState = gs->players[player];
	if(playerState.towns.empty())
	{
		if(playerState.daysWithoutCastle)
			++(*playerState.daysWithoutCastle);
		else playerState.daysWithoutCastle = 0;
	}
	else
	{
		playerState.daysWithoutCastle = boost::none;
	}
}

DLL_LINKAGE Component::Component(const CStackBasicDescriptor &stack)
	: id(CREATURE), subtype(stack.type->idNumber), val(stack.count), when(0)
{
	type = 2002;
}
