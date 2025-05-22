/*
 * MiscObjects.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MiscObjects.h"

#include "../bonuses/Propagators.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../constants/StringConstants.h"
#include "../entities/artifact/ArtifactUtils.h"
#include "../entities/artifact/CArtifact.h"
#include "../CConfigHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../CSkillHandler.h"
#include "../spells/CSpellHandler.h"
#include "../gameState/CGameState.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"
#include "../StartInfo.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/StackLocation.h"
#include "../lib/gameState/UpgradeInfo.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

///helpers
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return LIBRARY->generaltexth->allTexts[id];
}

void CTeamVisited::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	if(what == ObjProperty::VISITED)
		players.insert(identifier.as<PlayerColor>());
}

bool CTeamVisited::wasVisited(PlayerColor player) const
{
	return wasVisited(cb->getPlayerState(player)->team);
}

bool CTeamVisited::wasVisited(const CGHeroInstance * h) const
{
	return wasVisited(h->tempOwner);
}

bool CTeamVisited::wasVisited(const TeamID & team) const
{
	for(const auto & i : players)
	{
		if(cb->getPlayerState(i)->team == team)
			return true;
	}
	return false;
}

//CGMine
void CGMine::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	auto relations = cb->getPlayerRelations(h->tempOwner, tempOwner);

	if(relations == PlayerRelations::SAME_PLAYER) //we're visiting our mine
	{
		gameEvents.showGarrisonDialog(id,h->id,true);
		return;
	}
	else if (relations == PlayerRelations::ALLIES)//ally
		return;

	if(stacksCount()) //Mine is guarded
	{
		BlockingDialog ynd(true,false);
		ynd.player = h->tempOwner;
		ynd.text.appendLocalString(EMetaText::ADVOB_TXT, isAbandoned() ? 84 : 187);
		gameEvents.showBlockingDialog(this, &ynd);
		return;
	}

	flagMine(gameEvents, h->tempOwner);
}

void CGMine::initObj(IGameRandomizer & gameRandomizer)
{
	if(isAbandoned())
	{
		//set guardians
		int howManyTroglodytes = gameRandomizer.getDefault().nextInt(100, 199);
		auto troglodytes = std::make_unique<CStackInstance>(cb, CreatureID::TROGLODYTES, howManyTroglodytes);
		putStack(SlotID(0), std::move(troglodytes));

		assert(!abandonedMineResources.empty());
		if (!abandonedMineResources.empty())
		{
			producedResource = *RandomGeneratorUtil::nextItem(abandonedMineResources, gameRandomizer.getDefault());
		}
		else
		{
			logGlobal->error("Abandoned mine at (%s) has no valid resource candidates!", anchorPos().toString());
			producedResource = GameResID::GOLD;
		}
	}
	else
	{
		producedResource = GameResID(getObjTypeIndex().getNum());
	}
	producedQuantity = defaultResProduction();
}

bool CGMine::isAbandoned() const
{
	return subID.getNum() >= 7;
}

const IOwnableObject * CGMine::asOwnable() const
{
	return this;
}

std::vector<CreatureID> CGMine::providedCreatures() const
{
	return {};
}

ResourceSet CGMine::dailyIncome() const
{
	ResourceSet result;
	result[producedResource] += defaultResProduction();

	const auto & playerSettings = cb->getPlayerSettings(getOwner());
	result.applyHandicap(playerSettings->handicap.percentIncome);

	return result;
}

std::string CGMine::getObjectName() const
{
	return LIBRARY->generaltexth->translate("core.minename", getObjTypeIndex());
}

std::string CGMine::getHoverText(PlayerColor player) const
{
	std::string hoverName = CArmedInstance::getHoverText(player);

	if (tempOwner != PlayerColor::NEUTRAL)
		hoverName += "\n(" + LIBRARY->generaltexth->restypes[producedResource.getNum()] + ")";

	if(stacksCount())
	{
		hoverName += "\n";
		hoverName += LIBRARY->generaltexth->allTexts[202]; //Guarded by
		hoverName += " ";
		hoverName += getArmyDescription();
	}
	return hoverName;
}

void CGMine::flagMine(IGameEventCallback & gameEvents, const PlayerColor & player) const
{
	assert(tempOwner != player);
	gameEvents.setOwner(this, player); //not ours? flag it!

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.text.appendTextID(TextIdentifier("core.mineevnt", producedResource.getNum()).get()); //not use subID, abandoned mines uses default mine texts
	iw.player = player;
	iw.components.emplace_back(ComponentType::RESOURCE_PER_DAY, producedResource, getProducedQuantity());
	gameEvents.showInfoDialog(&iw);
}

ui32 CGMine::defaultResProduction() const
{
	switch(producedResource.toEnum())
	{
	case EGameResID::WOOD:
	case EGameResID::ORE:
		return 2;
	case EGameResID::GOLD:
		return 1000;
	default:
		return 1;
	}
}

ui32 CGMine::getProducedQuantity() const
{
	auto * playerSettings = cb->getPlayerSettings(getOwner());
	// always round up income - we don't want mines to always produce zero if handicap in use
	return vstd::divideAndCeil(producedQuantity * playerSettings->handicap.percentIncome, 100);
}

void CGMine::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == BattleSide::ATTACKER) //attacker won
	{
		if(isAbandoned())
		{
			hero->showInfoDialog(gameEvents, 85);
		}
		flagMine(gameEvents, hero->tempOwner);
	}
}

void CGMine::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
		gameEvents.startBattle(hero, this);
}

void CGMine::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CArmedInstance::serializeJsonOptions(handler);
	serializeJsonOwner(handler);
	if(isAbandoned())
	{
		if(handler.saving)
		{
			JsonNode node;
			for(const auto & resID : abandonedMineResources)
				node.Vector().emplace_back(GameConstants::RESOURCE_NAMES[resID.getNum()]);

			handler.serializeRaw("possibleResources", node, std::nullopt);
		}
		else
		{
			auto guard = handler.enterArray("possibleResources");
			const JsonNode & node = handler.getCurrent();

			auto names = node.convertTo<std::vector<std::string>>();

			for(const std::string & s : names)
			{
				int raw_res = vstd::find_pos(GameConstants::RESOURCE_NAMES, s);
				if(raw_res < 0)
					logGlobal->error("Invalid resource name: %s", s);
				else
					abandonedMineResources.emplace(raw_res);
			}
		}
	}
}

bool CGTeleport::isEntrance() const
{
	return type == BOTH || type == ENTRANCE;
}

bool CGTeleport::isExit() const
{
	return type == BOTH || type == EXIT;
}

bool CGTeleport::isChannelEntrance(const ObjectInstanceID & id) const
{
	return vstd::contains(getAllEntrances(), id);
}

bool CGTeleport::isChannelExit(const ObjectInstanceID & id) const
{
	return vstd::contains(getAllExits(), id);
}

std::vector<ObjectInstanceID> CGTeleport::getAllEntrances(bool excludeCurrent) const
{
	auto ret = cb->getTeleportChannelEntrances(channel);
	if(excludeCurrent)
		vstd::erase_if_present(ret, id);

	return ret;
}

std::vector<ObjectInstanceID> CGTeleport::getAllExits(bool excludeCurrent) const
{
	auto ret = cb->getTeleportChannelExits(channel);
	if(excludeCurrent)
		vstd::erase_if_present(ret, id);

	return ret;
}

ObjectInstanceID CGTeleport::getRandomExit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	auto passableExits = getPassableExits(cb->gameState(), h, getAllExits(true));
	if(!passableExits.empty())
		return *RandomGeneratorUtil::nextItem(passableExits, gameEvents.getRandomGenerator());

	return ObjectInstanceID();
}

bool CGTeleport::isTeleport(const CGObjectInstance * obj)
{
	return dynamic_cast<const CGTeleport *>(obj) != nullptr;
}

bool CGTeleport::isConnected(const CGTeleport * src, const CGTeleport * dst)
{
	return src && dst && src->isChannelExit(dst->id);
}

bool CGTeleport::isConnected(const CGObjectInstance * src, const CGObjectInstance * dst)
{
	const auto * srcObj = dynamic_cast<const CGTeleport *>(src);
	const auto * dstObj = dynamic_cast<const CGTeleport *>(dst);
	return isConnected(srcObj, dstObj);
}

bool CGTeleport::isExitPassable(const IGameInfoCallback & gameInfo, const CGHeroInstance * h, const CGObjectInstance * obj)
{
	ObjectInstanceID topObjectID = gameInfo.getTile(obj->visitablePos())->topVisitableObj();
	const CGObjectInstance * topObject = gameInfo.getObjInstance(topObjectID);

	if(topObject->ID == Obj::HERO)
	{
		if(h->id == topObject->id) // Just to be sure it's won't happen.
			return false;

		// Check if it's friendly hero or not
		if(gameInfo.getPlayerRelations(h->tempOwner, topObject->tempOwner) != PlayerRelations::ENEMIES)
		{
			// Exchange between heroes only possible via subterranean gates
			if(!dynamic_cast<const CGSubterraneanGate *>(obj))
				return false;
		}
	}
	return true;
}

std::vector<ObjectInstanceID> CGTeleport::getPassableExits(const IGameInfoCallback & gameInfo, const CGHeroInstance * h, std::vector<ObjectInstanceID> exits)
{
	vstd::erase_if(exits, [&](const ObjectInstanceID & exit) -> bool 
	{
		return !isExitPassable(gameInfo, h, gameInfo.getObj(exit));
	});
	return exits;
}

void CGTeleport::addToChannel(std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > &channelsList, const CGTeleport * obj)
{
	std::shared_ptr<TeleportChannel> tc;
	if(channelsList.find(obj->channel) == channelsList.end())
	{
		tc = std::make_shared<TeleportChannel>();
		channelsList.insert(std::make_pair(obj->channel, tc));
	}
	else
		tc = channelsList[obj->channel];

	if(obj->isEntrance() && !vstd::contains(tc->entrances, obj->id))
		tc->entrances.push_back(obj->id);

	if(obj->isExit() && !vstd::contains(tc->exits, obj->id))
		tc->exits.push_back(obj->id);

	if(!tc->entrances.empty() && !tc->exits.empty()
		&& (tc->entrances.size() != 1 || tc->entrances != tc->exits))
	{
		tc->passability = TeleportChannel::PASSABLE;
	}
}

TeleportChannelID CGMonolith::findMeChannel(const std::vector<Obj> & IDs, MapObjectSubID SubID) const
{
	for(auto teleportObj : cb->gameState().getMap().getObjects<CGMonolith>())
	{
		if(vstd::contains(IDs, teleportObj->ID) && teleportObj->subID == SubID)
			return teleportObj->channel;
	}
	return TeleportChannelID();
}

void CGMonolith::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	TeleportDialog td(h->id, channel);
	if(isEntrance())
	{
		if(cb->isTeleportChannelBidirectional(channel) && 1 < cb->getTeleportChannelExits(channel).size())
		{
			auto exits = cb->getTeleportChannelExits(channel);
			for(const auto & exit : exits)
			{
				td.exits.push_back(std::make_pair(exit, cb->getObj(exit)->visitablePos()));
			}
		}

		if(cb->isTeleportChannelImpassable(channel))
		{
			logGlobal->debug("Cannot find corresponding exit monolith for %d at %s", id.getNum(), anchorPos().toString());
			td.impassable = true;
		}
		else if(getRandomExit(gameEvents, h) == ObjectInstanceID())
			logGlobal->debug("All exits blocked for monolith %d at %s", id.getNum(), anchorPos().toString());
	}
	else
		h->showInfoDialog(gameEvents, 70);

	gameEvents.showTeleportDialog(&td);
}

void CGMonolith::teleportDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const
{
	int3 dPos;
	auto randomExit = getRandomExit(gameEvents, hero);
	auto realExits = getAllExits(true);
	if(!isEntrance() // Do nothing if hero visited exit only object
		|| (exits.empty() && realExits.empty()) // Do nothing if there no exits on this channel
		|| ObjectInstanceID() == randomExit) // Do nothing if all exits are blocked by friendly hero and it's not subterranean gate
	{
		return;
	}
	else if(vstd::isValidIndex(exits, answer))
		dPos = exits[answer].second;
	else
		dPos = cb->getObj(randomExit)->visitablePos();

	gameEvents.moveHero(hero->id, hero->convertFromVisitablePos(dPos), EMovementMode::MONOLITH);
}

void CGMonolith::initObj(IGameRandomizer & gameRandomizer)
{
	std::vector<Obj> IDs;
	IDs.push_back(ID);
	switch(ID.toEnum())
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		type = ENTRANCE;
		IDs.emplace_back(Obj::MONOLITH_ONE_WAY_EXIT);
		break;
	case Obj::MONOLITH_ONE_WAY_EXIT:
		type = EXIT;
		IDs.emplace_back(Obj::MONOLITH_ONE_WAY_ENTRANCE);
		break;
	case Obj::MONOLITH_TWO_WAY:
	default:
		type = BOTH;
		break;
	}

	channel = findMeChannel(IDs, subID);
	if(channel == TeleportChannelID())
		channel = TeleportChannelID(static_cast<si32>(cb->gameState().getMap().teleportChannels.size()));

	addToChannel(cb->gameState().getMap().teleportChannels, this);
}

void CGSubterraneanGate::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	TeleportDialog td(h->id, channel);
	if(cb->isTeleportChannelImpassable(channel))
	{
		h->showInfoDialog(gameEvents, 153);//Just inside the entrance you find a large pile of rubble blocking the tunnel. You leave discouraged.
		logGlobal->debug("Cannot find exit subterranean gate for  %d at %s", id.getNum(), anchorPos().toString());
		td.impassable = true;
	}
	else
	{
		auto exit = getRandomExit(gameEvents, h);
		td.exits.push_back(std::make_pair(exit, cb->getObj(exit)->visitablePos()));
	}

	gameEvents.showTeleportDialog(&td);
}

void CGSubterraneanGate::initObj(IGameRandomizer & gameRandomizer)
{
	type = BOTH;
}

void CGSubterraneanGate::postInit(IGameInfoCallback * cb) //matches subterranean gates into pairs
{
	//split on underground and surface gates
	std::vector<CGSubterraneanGate *> gatesSplit[2]; //surface and underground gates
	for(auto gate : cb->gameState().getMap().getObjects<CGSubterraneanGate>())
	{
		gatesSplit[gate->visitablePos().z].push_back(gate);
	}

	//sort by position
	std::sort(gatesSplit[0].begin(), gatesSplit[0].end(), [](const CGObjectInstance * a, const CGObjectInstance * b)
	{
		return a->visitablePos() < b->visitablePos();
	});

	auto assignToChannel = [&](CGSubterraneanGate * obj)
	{
		if(obj->channel == TeleportChannelID())
		{ // if object not linked to channel then create new channel
			obj->channel = TeleportChannelID(static_cast<si32>(cb->gameState().getMap().teleportChannels.size()));
			addToChannel(cb->gameState().getMap().teleportChannels, obj);
		}
	};

	for(size_t i = 0; i < gatesSplit[0].size(); i++)
	{
		CGSubterraneanGate * objCurrent = gatesSplit[0][i];

		//find nearest underground exit
		std::pair<int, si32> best(-1, std::numeric_limits<si32>::max()); //pair<pos_in_vector, distance^2>
		for(int j = 0; j < gatesSplit[1].size(); j++)
		{
			CGSubterraneanGate *checked = gatesSplit[1][j];
			if(checked->channel != TeleportChannelID())
				continue;
			si32 hlp = checked->visitablePos().dist2dSQ(objCurrent->visitablePos());
			if(hlp < best.second)
			{
				best.first = j;
				best.second = hlp;
			}
		}

		assignToChannel(objCurrent);
		if(best.first >= 0) //found pair
		{
			gatesSplit[1][best.first]->channel = objCurrent->channel;
			addToChannel(cb->gameState().getMap().teleportChannels, gatesSplit[1][best.first]);
		}
	}

	// we should assign empty channels to underground gates if they don't have matching overground gates
	for(auto & i : gatesSplit[1])
		assignToChannel(i);
}

void CGWhirlpool::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	TeleportDialog td(h->id, channel);
	if(cb->isTeleportChannelImpassable(channel))
	{
		logGlobal->debug("Cannot find exit whirlpool for %d at %s", id.getNum(), anchorPos().toString());
		td.impassable = true;
	}
	else if(getRandomExit(gameEvents, h) == ObjectInstanceID())
		logGlobal->debug("All exits are blocked for whirlpool  %d at %s", id.getNum(), anchorPos().toString());

	if(!isProtected(h))
	{
		SlotID targetstack = h->Slots().begin()->first; //slot numbers may vary
		for(auto i = h->Slots().rbegin(); i != h->Slots().rend(); i++)
		{
			if(h->getPower(targetstack) > h->getPower(i->first))
				targetstack = (i->first);
		}

		auto countToTake = static_cast<TQuantity>(h->getStackCount(targetstack) * 0.5);
		vstd::amax(countToTake, 1);

		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = h->tempOwner;
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 168);
		iw.components.emplace_back(ComponentType::CREATURE, h->getCreature(targetstack)->getId(), -countToTake);
		gameEvents.showInfoDialog(&iw);
		gameEvents.changeStackCount(StackLocation(h->id, targetstack), -countToTake, ChangeValueMode::RELATIVE);
	}
	else
	{
		auto exits = getAllExits();
		for(const auto & exit : exits)
		{
			auto blockedPosList = cb->getObj(exit)->getBlockedPos();
			for(const auto & bPos : blockedPosList)
				td.exits.push_back(std::make_pair(exit, bPos));
		}
	}

	gameEvents.showTeleportDialog(&td);
}

void CGWhirlpool::teleportDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const
{
	int3 dPos;
	auto realExits = getAllExits();
	if(exits.empty() && realExits.empty())
		return;
	else if(vstd::isValidIndex(exits, answer))
		dPos = exits[answer].second;
	else
	{
		auto exit = getRandomExit(gameEvents, hero);

		if(exit == ObjectInstanceID())
			return;

		const auto * obj = cb->getObj(exit);
		std::set<int3> tiles = obj->getBlockedPos();
		dPos = *RandomGeneratorUtil::nextItem(tiles, gameEvents.getRandomGenerator());
	}

	gameEvents.moveHero(hero->id, hero->convertFromVisitablePos(dPos), EMovementMode::MONOLITH);
}

bool CGWhirlpool::isProtected(const CGHeroInstance * h)
{
	return h->hasBonusOfType(BonusType::WHIRLPOOL_PROTECTION)
		|| (h->stacksCount() == 1 && h->Slots().begin()->second->getCount() == 1)
		|| (h->stacksCount() == 0 && h->getCommander() && h->getCommander()->alive);
}

const CArtifactInstance * CGArtifact::getArtifactInstance() const
{
	return cb->getArtInstance(storedArtifact);
}

ArtifactID CGArtifact::getArtifactType() const
{
	if(ID == Obj::SPELL_SCROLL)
		return ArtifactID::SPELL_SCROLL;
	else
		return getObjTypeIndex().getNum();
}

void CGArtifact::pickRandomObject(IGameRandomizer & gameRandomizer)
{
	switch(ID.toEnum())
	{
		case MapObjectID::RANDOM_ART:
			subID = gameRandomizer.rollArtifact();
			break;
		case MapObjectID::RANDOM_TREASURE_ART:
			subID = gameRandomizer.rollArtifact(EArtifactClass::ART_TREASURE);
			break;
		case MapObjectID::RANDOM_MINOR_ART:
			subID = gameRandomizer.rollArtifact(EArtifactClass::ART_MINOR);
			break;
		case MapObjectID::RANDOM_MAJOR_ART:
			subID = gameRandomizer.rollArtifact(EArtifactClass::ART_MAJOR);
			break;
		case MapObjectID::RANDOM_RELIC_ART:
			subID = gameRandomizer.rollArtifact(EArtifactClass::ART_RELIC);
			break;
	}

	if (ID != MapObjectID::SPELL_SCROLL && ID != MapObjectID::ARTIFACT)
	{
		ID = MapObjectID::ARTIFACT;
		setType(ID, subID);
	}
	else if (ID != MapObjectID::SPELL_SCROLL)
		ID = MapObjectID::ARTIFACT;
}

void CGArtifact::setArtifactInstance(const CArtifactInstance * instance)
{
	storedArtifact = instance->getId();
}

void CGArtifact::initObj(IGameRandomizer & gameRandomizer)
{
	blockVisit = true;
	if(ID == Obj::ARTIFACT)
	{
		assert(getArtifactType().hasValue());

		if (!storedArtifact.hasValue())
			setArtifactInstance(cb->gameState().createArtifact(getArtifactType()));
	}
	if(ID == Obj::SPELL_SCROLL)
		subID = 1;

	assert(getArtifactInstance()->getType());
	assert(!getArtifactInstance()->getParentNodes().empty());
}

std::string CGArtifact::getObjectName() const
{
	return getArtifactType().toEntity(LIBRARY)->getNameTranslated();
}

std::string CGArtifact::getPopupText(PlayerColor player) const
{
	if (settings["general"]["enableUiEnhancements"].Bool())
	{
		std::string description = getArtifactType().toEntity(LIBRARY)->getDescriptionTranslated();
		if (getArtifactType() == ArtifactID::SPELL_SCROLL)
			ArtifactUtils::insertScrrollSpellName(description, SpellID::NONE); // erase text placeholder
		return description;
	}
	else
		return getObjectName();
}

std::string CGArtifact::getPopupText(const CGHeroInstance * hero) const
{
	return getPopupText(hero->getOwner());
}

std::vector<Component> CGArtifact::getPopupComponents(PlayerColor player) const
{
	return {
		Component(ComponentType::ARTIFACT, getArtifactType())
	};
}

void CGArtifact::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(!stacksCount())
	{
		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = h->tempOwner;

		if(getArtifactInstance()->getType()->canBePutAt(h))
		{
			switch (ID.toEnum())
			{
			case Obj::ARTIFACT:
			{
				iw.components.emplace_back(ComponentType::ARTIFACT, getArtifactType());
				if(!message.empty())
					iw.text = message;
				else
					iw.text.appendTextID(getArtifactType().toArtifact()->getEventTextID());
			}
			break;
			case Obj::SPELL_SCROLL:
			{
				SpellID spell = getArtifactInstance()->getScrollSpellID();
				iw.components.emplace_back(ComponentType::SPELL, spell);
				if(!message.empty())
					iw.text = message;
				else
				{
					iw.text.appendLocalString(EMetaText::ADVOB_TXT,135);
					iw.text.replaceName(spell);
				}
			}
			break;
			}
		}
		else
		{
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, 2);
		}
		gameEvents.showInfoDialog(&iw);
		pick(gameEvents, h);
	}
	else
	{
		switch(ID.toEnum())
		{
		case Obj::ARTIFACT:
			{
				BlockingDialog ynd(true,false);
				ynd.player = h->getOwner();
				if(!message.empty())
					ynd.text = message;
				else
				{
					// TODO: Guard text is more complex in H3, see mantis issue 2325 for details
					ynd.text.appendLocalString(EMetaText::GENERAL_TXT, 420);
					ynd.text.replaceRawString("");
					ynd.text.replaceRawString(getArmyDescription());
					ynd.text.replaceLocalString(EMetaText::GENERAL_TXT, 43); // creatures
				}
				gameEvents.showBlockingDialog(this, &ynd);
			}
			break;
		case Obj::SPELL_SCROLL:
			{
				if(!message.empty())
				{
					BlockingDialog ynd(true,false);
					ynd.player = h->getOwner();
					ynd.text = message;
					gameEvents.showBlockingDialog(this, &ynd);
				}
				else
					blockingDialogAnswered(gameEvents, h, true);
			}
			break;
		}
	}
}

void CGArtifact::pick(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(gameEvents.putArtifact(ArtifactLocation(h->id, ArtifactPosition::FIRST_AVAILABLE), getArtifactInstance()->getId()))
		gameEvents.removeObject(this, h->getOwner());
}

BattleField CGArtifact::getBattlefield() const
{
	return BattleField::NONE;
}

void CGArtifact::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == BattleSide::ATTACKER) //attacker won
		pick(gameEvents, hero);
}

void CGArtifact::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
		gameEvents.startBattle(hero, this);
}

void CGArtifact::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeStruct("guardMessage", message);
	CArmedInstance::serializeJsonOptions(handler);
	if(!handler.saving && !handler.getCurrent()["guards"].Vector().empty())
		CCreatureSet::serializeJson(handler, "guards", 7);

	if(handler.saving && ID == Obj::SPELL_SCROLL)
	{
		const auto & b = getArtifactInstance()->getFirstBonus(Selector::type()(BonusType::SPELL));
		SpellID spellId(b->subtype.as<SpellID>());

		handler.serializeId("spell", spellId, SpellID::NONE);
	}
}

void CGSignBottle::initObj(IGameRandomizer & gameRandomizer)
{
	//if no text is set than we pick random from the predefined ones
	if(message.empty())
	{
		auto vector = LIBRARY->generaltexth->findStringsWithPrefix("core.randsign");
		std::string messageIdentifier = *RandomGeneratorUtil::nextItem(vector, gameRandomizer.getDefault());
		message.appendTextID(messageIdentifier);
	}

	if(ID == Obj::OCEAN_BOTTLE)
	{
		blockVisit = true;
	}
}

void CGSignBottle::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	iw.text = message;
	gameEvents.showInfoDialog(&iw);

	if(ID == Obj::OCEAN_BOTTLE)
		gameEvents.removeObject(this, h->getOwner());
}

void CGSignBottle::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeStruct("text", message);
}

const IOwnableObject * CGGarrison::asOwnable() const
{
	return this;
}

ResourceSet CGGarrison::dailyIncome() const
{
	return {};
}

std::vector<CreatureID> CGGarrison::providedCreatures() const
{
	return {};
}

void CGGarrison::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	auto relations = cb->getPlayerRelations(h->tempOwner, tempOwner);
	if (relations == PlayerRelations::ENEMIES && stacksCount() > 0) {
		//TODO: Find a way to apply magic garrison effects in battle.
		gameEvents.startBattle(h, this);
		return;
	}

	//New owner.
	if (relations == PlayerRelations::ENEMIES)
		gameEvents.setOwner(this, h->tempOwner);

	gameEvents.showGarrisonDialog(id, h->id, removableUnits);
}

bool CGGarrison::passableFor(PlayerColor player) const
{
	//FIXME: identical to same method in CGTownInstance

	if ( !stacksCount() )//empty - anyone can visit
		return true;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return false;

	if (cb->getPlayerRelations(tempOwner, player) != PlayerRelations::ENEMIES)
		return true;
	return false;
}

void CGGarrison::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == BattleSide::ATTACKER)
		onHeroVisit(gameEvents, hero);
}

void CGGarrison::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeBool("removableUnits", removableUnits);
	serializeJsonOwner(handler);
	CArmedInstance::serializeJsonOptions(handler);
}

void CGGarrison::initObj(IGameRandomizer & gameRandomizer)
{
	if(this->subID == MapObjectSubID::decode(this->ID, "antiMagic"))
		addAntimagicGarrisonBonus();
}

void CGGarrison::addAntimagicGarrisonBonus()
{
	auto bonus = std::make_shared<Bonus>();
	bonus->type = BonusType::BLOCK_ALL_MAGIC;
	bonus->source = BonusSource::OBJECT_TYPE;
	bonus->sid = BonusSourceID(this->ID);
	bonus->propagator = std::make_shared<CPropagatorNodeType>(CBonusSystemNode::BATTLE);
	bonus->duration = BonusDuration::PERMANENT;
	this->addNewBonus(bonus);
}

void CGMagi::initObj(IGameRandomizer & gameRandomizer)
{
	if (ID == Obj::EYE_OF_MAGI)
		blockVisit = true;
}

void CGMagi::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if (ID == Obj::HUT_OF_MAGI)
	{
		h->showInfoDialog(gameEvents, 61);

		std::vector<const CGObjectInstance *> eyes;

		for (const auto & object : cb->gameState().getMap().getObjects<CGMagi>())
		{
			if (object->ID == Obj::EYE_OF_MAGI && object->subID == this->subID)
				eyes.push_back(object);
		}

		if (!eyes.empty())
		{
			CenterView cv;
			cv.player = h->tempOwner;
			cv.focusTime = 2000;

			FoWChange fw;
			fw.player = h->tempOwner;
			fw.mode = ETileVisibility::REVEALED;
			fw.waitForDialogs = true;

			for(const auto & eye : eyes)
			{
				cb->getTilesInRange (fw.tiles, eye->visitablePos(), 10, ETileVisibility::HIDDEN, h->tempOwner);
				gameEvents.sendAndApply(fw);
				cv.pos = eye->visitablePos();

				gameEvents.sendAndApply(cv);
			}
			cv.pos = h->visitablePos();
			cv.focusTime = 0;
			gameEvents.sendAndApply(cv);
		}
	}
	else if (ID == Obj::EYE_OF_MAGI)
	{
		h->showInfoDialog(gameEvents, 48);
	}
}

CGBoat::CGBoat(IGameInfoCallback * cb)
	: CGObjectInstance(cb)
{
	direction = 4;
	layer = EPathfindingLayer::SAIL;
}

bool CGBoat::isCoastVisitable() const
{
	return true;
}

void CGBoat::setBoardedHero(const CGHeroInstance * hero)
{
	if (hero)
		boardedHeroID = hero->id;
	else
		boardedHeroID = ObjectInstanceID();
}

const CGHeroInstance * CGBoat::getBoardedHero() const
{
	if (boardedHeroID.hasValue())
		return cb->getHero(boardedHeroID);
	else
		return nullptr;
}

void CGSirens::initObj(IGameRandomizer & gameRandomizer)
{
	blockVisit = true;
}

std::string CGSirens::getHoverText(const CGHeroInstance * hero) const
{
	return getObjectName() + " " + visitedTxt(hero->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(ID)));
}

void CGSirens::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	if(h->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(ID))) //has already visited Sirens
	{
		iw.type = EInfoWindowMode::AUTO;
		iw.text.appendLocalString(EMetaText::ADVOB_TXT,133);
	}
	else
	{
		giveDummyBonus(gameEvents, h->id, BonusDuration::ONE_BATTLE);
		TExpType xp = 0;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			// 1-sized stacks are not affected by sirens
			if (i->second->getCount() == 1)
				continue;

			// tested H3 behavior: 30% (rounded up) of stack drowns
			TQuantity drown = std::ceil(i->second->getCount() * 0.3);

			if(drown)
			{
				gameEvents.changeStackCount(StackLocation(h->id, i->first), -drown, ChangeValueMode::RELATIVE);
				xp += drown * i->second->getType()->getMaxHealth();
			}
		}

		if(xp)
		{
			xp = h->calculateXp(static_cast<int>(xp));
			iw.text.appendLocalString(EMetaText::ADVOB_TXT,132);
			iw.text.replaceNumber(static_cast<int>(xp));
			gameEvents.giveExperience(h, xp);
		}
		else
		{
			iw.text.appendLocalString(EMetaText::ADVOB_TXT,134);
		}
	}
	gameEvents.showInfoDialog(&iw);
}

void CGShipyard::getOutOffsets( std::vector<int3> &offsets ) const
{
	// H J L K I
	// A x S x B
	// C E G F D
	offsets = {
		{-2, 0,  0}, // A
		{+2, 0,  0}, // B
		{-2, 1,  0}, // C
		{+2, 1,  0}, // D
		{-1, 1,  0}, // E
		{+1, 1,  0}, // F
		{0,  1,  0}, // G
		{-2, -1, 0}, // H
		{+2, -1, 0}, // I
		{-1, -1, 0}, // G
		{+1, -1, 0}, // K
		{0,  -1, 0}, // L
	};
}

const IObjectInterface * CGShipyard::getObject() const
{
	return this;
}

void CGShipyard::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(cb->getPlayerRelations(tempOwner, h->tempOwner) == PlayerRelations::ENEMIES)
		gameEvents.setOwner(this, h->tempOwner);

	if(shipyardStatus() != IBoatGenerator::GOOD)
	{
		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = tempOwner;
		getProblemText(iw.text, h);
		gameEvents.showInfoDialog(&iw);
	}
	else
	{
		gameEvents.showObjectWindow(this, EOpenWindowMode::SHIPYARD_WINDOW, h, false);
	}
}

void CGShipyard::serializeJsonOptions(JsonSerializeFormat& handler)
{
	serializeJsonOwner(handler);
}

BoatId CGShipyard::getBoatType() const
{
	return createdBoat;
}

const IOwnableObject * CGShipyard::asOwnable() const
{
	return this;
}

ResourceSet CGShipyard::dailyIncome() const
{
	return {};
}

std::vector<CreatureID> CGShipyard::providedCreatures() const
{
	return {};
}

void CGDenOfthieves::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	gameEvents.showObjectWindow(this, EOpenWindowMode::THIEVES_GUILD, h, false);
}

void CGObelisk::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->tempOwner;
	const TeamState *ts = cb->getPlayerTeam(h->tempOwner);
	assert(ts);
	TeamID team = ts->id;

	if(!wasVisited(team))
	{
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 96);
		gameEvents.sendAndApply(iw);

		// increment general visited obelisks counter
		gameEvents.setObjPropertyID(id, ObjProperty::OBELISK_VISITED, team);
		gameEvents.showObjectWindow(this, EOpenWindowMode::PUZZLE_MAP, h, false);

		// mark that particular obelisk as visited for all players in the team
		for(const auto & color : ts->players)
		{
			gameEvents.setObjPropertyID(id, ObjProperty::VISITED, color);
		}
	}
	else
	{
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 97);
		gameEvents.sendAndApply(iw);
	}

}

void CGObelisk::initObj(IGameRandomizer & gameRandomizer)
{
	cb->gameState().getMap().obeliskCount++;
}

std::string CGObelisk::getHoverText(PlayerColor player) const
{
	return getObjectName() + " " + visitedTxt(wasVisited(player));
}

std::string CGObelisk::getObjectDescription(PlayerColor player) const
{
	return visitedTxt(wasVisited(player));
}

void CGObelisk::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch(what)
	{
		case ObjProperty::OBELISK_VISITED:
			{
				auto progress = ++cb->gameState().getMap().obelisksVisited[identifier.as<TeamID>()];
				logGlobal->debug("Player %d: obelisk progress %d / %d", identifier.getNum(), static_cast<int>(progress) , static_cast<int>(cb->gameState().getMap().obeliskCount));

				if(progress > cb->gameState().getMap().obeliskCount)
				{
					logGlobal->error("Visited %d of %d", static_cast<int>(progress), cb->gameState().getMap().obeliskCount);
					throw std::runtime_error("Player visited " + std::to_string(progress) + " obelisks out of " + std::to_string(cb->gameState().getMap().obeliskCount) + " present on map!");
				}

				break;
			}
		default:
			CTeamVisited::setPropertyDer(what, identifier);
			break;
	}
}

void HillFort::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	gameEvents.showObjectWindow(this, EOpenWindowMode::HILL_FORT_WINDOW, h, false);
}

void HillFort::fillUpgradeInfo(UpgradeInfo & info, const CStackInstance &stack) const
{
	int32_t level = stack.getType()->getLevel();
	int32_t index = std::clamp<int32_t>(level - 1, 0, upgradeCostPercentage.size() - 1);

	int costModifier = upgradeCostPercentage[index];

	for(const auto & nid : stack.getCreature()->upgrades)
	{
		info.addUpgrade(nid, stack.getType(), costModifier);
	}
}

std::string HillFort::getPopupText(PlayerColor player) const
{
	MetaString message = MetaString::createFromRawString("{%s}\r\n\r\n%s");

	message.replaceName(ID, subID);
	message.replaceTextID(getDescriptionToolTip());

	return message.toString();
}

std::string HillFort::getPopupText(const CGHeroInstance * hero) const
{
	return getPopupText(hero->getOwner());
}


std::string HillFort::getDescriptionToolTip() const
{
	return TextIdentifier(getObjectHandler()->getBaseTextID(), "description").get();
}

std::string HillFort::getUnavailableUpgradeMessage() const
{
	assert(getObjectHandler()->getModScope() != "core");
	return TextIdentifier(getObjectHandler()->getBaseTextID(), "unavailableUpgradeMessage").get();
}

VCMI_LIB_NAMESPACE_END
