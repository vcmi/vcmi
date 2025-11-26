/*
 * GameStatePackVisitor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameStatePackVisitor.h"

#include "CGameState.h"
#include "TavernHeroesPool.h"

#include "../CPlayerState.h"
#include "../CStack.h"
#include "../IGameSettings.h"

#include "../campaign/CampaignState.h"
#include "../entities/artifact/ArtifactUtils.h"
#include "../entities/artifact/CArtifact.h"
#include "../entities/artifact/CArtifactFittingSet.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/FlaggableMapObject.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/TownBuildingInstance.h"
#include "../mapping/CMap.h"
#include "../networkPacks/StackLocation.h"

VCMI_LIB_NAMESPACE_BEGIN

void GameStatePackVisitor::visitSetResources(SetResources & pack)
{
	assert(pack.player.isValidPlayer());
	if(pack.mode == ChangeValueMode::ABSOLUTE)
		gs.getPlayerState(pack.player)->resources = pack.res;
	else
		gs.getPlayerState(pack.player)->resources += pack.res;
	gs.getPlayerState(pack.player)->resources.amin(GameConstants::PLAYER_RESOURCES_CAP);

	//just ensure that player resources are not negative
	//server is responsible to check if player can afford deal
	//but events on server side are allowed to take more than player have
	gs.getPlayerState(pack.player)->resources.positive();
}

void GameStatePackVisitor::visitSetPrimarySkill(SetPrimarySkill & pack)
{
	CGHeroInstance * hero = gs.getHero(pack.id);
	assert(hero);
	hero->setPrimarySkill(pack.which, pack.val, pack.mode);
}

void GameStatePackVisitor::visitSetHeroExperience(SetHeroExperience & pack)
{
	CGHeroInstance * hero = gs.getHero(pack.id);
	assert(hero);
	hero->setExperience(pack.val, pack.mode);
}

void GameStatePackVisitor::visitGiveStackExperience(GiveStackExperience & pack)
{
	auto * army = gs.getArmyInstance(pack.id);

	for (const auto & slot : pack.val)
		army->getStackPtr(slot.first)->giveAverageStackExperience(slot.second);

	army->nodeHasChanged();
}

void GameStatePackVisitor::visitSetSecSkill(SetSecSkill & pack)
{
	CGHeroInstance *hero = gs.getHero(pack.id);
	hero->setSecSkillLevel(pack.which, pack.val, pack.mode);
}

void GameStatePackVisitor::visitSetCommanderProperty(SetCommanderProperty & pack)
{
	const auto & commander = gs.getHero(pack.heroid)->getCommander();
	assert (commander);

	switch (pack.which)
	{
		case SetCommanderProperty::BONUS:
			commander->accumulateBonus (std::make_shared<Bonus>(pack.accumulatedBonus));
			break;
		case SetCommanderProperty::SPECIAL_SKILL:
			commander->accumulateBonus (std::make_shared<Bonus>(pack.accumulatedBonus));
			commander->specialSkills.insert (pack.additionalInfo);
			break;
		case SetCommanderProperty::SECONDARY_SKILL:
			commander->secondarySkills[pack.additionalInfo] = static_cast<ui8>(pack.amount);
			break;
		case SetCommanderProperty::ALIVE:
			if (pack.amount)
				commander->setAlive(true);
			else
				commander->setAlive(false);
			break;
		case SetCommanderProperty::EXPERIENCE:
			commander->giveTotalStackExperience(pack.amount);
			commander->nodeHasChanged();
			break;
	}
}

void GameStatePackVisitor::visitAddQuest(AddQuest & pack)
{
	assert(vstd::contains(gs.players, pack.player));
	auto * vec = &gs.players.at(pack.player).quests;
	if (!vstd::contains(*vec, pack.quest))
		vec->push_back(pack.quest);
	else
		logNetwork->warn("Warning! Attempt to add duplicated quest");
}

void GameStatePackVisitor::visitChangeFormation(ChangeFormation & pack)
{
	gs.getHero(pack.hid)->setFormation(pack.formation);
}

void GameStatePackVisitor::visitChangeTownName(ChangeTownName & pack)
{
	gs.getTown(pack.tid)->setCustomName(pack.name);
}

void GameStatePackVisitor::visitHeroVisitCastle(HeroVisitCastle & pack)
{
	CGHeroInstance *h = gs.getHero(pack.hid);
	CGTownInstance *t = gs.getTown(pack.tid);

	assert(h);
	assert(t);

	if(pack.start())
		t->setVisitingHero(h);
	else
		t->setVisitingHero(nullptr);
}

void GameStatePackVisitor::visitChangeSpells(ChangeSpells & pack)
{
	CGHeroInstance *hero = gs.getHero(pack.hid);

	if(pack.learn)
		for(const auto & sid : pack.spells)
			hero->addSpellToSpellbook(sid);
	else
		for(const auto & sid : pack.spells)
			hero->removeSpellFromSpellbook(sid);
}

void GameStatePackVisitor::visitSetResearchedSpells(SetResearchedSpells & pack)
{
	CGTownInstance *town = gs.getTown(pack.tid);

	town->spells[pack.level] = pack.spells;
	town->spellResearchCounterDay++;
	if(pack.accepted)
		town->spellResearchAcceptedCounter++;
}

void GameStatePackVisitor::visitSetMana(SetMana & pack)
{
	CGHeroInstance * hero = gs.getHero(pack.hid);

	assert(hero);

	if(pack.mode == ChangeValueMode::ABSOLUTE)
		hero->mana = pack.val;
	else
		hero->mana += pack.val;

	vstd::amax(hero->mana, 0); //not less than 0
}

void GameStatePackVisitor::visitSetMovePoints(SetMovePoints & pack)
{
	CGHeroInstance *hero = gs.getHero(pack.hid);
	assert(hero);
	hero->setMovementPoints(pack.val);
}

void GameStatePackVisitor::visitFoWChange(FoWChange & pack)
{
	TeamState * team = gs.getPlayerTeam(pack.player);
	auto & fogOfWarMap = team->fogOfWarMap;
	for(const int3 & t : pack.tiles)
		fogOfWarMap[t.z][t.x][t.y] = pack.mode != ETileVisibility::HIDDEN;

	if (pack.mode == ETileVisibility::HIDDEN) //do not hide too much
	{
		FowTilesType tilesRevealed;
		for (auto & o : gs.getMap().getObjects())
		{
			if (o->asOwnable())
			{
				if(vstd::contains(team->players, o->getOwner())) //check owned observators
					gs.getTilesInRange(tilesRevealed, o->getSightCenter(), o->getSightRadius(), ETileVisibility::HIDDEN, o->tempOwner);
			}
		}
		for(const int3 & t : tilesRevealed) //probably not the most optimal solution ever
			fogOfWarMap[t.z][t.x][t.y] = 1;
	}
}

void GameStatePackVisitor::visitSetAvailableHero(SetAvailableHero & pack)
{
	gs.heroesPool->setHeroForPlayer(pack.player, pack.slotID, pack.hid, pack.army, pack.roleID, pack.replenishPoints);
}

void GameStatePackVisitor::visitGiveBonus(GiveBonus & pack)
{
	CBonusSystemNode *cbsn = nullptr;
	switch(pack.who)
	{
		case GiveBonus::ETarget::OBJECT:
			cbsn = dynamic_cast<CBonusSystemNode*>(gs.getObjInstance(pack.id.as<ObjectInstanceID>()));
			break;
		case GiveBonus::ETarget::HERO_COMMANDER:
			cbsn = gs.getHero(pack.id.as<ObjectInstanceID>())->getCommander();
			break;
		case GiveBonus::ETarget::PLAYER:
			cbsn = gs.getPlayerState(pack.id.as<PlayerColor>());
			break;
		case GiveBonus::ETarget::BATTLE:
			assert(Bonus::OneBattle(&pack.bonus));
			cbsn = dynamic_cast<CBonusSystemNode*>(gs.getBattle(pack.id.as<BattleID>()));
			break;
	}

	assert(cbsn);

	if(Bonus::OneWeek(&pack.bonus))
		pack.bonus.turnsRemain = 8 - gs.getDate(Date::DAY_OF_WEEK); // set correct number of days before adding bonus

	auto b = std::make_shared<Bonus>(pack.bonus);
	cbsn->addNewBonus(b);
}

void GameStatePackVisitor::visitChangeObjPos(ChangeObjPos & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objid);
	if(!obj)
	{
		logNetwork->error("Wrong ChangeObjPos: object %d doesn't exist!", pack.objid.getNum());
		return;
	}
	gs.getMap().moveObject(pack.objid, pack.nPos + obj->getVisitableOffset());
}

void GameStatePackVisitor::visitChangeObjectVisitors(ChangeObjectVisitors & pack)
{
	auto objectPtr = gs.getObjInstance(pack.object);

	switch (pack.mode)
	{
		case ChangeObjectVisitors::VISITOR_ADD_HERO:
			gs.getHero(pack.hero)->visitedObjects.insert(pack.object);
			[[fallthrough]];
		case ChangeObjectVisitors::VISITOR_ADD_PLAYER:
			gs.getPlayerTeam(gs.getHero(pack.hero)->tempOwner)->scoutedObjects.insert(pack.object);
			gs.getPlayerState(gs.getHero(pack.hero)->tempOwner)->visitedObjects.insert(pack.object);
			gs.getPlayerState(gs.getHero(pack.hero)->tempOwner)->visitedObjectsGlobal.insert({objectPtr->ID, objectPtr->subID});
			break;

		case ChangeObjectVisitors::VISITOR_CLEAR:
			// remove visit info from all heroes, including those that are not present on map
			for (auto heroID : gs.getMap().getHeroesOnMap())
				gs.getHero(heroID)->visitedObjects.erase(pack.object);

			for (auto heroID : gs.getMap().getHeroesInPool())
				gs.getMap().tryGetFromHeroPool(heroID)->visitedObjects.erase(pack.object);

			for(auto &elem : gs.players)
				elem.second.visitedObjects.erase(pack.object);

			for(auto &elem : gs.teams)
				elem.second.scoutedObjects.erase(pack.object);

			break;
		case ChangeObjectVisitors::VISITOR_SCOUTED:
			gs.getPlayerTeam(gs.getHero(pack.hero)->tempOwner)->scoutedObjects.insert(pack.object);
			break;
	}
}

void GameStatePackVisitor::visitChangeArtifactsCostume(ChangeArtifactsCostume & pack)
{
	auto & allCostumes = gs.getPlayerState(pack.player)->costumesArtifacts;
	if(const auto & costume = allCostumes.find(pack.costumeIdx); costume != allCostumes.end())
		costume->second = pack.costumeSet;
	else
		allCostumes.try_emplace(pack.costumeIdx, pack.costumeSet);
}

void GameStatePackVisitor::visitPlayerEndsGame(PlayerEndsGame & pack)
{
	PlayerState *p = gs.getPlayerState(pack.player);
	if(pack.victoryLossCheckResult.victory())
	{
		p->status = EPlayerStatus::WINNER;

		// TODO: Campaign-specific code might as well go somewhere else
		// keep all heroes from the winning player
		if(p->human && gs.getStartInfo()->campState)
		{
			std::vector<CGHeroInstance *> crossoverHeroes;
			for (auto hero : p->getHeroes())
				if (hero->tempOwner == pack.player)
					crossoverHeroes.push_back(hero);

			gs.getStartInfo()->campState->setCurrentMapAsConquered(crossoverHeroes);
		}
	}
	else
	{
		p->status = EPlayerStatus::LOSER;
	}

	// defeated player may be making turn right now
	gs.actingPlayers.erase(pack.player);
}

void GameStatePackVisitor::visitRemoveBonus(RemoveBonus & pack)
{
	CBonusSystemNode *node = nullptr;
	switch(pack.who)
	{
		case GiveBonus::ETarget::OBJECT:
			node = dynamic_cast<CBonusSystemNode*>(gs.getObjInstance(pack.whoID.as<ObjectInstanceID>()));
			break;
		case GiveBonus::ETarget::PLAYER:
			node = gs.getPlayerState(pack.whoID.as<PlayerColor>());
			break;
		case GiveBonus::ETarget::BATTLE:
			assert(Bonus::OneBattle(&pack.bonus));
			node = dynamic_cast<CBonusSystemNode*>(gs.getBattle(pack.whoID.as<BattleID>()));
			break;
	}

	BonusList &bonuses = node->getExportedBonusList();

	for(const auto & b : bonuses)
	{
		if(b->source == pack.source && b->sid == pack.id)
		{
			pack.bonus = *b; //backup bonus (to show to interfaces later)
			node->removeBonus(b);
			break;
		}
	}
}

void GameStatePackVisitor::visitRemoveObject(RemoveObject & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objectID);
	logGlobal->debug("removing object id=%d; address=%x; name=%s", pack.objectID, (intptr_t)obj, obj->getObjectName());

	if (pack.initiator.isValidPlayer())
		gs.getPlayerState(pack.initiator)->destroyedObjects.insert(pack.objectID);

	if(obj->getOwner().isValidPlayer())
		gs.getPlayerState(obj->getOwner())->removeOwnedObject(obj); //object removed via map event or hero got beaten

	if(obj->ID == Obj::HERO) //remove beaten hero
	{
		auto beatenHero = dynamic_cast<CGHeroInstance*>(obj);
		assert(beatenHero);

		vstd::erase_if(beatenHero->artifactsInBackpack, [](const ArtSlotInfo& asi)
		{
			return asi.getArt()->getTypeId() == ArtifactID::GRAIL;
		});

		if(beatenHero->getVisitedTown())
		{
			if(beatenHero->getVisitedTown()->getGarrisonHero() == beatenHero)
				beatenHero->getVisitedTown()->setGarrisonedHero(nullptr);
			else
				beatenHero->getVisitedTown()->setVisitingHero(nullptr);

			beatenHero->setVisitedTown(nullptr, false);
		}

		//If hero on Boat is removed, the Boat disappears
		if(beatenHero->inBoat())
		{
			auto boat = beatenHero->getBoat();
			beatenHero->setBoat(nullptr);
			gs.getMap().eraseObject(boat->id);
		}

		beatenHero->detachFromBonusSystem(gs);
		beatenHero->tempOwner = PlayerColor::NEUTRAL; //no one owns beaten hero
		auto beatenObject = gs.getMap().eraseObject(obj->id);

		//return hero to the pool, so he may reappear in tavern
		gs.heroesPool->addHeroToPool(beatenHero->getHeroTypeID());
		gs.getMap().addToHeroPool(std::dynamic_pointer_cast<CGHeroInstance>(beatenObject));
		return;
	}

	if(obj->ID == Obj::TOWN)
	{
		auto * town = dynamic_cast<CGTownInstance *>(obj);
		town->setVisitingHero(nullptr);

		if (town->getGarrisonHero())
		{
			gs.getMap().showObject(gs.getHero(town->getGarrisonHero()->id));
			town->setGarrisonedHero(nullptr);
		}
	}

	const auto * quest = dynamic_cast<const IQuestObject *>(obj);
	if (quest)
	{
		for (auto &player : gs.players)
		{
			vstd::erase_if(player.second.quests, [obj](const QuestInfo & q){
				return q.obj == obj->id;
			});
		}
	}

	obj->detachFromBonusSystem(gs);
	gs.getMap().eraseObject(pack.objectID);
	gs.getMap().calculateGuardingGreaturePositions();//FIXME: excessive, update only affected tiles
}

static int getDir(const int3 & src, const int3 & dst)
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

void GameStatePackVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);
	if (!h)
	{
		logGlobal->error("Attempt ot move unavailable hero %d", pack.id.getNum());
		return;
	}

	const TerrainTile & fromTile = gs.getMap().getTile(h->convertToVisitablePos(pack.start));
	const TerrainTile & destTile = gs.getMap().getTile(h->convertToVisitablePos(pack.end));

	h->setMovementPoints(pack.movePoints);

	if((pack.result == TryMoveHero::SUCCESS || pack.result == TryMoveHero::BLOCKING_VISIT || pack.result == TryMoveHero::EMBARK || pack.result == TryMoveHero::DISEMBARK) && pack.start != pack.end)
	{
		auto dir = getDir(pack.start, pack.end);
		if(dir > 0  &&  dir <= 8)
			h->moveDir = dir;
		//else don`t change move direction - hero might have traversed the subterranean gate, direction should be kept
	}

	if(pack.result == TryMoveHero::EMBARK) //hero enters boat at destination tile
	{
		const TerrainTile &tt = gs.getMap().getTile(h->convertToVisitablePos(pack.end));
		ObjectInstanceID topObjectID = tt.visitableObjects.back();
		CGObjectInstance * topObject = gs.getObjInstance(topObjectID);
		assert(tt.visitableObjects.size() >= 1 && topObject->ID == Obj::BOAT); //the only visitable object at destination is Boat
		auto * boat = dynamic_cast<CGBoat *>(topObject);
		assert(boat);

		gs.getMap().hideObject(boat); //hero blockvis mask will be used, we don't need to duplicate it with boat
		h->setBoat(boat);
	}
	else if(pack.result == TryMoveHero::DISEMBARK) //hero leaves boat to destination tile
	{
		auto * b = h->getBoat();
		b->direction = h->moveDir;
		b->pos = pack.start;
		gs.getMap().showObject(b);
		h->setBoat(nullptr);
	}

	if(pack.start != pack.end && (pack.result == TryMoveHero::SUCCESS || pack.result == TryMoveHero::TELEPORTATION || pack.result == TryMoveHero::EMBARK || pack.result == TryMoveHero::DISEMBARK))
	{
		gs.getMap().hideObject(h);
		h->setAnchorPos(pack.end);
		if(auto * b = h->getBoat())
			b->setAnchorPos(pack.end);
		gs.getMap().showObject(h);
	}

	auto & fogOfWarMap = gs.getPlayerTeam(h->getOwner())->fogOfWarMap;
	for(const int3 & t : pack.fowRevealed)
		fogOfWarMap[t.z][t.x][t.y] = 1;

	if (fromTile.getTerrainID() != destTile.getTerrainID())
		h->nodeHasChanged(); // update bonuses with terrain limiter
}

void GameStatePackVisitor::visitNewStructures(NewStructures & pack)
{
	CGTownInstance *t = gs.getTown(pack.tid);

	for(const auto & id : pack.bid)
	{
		assert(t->getTown()->buildings.at(id) != nullptr);
		t->addBuilding(id);
	}
	t->updateAppearance();
	t->built = pack.built;
	t->recreateBuildingsBonuses();
}

void GameStatePackVisitor::visitRazeStructures(RazeStructures & pack)
{
	CGTownInstance *t = gs.getTown(pack.tid);
	for(const auto & id : pack.bid)
	{
		t->removeBuilding(id);

		t->updateAppearance();
	}
	t->destroyed = pack.destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

void GameStatePackVisitor::visitSetAvailableCreatures(SetAvailableCreatures & pack)
{
	auto * dw = dynamic_cast<CGDwelling *>(gs.getObjInstance(pack.tid));
	assert(dw);
	dw->creatures = pack.creatures;
}

void GameStatePackVisitor::visitSetHeroesInTown(SetHeroesInTown & pack)
{
	CGTownInstance *t = gs.getTown(pack.tid);

	CGHeroInstance * v = gs.getHero(pack.visiting);
	CGHeroInstance * g = gs.getHero(pack.garrison);

	bool newVisitorComesFromGarrison = v && v == t->getGarrisonHero();
	bool newGarrisonComesFromVisiting = g && g == t->getVisitingHero();

	if(newVisitorComesFromGarrison)
		t->setGarrisonedHero(nullptr);
	if(newGarrisonComesFromVisiting)
		t->setVisitingHero(nullptr);
	if(!newGarrisonComesFromVisiting || v)
		t->setVisitingHero(v);
	if(!newVisitorComesFromGarrison || g)
		t->setGarrisonedHero(g);

	if(v)
		gs.getMap().showObject(v);

	if(g)
		gs.getMap().hideObject(g);
}

void GameStatePackVisitor::visitHeroRecruited(HeroRecruited & pack)
{
	auto h = gs.heroesPool->takeHeroFromPool(pack.hid);
	CGTownInstance *t = gs.getTown(pack.tid);
	PlayerState *p = gs.getPlayerState(pack.player);

	if (pack.boatId.hasValue())
	{
		CGObjectInstance *obj = gs.getObjInstance(pack.boatId);
		auto * boat = dynamic_cast<CGBoat *>(obj);
		if (boat)
		{
			gs.getMap().hideObject(boat);
			h->setBoat(boat);
		}
	}

	h->setOwner(pack.player);
	h->pos = pack.tile;
	h->updateAppearance();

	// Generate unique instance name before adding to map
	if (h->instanceName.empty())
		gs.getMap().generateUniqueInstanceName(h.get());

	gs.getMap().addNewObject(h);
	assert(h->id.hasValue());

	p->addOwnedObject(h.get());
	h->attachToBonusSystem(gs);

	if(t)
		t->setVisitingHero(h.get());
}

void GameStatePackVisitor::visitGiveHero(GiveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);

	if (pack.boatId.hasValue())
	{
		CGObjectInstance *obj = gs.getObjInstance(pack.boatId);
		auto * boat = dynamic_cast<CGBoat *>(obj);
		if (boat)
		{
			gs.getMap().hideObject(boat);
			h->setBoat(boat);
		}
	}

	//bonus system
	h->detachFrom(gs.globalEffects);
	h->attachTo(*gs.getPlayerState(pack.player));

	auto oldVisitablePos = h->visitablePos();
	gs.getMap().hideObject(h);
	h->updateAppearance();

	h->setOwner(pack.player);
	h->setMovementPoints(h->movementPointsLimit(true));
	h->setAnchorPos(h->convertFromVisitablePos(oldVisitablePos));
	gs.getMap().heroAddedToMap(h);
	gs.getPlayerState(h->getOwner())->addOwnedObject(h);

	gs.getMap().showObject(h);
	h->setVisitedTown(nullptr, false);
}

void GameStatePackVisitor::visitNewObject(NewObject & pack)
{
	gs.getMap().addNewObject(pack.newObject);
	gs.getMap().calculateGuardingGreaturePositions();

	// attach newly spawned wandering monster to global bonus system node
	auto newArmy = std::dynamic_pointer_cast<CArmedInstance>(pack.newObject);
	if (newArmy)
		newArmy->attachToBonusSystem(gs);

	logGlobal->debug("Added object id=%d; name=%s", pack.newObject->id, pack.newObject->getObjectName());
}

void GameStatePackVisitor::visitNewArtifact(NewArtifact & pack)
{
	auto art = gs.createArtifact(pack.artId, pack.spellId);
	PutArtifact pa(art->getId(), ArtifactLocation(pack.artHolder, pack.pos), false);
	pa.visit(*this);
}

void GameStatePackVisitor::visitChangeStackCount(ChangeStackCount & pack)
{
	auto * srcObj = gs.getArmyInstance(pack.army);
	if(!srcObj)
		throw std::runtime_error("ChangeStackCount: invalid army object " + std::to_string(pack.army.getNum()) + ", possible game state corruption.");

	if(pack.mode == ChangeValueMode::ABSOLUTE)
		srcObj->setStackCount(pack.slot, pack.count);
	else
		srcObj->changeStackCount(pack.slot, pack.count);
}

void GameStatePackVisitor::visitSetStackType(SetStackType & pack)
{
	auto * srcObj = gs.getArmyInstance(pack.army);
	if(!srcObj)
		throw std::runtime_error("SetStackType: invalid army object " + std::to_string(pack.army.getNum()) + ", possible game state corruption.");

	srcObj->setStackType(pack.slot, pack.type);
}

void GameStatePackVisitor::visitEraseStack(EraseStack & pack)
{
	auto * srcObj = gs.getArmyInstance(pack.army);
	if(!srcObj)
		throw std::runtime_error("EraseStack: invalid army object " + std::to_string(pack.army.getNum()) + ", possible game state corruption.");

	srcObj->eraseStack(pack.slot);
}

void GameStatePackVisitor::visitSwapStacks(SwapStacks & pack)
{
	auto * srcObj = gs.getArmyInstance(pack.srcArmy);
	if(!srcObj)
		throw std::runtime_error("SwapStacks: invalid army object " + std::to_string(pack.srcArmy.getNum()) + ", possible game state corruption.");

	auto * dstObj = gs.getArmyInstance(pack.dstArmy);
	if(!dstObj)
		throw std::runtime_error("SwapStacks: invalid army object " + std::to_string(pack.dstArmy.getNum()) + ", possible game state corruption.");

	auto s1 = srcObj->detachStack(pack.srcSlot);
	auto s2 = dstObj->detachStack(pack.dstSlot);

	srcObj->putStack(pack.srcSlot, std::move(s2));
	dstObj->putStack(pack.dstSlot, std::move(s1));
}

void GameStatePackVisitor::visitInsertNewStack(InsertNewStack & pack)
{
	if(auto * obj = gs.getArmyInstance(pack.army))
		obj->putStack(pack.slot, std::make_unique<CStackInstance>(&gs, pack.type, pack.count));
	else
		throw std::runtime_error("InsertNewStack: invalid army object " + std::to_string(pack.army.getNum()) + ", possible game state corruption.");
}

void GameStatePackVisitor::visitRebalanceStacks(RebalanceStacks & pack)
{
	auto * srcObj = gs.getArmyInstance(pack.srcArmy);
	if(!srcObj)
		throw std::runtime_error("RebalanceStacks: invalid army object " + std::to_string(pack.srcArmy.getNum()) + ", possible game state corruption.");

	auto * dstObj = gs.getArmyInstance(pack.dstArmy);
	if(!dstObj)
		throw std::runtime_error("RebalanceStacks: invalid army object " + std::to_string(pack.dstArmy.getNum()) + ", possible game state corruption.");

	StackLocation src(srcObj->id, pack.srcSlot);
	StackLocation dst(dstObj->id, pack.dstSlot);

	[[maybe_unused]] const CCreature * srcType = srcObj->getCreature(src.slot);
	const CCreature * dstType = dstObj->getCreature(dst.slot);
	TQuantity srcCount = srcObj->getStackCount(src.slot);

	if(srcCount == pack.count) //moving whole stack
	{
		if(dstType) //stack at dest -> merge
		{
			assert(dstType == srcType);
			const auto srcHero = dynamic_cast<CGHeroInstance*>(srcObj);
			const auto dstHero = dynamic_cast<CGHeroInstance*>(dstObj);
			auto srcStack = const_cast<CStackInstance*>(srcObj->getStackPtr(src.slot));
			auto dstStack = const_cast<CStackInstance*>(dstObj->getStackPtr(dst.slot));
			if(srcStack->getArt(ArtifactPosition::CREATURE_SLOT))
			{
				if(auto dstArt = dstStack->getArt(ArtifactPosition::CREATURE_SLOT))
				{
					bool artifactIsLost = true;

					if(srcHero)
					{
						auto dstSlot = ArtifactUtils::getArtBackpackPosition(srcHero, dstArt->getTypeId());
						if (dstSlot != ArtifactPosition::PRE_FIRST)
						{
							gs.getMap().moveArtifactInstance(*dstStack, ArtifactPosition::CREATURE_SLOT, *srcHero, dstSlot);
							artifactIsLost = false;
						}
					}

					if (artifactIsLost)
					{
						BulkEraseArtifacts ea;
						ea.artHolder = dstHero->id;
						ea.posPack.emplace_back(ArtifactPosition::CREATURE_SLOT);
						ea.creature = dst.slot;
						ea.visit(*this);
						logNetwork->warn("Cannot move artifact! No free slots");
					}
					gs.getMap().moveArtifactInstance(*srcStack, ArtifactPosition::CREATURE_SLOT, *dstStack, ArtifactPosition::CREATURE_SLOT);
					//TODO: choose from dialog
				}
				else //just move to the other slot before stack gets erased
				{
					gs.getMap().moveArtifactInstance(*srcStack, ArtifactPosition::CREATURE_SLOT, *dstStack, ArtifactPosition::CREATURE_SLOT);
				}
			}

			auto movedStack = srcObj->detachStack(src.slot);
			dstObj->joinStack(dst.slot, std::move(movedStack));
		}
		else
		{
			auto movedStack = srcObj->detachStack(src.slot);
			dstObj->putStack(dst.slot, std::move(movedStack));
		}
	}
	else
	{
		auto movedStack = srcObj->splitStack(src.slot, pack.count);
		if(dstType) //stack at dest -> rebalance
		{
			assert(dstType == srcType);
			dstObj->joinStack(dst.slot, std::move(movedStack));
		}
		else //move new stack to an empty slot
		{
			dstObj->putStack(dst.slot, std::move(movedStack));
		}
	}

	srcObj->nodeHasChanged();
	if (srcObj != dstObj)
		dstObj->nodeHasChanged();
}

void GameStatePackVisitor::visitBulkRebalanceStacks(BulkRebalanceStacks & pack)
{
	for(auto & move : pack.moves)
		move.visit(*this);
}

void GameStatePackVisitor::visitGrowUpArtifact(GrowUpArtifact & pack)
{
	auto artInst = gs.getArtInstance(pack.id);
	assert(artInst);
	artInst->growingUp();
}

void GameStatePackVisitor::visitPutArtifact(PutArtifact & pack)
{
	auto art = gs.getArtInstance(pack.id);
	assert(!art->getParentNodes().empty());
	auto hero = gs.getHero(pack.al.artHolder);
	assert(hero);
	assert(art && art->canBePutAt(hero, pack.al.slot));
	assert(ArtifactUtils::checkIfSlotValid(*hero, pack.al.slot));
	gs.getMap().putArtifactInstance(*hero, art->getId(), pack.al.slot);
}

void GameStatePackVisitor::visitBulkEraseArtifacts(BulkEraseArtifacts & pack)
{
	const auto artSet = gs.getArtSet(pack.artHolder);
	assert(artSet);

	std::sort(pack.posPack.begin(), pack.posPack.end(), [](const ArtifactPosition & slot0, const ArtifactPosition & slot1) -> bool
	{
		return slot0.num > slot1.num;
	});

	for(const auto & slot : pack.posPack)
	{
		const auto slotInfo = artSet->getSlot(slot);
		const ArtifactInstanceID artifactID = slotInfo->artifactID;
		const CArtifactInstance * artifact = gs.getArtInstance(artifactID);
		if(slotInfo->locked)
		{
			logGlobal->debug("Erasing locked artifact: %s", artifact->getType()->getNameTranslated());
			DisassembledArtifact dis;
			dis.al.artHolder = pack.artHolder;

			for(auto & slotInfoWorn : artSet->artifactsWorn)
			{
				auto art = slotInfoWorn.second.getArt();
				if(art->isCombined() && art->isPart(artifact))
				{
					dis.al.slot = artSet->getArtPos(art);
					break;
				}
			}
			assert((dis.al.slot != ArtifactPosition::PRE_FIRST) && "Failed to determine the assembly this locked artifact belongs to");
			logGlobal->debug("Found the corresponding assembly: %s", artSet->getArt(dis.al.slot)->getType()->getNameTranslated());
			dis.visit(*this);
		}
		else
		{
			logGlobal->debug("Erasing artifact %s", artifact->getType()->getNameTranslated());
		}
		gs.getMap().removeArtifactInstance(*artSet, slot);
	}
}

void GameStatePackVisitor::visitBulkMoveArtifacts(BulkMoveArtifacts & pack)
{
	const auto bulkArtsRemove = [this](std::vector<MoveArtifactInfo> & artsPack, CArtifactSet & artSet)
	{
		std::vector<ArtifactPosition> packToRemove;
		for(const auto & slotsPair : artsPack)
			packToRemove.push_back(slotsPair.srcPos);
		std::sort(packToRemove.begin(), packToRemove.end(), [](const ArtifactPosition & slot0, const ArtifactPosition & slot1) -> bool
		{
			return slot0.num > slot1.num;
		});

		for(const auto & slot : packToRemove)
			gs.getMap().removeArtifactInstance(artSet, slot);
	};

	const auto bulkArtsPut = [this](std::vector<MoveArtifactInfo> & artsPack, CArtifactSet & initArtSet, CArtifactSet & dstArtSet)
	{
		for(const auto & slotsPair : artsPack)
		{
			auto * art = initArtSet.getArt(slotsPair.srcPos);
			assert(art);
			gs.getMap().putArtifactInstance(dstArtSet, art->getId(), slotsPair.dstPos);
		}
	};

	auto * leftSet = gs.getArtSet(ArtifactLocation(pack.srcArtHolder, pack.srcCreature));
	assert(leftSet);
	auto * rightSet = gs.getArtSet(ArtifactLocation(pack.dstArtHolder, pack.dstCreature));
	assert(rightSet);
	CArtifactFittingSet artInitialSetLeft(*leftSet);
	bulkArtsRemove(pack.artsPack0, *leftSet);
	if(!pack.artsPack1.empty())
	{
		CArtifactFittingSet artInitialSetRight(*rightSet);
		bulkArtsRemove(pack.artsPack1, *rightSet);
		bulkArtsPut(pack.artsPack1, artInitialSetRight, *leftSet);
	}
	bulkArtsPut(pack.artsPack0, artInitialSetLeft, *rightSet);
}

void GameStatePackVisitor::visitDischargeArtifact(DischargeArtifact & pack)
{
	auto artInst = gs.getArtInstance(pack.id);
	assert(artInst);
	artInst->discharge(pack.charges);
	if(artInst->getType()->getRemoveOnDepletion() && artInst->getCharges() == 0 && pack.artLoc.has_value())
	{
		BulkEraseArtifacts ePack;
		ePack.artHolder = pack.artLoc.value().artHolder;
		ePack.creature = pack.artLoc.value().creature;
		ePack.posPack.push_back(pack.artLoc.value().slot);
		ePack.visit(*this);
	}
	// Workaround to inform hero bonus node about changes. Obviously this has to be done somehow differently.
	if(pack.artLoc.has_value())
		gs.getHero(pack.artLoc.value().artHolder)->nodeHasChanged();
}

void GameStatePackVisitor::visitAssembledArtifact(AssembledArtifact & pack)
{
	auto artSet = gs.getArtSet(pack.al.artHolder);
	assert(artSet);
	const auto transformedArt = artSet->getArt(pack.al.slot);
	assert(transformedArt);
	const auto builtArt = pack.artId.toArtifact();
	assert(vstd::contains_if(ArtifactUtils::assemblyPossibilities(artSet, transformedArt->getTypeId()), [=](const CArtifact * art)->bool
	{
		return art->getId() == builtArt->getId();
	}));

	auto * combinedArt = gs.getMap().createArtifactComponent(pack.artId);

	// Find slots for all involved artifacts
	std::set<ArtifactPosition, std::greater<>> slotsInvolved = { pack.al.slot };
	CArtifactFittingSet fittingSet(*artSet);
	auto parts = builtArt->getConstituents();
	parts.erase(std::find(parts.begin(), parts.end(), transformedArt->getType()));
	for(const auto constituent : parts)
	{
		const auto slot = fittingSet.getArtPos(constituent->getId(), false, false);
		fittingSet.lockSlot(slot);
		assert(slot != ArtifactPosition::PRE_FIRST);
		slotsInvolved.insert(slot);
	}

	// Find a slot for combined artifact
	if(ArtifactUtils::isSlotEquipment(pack.al.slot) && ArtifactUtils::isSlotBackpack(*slotsInvolved.begin()))
	{
		pack.al.slot = ArtifactPosition::BACKPACK_START;
	}
	else if(ArtifactUtils::isSlotBackpack(pack.al.slot))
	{
		for(const auto & slot : slotsInvolved)
			if(ArtifactUtils::isSlotBackpack(slot))
				pack.al.slot = slot;
	}
	else
	{
		for(const auto & slot : slotsInvolved)
			if(!vstd::contains(builtArt->getPossibleSlots().at(artSet->bearerType()), pack.al.slot)
			   && vstd::contains(builtArt->getPossibleSlots().at(artSet->bearerType()), slot))
			{
				pack.al.slot = slot;
				break;
			}
	}

	// Delete parts from hero
	for(const auto & slot : slotsInvolved)
	{
		const auto constituentInstance = artSet->getArt(slot);
		gs.getMap().removeArtifactInstance(*artSet, slot);

		if(!combinedArt->getType()->isFused())
		{
			if(ArtifactUtils::isSlotEquipment(pack.al.slot) && slot != pack.al.slot)
				combinedArt->addPart(constituentInstance, slot);
			else
				combinedArt->addPart(constituentInstance, ArtifactPosition::PRE_FIRST);
		}
	}

	// Put new combined artifacts
	gs.getMap().putArtifactInstance(*artSet, combinedArt->getId(), pack.al.slot);
}

void GameStatePackVisitor::visitDisassembledArtifact(DisassembledArtifact & pack)
{
	auto hero = gs.getHero(pack.al.artHolder);
	assert(hero);
	auto disassembledArtID = hero->getArtID(pack.al.slot);
	auto disassembledArt = gs.getArtInstance(disassembledArtID);
	assert(disassembledArt);

	const auto parts = disassembledArt->getPartsInfo();
	gs.getMap().removeArtifactInstance(*hero, pack.al.slot);
	for(auto & part : parts)
	{
		// ArtifactPosition::PRE_FIRST is value of main part slot -> it'll replace combined artifact in its pos
		auto slot = (ArtifactUtils::isSlotEquipment(part.slot) ? part.slot : pack.al.slot);
		disassembledArt->detachFromSource(*part.getArtifact());
		gs.getMap().putArtifactInstance(*hero, part.getArtifact()->getId(), slot);
	}
	gs.getMap().eraseArtifactInstance(disassembledArt->getId());
}

void GameStatePackVisitor::visitHeroVisit(HeroVisit & pack)
{
}

void GameStatePackVisitor::visitSetAvailableArtifacts(SetAvailableArtifacts & pack)
{
	if(pack.id != ObjectInstanceID::NONE)
	{
		if(auto * bm = dynamic_cast<CGBlackMarket *>(gs.getObjInstance(pack.id)))
		{
			bm->artifacts = pack.arts;
		}
		else
		{
			logNetwork->error("Wrong black market id!");
		}
	}
	else
	{
		gs.getMap().townMerchantArtifacts = pack.arts;
	}
}

void GameStatePackVisitor::visitNewTurn(NewTurn & pack)
{
	gs.day = pack.day;

	// Update bonuses before doing anything else so hero don't get more MP than needed
	gs.globalEffects.removeBonusesRecursive(Bonus::OneDay); //works for children -> all game objs
	gs.globalEffects.reduceBonusDurations(Bonus::NDays);
	gs.globalEffects.reduceBonusDurations(Bonus::OneWeek);
	//TODO not really a single root hierarchy, what about bonuses placed elsewhere? [not an issue with H3 mechanics but in the future...]

	for(auto & manaPack : pack.heroesMana)
		manaPack.visit(*this);

	for(auto & movePack : pack.heroesMovement)
		movePack.visit(*this);

	gs.heroesPool->onNewDay();

	for(auto & entry : pack.playerIncome)
	{
		gs.getPlayerState(entry.first)->resources += entry.second;
		gs.getPlayerState(entry.first)->resources.amin(GameConstants::PLAYER_RESOURCES_CAP);
	}

	for(auto & creatureSet : pack.availableCreatures) //set available creatures in towns
		creatureSet.visit(*this);

	for (const auto & townID : gs.getMap().getAllTowns())
	{
		auto t = gs.getTown(townID);
		t->built = 0;
		t->spellResearchCounterDay = 0;
	}

	if(pack.newRumor)
		gs.currentRumor = *pack.newRumor;
}

void GameStatePackVisitor::visitSetObjectProperty(SetObjectProperty & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.id);
	if(!obj)
	{
		logNetwork->error("Wrong object ID - property cannot be set!");
		return;
	}

	if(pack.what == ObjProperty::OWNER && obj->asOwnable())
	{
		PlayerColor oldOwner = obj->getOwner();
		PlayerColor newOwner = pack.identifier.as<PlayerColor>();
		if(oldOwner.isValidPlayer())
			gs.getPlayerState(oldOwner)->removeOwnedObject(obj);

		if(newOwner.isValidPlayer())
			gs.getPlayerState(newOwner)->addOwnedObject(obj);
	}

	if(pack.what == ObjProperty::OWNER)
	{
		if(obj->ID == Obj::TOWN)
		{
			auto * t = dynamic_cast<CGTownInstance *>(obj);
			assert(t);

			PlayerColor oldOwner = t->tempOwner;
			if(oldOwner.isValidPlayer())
			{
				auto * state = gs.getPlayerState(oldOwner);
				if(state->getTowns().empty())
					state->daysWithoutCastle = 0;
			}
			if(pack.identifier.as<PlayerColor>().isValidPlayer())
			{
				//reset counter before NewTurn to avoid no town message if game loaded at turn when one already captured
				PlayerState * p = gs.getPlayerState(pack.identifier.as<PlayerColor>());
				if(p->daysWithoutCastle)
					p->daysWithoutCastle = std::nullopt;
			}
		}

		obj->detachFromBonusSystem(gs);
		obj->setProperty(pack.what, pack.identifier);
		obj->attachToBonusSystem(gs);
	}
	else //not an armed instance
	{
		obj->setProperty(pack.what, pack.identifier);
	}
}

void GameStatePackVisitor::visitHeroLevelUp(HeroLevelUp & pack)
{
	auto * hero = gs.getHero(pack.heroId);
	assert(hero);
	hero->levelUp();
}

void GameStatePackVisitor::visitCommanderLevelUp(CommanderLevelUp & pack)
{
	auto * hero = gs.getHero(pack.heroId);
	assert(hero);
	const auto & commander = hero->getCommander();
	assert(commander);
	commander->levelUp();
}

void GameStatePackVisitor::visitBattleStart(BattleStart & pack)
{
	assert(pack.battleID == gs.nextBattleID);

	pack.info->battleID = gs.nextBattleID;
	pack.info->localInit();

	if (pack.info->getDefendedTown() && pack.info->getSide(BattleSide::DEFENDER).heroID.hasValue())
	{
		CGTownInstance * town = gs.getTown(pack.info->townID);
		CGHeroInstance * hero = gs.getHero(pack.info->getSideHero(BattleSide::DEFENDER)->id);

		if (hero)
		{
			hero->detachFrom(town->townAndVis);
			hero->attachTo(*town);
		}
	}

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if (pack.info->getSide(i).heroID.hasValue())
		{
			CGHeroInstance * hero = gs.getHero(pack.info->getSideHero(i)->id);
			hero->mana = pack.info->getSide(i).initialMana + pack.info->getSide(i).additionalMana;
		}
	}

	gs.currentBattles.push_back(std::move(pack.info));
	gs.nextBattleID = BattleID(gs.nextBattleID.getNum() + 1);
}

void GameStatePackVisitor::visitBattleNextRound(BattleNextRound & pack)
{
	gs.getBattle(pack.battleID)->nextRound();
}

void GameStatePackVisitor::visitBattleSetActiveStack(BattleSetActiveStack & pack)
{
	gs.getBattle(pack.battleID)->nextTurn(pack.stack, pack.reason);
}

void GameStatePackVisitor::visitBattleTriggerEffect(BattleTriggerEffect & pack)
{
	CStack * st = gs.getBattle(pack.battleID)->getStack(pack.stackID);
	assert(st);
	switch(pack.effect)
	{
		case BonusType::HP_REGENERATION:
		{
			int64_t toHeal = pack.val;
			st->heal(toHeal, EHealLevel::HEAL, EHealPower::PERMANENT);
			break;
		}
		case BonusType::MANA_DRAIN:
		{
			CGHeroInstance * h = gs.getHero(ObjectInstanceID(pack.additionalInfo));
			st->drainedMana = true;
			h->mana -= pack.val;
			vstd::amax(h->mana, 0);
			break;
		}
		case BonusType::POISON:
		{
			auto b = st->getLocalBonus(Selector::source(BonusSource::SPELL_EFFECT, SpellID(SpellID::POISON))
										   .And(Selector::type()(BonusType::STACK_HEALTH)));
			if (b)
				b->val = pack.val;
			break;
		}
		case BonusType::ENCHANTER:
		case BonusType::MORALE:
			break;
		case BonusType::FEARFUL:
			st->fear = true;
			break;
		default:
			logNetwork->error("Unrecognized trigger effect type %d", static_cast<int>(pack.effect));
	}
}

void GameStatePackVisitor::visitBattleUpdateGateState(BattleUpdateGateState & pack)
{
	if(gs.getBattle(pack.battleID))
		gs.getBattle(pack.battleID)->si.gateState = pack.state;
}

void GameStatePackVisitor::visitBattleResultAccepted(BattleResultAccepted & pack)
{
	// Remove any "until next battle" bonuses
	if(const auto attackerHero = gs.getHero(pack.heroResult[BattleSide::ATTACKER].heroID))
		attackerHero->removeBonusesRecursive(Bonus::OneBattle);
	if(const auto defenderHero = gs.getHero(pack.heroResult[BattleSide::DEFENDER].heroID))
		defenderHero->removeBonusesRecursive(Bonus::OneBattle);
}

void GameStatePackVisitor::visitBattleStackMoved(BattleStackMoved & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::visitBattleAttack(BattleAttack & pack)
{
	CStack * attacker = gs.getBattle(pack.battleID)->getStack(pack.stackAttacking);
	assert(attacker);

	pack.attackerChanges.visit(*this);

	for(BattleStackAttacked & stack : pack.bsa)
		gs.getBattle(pack.battleID)->setUnitState(stack.newState.id, stack.newState.data, stack.newState.healthDelta);

	attacker->removeBonusesRecursive(Bonus::UntilAttack);

	if(!pack.counter())
		attacker->removeBonusesRecursive(Bonus::UntilOwnAttack);
}

void GameStatePackVisitor::visitStartAction(StartAction & pack)
{
	CStack *st = gs.getBattle(pack.battleID)->getStack(pack.ba.stackNumber);

	if(pack.ba.actionType == EActionType::END_TACTIC_PHASE)
	{
		gs.getBattle(pack.battleID)->tacticDistance = 0;
		return;
	}

	if(gs.getBattle(pack.battleID)->tacticDistance)
	{
		// moves in tactics phase do not affect creature status
		// (tactics stack queue is managed by client)
		return;
	}

	if (pack.ba.isUnitAction())
	{
		assert(st); // stack must exists for all non-hero actions

		switch(pack.ba.actionType)
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
				st->castSpellThisTurn = pack.ba.actionType == EActionType::MONSTER_SPELL;
				break;
		}
	}
	else
	{
		if(pack.ba.actionType == EActionType::HERO_SPELL)
			gs.getBattle(pack.battleID)->getSide(pack.ba.side).usedSpellsHistory.push_back(pack.ba.spell);
	}
}

void GameStatePackVisitor::visitBattleSpellCast(BattleSpellCast & pack)
{
	if(pack.castByHero && pack.side != BattleSide::NONE)
		gs.getBattle(pack.battleID)->getSide(pack.side).castSpellsCount++;
}

void GameStatePackVisitor::visitSetStackEffect(SetStackEffect & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::visitStacksInjured(StacksInjured & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::visitBattleUnitsChanged(BattleUnitsChanged & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::restorePreBattleState(BattleID battleID)
{
	auto battleIter = boost::range::find_if(gs.currentBattles, [&](const auto & battle)
	{
		return battle->battleID == battleID;
	});

	const auto & currentBattle = **battleIter;

	if (currentBattle.getDefendedTown() && currentBattle.getSideHero(BattleSide::DEFENDER))
	{
		CGTownInstance * town = gs.getTown(currentBattle.townID);
		CGHeroInstance * hero = gs.getHero(currentBattle.getSideHero(BattleSide::DEFENDER)->id);

		if (hero)
		{
			hero->detachFrom(*town);
			hero->attachTo(town->townAndVis);
		}
	}
}

void GameStatePackVisitor::visitBattleCancelled(BattleCancelled & pack)
{
	restorePreBattleState(pack.battleID);

	auto battleIter = boost::range::find_if(gs.currentBattles, [&](const auto & battle)
	{
		return battle->battleID == pack.battleID;
	});

	const auto & currentBattle = **battleIter;

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if (currentBattle.getSide(i).heroID.hasValue())
		{
			CGHeroInstance * hero = gs.getHero(currentBattle.getSideHero(i)->id);
			hero->mana = currentBattle.getSide(i).initialMana;
		}
	}

	assert(battleIter != gs.currentBattles.end());
	gs.currentBattles.erase(battleIter);
}

void GameStatePackVisitor::visitBattleResultsApplied(BattleResultsApplied & pack)
{
	restorePreBattleState(pack.battleID);
	pack.learnedSpells.visit(*this);

	for(auto & discharging : pack.dischargingArtifacts)
		discharging.visit(*this);

	for(auto & growing : pack.growingArtifacts)
		growing.visit(*this);

	for(auto & movingPack : pack.movingArtifacts)
		movingPack.visit(*this);

	auto battleIter = boost::range::find_if(gs.currentBattles, [&](const auto & battle)
	{
		return battle->battleID == pack.battleID;
	});
	const auto & currentBattle = **battleIter;

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if (currentBattle.getSide(i).heroID.hasValue())
		{
			CGHeroInstance * hero = gs.getHero(currentBattle.getSideHero(i)->id);
			hero->mana = std::min(hero->mana, currentBattle.getSide(i).initialMana);
		}
	}
}

void GameStatePackVisitor::visitBattleEnded(BattleEnded & pack)
{
	auto battleIter = boost::range::find_if(gs.currentBattles, [&](const auto & battle)
	{
		return battle->battleID == pack.battleID;
	});
	assert(battleIter != gs.currentBattles.end());
	gs.currentBattles.erase(battleIter);
}

void GameStatePackVisitor::visitBattleObstaclesChanged(BattleObstaclesChanged & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::visitCatapultAttack(CatapultAttack & pack)
{
	BattleStatePackVisitor battleVisitor(*gs.getBattle(pack.battleID));
	pack.visitTyped(battleVisitor);
}

void GameStatePackVisitor::visitBattleSetStackProperty(BattleSetStackProperty & pack)
{
	CStack * stack = gs.getBattle(pack.battleID)->getStack(pack.stackID, false);
	switch(pack.which)
	{
		case BattleSetStackProperty::CASTS:
		{
			if(pack.absolute)
				logNetwork->error("Can not change casts in absolute mode");
			else
				stack->casts.use(-pack.val);
			break;
		}
		case BattleSetStackProperty::ENCHANTER_COUNTER:
		{
			auto & counter = gs.getBattle(pack.battleID)->getSide(gs.getBattle(pack.battleID)->whatSide(stack->unitOwner())).enchanterCounter;
			if(pack.absolute)
				counter = pack.val;
			else
				counter += pack.val;
			vstd::amax(counter, 0);
			break;
		}
		case BattleSetStackProperty::UNBIND:
		{
			stack->removeBonusesRecursive(Selector::type()(BonusType::BIND_EFFECT));
			break;
		}
		case BattleSetStackProperty::CLONED:
		{
			stack->cloned = true;
			break;
		}
		case BattleSetStackProperty::HAS_CLONE:
		{
			stack->cloneID = pack.val;
			break;
		}
	}
}

void GameStatePackVisitor::visitPlayerCheated(PlayerCheated & pack)
{
	assert(pack.player.isValidPlayer());

	gs.getPlayerState(pack.player)->enteredLosingCheatCode = pack.losingCheatCode;
	gs.getPlayerState(pack.player)->enteredWinningCheatCode = pack.winningCheatCode;
	gs.getPlayerState(pack.player)->cheated = !pack.localOnlyCheat;
}

void GameStatePackVisitor::visitPlayerStartsTurn(PlayerStartsTurn & pack)
{
	//assert(gs.actingPlayers.count(player) == 0);//Legal - may happen after loading of deserialized map state
	gs.actingPlayers.insert(pack.player);
}

void GameStatePackVisitor::visitPlayerEndsTurn(PlayerEndsTurn & pack)
{
	assert(gs.actingPlayers.count(pack.player) == 1);
	gs.actingPlayers.erase(pack.player);
}

void GameStatePackVisitor::visitDaysWithoutTown(DaysWithoutTown & pack)
{
	auto & playerState = gs.players.at(pack.player);
	playerState.daysWithoutCastle = pack.daysWithoutCastle;
}

void GameStatePackVisitor::visitTurnTimeUpdate(TurnTimeUpdate & pack)
{
	auto & playerState = gs.players.at(pack.player);
	playerState.turnTimer = pack.turnTimer;
}

void GameStatePackVisitor::visitEntitiesChanged(EntitiesChanged & pack)
{
	for(const auto & change : pack.changes)
		gs.updateEntity(change.metatype, change.entityIndex, change.data);
}

void GameStatePackVisitor::visitSetRewardableConfiguration(SetRewardableConfiguration & pack)
{
	auto * objectPtr = gs.getObjInstance(pack.objectID);

	if (!pack.buildingID.hasValue())
	{
		auto * rewardablePtr = dynamic_cast<CRewardableObject *>(objectPtr);
		assert(rewardablePtr);
		rewardablePtr->configuration = pack.configuration;
		rewardablePtr->initializeGuards();
	}
	else
	{
		auto * townPtr = dynamic_cast<CGTownInstance*>(objectPtr);
		TownBuildingInstance * buildingPtr = nullptr;

		for (auto & building : townPtr->rewardableBuildings)
			if (building.second->getBuildingType() == pack.buildingID)
				buildingPtr = building.second.get();

		auto * rewardablePtr = dynamic_cast<TownRewardableBuildingInstance *>(buildingPtr);
		assert(rewardablePtr);
		rewardablePtr->configuration = pack.configuration;
	}
}

void BattleStatePackVisitor::visitBattleStackMoved(BattleStackMoved & pack)
{
	battleState.moveUnit(pack.stack, pack.tilesToMove.back());
}

void BattleStatePackVisitor::visitCatapultAttack(CatapultAttack & pack)
{
	const auto * town = battleState.getDefendedTown();
	if(!town)
		throw std::runtime_error("CatapultAttack without town!");

	if(town->fortificationsLevel().wallsHealth == 0)
		throw std::runtime_error("CatapultAttack without walls!");

	for(const auto & part : pack.attackedParts)
	{
		auto newWallState = SiegeInfo::applyDamage(battleState.getWallState(part.attackedPart), part.damageDealt);
		battleState.setWallState(part.attackedPart, newWallState);
	}
}

void BattleStatePackVisitor::visitBattleObstaclesChanged(BattleObstaclesChanged & pack)
{
	for(const auto & change : pack.changes)
	{
		switch(change.operation)
		{
			case BattleChanges::EOperation::REMOVE:
				battleState.removeObstacle(change.id);
				break;
			case BattleChanges::EOperation::ADD:
				battleState.addObstacle(change);
				break;
			case BattleChanges::EOperation::UPDATE:
				battleState.updateObstacle(change);
				break;
			default:
				throw std::runtime_error("Unknown obstacle operation");
				break;
		}
	}
}

void BattleStatePackVisitor::visitSetStackEffect(SetStackEffect & pack)
{
	for(const auto & stackData : pack.toRemove)
		battleState.removeUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : pack.toUpdate)
		battleState.updateUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : pack.toAdd)
		battleState.addUnitBonus(stackData.first, stackData.second);
}

void BattleStatePackVisitor::visitStacksInjured(StacksInjured & pack)
{
	for(const BattleStackAttacked & stack : pack.stacks)
	{
		battleState.setUnitState(stack.newState.id, stack.newState.data, stack.newState.healthDelta);
	}
}

void BattleStatePackVisitor::visitBattleUnitsChanged(BattleUnitsChanged & pack)
{
	for(auto & elem : pack.changedStacks)
	{
		switch(elem.operation)
		{
			case BattleChanges::EOperation::RESET_STATE:
				battleState.setUnitState(elem.id, elem.data, elem.healthDelta);
				break;
			case BattleChanges::EOperation::REMOVE:
				battleState.removeUnit(elem.id);
				break;
			case BattleChanges::EOperation::ADD:
				battleState.addUnit(elem.id, elem.data);
				break;
			case BattleChanges::EOperation::UPDATE:
				battleState.updateUnit(elem.id, elem.data);
				break;
			default:
				throw std::runtime_error("Unknown unit operation");
				break;
		}
	}
}

VCMI_LIB_NAMESPACE_END
