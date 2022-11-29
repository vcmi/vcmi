/*
 * NetPacksLib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
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
#include "spells/CSpellHandler.h"
#include "CCreatureHandler.h"
#include "CGameState.h"
#include "CStack.h"
#include "battle/BattleInfo.h"
#include "CTownHandler.h"
#include "mapping/CMapInfo.h"
#include "StartInfo.h"
#include "CPlayerState.h"

VCMI_LIB_NAMESPACE_BEGIN


DLL_LINKAGE void SetResources::applyGs(CGameState *gs)
{
	assert(player < PlayerColor::PLAYER_LIMIT);
	if(abs)
		gs->getPlayerState(player)->resources = res;
	else
		gs->getPlayerState(player)->resources += res;

	//just ensure that player resources are not negative
	//server is responsible to check if player can afford deal
	//but events on server side are allowed to take more than player have
	gs->getPlayerState(player)->resources.positive();
}

DLL_LINKAGE void SetPrimSkill::applyGs(CGameState *gs)
{
	CGHeroInstance * hero = gs->getHero(id);
	assert(hero);
	hero->setPrimarySkill(which, val, abs);
}

DLL_LINKAGE void SetSecSkill::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(id);
	hero->setSecSkillLevel(which, val, abs);
}

DLL_LINKAGE void SetCommanderProperty::applyGs(CGameState *gs)
{
	CCommanderInstance * commander = gs->getHero(heroid)->commander;
	assert (commander);

	switch (which)
	{
		case BONUS:
			commander->accumulateBonus (std::make_shared<Bonus>(accumulatedBonus));
			break;
		case SPECIAL_SKILL:
			commander->accumulateBonus (std::make_shared<Bonus>(accumulatedBonus));
			commander->specialSKills.insert (additionalInfo);
			break;
		case SECONDARY_SKILL:
			commander->secondarySkills[additionalInfo] = static_cast<ui8>(amount);
			break;
		case ALIVE:
			if (amount)
				commander->setAlive(true);
			else
				commander->setAlive(false);
			break;
		case EXPERIENCE:
			commander->giveStackExp(amount); //TODO: allow setting exp for stacks via netpacks
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
		logNetwork->warn("Warning! Attempt to add duplicated quest");
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

DLL_LINKAGE void ChangeFormation::applyGs(CGameState *gs)
{
	gs->getHero(hid)->setFormation(formation);
}

DLL_LINKAGE void HeroVisitCastle::applyGs(CGameState *gs)
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

DLL_LINKAGE void ChangeSpells::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(hid);

	if(learn)
		for(auto sid : spells)
			hero->addSpellToSpellbook(sid);
	else
		for(auto sid : spells)
			hero->removeSpellFromSpellbook(sid);
}

DLL_LINKAGE void SetMana::applyGs(CGameState *gs)
{
	CGHeroInstance * hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->mana = val;
	else
		hero->mana += val;

	vstd::amax(hero->mana, 0); //not less than 0
}

DLL_LINKAGE void SetMovePoints::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->movement = val;
	else
		hero->movement += val;

	vstd::amax(hero->movement, 0); //not less than 0
}

DLL_LINKAGE void FoWChange::applyGs(CGameState *gs)
{
	TeamState * team = gs->getPlayerTeam(player);
	auto fogOfWarMap = team->fogOfWarMap;
	for(int3 t : tiles)
		(*fogOfWarMap)[t.z][t.x][t.y] = mode;
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
						gs->getTilesInRange(tilesRevealed, o->getSightCenter(), o->getSightRadius(), o->tempOwner, 1);
					break;
				}
			}
		}
		for(int3 t : tilesRevealed) //probably not the most optimal solution ever
			(*fogOfWarMap)[t.z][t.x][t.y] = 1;
	}
}

DLL_LINKAGE void SetAvailableHeroes::applyGs(CGameState *gs)
{
	PlayerState *p = gs->getPlayerState(player);
	p->availableHeroes.clear();

	for (int i = 0; i < GameConstants::AVAILABLE_HEROES_PER_PLAYER; i++)
	{
		CGHeroInstance *h = (hid[i]>=0 ?  gs->hpool.heroesPool[hid[i]].get() : nullptr);
		if(h && army[i])
			h->setToArmy(army[i]);
		p->availableHeroes.push_back(h);
	}
}

DLL_LINKAGE void GiveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode *cbsn = nullptr;
	switch(who)
	{
	case HERO:
		cbsn = gs->getHero(ObjectInstanceID(id));
		break;
	case PLAYER:
		cbsn = gs->getPlayerState(PlayerColor(id));
		break;
	case TOWN:
		cbsn = gs->getTown(ObjectInstanceID(id));
		break;
	}

	assert(cbsn);

	if(Bonus::OneWeek(&bonus))
		bonus.turnsRemain = 8 - gs->getDate(Date::DAY_OF_WEEK); // set correct number of days before adding bonus

	auto b = std::make_shared<Bonus>(bonus);
	cbsn->addNewBonus(b);

	std::string &descr = b->description;

	if(!bdescr.message.size()
		&& (bonus.type == Bonus::LUCK || bonus.type == Bonus::MORALE))
	{
		if (bonus.source == Bonus::OBJECT)
		{
			descr = VLC->generaltexth->arraytxt[bonus.val > 0 ? 110 : 109]; //+/-%d Temporary until next battle"
		}
		else if(bonus.source == Bonus::TOWN_STRUCTURE)
		{
			descr = bonus.description;
			return;
		}
		else
		{
			bdescr.toString(descr);
		}
	}
	else
	{
		bdescr.toString(descr);
	}
	// Some of(?) versions of H3 use %s here instead of %d. Try to replace both of them
	boost::replace_first(descr,"%d",boost::lexical_cast<std::string>(std::abs(bonus.val)));
	boost::replace_first(descr,"%s",boost::lexical_cast<std::string>(std::abs(bonus.val)));
}

DLL_LINKAGE void ChangeObjPos::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(objid);
	if(!obj)
	{
		logNetwork->error("Wrong ChangeObjPos: object %d doesn't exist!", objid.getNum());
		return;
	}
	gs->map->removeBlockVisTiles(obj);
	obj->pos = nPos;
	gs->map->addBlockVisTiles(obj);
}

DLL_LINKAGE void ChangeObjectVisitors::applyGs(CGameState *gs)
{
	switch (mode) {
		case VISITOR_ADD:
			gs->getHero(hero)->visitedObjects.insert(object);
			gs->getPlayerState(gs->getHero(hero)->tempOwner)->visitedObjects.insert(object);
			break;
		case VISITOR_ADD_TEAM:
			{
				TeamState *ts = gs->getPlayerTeam(gs->getHero(hero)->tempOwner);
				for (auto & color : ts->players)
				{
					gs->getPlayerState(color)->visitedObjects.insert(object);
				}
			}
			break;
		case VISITOR_CLEAR:
			for (CGHeroInstance * hero : gs->map->allHeroes)
			{
				if (hero)
				{
					hero->visitedObjects.erase(object); // remove visit info from all heroes, including those that are not present on map
				}
			}

			for(auto &elem : gs->players)
			{
				elem.second.visitedObjects.erase(object);
			}

			break;
		case VISITOR_REMOVE:
			gs->getHero(hero)->visitedObjects.erase(object);
			break;
	}
}

DLL_LINKAGE void PlayerEndsGame::applyGs(CGameState *gs)
{
	PlayerState *p = gs->getPlayerState(player);
	if(victoryLossCheckResult.victory())
	{
		p->status = EPlayerStatus::WINNER;

		// TODO: Campaign-specific code might as well go somewhere else
		if(p->human && gs->scenarioOps->campState)
		{
			std::vector<CGHeroInstance *> crossoverHeroes;
			for (CGHeroInstance * hero : gs->map->heroesOnMap)
			{
				if (hero->tempOwner == player)
				{
					// keep all heroes from the winning player
					crossoverHeroes.push_back(hero);
				}
				else if (vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(hero->subID)))
				{
					// keep hero whether lost or won (like Xeron in AB campaign)
					crossoverHeroes.push_back(hero);
				}
			}
			// keep lost heroes which are in heroes pool
			for (auto & heroPair : gs->hpool.heroesPool)
			{
				if (vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(heroPair.first)))
				{
					crossoverHeroes.push_back(heroPair.second.get());
				}
			}

			gs->scenarioOps->campState->setCurrentMapAsConquered(crossoverHeroes);
		}
	}
	else
	{
		p->status = EPlayerStatus::LOSER;
	}
}

DLL_LINKAGE void PlayerReinitInterface::applyGs(CGameState *gs)
{
	if(!gs || !gs->scenarioOps)
		return;
	
	//TODO: what does mean if more that one player connected?
	if(playerConnectionId == PlayerSettings::PLAYER_AI)
	{
		for(auto player : players)
			gs->scenarioOps->getIthPlayersSettings(player).connectedPlayerIDs.clear();
	}
}

DLL_LINKAGE void RemoveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode *node;
	if (who == HERO)
		node = gs->getHero(ObjectInstanceID(whoID));
	else
		node = gs->getPlayerState(PlayerColor(whoID));

	BonusList &bonuses = node->getExportedBonusList();

	for (int i = 0; i < bonuses.size(); i++)
	{
		auto b = bonuses[i];
		if(b->source == source && b->sid == id)
		{
			bonus = *b; //backup bonus (to show to interfaces later)
			node->removeBonus(b);
			break;
		}
	}
}

DLL_LINKAGE void RemoveObject::applyGs(CGameState *gs)
{

	CGObjectInstance *obj = gs->getObjInstance(id);
	logGlobal->debug("removing object id=%d; address=%x; name=%s", id, (intptr_t)obj, obj->getObjectName());
	//unblock tiles
	gs->map->removeBlockVisTiles(obj);

	if(obj->ID == Obj::HERO) //remove beaten hero
	{
		CGHeroInstance * beatenHero = static_cast<CGHeroInstance*>(obj);
		PlayerState * p = gs->getPlayerState(beatenHero->tempOwner);
		gs->map->heroesOnMap -= beatenHero;
		p->heroes -= beatenHero;
		beatenHero->detachFrom(*beatenHero->whereShouldBeAttachedOnSiege(gs));
		beatenHero->tempOwner = PlayerColor::NEUTRAL; //no one owns beaten hero
		vstd::erase_if(beatenHero->artifactsInBackpack, [](const ArtSlotInfo& asi)
		{
			return asi.artifact->artType->id == ArtifactID::GRAIL;
		});

		if(beatenHero->visitedTown)
		{
			if(beatenHero->visitedTown->garrisonHero == beatenHero)
				beatenHero->visitedTown->garrisonHero = nullptr;
			else
				beatenHero->visitedTown->visitingHero = nullptr;

			beatenHero->visitedTown = nullptr;
			beatenHero->inTownGarrison = false;
		}
		//return hero to the pool, so he may reappear in tavern
		gs->hpool.heroesPool[beatenHero->subID] = beatenHero;

		if(!vstd::contains(gs->hpool.pavailable, beatenHero->subID))
			gs->hpool.pavailable[beatenHero->subID] = 0xff;

		gs->map->objects[id.getNum()] = nullptr;

		//If hero on Boat is removed, the Boat disappears
		if(beatenHero->boat)
		{
			gs->map->instanceNames.erase(beatenHero->boat->instanceName);
			gs->map->objects[beatenHero->boat->id.getNum()].dellNull();
			beatenHero->boat = nullptr;
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

	for(TriggeredEvent & event : gs->map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if(cond.object == nullptr && cond.condition == EventCondition::DESTROY_0 && cond.value > 0)
			{
				if(obj->ID == cond.objectType && cond.metaType == EMetaclass::OBJECT)
				{
					if(cond.value == 1)
						cond.condition = EventCondition::CONST_VALUE;
					else
						cond.value--;
				}
			}
			else if(cond.object == obj)
			{
				if(cond.condition == EventCondition::DESTROY || cond.condition == EventCondition::DESTROY_0)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 1; // destroyed object, from now on always fulfilled
				}
				else if(cond.condition == EventCondition::CONTROL || cond.condition == EventCondition::HAVE_0)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 0; // destroyed object, from now on can not be fulfilled
				}
				cond.object = nullptr;
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
	gs->map->instanceNames.erase(obj->instanceName);
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

void TryMoveHero::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->getHero(id);
	if (!h)
	{
		logGlobal->error("Attempt ot move unavailable hero %d", id.getNum());
		return;
	}

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

	auto fogOfWarMap = gs->getPlayerTeam(h->getOwner())->fogOfWarMap;
	for(int3 t : fowRevealed)
		(*fogOfWarMap)[t.z][t.x][t.y] = 1;
}

DLL_LINKAGE void NewStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);

	for(const auto & id : bid)
	{
		assert(t->town->buildings.at(id) != nullptr);
		t->builtBuildings.insert(id);
		t->updateAppearance();
		auto currentBuilding = t->town->buildings.at(id);

		if(currentBuilding->overrideBids.empty())
			continue;

		for(auto overrideBid : currentBuilding->overrideBids)
		{
			t->overriddenBuildings.insert(overrideBid);
			t->deleteTownBonus(overrideBid);
		}
	}
	t->builded = builded;
	t->recreateBuildingsBonuses();
}

DLL_LINKAGE void RazeStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);
	for(const auto & id : bid)
	{
		t->builtBuildings.erase(id);

		t->updateAppearance();
	}
	t->destroyed = destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

DLL_LINKAGE void SetAvailableCreatures::applyGs(CGameState *gs)
{
	CGDwelling *dw = dynamic_cast<CGDwelling*>(gs->getObjInstance(tid));
	assert(dw);
	dw->creatures = creatures;
}

DLL_LINKAGE void SetHeroesInTown::applyGs(CGameState *gs)
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

DLL_LINKAGE void HeroRecruited::applyGs(CGameState *gs)
{
	assert(vstd::contains(gs->hpool.heroesPool, hid));
	CGHeroInstance *h = gs->hpool.heroesPool[hid];
	CGTownInstance *t = gs->getTown(tid);
	PlayerState *p = gs->getPlayerState(player);

	assert(!h->boat);

	h->setOwner(player);
	h->pos = tile;
	bool fresh = !h->isInitialized();
	if(fresh)
	{ // this is a fresh hero who hasn't appeared yet
		h->movement = h->maxMovePoints(true);
	}

	gs->hpool.heroesPool.erase(hid);
	if(h->id == ObjectInstanceID())
	{
		h->id = ObjectInstanceID((si32)gs->map->objects.size());
		gs->map->objects.push_back(h);
	}
	else
		gs->map->objects[h->id.getNum()] = h;

	gs->map->heroesOnMap.push_back(h);
	p->heroes.push_back(h);
	h->attachTo(*p);
	if(fresh)
	{
		h->initObj(gs->getRandomGenerator());
	}
	gs->map->addBlockVisTiles(h);

	if(t)
	{
		t->setVisitingHero(h);
	}
}

DLL_LINKAGE void GiveHero::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->getHero(id);

	//bonus system
	h->detachFrom(gs->globalEffects);
	h->attachTo(*gs->getPlayerState(player));
	h->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, h->type->heroClass->getIndex())->getTemplates().front();

	gs->map->removeBlockVisTiles(h,true);
	h->setOwner(player);
	h->movement =  h->maxMovePoints(true);
	gs->map->heroesOnMap.push_back(h);
	gs->getPlayerState(h->getOwner())->heroes.push_back(h);
	gs->map->addBlockVisTiles(h);
	h->inTownGarrison = false;
}

DLL_LINKAGE void NewObject::applyGs(CGameState *gs)
{
	TerrainId terrainType = Terrain::BORDER;

	if(ID == Obj::BOAT && !gs->isInTheMap(pos)) //special handling for bug #3060 - pos outside map but visitablePos is not
	{
		CGObjectInstance testObject = CGObjectInstance();
		testObject.pos = pos;
		testObject.appearance = VLC->objtypeh->getHandlerFor(ID, subID)->getTemplates(Terrain::WATER).front();

		const int3 previousXAxisTile = int3(pos.x - 1, pos.y, pos.z);
		assert(gs->isInTheMap(previousXAxisTile) && (testObject.visitablePos() == previousXAxisTile));
		UNUSED(previousXAxisTile);
	}
	else
	{
		const TerrainTile & t = gs->map->getTile(pos);
		terrainType = t.terType->id;
	}

	CGObjectInstance *o = nullptr;
	switch(ID)
	{
	case Obj::BOAT:
		o = new CGBoat();
		terrainType = Terrain::WATER; //TODO: either boat should only spawn on water, or all water objects should be handled this way
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
	id = o->id = ObjectInstanceID((si32)gs->map->objects.size());

	gs->map->objects.push_back(o);
	gs->map->addBlockVisTiles(o);
	o->initObj(gs->getRandomGenerator());
	gs->map->calculateGuardingGreaturePositions();

	logGlobal->debug("Added object id=%d; address=%x; name=%s", id, (intptr_t)o, o->getObjectName());
}

DLL_LINKAGE void NewArtifact::applyGs(CGameState *gs)
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
		logNetwork->warn("%s don't have a stack at slot %d", army->nodeName(), slot.getNum());
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
	auto s = getSlot();
	if(s)
		return s->getArt();
	else
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

DLL_LINKAGE void ChangeStackCount::applyGs(CGameState * gs)
{
	auto srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] ChangeStackCount: invalid army object %d, possible game state corruption.", army.getNum());

	if(absoluteValue)
		srcObj->setStackCount(slot, count);
	else
		srcObj->changeStackCount(slot, count);
}

DLL_LINKAGE void SetStackType::applyGs(CGameState * gs)
{
	auto srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] SetStackType: invalid army object %d, possible game state corruption.", army.getNum());

	srcObj->setStackType(slot, type);
}

DLL_LINKAGE void EraseStack::applyGs(CGameState * gs)
{
	auto srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] EraseStack: invalid army object %d, possible game state corruption.", army.getNum());

	srcObj->eraseStack(slot);
}

DLL_LINKAGE void SwapStacks::applyGs(CGameState * gs)
{
	auto srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		logNetwork->error("[CRITICAL] SwapStacks: invalid army object %d, possible game state corruption.", srcArmy.getNum());

	auto dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		logNetwork->error("[CRITICAL] SwapStacks: invalid army object %d, possible game state corruption.", dstArmy.getNum());

	CStackInstance * s1 = srcObj->detachStack(srcSlot);
	CStackInstance * s2 = dstObj->detachStack(dstSlot);

	srcObj->putStack(srcSlot, s2);
	dstObj->putStack(dstSlot, s1);
}

DLL_LINKAGE void InsertNewStack::applyGs(CGameState *gs)
{
	if(auto obj = gs->getArmyInstance(army))
		obj->putStack(slot, new CStackInstance(type, count));
	else
		logNetwork->error("[CRITICAL] InsertNewStack: invalid army object %d, possible game state corruption.", army.getNum());
}

DLL_LINKAGE void RebalanceStacks::applyGs(CGameState * gs)
{
	auto srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		logNetwork->error("[CRITICAL] RebalanceStacks: invalid army object %d, possible game state corruption.", srcArmy.getNum());

	auto dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		logNetwork->error("[CRITICAL] RebalanceStacks: invalid army object %d, possible game state corruption.", dstArmy.getNum());

	StackLocation src(srcObj, srcSlot);
	StackLocation dst(dstObj, dstSlot);

	const CCreature * srcType = src.army->getCreature(src.slot);
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
						logNetwork->warn("Artifact is present at destination slot!");
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

DLL_LINKAGE void BulkRebalanceStacks::applyGs(CGameState * gs)
{
	for(auto & move : moves)
		move.applyGs(gs);
}

DLL_LINKAGE void BulkSmartRebalanceStacks::applyGs(CGameState * gs)
{
	for(auto & move : moves)
		move.applyGs(gs);

	for(auto & change : changes)
		change.applyGs(gs);
}

DLL_LINKAGE void PutArtifact::applyGs(CGameState *gs)
{
	assert(art->canBePutAt(al));
	art->putAt(al);
	//al.hero->putArtifact(al.slot, art);
}

DLL_LINKAGE void EraseArtifact::applyGs(CGameState *gs)
{
	auto slot = al.getSlot();
	if(slot->locked)
	{
		logGlobal->debug("Erasing locked artifact: %s", slot->artifact->artType->getName());
		DisassembledArtifact dis;
		dis.al.artHolder = al.artHolder;
		auto aset = al.getHolderArtSet();
		#ifndef NDEBUG
		bool found = false;
        #endif
		for(auto& p : aset->artifactsWorn)
		{
			auto art = p.second.artifact;
			if(art->canBeDisassembled() && art->isPart(slot->artifact))
			{
				dis.al.slot = aset->getArtPos(art);
				#ifndef NDEBUG
				found = true;
                #endif
				break;
			}
		}
		assert(found && "Failed to determine the assembly this locked artifact belongs to");
		logGlobal->debug("Found the corresponding assembly: %s", dis.al.getSlot()->artifact->artType->getName());
		dis.applyGs(gs);
	}
	else
	{
		logGlobal->debug("Erasing artifact %s", slot->artifact->artType->getName());
	}
	al.removeArtifact();
}

DLL_LINKAGE void MoveArtifact::applyGs(CGameState * gs)
{
	CArtifactInstance * art = src.getArt();
	if(!ArtifactUtils::isSlotBackpack(dst.slot))
		assert(!dst.getArt());

	art->move(src, dst);
}

DLL_LINKAGE void BulkMoveArtifacts::applyGs(CGameState * gs)
{
	enum class EBulkArtsOp
	{
		BULK_MOVE,
		BULK_REMOVE,
		BULK_PUT
	};

	auto bulkArtsOperation = [this](std::vector<LinkedSlots> & artsPack, 
		CArtifactSet * artSet, EBulkArtsOp operation) -> void
	{
		int numBackpackArtifactsMoved = 0;
		for(auto & slot : artsPack)
		{
			// When an object gets removed from the backpack, the backpack shrinks
			// so all the following indices will be affected. Thus, we need to update
			// the subsequent artifact slots to account for that
			auto srcPos = slot.srcPos;
			if(ArtifactUtils::isSlotBackpack(srcPos) && (operation != EBulkArtsOp::BULK_PUT))
			{
				srcPos = ArtifactPosition(srcPos.num - numBackpackArtifactsMoved);
			}
			auto slotInfo = artSet->getSlot(srcPos);
			assert(slotInfo);
			auto art = const_cast<CArtifactInstance*>(slotInfo->getArt());
			assert(art);
			switch(operation)
			{
			case EBulkArtsOp::BULK_MOVE:
				const_cast<CArtifactInstance*>(art)->move(
					ArtifactLocation(srcArtHolder, srcPos), ArtifactLocation(dstArtHolder, slot.dstPos));
				break;
			case EBulkArtsOp::BULK_REMOVE:
				art->removeFrom(ArtifactLocation(dstArtHolder, srcPos));
				break;
			case EBulkArtsOp::BULK_PUT:
				art->putAt(ArtifactLocation(srcArtHolder, slot.dstPos));
				break;
			default:
				break;
			}

			if(srcPos >= GameConstants::BACKPACK_START)
			{
				numBackpackArtifactsMoved++;
			}
		}
	};
	
	if(swap)
	{
		// Swap
		auto leftSet = getSrcHolderArtSet();
		auto rightSet = getDstHolderArtSet();
		CArtifactFittingSet artFittingSet(leftSet->bearerType());

		artFittingSet.artifactsWorn = rightSet->artifactsWorn;
		artFittingSet.artifactsInBackpack = rightSet->artifactsInBackpack;

		bulkArtsOperation(artsPack1, rightSet, EBulkArtsOp::BULK_REMOVE);
		bulkArtsOperation(artsPack0, leftSet, EBulkArtsOp::BULK_MOVE);
		bulkArtsOperation(artsPack1, &artFittingSet, EBulkArtsOp::BULK_PUT);
	}
	else
	{
		bulkArtsOperation(artsPack0, getSrcHolderArtSet(), EBulkArtsOp::BULK_MOVE);
	}
}

DLL_LINKAGE void AssembledArtifact::applyGs(CGameState *gs)
{
	CArtifactSet * artSet = al.getHolderArtSet();
	const CArtifactInstance *transformedArt = al.getArt();
	assert(transformedArt);
	bool combineEquipped = !ArtifactUtils::isSlotBackpack(al.slot);
	assert(vstd::contains(transformedArt->assemblyPossibilities(artSet, combineEquipped), builtArt));
	UNUSED(transformedArt);

	auto combinedArt = new CCombinedArtifactInstance(builtArt);
	gs->map->addNewArtifactInstance(combinedArt);
	// Retrieve all constituents
	for(const CArtifact * constituent : *builtArt->constituents)
	{
		ArtifactPosition pos = combineEquipped ? artSet->getArtPos(constituent->id, true, false) :
			artSet->getArtBackpackPos(constituent->id);
		assert(pos >= 0);
		CArtifactInstance * constituentInstance = artSet->getArt(pos);

		//move constituent from hero to be part of new, combined artifact
		constituentInstance->removeFrom(ArtifactLocation(al.artHolder, pos));
		combinedArt->addAsConstituent(constituentInstance, pos);
		if(combineEquipped)
		{
			if(!vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], al.slot)
				&& vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], pos))
				al.slot = pos;
		}
		else
		{
			al.slot = std::min(al.slot, pos);
		}
	}

	//put new combined artifacts
	combinedArt->putAt(al);
}

DLL_LINKAGE void DisassembledArtifact::applyGs(CGameState *gs)
{
	CCombinedArtifactInstance *disassembled = dynamic_cast<CCombinedArtifactInstance*>(al.getArt());
	assert(disassembled);

	std::vector<CCombinedArtifactInstance::ConstituentInfo> constituents = disassembled->constituentsInfo;
	disassembled->removeFrom(al);
	for(CCombinedArtifactInstance::ConstituentInfo &ci : constituents)
	{
		ArtifactLocation constituentLoc = al;
		constituentLoc.slot = (ci.slot >= 0 ? ci.slot : al.slot); //-1 is slot of main constituent -> it'll replace combined artifact in its pos
		disassembled->detachFrom(*ci.art);
		ci.art->putAt(constituentLoc);
	}

	gs->map->eraseArtifactInstance(disassembled);
}

DLL_LINKAGE void HeroVisit::applyGs(CGameState *gs)
{
}

DLL_LINKAGE void SetAvailableArtifacts::applyGs(CGameState *gs)
{
	if(id >= 0)
	{
		if(CGBlackMarket *bm = dynamic_cast<CGBlackMarket*>(gs->map->objects[id].get()))
		{
			bm->artifacts = arts;
		}
		else
		{
			logNetwork->error("Wrong black market id!");
		}
	}
	else
	{
		CGTownInstance::merchantArtifacts = arts;
	}
}

DLL_LINKAGE void NewTurn::applyGs(CGameState *gs)
{
	gs->day = day;

	// Update bonuses before doing anything else so hero don't get more MP than needed
	gs->globalEffects.removeBonusesRecursive(Bonus::OneDay); //works for children -> all game objs
	gs->globalEffects.reduceBonusDurations(Bonus::NDays);
	gs->globalEffects.reduceBonusDurations(Bonus::OneWeek);
	//TODO not really a single root hierarchy, what about bonuses placed elsewhere? [not an issue with H3 mechanics but in the future...]

	for(NewTurn::Hero h : heroes) //give mana/movement point
	{
		CGHeroInstance *hero = gs->getHero(h.id);
		if(!hero)
		{
			// retreated or surrendered hero who has not been reset yet
			for(auto& hp : gs->hpool.heroesPool)
			{
				if(hp.second->id == h.id)
				{
					hero = hp.second;
					break;
				}
			}
		}
		if(!hero)
		{
			logGlobal->error("Hero %d not found in NewTurn::applyGs", h.id.getNum());
			continue;
		}
		hero->movement = h.move;
		hero->mana = h.mana;
	}

	for(auto i = res.cbegin(); i != res.cend(); i++)
	{
		assert(i->first < PlayerColor::PLAYER_LIMIT);
		gs->getPlayerState(i->first)->resources = i->second;
	}

	for(auto creatureSet : cres) //set available creatures in towns
		creatureSet.second.applyGs(gs);

	for(CGTownInstance* t : gs->map->towns)
		t->builded = 0;

	if(gs->getDate(Date::DAY_OF_WEEK) == 1)
		gs->updateRumor();

	//count days without town for all players, regardless of their turn order
	for (auto &p : gs->players)
	{
		PlayerState & playerState = p.second;
		if (playerState.status == EPlayerStatus::INGAME)
		{
			if (playerState.towns.empty())
			{
				if (playerState.daysWithoutCastle)
					++(*playerState.daysWithoutCastle);
				else
					playerState.daysWithoutCastle = boost::make_optional(0);
			}
			else
			{
				playerState.daysWithoutCastle = boost::none;
			}
		}
	}
}

DLL_LINKAGE void SetObjectProperty::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(id);
	if(!obj)
	{
		logNetwork->error("Wrong object ID - property cannot be set!");
		return;
	}

	CArmedInstance *cai = dynamic_cast<CArmedInstance *>(obj);
	if(what == ObjProperty::OWNER && cai)
	{
		if(obj->ID == Obj::TOWN)
		{
			CGTownInstance *t = static_cast<CGTownInstance*>(obj);
			if(t->tempOwner < PlayerColor::PLAYER_LIMIT)
				gs->getPlayerState(t->tempOwner)->towns -= t;
			if(val < PlayerColor::PLAYER_LIMIT_I)
			{
				PlayerState * p = gs->getPlayerState(PlayerColor(val));
				p->towns.push_back(t);

				//reset counter before NewTurn to avoid no town message if game loaded at turn when one already captured
				if(p->daysWithoutCastle)
					p->daysWithoutCastle = boost::none;
			}
		}

		CBonusSystemNode & nodeToMove = cai->whatShouldBeAttached();
		nodeToMove.detachFrom(cai->whereShouldBeAttached(gs));
		obj->setProperty(what,val);
		nodeToMove.attachTo(cai->whereShouldBeAttached(gs));
	}
	else //not an armed instance
	{
		obj->setProperty(what,val);
	}
}

DLL_LINKAGE void PrepareHeroLevelUp::applyGs(CGameState * gs)
{
	auto hero = gs->getHero(heroId);
	assert(hero);

	auto proposedSkills = hero->getLevelUpProposedSecondarySkills();

	if(skills.size() == 1 || hero->tempOwner == PlayerColor::NEUTRAL) //choose skill automatically
	{
		skills.push_back(*RandomGeneratorUtil::nextItem(proposedSkills, hero->skillsInfo.rand));
	}
	else
	{
		skills = proposedSkills;
	}
}

DLL_LINKAGE void HeroLevelUp::applyGs(CGameState * gs)
{
	auto hero = gs->getHero(heroId);
	assert(hero);
	hero->levelUp(skills);
}

DLL_LINKAGE void CommanderLevelUp::applyGs(CGameState * gs)
{
	auto hero = gs->getHero(heroId);
	assert(hero);
	auto commander = hero->commander;
	assert(commander);
	commander->levelUp();
}

DLL_LINKAGE void BattleStart::applyGs(CGameState *gs)
{
	gs->curB = info;
	gs->curB->localInit();
}

DLL_LINKAGE void BattleNextRound::applyGs(CGameState *gs)
{
	gs->curB->nextRound(round);
}

DLL_LINKAGE void BattleSetActiveStack::applyGs(CGameState *gs)
{
	gs->curB->nextTurn(stack);
}

DLL_LINKAGE void BattleTriggerEffect::applyGs(CGameState *gs)
{
	CStack * st = gs->curB->getStack(stackID);
	assert(st);
	switch(effect)
	{
	case Bonus::HP_REGENERATION:
	{
		int64_t toHeal = val;
		st->heal(toHeal, EHealLevel::HEAL, EHealPower::PERMANENT);
		break;
	}
	case Bonus::MANA_DRAIN:
	{
		CGHeroInstance * h = gs->getHero(ObjectInstanceID(additionalInfo));
		st->drainedMana = true;
		h->mana -= val;
		vstd::amax(h->mana, 0);
		break;
	}
	case Bonus::POISON:
	{
		auto b = st->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT, SpellID::POISON)
				.And(Selector::type()(Bonus::STACK_HEALTH)));
		if (b)
			b->val = val;
		break;
	}
	case Bonus::ENCHANTER:
		break;
	case Bonus::FEAR:
		st->fear = true;
		break;
	default:
		logNetwork->error("Unrecognized trigger effect type %d", effect);
	}
}

DLL_LINKAGE void BattleUpdateGateState::applyGs(CGameState *gs)
{
	if(gs->curB)
		gs->curB->si.gateState = state;
}

void BattleResult::applyGs(CGameState *gs)
{
	for (auto & elem : gs->curB->stacks)
		delete elem;


	for(int i = 0; i < 2; ++i)
	{
		if(auto h = gs->curB->battleGetFightingHero(i))
		{
			h->removeBonusesRecursive(Bonus::OneBattle); 	//remove any "until next battle" bonuses
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

DLL_LINKAGE void BattleLogMessage::applyGs(CGameState *gs)
{
	//nothing
}

DLL_LINKAGE void BattleLogMessage::applyBattle(IBattleState * battleState)
{
	//nothing
}

DLL_LINKAGE void BattleStackMoved::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
}

DLL_LINKAGE void BattleStackMoved::applyBattle(IBattleState * battleState)
{
	battleState->moveUnit(stack, tilesToMove.back());
}

DLL_LINKAGE void BattleStackAttacked::applyGs(CGameState * gs)
{
	applyBattle(gs->curB);
}

DLL_LINKAGE void BattleStackAttacked::applyBattle(IBattleState * battleState)
{
	battleState->setUnitState(newState.id, newState.data, newState.healthDelta);
}

DLL_LINKAGE void BattleAttack::applyGs(CGameState * gs)
{
	CStack * attacker = gs->curB->getStack(stackAttacking);
	assert(attacker);

	attackerChanges.applyGs(gs);

	for(BattleStackAttacked & stackAttacked : bsa)
		stackAttacked.applyGs(gs);

	attacker->removeBonusesRecursive(Bonus::UntilAttack);
}

DLL_LINKAGE void StartAction::applyGs(CGameState *gs)
{
	CStack *st = gs->curB->getStack(ba.stackNumber);

	if(ba.actionType == EActionType::END_TACTIC_PHASE)
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

	if(ba.actionType != EActionType::HERO_SPELL) //don't check for stack if it's custom action by hero
	{
		assert(st);
	}
	else
	{
		gs->curB->sides[ba.side].usedSpellsHistory.push_back(SpellID(ba.actionSubtype));
	}

	switch(ba.actionType)
	{
	case EActionType::DEFEND:
		st->waiting = false;
		st->defending = true;
		st->defendingAnim = true;
		break;
	case EActionType::WAIT:
		st->defendingAnim = false;
		st->waiting = true;
		st->waitedThisTurn = true;
		break;
	case EActionType::HERO_SPELL: //no change in current stack state
		break;
	default: //any active stack action - attack, catapult, heal, spell...
		st->waiting = false;
		st->defendingAnim = false;
		st->movedThisRound = true;
		break;
	}
}

DLL_LINKAGE void BattleSpellCast::applyGs(CGameState *gs)
{
	assert(gs->curB);

	if(castByHero)
	{
		if(side < 2)
		{
			gs->curB->sides[side].castSpellsCount++;
		}
	}
}

DLL_LINKAGE void SetStackEffect::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
}

DLL_LINKAGE void SetStackEffect::applyBattle(IBattleState * battleState)
{
	for(const auto & stackData : toRemove)
		battleState->removeUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : toUpdate)
		battleState->updateUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : toAdd)
		battleState->addUnitBonus(stackData.first, stackData.second);
}


DLL_LINKAGE void StacksInjured::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
}

DLL_LINKAGE void StacksInjured::applyBattle(IBattleState * battleState)
{
	for(BattleStackAttacked stackAttacked : stacks)
		stackAttacked.applyBattle(battleState);
}

DLL_LINKAGE void BattleUnitsChanged::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
}

DLL_LINKAGE void BattleUnitsChanged::applyBattle(IBattleState * battleState)
{
	for(auto & elem : changedStacks)
	{
		switch(elem.operation)
		{
		case BattleChanges::EOperation::RESET_STATE:
			battleState->setUnitState(elem.id, elem.data, elem.healthDelta);
			break;
		case BattleChanges::EOperation::REMOVE:
			battleState->removeUnit(elem.id);
			break;
		case BattleChanges::EOperation::ADD:
			battleState->addUnit(elem.id, elem.data);
			break;
		case BattleChanges::EOperation::UPDATE:
			battleState->updateUnit(elem.id, elem.data);
			break;
		default:
			logNetwork->error("Unknown unit operation %d", (int)elem.operation);
			break;
		}
	}
}

DLL_LINKAGE void BattleObstaclesChanged::applyGs(CGameState * gs)
{
	if(gs->curB)
		applyBattle(gs->curB);
}

DLL_LINKAGE void BattleObstaclesChanged::applyBattle(IBattleState * battleState)
{
	for(const auto & change : changes)
	{
		switch(change.operation)
		{
		case BattleChanges::EOperation::REMOVE:
			battleState->removeObstacle(change.id);
			break;
		case BattleChanges::EOperation::ADD:
			battleState->addObstacle(change);
			break;
		case BattleChanges::EOperation::UPDATE:
			battleState->updateObstacle(change);
			break;
		default:
			logNetwork->error("Unknown obstacle operation %d", (int)change.operation);
			break;
		}
	}
}

DLL_LINKAGE CatapultAttack::CatapultAttack()
{
	attacker = -1;
}

DLL_LINKAGE CatapultAttack::~CatapultAttack()
{
}

DLL_LINKAGE void CatapultAttack::applyGs(CGameState * gs)
{
	if(gs->curB)
		applyBattle(gs->curB);
}

DLL_LINKAGE void CatapultAttack::applyBattle(IBattleState * battleState)
{
	auto town = battleState->getDefendedTown();
	if(!town)
		return;

	if(town->fortLevel() == CGTownInstance::NONE)
		return;

	for(const auto & part : attackedParts)
	{
		auto newWallState = SiegeInfo::applyDamage(EWallState::EWallState(battleState->getWallState(part.attackedPart)), part.damageDealt);
		battleState->setWallState(part.attackedPart, newWallState);
	}
}

DLL_LINKAGE void BattleSetStackProperty::applyGs(CGameState * gs)
{
	CStack * stack = gs->curB->getStack(stackID);
	switch(which)
	{
		case CASTS:
		{
			if(absolute)
				logNetwork->error("Can not change casts in absolute mode");
			else
				stack->casts.use(-val);
			break;
		}
		case ENCHANTER_COUNTER:
		{
			auto & counter = gs->curB->sides[gs->curB->whatSide(stack->owner)].enchanterCounter;
			if(absolute)
				counter = val;
			else
				counter += val;
			vstd::amax(counter, 0);
			break;
		}
		case UNBIND:
		{
			stack->removeBonusesRecursive(Selector::type()(Bonus::BIND_EFFECT));
			break;
		}
		case CLONED:
		{
			stack->cloned = true;
			break;
		}
		case HAS_CLONE:
		{
			stack->cloneID = val;
			break;
		}
	}
}

DLL_LINKAGE void PlayerCheated::applyGs(CGameState *gs)
{
	if(!player.isValidPlayer())
		return;

	gs->getPlayerState(player)->enteredLosingCheatCode = losingCheatCode;
	gs->getPlayerState(player)->enteredWinningCheatCode = winningCheatCode;
}

DLL_LINKAGE void YourTurn::applyGs(CGameState *gs)
{
	gs->currentPlayer = player;

	auto & playerState = gs->players[player];
	playerState.daysWithoutCastle = daysWithoutCastle;
}

DLL_LINKAGE Component::Component(const CStackBasicDescriptor &stack)
	: id(CREATURE), subtype(stack.type->idNumber), val(stack.count), when(0)
{
}

DLL_LINKAGE void EntitiesChanged::applyGs(CGameState * gs)
{
	for(const auto & change : changes)
		gs->updateEntity(change.metatype, change.entityIndex, change.data);
}

const CArtifactInstance * ArtSlotInfo::getArt() const
{
	if(locked)
	{
		logNetwork->warn("ArtifactLocation::getArt: This location is locked!");
		return nullptr;
	}
	return artifact;
}

CArtifactSet * BulkMoveArtifacts::getSrcHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), srcArtHolder);
}

CArtifactSet * BulkMoveArtifacts::getDstHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), dstArtHolder);
}

VCMI_LIB_NAMESPACE_END
