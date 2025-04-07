/*
 * CGameHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameHandler.h"

#include "CVCMIServer.h"
#include "TurnTimerHandler.h"
#include "ServerNetPackVisitors.h"
#include "ServerSpellCastEnvironment.h"
#include "battles/BattleProcessor.h"
#include "processors/HeroPoolProcessor.h"
#include "processors/NewTurnProcessor.h"
#include "processors/PlayerMessageProcessor.h"
#include "processors/TurnOrderProcessor.h"
#include "queries/QueriesProcessor.h"
#include "queries/MapQueries.h"
#include "queries/VisitQueries.h"

#include "../lib/ArtifactUtils.h"
#include "../lib/CArtHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CCreatureSet.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/CPlayerState.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CSoundBase.h"
#include "../lib/CThreadHelper.h"
#include "../lib/GameConstants.h"
#include "../lib/UnlockGuard.h"
#include "../lib/IGameSettings.h"
#include "../lib/ScriptHandler.h"
#include "../lib/StartInfo.h"
#include "../lib/TerrainHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/int3.h"

#include "../lib/battle/BattleInfo.h"

#include "../lib/entities/building/CBuilding.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/entities/hero/CHeroHandler.h"

#include "../lib/filesystem/FileInfo.h"
#include "../lib/filesystem/Filesystem.h"

#include "../lib/gameState/CGameState.h"
#include "../lib/gameState/UpgradeInfo.h"

#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"

#include "../lib/mapObjects/CGCreature.h"
#include "../lib/mapObjects/CGMarket.h"
#include "../lib/mapObjects/TownBuildingInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"

#include "../lib/modding/ModIncompatibility.h"

#include "../lib/networkPacks/StackLocation.h"

#include "../lib/pathfinder/CPathfinder.h"
#include "../lib/pathfinder/PathfinderOptions.h"
#include "../lib/pathfinder/TurnInfo.h"

#include "../lib/rmg/CMapGenOptions.h"

#include "../lib/serializer/CSaveFile.h"
#include "../lib/serializer/CLoadFile.h"
#include "../lib/serializer/Connection.h"

#include "../lib/spells/CSpellHandler.h"

#include <vstd/RNG.h>
#include <vstd/CLoggerBase.h>
#include <vcmi/events/EventBus.h>
#include <vcmi/events/GenericEvents.h>
#include <vcmi/events/AdventureEvents.h>

#include <boost/lexical_cast.hpp>

#define COMPLAIN_RET_IF(cond, txt) do {if (cond){complain(txt); return;}} while(0)
#define COMPLAIN_RET_FALSE_IF(cond, txt) do {if (cond){complain(txt); return false;}} while(0)
#define COMPLAIN_RET(txt) {complain(txt); return false;}
#define COMPLAIN_RETF(txt, FORMAT) {complain(boost::str(boost::format(txt) % FORMAT)); return false;}

static inline double distance(int3 a, int3 b)
{
	return std::sqrt((double)(a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

template <typename T>
void callWith(std::vector<T> args, std::function<void(T)> fun, ui32 which)
{
	fun(args[which]);
}

const Services * CGameHandler::services() const
{
	return LIBRARY;
}

const CGameHandler::BattleCb * CGameHandler::battle(const BattleID & battleID) const
{
	return gs->getBattle(battleID);
}

const CGameHandler::GameCb * CGameHandler::game() const
{
	return this;
}

vstd::CLoggerBase * CGameHandler::logger() const
{
	return logGlobal;
}

events::EventBus * CGameHandler::eventBus() const
{
	return serverEventBus.get();
}

CVCMIServer & CGameHandler::gameLobby() const
{
	return *lobby;
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero, SecondarySkill skill)
{
	changeSecSkill(hero, skill, 1, 0);
	expGiven(hero);
}

void CGameHandler::levelUpHero(const CGHeroInstance * hero)
{
	// required exp for at least 1 lvl-up hasn't been reached
	if (!hero->gainsLevel())
	{
		return;
	}

	// give primary skill
	logGlobal->trace("%s got level %d", hero->getNameTranslated(), hero->level);
	auto primarySkill = hero->nextPrimarySkill(getRandomGenerator());

	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = primarySkill;
	sps.abs = false;
	sps.val = 1;
	sendAndApply(sps);

	HeroLevelUp hlu;
	hlu.player = hero->tempOwner;
	hlu.heroId = hero->id;
	hlu.primskill = primarySkill;
	hlu.skills = hero->getLevelUpProposedSecondarySkills(heroPool->getHeroSkillsRandomGenerator(hero->getHeroTypeID()));

	if (hlu.skills.size() == 0)
	{
		sendAndApply(hlu);
		levelUpHero(hero);
	}
	else if (hlu.skills.size() == 1 || !hero->getOwner().isValidPlayer())
	{
		sendAndApply(hlu);
		levelUpHero(hero, hlu.skills.front());
	}
	else if (hlu.skills.size() > 1)
	{
		auto levelUpQuery = std::make_shared<CHeroLevelUpDialogQuery>(this, hlu, hero);
		hlu.queryID = levelUpQuery->queryID;
		queries->addQuery(levelUpQuery);
		sendAndApply(hlu);
		//level up will be called on query reply
	}
}

void CGameHandler::levelUpCommander (const CCommanderInstance * c, int skill)
{
	SetCommanderProperty scp;

	auto hero = dynamic_cast<const CGHeroInstance *>(c->armyObj);
	if (hero)
		scp.heroid = hero->id;
	else
	{
		complain ("Commander is not led by hero!");
		return;
	}

	scp.accumulatedBonus.additionalInfo = 0;
	scp.accumulatedBonus.duration = BonusDuration::PERMANENT;
	scp.accumulatedBonus.turnsRemain = 0;
	scp.accumulatedBonus.source = BonusSource::COMMANDER;
	scp.accumulatedBonus.valType = BonusValueType::BASE_NUMBER;
	if (skill <= ECommander::SPELL_POWER)
	{
		scp.which = SetCommanderProperty::BONUS;

		auto difference = [](std::vector< std::vector <ui8> > skillLevels, std::vector <ui8> secondarySkills, int skill)->int
		{
			int s = std::min (skill, (int)ECommander::SPELL_POWER); //spell power level controls also casts and resistance
			return skillLevels.at(skill).at(secondarySkills.at(s)) - (secondarySkills.at(s) ? skillLevels.at(skill).at(secondarySkills.at(s)-1) : 0);
		};

		switch (skill)
		{
			case ECommander::ATTACK:
				scp.accumulatedBonus.type = BonusType::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = BonusSubtypeID(PrimarySkill::ATTACK);
				break;
			case ECommander::DEFENSE:
				scp.accumulatedBonus.type = BonusType::PRIMARY_SKILL;
				scp.accumulatedBonus.subtype = BonusSubtypeID(PrimarySkill::DEFENSE);
				break;
			case ECommander::HEALTH:
				scp.accumulatedBonus.type = BonusType::STACK_HEALTH;
				scp.accumulatedBonus.valType = BonusValueType::PERCENT_TO_ALL; //TODO: check how it accumulates in original WoG with artifacts such as vial of life blood, elixir of life etc.
				break;
			case ECommander::DAMAGE:
				scp.accumulatedBonus.type = BonusType::CREATURE_DAMAGE;
				scp.accumulatedBonus.subtype = BonusCustomSubtype::creatureDamageBoth;
				scp.accumulatedBonus.valType = BonusValueType::PERCENT_TO_ALL;
				break;
			case ECommander::SPEED:
				scp.accumulatedBonus.type = BonusType::STACKS_SPEED;
				break;
			case ECommander::SPELL_POWER:
				scp.accumulatedBonus.type = BonusType::SPELL_DAMAGE_REDUCTION;
				scp.accumulatedBonus.subtype = BonusSubtypeID(SpellSchool::ANY);
				scp.accumulatedBonus.val = difference (LIBRARY->creh->skillLevels, c->secondarySkills, ECommander::RESISTANCE);
				sendAndApply(scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CREATURE_SPELL_POWER;
				scp.accumulatedBonus.val = difference (LIBRARY->creh->skillLevels, c->secondarySkills, ECommander::SPELL_POWER) * 100; //like hero with spellpower = ability level
				sendAndApply(scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CASTS;
				scp.accumulatedBonus.val = difference (LIBRARY->creh->skillLevels, c->secondarySkills, ECommander::CASTS);
				sendAndApply(scp); //additional pack
				scp.accumulatedBonus.type = BonusType::CREATURE_ENCHANT_POWER; //send normally
				break;
		}

		scp.accumulatedBonus.val = difference (LIBRARY->creh->skillLevels, c->secondarySkills, skill);
		sendAndApply(scp);

		scp.which = SetCommanderProperty::SECONDARY_SKILL;
		scp.additionalInfo = skill;
		scp.amount = c->secondarySkills.at(skill) + 1;
		sendAndApply(scp);
	}
	else if (skill >= 100)
	{
		for(const auto & bonus : LIBRARY->creh->skillRequirements.at(skill - 100).first)
		{
			scp.which = SetCommanderProperty::SPECIAL_SKILL;
			scp.accumulatedBonus = *bonus;
			scp.additionalInfo = skill; //unnormalized
			sendAndApply(scp);
		}
	}
	expGiven(hero);
}

void CGameHandler::levelUpCommander(const CCommanderInstance * c)
{
	if (!c->gainsLevel())
	{
		return;
	}
	CommanderLevelUp clu;

	auto hero = dynamic_cast<const CGHeroInstance *>(c->armyObj);
	if(hero)
	{
		clu.heroId = hero->id;
		clu.player = hero->tempOwner;
	}
	else
	{
		complain ("Commander is not led by hero!");
		return;
	}

	//picking sec. skills for choice

	for (int i = 0; i <= ECommander::SPELL_POWER; ++i)
	{
		if (c->secondarySkills.at(i) < ECommander::MAX_SKILL_LEVEL)
			clu.skills.push_back(i);
	}
	int i = 100;
	for (const auto & specialSkill : LIBRARY->creh->skillRequirements)
	{
		if (c->secondarySkills.at(specialSkill.second.first) >= ECommander::MAX_SKILL_LEVEL - 1
			&&  c->secondarySkills.at(specialSkill.second.second) >= ECommander::MAX_SKILL_LEVEL - 1
			&&  !vstd::contains (c->specialSkills, i))
			clu.skills.push_back (i);
		++i;
	}
	int skillAmount = static_cast<int>(clu.skills.size());

	if (!skillAmount)
	{
		sendAndApply(clu);
		levelUpCommander(c);
	}
	else if (skillAmount == 1  ||  hero->tempOwner == PlayerColor::NEUTRAL) //choose skill automatically
	{
		sendAndApply(clu);
		levelUpCommander(c, *RandomGeneratorUtil::nextItem(clu.skills, getRandomGenerator()));
	}
	else if (skillAmount > 1) //apply and ask for secondary skill
	{
		auto commanderLevelUp = std::make_shared<CCommanderLevelUpDialogQuery>(this, clu, hero);
		clu.queryID = commanderLevelUp->queryID;
		queries->addQuery(commanderLevelUp);
		sendAndApply(clu);
	}
}

void CGameHandler::expGiven(const CGHeroInstance *hero)
{
	if (hero->gainsLevel())
		levelUpHero(hero);
	else if (hero->commander && hero->commander->gainsLevel())
		levelUpCommander(hero->commander);

	//if (hero->commander && hero->level > hero->commander->level && hero->commander->gainsLevel())
// 		levelUpCommander(hero->commander);
// 	else
// 		levelUpHero(hero);
}

void CGameHandler::giveExperience(const CGHeroInstance * hero, TExpType amountToGain)
{
	TExpType maxExp = LIBRARY->heroh->reqExp(LIBRARY->heroh->maxSupportedLevel());
	TExpType currExp = hero->exp;

	if (gs->getMap().levelLimit != 0)
		maxExp = LIBRARY->heroh->reqExp(gs->getMap().levelLimit);

	TExpType canGainExp = 0;
	if (maxExp > currExp)
		canGainExp = maxExp - currExp;

	if (amountToGain > canGainExp)
	{
		// set given experience to max possible, but don't decrease if hero already over top
		amountToGain = canGainExp;

		InfoWindow iw;
		iw.player = hero->tempOwner;
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 1); //can gain no more XP
		iw.text.replaceTextID(hero->getNameTextID());
		sendAndApply(iw);
	}

	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = PrimarySkill::EXPERIENCE;
	sps.abs = false;
	sps.val = amountToGain;
	sendAndApply(sps);

	//hero may level up
	if (hero->commander && hero->commander->alive)
	{
		//FIXME: trim experience according to map limit?
		SetCommanderProperty scp;
		scp.heroid = hero->id;
		scp.which = SetCommanderProperty::EXPERIENCE;
		scp.amount = amountToGain;
		sendAndApply(scp);
	}

	expGiven(hero);
}

void CGameHandler::changePrimSkill(const CGHeroInstance * hero, PrimarySkill which, si64 val, bool abs)
{
	SetPrimSkill sps;
	sps.id = hero->id;
	sps.which = which;
	sps.abs = abs;
	sps.val = val;
	sendAndApply(sps);
}

void CGameHandler::changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs)
{
	if(!hero)
	{
		logGlobal->error("changeSecSkill provided no hero");
		return;
	}
	SetSecSkill sss;
	sss.id = hero->id;
	sss.which = which;
	sss.val = val;
	sss.abs = abs;
	sendAndApply(sss);

	if (hero->visitedTown)
		giveSpells(hero->visitedTown, hero);

	// Our scouting range may have changed - update it
	if (hero->getOwner().isValidPlayer())
		changeFogOfWar(hero->getSightCenter(), hero->getSightRadius(), hero->getOwner(), ETileVisibility::REVEALED);

}

void CGameHandler::handleClientDisconnection(std::shared_ptr<CConnection> c)
{
	if(gameLobby().getState() == EServerState::SHUTDOWN || !gs || !gs->getStartInfo())
	{
		assert(0); // game should have shut down before reaching this point!
		return;
	}
	
	for(auto & playerConnections : connections)
	{
		PlayerColor playerId = playerConnections.first;
		auto * playerSettings = gs->getStartInfo()->getPlayersSettings(playerId.getNum());
		if(!playerSettings)
			continue;
		
		auto playerConnection = vstd::find(playerConnections.second, c);
		if(playerConnection == playerConnections.second.end())
			continue;

		logGlobal->trace("Player %s disconnected. Notifying remaining players", playerId.toString());

		// this player have left the game - broadcast infowindow to all in-game players
		for (auto i = gs->players.cbegin(); i!=gs->players.cend(); i++)
		{
			if (i->first == playerId)
				continue;

			if (getPlayerState(i->first)->status != EPlayerStatus::INGAME)
				continue;

			logGlobal->trace("Notifying player %s", i->first);

			InfoWindow out;
			out.player = i->first;
			out.text.appendTextID("vcmi.server.errors.playerLeft");
			out.text.replaceName(playerId);
			out.components.emplace_back(ComponentType::FLAG, playerId);
			sendAndApply(out);
		}
	}
}

void CGameHandler::handleReceivedPack(std::shared_ptr<CConnection> connection, CPackForServer & pack)
{
	//prepare struct informing that action was applied
	auto sendPackageResponse = [&](bool successfullyApplied)
	{
		PackageApplied applied;
		applied.player = pack.player;
		applied.result = successfullyApplied;
		applied.packType = CTypeList::getInstance().getTypeID(&pack);
		applied.requestID = pack.requestID;
		connection->sendPack(applied);
	};

	if(isBlockedByQueries(&pack, pack.player))
	{
		sendPackageResponse(false);
	}
	else
	{
		bool result;
		try
		{
			ApplyGhNetPackVisitor applier(*this, connection);
			pack.visit(applier);
			result = applier.getResult();
		}
		catch(ExceptionNotAllowedAction &)
		{
			result = false;
		}

		if(result)
			logGlobal->trace("Message %s successfully applied!", typeid(pack).name());
		else
			complain((boost::format("Got false in applying %s... that request must have been fishy!")
				% typeid(pack).name()).str());

		sendPackageResponse(true);
	}
}

CGameHandler::CGameHandler(CVCMIServer * lobby)
	: lobby(lobby)
	, heroPool(std::make_unique<HeroPoolProcessor>(this))
	, battles(std::make_unique<BattleProcessor>(this))
	, turnOrder(std::make_unique<TurnOrderProcessor>(this))
	, queries(std::make_unique<QueriesProcessor>())
	, playerMessages(std::make_unique<PlayerMessageProcessor>(this))
	, randomNumberGenerator(std::make_unique<CRandomGenerator>())
	, complainNoCreatures("No creatures to split")
	, complainNotEnoughCreatures("Cannot split that stack, not enough creatures!")
	, complainInvalidSlot("Invalid slot accessed!")
	, turnTimerHandler(std::make_unique<TurnTimerHandler>(*this))
	, newTurnProcessor(std::make_unique<NewTurnProcessor>(this))
{
	QID = 1;

	spellEnv = new ServerSpellCastEnvironment(this);
}

CGameHandler::~CGameHandler()
{
	delete spellEnv;
	delete gs;
	gs = nullptr;
}

void CGameHandler::reinitScripting()
{
	serverEventBus = std::make_unique<events::EventBus>();
#if SCRIPTING_ENABLED
	serverScripts.reset(new scripting::PoolImpl(this, spellEnv));
#endif
}

void CGameHandler::init(StartInfo *si, Load::ProgressAccumulator & progressTracking)
{
	int requestedSeed = settings["server"]["seed"].Integer();
	if (requestedSeed != 0)
		randomNumberGenerator->setSeed(requestedSeed);
	logGlobal->info("Using random seed: %d", randomNumberGenerator->nextInt());

	CMapService mapService;
	gs = new CGameState();
	gs->preInit(LIBRARY, this);
	logGlobal->info("Gamestate created!");
	gs->init(&mapService, si, progressTracking);
	logGlobal->info("Gamestate initialized!");

	for (auto & elem : gs->players)
		turnOrder->addPlayer(elem.first);

	for (auto & elem : gs->getMap().allHeroes)
	{
		if(elem)
			heroPool->getHeroSkillsRandomGenerator(elem->getHeroTypeID()); // init RMG seed
	}

	reinitScripting();
}

void CGameHandler::setPortalDwelling(const CGTownInstance * town, bool forced=false, bool clear = false)
{// bool forced = true - if creature should be replaced, if false - only if no creature was set

	const PlayerState * p = getPlayerState(town->tempOwner);
	if (!p)
	{
		assert(town->tempOwner == PlayerColor::NEUTRAL);
		return;
	}

	if (forced || town->creatures.at(town->getTown()->creatures.size()).second.empty())//we need to change creature
		{
			SetAvailableCreatures ssi;
			ssi.tid = town->id;
			ssi.creatures = town->creatures;
			ssi.creatures[town->getTown()->creatures.size()].second.clear();//remove old one

			std::set<CreatureID> availableCreatures;
			for (const auto & dwelling : p->getOwnedObjects())
			{
				const auto & dwellingCreatures = dwelling->asOwnable()->providedCreatures();
				availableCreatures.insert(dwellingCreatures.begin(), dwellingCreatures.end());
			}

			if (availableCreatures.empty())
				return;

			CreatureID creatureId = *RandomGeneratorUtil::nextItem(availableCreatures, getRandomGenerator());

			if (clear)
			{
				ssi.creatures[town->getTown()->creatures.size()].first = std::max(1, (creatureId.toEntity(LIBRARY)->getGrowth())/2);
			}
			else
			{
				ssi.creatures[town->getTown()->creatures.size()].first = creatureId.toEntity(LIBRARY)->getGrowth();
			}
			ssi.creatures[town->getTown()->creatures.size()].second.push_back(creatureId);
			sendAndApply(ssi);
		}
}

void CGameHandler::onPlayerTurnStarted(PlayerColor which)
{
	events::PlayerGotTurn::defaultExecute(serverEventBus.get(), which);
	turnTimerHandler->onPlayerGetTurn(which);
	newTurnProcessor->onPlayerTurnStarted(which);
}

void CGameHandler::onPlayerTurnEnded(PlayerColor which)
{
	newTurnProcessor->onPlayerTurnEnded(which);
}

void CGameHandler::addStatistics(StatisticDataSet &stat) const
{
	for (const auto & elem : gs->players)
	{
		if (elem.first == PlayerColor::NEUTRAL || !elem.first.isValidPlayer())
			continue;

		auto data = StatisticDataSet::createEntry(&elem.second, gs);

		stat.add(data);
	}
}

void CGameHandler::onNewTurn()
{
	logGlobal->trace("Turn %d", gs->day+1);

	bool firstTurn = !getDate(Date::DAY);
	bool newMonth = getDate(Date::DAY_OF_MONTH) == 28;

	if (firstTurn)
	{
		for (auto obj : gs->getMap().objects)
		{
			if (obj && obj->ID == Obj::PRISON) //give imprisoned hero 0 exp to level him up. easiest to do at this point
			{
				giveExperience(getHero(obj->id), 0);
			}
		}

		for (auto & elem : gs->players)
			heroPool->onNewWeek(elem.first);

	}
	else
	{
		addStatistics(gameState()->statistic); // write at end of turn
	}

	for (CGTownInstance *t : gs->getMap().towns)
	{
		PlayerColor player = t->tempOwner;

		if(t->hasBuilt(BuildingID::GRAIL)
			&& t->getTown()->buildings.at(BuildingID::GRAIL)->height == CBuilding::HEIGHT_SKYSHIP)
		{
			// Skyship, probably easier to handle same as Veil of darkness
			// do it every new day before veils
			if (player.isValidPlayer())
				changeFogOfWar(t->getSightCenter(), t->getSightRadius(), player, ETileVisibility::REVEALED);
		}
	}

	for (CGTownInstance *t : gs->getMap().towns)
	{
		if (t->hasBonusOfType (BonusType::DARKNESS))
		{
			for (auto & player : gs->players)
			{
				if (getPlayerStatus(player.first) == EPlayerStatus::INGAME &&
					getPlayerRelations(player.first, t->tempOwner) == PlayerRelations::ENEMIES)
					changeFogOfWar(t->getSightCenter(), t->valOfBonuses(BonusType::DARKNESS), player.first, ETileVisibility::HIDDEN);
			}
		}
	}

	if (newMonth)
	{
		SetAvailableArtifacts saa;
		saa.id = ObjectInstanceID::NONE;
		pickAllowedArtsSet(saa.arts, getRandomGenerator());
		sendAndApply(saa);
	}

	newTurnProcessor->onNewTurn();

	if (!firstTurn)
		checkVictoryLossConditionsForAll(); // check for map turn limit

	//call objects
	for (auto & elem : gs->getMap().objects)
	{
		if (elem)
			elem->newTurn(getRandomGenerator());
	}

	synchronizeArtifactHandlerLists(); //new day events may have changed them. TODO better of managing that
}

void CGameHandler::start(bool resume)
{
	LOG_TRACE_PARAMS(logGlobal, "resume=%d", resume);

	for (auto cc : gameLobby().activeConnections)
	{
		auto players = gameLobby().getAllClientPlayers(cc->connectionID);
		std::stringstream sbuffer;
		sbuffer << "Connection " << cc->connectionID << " will handle " << players.size() << " player: ";
		for (PlayerColor color : players)
		{
			sbuffer << color << " ";
			connections[color].insert(cc);
		}
		logGlobal->info(sbuffer.str());
	}

#if SCRIPTING_ENABLED
	services()->scripts()->run(serverScripts);
#endif

	if (!resume)
	{
		onNewTurn();
		events::TurnStarted::defaultExecute(serverEventBus.get());
		for(auto & player : gs->players)
			turnTimerHandler->onGameplayStart(player.first);
	}
	else
		events::GameResumed::defaultExecute(serverEventBus.get());

	turnOrder->onGameStarted();
}

void CGameHandler::tick(int millisecondsPassed)
{
	turnTimerHandler->update(millisecondsPassed);
}

void CGameHandler::giveSpells(const CGTownInstance *t, const CGHeroInstance *h)
{
	if (!h->hasSpellbook())
		return; //hero hasn't spellbook
	ChangeSpells cs;
	cs.hid = h->id;
	cs.learn = true;
	if (t->hasBuilt(BuildingID::GRAIL, ETownType::CONFLUX) && t->hasBuilt(BuildingID::MAGES_GUILD_1))
	{
		// Aurora Borealis give spells of all levels even if only level 1 mages guild built
		for (int i = 0; i < h->maxSpellLevel(); i++)
		{
			std::vector<SpellID> spells;
			getAllowedSpells(spells, i+1);
			for (auto & spell : spells)
				cs.spells.insert(spell);
		}
	}
	else
	{
		for (int i = 0; i < std::min(t->mageGuildLevel(), h->maxSpellLevel()); i++)
		{
			for (int j = 0; j < t->spellsAtLevel(i+1, true) && j < t->spells.at(i).size(); j++)
			{
				if (!h->spellbookContainsSpell(t->spells.at(i).at(j)))
					cs.spells.insert(t->spells.at(i).at(j));
			}
		}
	}
	if (!cs.spells.empty())
		sendAndApply(cs);
}

bool CGameHandler::removeObject(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	if (!obj || !getObj(obj->id))
	{
		logGlobal->error("Something wrong, that object already has been removed or hasn't existed!");
		return false;
	}

	RemoveObject ro;
	ro.objectID = obj->id;
	ro.initiator = initiator;
	sendAndApply(ro);

	checkVictoryLossConditionsForAll(); //e.g. if monster escaped (removing objs after battle is done directly by endBattle, not this function)
	return true;
}

bool CGameHandler::moveHero(ObjectInstanceID hid, int3 dst, EMovementMode movementMode, bool transit, PlayerColor asker)
{
	const CGHeroInstance *h = getHero(hid);
	// not turn of that hero or player can't simply teleport hero (at least not with this function)
	if(!h || (asker != PlayerColor::NEUTRAL && movementMode != EMovementMode::STANDARD))
	{
		if(h && getStartInfo()->turnTimerInfo.isEnabled() && gs->players[h->getOwner()].turnTimer.turnTimer == 0)
			return true; //timer expired, no error
		
		logGlobal->error("Illegal call to move hero!");
		return false;
	}

	logGlobal->trace("Player %d (%s) wants to move hero %d from %s to %s", asker, asker.toString(), hid.getNum(), h->anchorPos().toString(), dst.toString());
	const int3 hmpos = h->convertToVisitablePos(dst);

	if (!gs->getMap().isInTheMap(hmpos))
	{
		logGlobal->error("Destination tile is outside the map!");
		return false;
	}

	const TerrainTile t = *getTile(hmpos);
	const int3 guardPos = gs->guardingCreaturePosition(hmpos);
	CGObjectInstance * objectToVisit = nullptr;
	CGObjectInstance * guardian = nullptr;

	if (!t.visitableObjects.empty())
		objectToVisit = t.visitableObjects.back();

	if (isInTheMap(guardPos))
	{
		for (auto const & object : getTile(guardPos)->visitableObjects)
			if (object->ID == MapObjectID::MONSTER) // exclude other objects, such as hero flying above monster
				guardian = object;
	}

	const bool embarking = !h->boat && objectToVisit && objectToVisit->ID == Obj::BOAT;
	const bool disembarking = h->boat
		&& t.isLand()
		&& (dst == h->pos || (h->boat->layer == EPathfindingLayer::SAIL && !t.blocked()));

	//result structure for start - movement failed, no move points used
	TryMoveHero tmh;
	tmh.id = hid;
	tmh.start = h->pos;
	tmh.end = dst;
	tmh.result = TryMoveHero::FAILED;
	tmh.movePoints = h->movementPointsRemaining();

	//check if destination tile is available
	auto pathfinderHelper = std::make_unique<CPathfinderHelper>(gs, h, PathfinderOptions(this));
	auto ti = pathfinderHelper->getTurnInfo();

	const bool canFly = ti->hasFlyingMovement() || (h->boat && h->boat->layer == EPathfindingLayer::AIR);
	const bool canWalkOnSea = ti->hasWaterWalking() || (h->boat && h->boat->layer == EPathfindingLayer::WATER);
	const int cost = pathfinderHelper->getMovementCost(h->visitablePos(), hmpos, nullptr, nullptr, h->movementPointsRemaining());

	const bool movingOntoObstacle = t.blocked() && !t.visitable();
	const bool objectCoastVisitable = objectToVisit && objectToVisit->isCoastVisitable();
	const bool movingOntoWater = !h->boat && t.isWater() && !objectCoastVisitable;

	const auto complainRet = [&](const std::string & message)
	{
		//send info about movement failure
		complain(message);
		sendAndApply(tmh);
		return false;
	};

	if (guardian && getVisitingHero(guardian) != nullptr)
		return complainRet("You cannot move your hero there. Simultaneous turns are active and another player is interacting with this wandering monster!");

	if (objectToVisit && getVisitingHero(objectToVisit) != nullptr && getVisitingHero(objectToVisit) != h)
		return complainRet("You cannot move your hero there. Simultaneous turns are active and another player is interacting with this map object!");

	if (objectToVisit &&
		objectToVisit->getOwner().isValidPlayer() &&
		getPlayerRelations(objectToVisit->getOwner(), h->getOwner()) == PlayerRelations::ENEMIES &&
		!turnOrder->isContactAllowed(objectToVisit->getOwner(), h->getOwner()))
		return complainRet("You cannot move your hero there. This object belongs to another player and simultaneous turns are still active!");

	//it's a rock or blocked and not visitable tile
	//OR hero is on land and dest is water and (there is not present only one object - boat)
	if (!t.getTerrain()->isPassable() || (movingOntoObstacle && !canFly))
		return complainRet("Cannot move hero, destination tile is blocked!");

	//hero is not on boat/water walking and dst water tile doesn't contain boat/hero (objs visitable from land) -> we test back cause boat may be on top of another object (#276)
	if(movingOntoWater && !canFly && !canWalkOnSea)
		return complainRet("Cannot move hero, destination tile is on water!");

	if(h->boat && h->boat->layer == EPathfindingLayer::SAIL && t.isLand() && t.blocked())
		return complainRet("Cannot disembark hero, tile is blocked!");

	if(distance(h->pos, dst) >= 1.5 && movementMode == EMovementMode::STANDARD)
		return complainRet("Tiles " + h->pos.toString()+ " and "+ dst.toString() +" are not neighboring!");

	if(h->inTownGarrison)
		return complainRet("Can not move garrisoned hero!");

	if(h->movementPointsRemaining() < cost && dst != h->pos && movementMode == EMovementMode::STANDARD)
		return complainRet("Hero doesn't have any movement points left!");

	if (transit && !canFly && !(canWalkOnSea && t.isWater()) && !CGTeleport::isTeleport(objectToVisit))
		return complainRet("Hero cannot transit over this tile!");

	//several generic blocks of code

	// should be called if hero changes tile but before applying TryMoveHero package
	auto leaveTile = [&]()
	{
		for (CGObjectInstance *obj : gs->getMap().getTile(h->visitablePos()).visitableObjects)
		{
			obj->onHeroLeave(h);
		}
		this->getTilesInRange(tmh.fowRevealed, h->getSightCenter()+(tmh.end-tmh.start), h->getSightRadius(), ETileVisibility::HIDDEN, h->tempOwner);
	};

	auto doMove = [&](TryMoveHero::EResult result, EGuardLook lookForGuards,
								EVisitDest visitDest, ELEaveTile leavingTile) -> bool
	{
		LOG_TRACE_PARAMS(logGlobal, "Hero %s starts movement from %s to %s", h->getNameTranslated() % tmh.start.toString() % tmh.end.toString());

		auto moveQuery = std::make_shared<CHeroMovementQuery>(this, tmh, h);
		queries->addQuery(moveQuery);

		if (leavingTile == LEAVING_TILE)
			leaveTile();

		if (lookForGuards == CHECK_FOR_GUARDS && isInTheMap(guardPos))
			tmh.attackedFrom = guardPos;

		tmh.result = result;
		sendAndApply(tmh);

		if (visitDest == VISIT_DEST && objectToVisit && objectToVisit->id == h->id)
		{ // Hero should be always able to visit any object he is staying on even if there are guards around
			visitObjectOnTile(t, h);
		}
		else if (lookForGuards == CHECK_FOR_GUARDS && isInTheMap(guardPos))
		{
			objectVisited(guardian, h);

			moveQuery->visitDestAfterVictory = visitDest==VISIT_DEST;
		}
		else if (visitDest == VISIT_DEST)
		{
			visitObjectOnTile(t, h);
		}

		queries->popIfTop(moveQuery);
		logGlobal->trace("Hero %s ends movement", h->getNameTranslated());
		return result != TryMoveHero::FAILED;
	};

	//interaction with blocking object (like resources)
	auto blockingVisit = [&]() -> bool
	{
		for (CGObjectInstance *obj : t.visitableObjects)
		{
			if(h->boat && !obj->isBlockedVisitable() && !h->boat->onboardVisitAllowed)
				return doMove(TryMoveHero::SUCCESS, this->IGNORE_GUARDS, DONT_VISIT_DEST, REMAINING_ON_TILE);
			
			if (obj != h && obj->isBlockedVisitable() && !obj->passableFor(h->tempOwner))
			{
				EVisitDest visitDest = VISIT_DEST;
				if(h->boat && !h->boat->onboardVisitAllowed)
					visitDest = DONT_VISIT_DEST;
				
				return doMove(TryMoveHero::BLOCKING_VISIT, this->IGNORE_GUARDS, visitDest, REMAINING_ON_TILE);
			}
		}
		return false;
	};


	if (!transit && embarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movementPointsRemaining(), cost, false, ti);
		return doMove(TryMoveHero::EMBARK, IGNORE_GUARDS, DONT_VISIT_DEST, LEAVING_TILE);
		// In H3 embark ignore guards
	}

	if (disembarking)
	{
		tmh.movePoints = h->movementPointsAfterEmbark(h->movementPointsRemaining(), cost, true, ti);
		return doMove(TryMoveHero::DISEMBARK, CHECK_FOR_GUARDS, VISIT_DEST, LEAVING_TILE);
	}

	if (movementMode != EMovementMode::STANDARD)
	{
		if (blockingVisit()) // e.g. hero on the other side of teleporter
			return true;

		EGuardLook guardsCheck = (getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_TRIGGERS_GUARDS) && movementMode == EMovementMode::DIMENSION_DOOR)
			? CHECK_FOR_GUARDS
			: IGNORE_GUARDS;

		doMove(TryMoveHero::TELEPORTATION, guardsCheck, DONT_VISIT_DEST, LEAVING_TILE);

		// visit town for town portal \ castle gates
		// do not visit any other objects, e.g. monoliths to avoid double-teleporting
		if (objectToVisit)
		{
			if (CGTownInstance * town = dynamic_cast<CGTownInstance *>(objectToVisit))
				objectVisited(town, h);
		}

		return true;
	}
	

	//still here? it is standard movement!
	{
		tmh.movePoints = (int)h->movementPointsRemaining() >= cost
						? h->movementPointsRemaining() - cost
						: 0;

		EGuardLook lookForGuards = CHECK_FOR_GUARDS;
		EVisitDest visitDest = VISIT_DEST;
		if (transit)
		{
			if (CGTeleport::isTeleport(objectToVisit))
				visitDest = DONT_VISIT_DEST;

			if (canFly || (canWalkOnSea && t.isWater()))
			{
				lookForGuards = IGNORE_GUARDS;
				visitDest = DONT_VISIT_DEST;
			}
		}
		else if (blockingVisit())
			return true;
		
		if(h->boat && !h->boat->onboardAssaultAllowed)
			lookForGuards = IGNORE_GUARDS;

		turnTimerHandler->setEndTurnAllowed(h->getOwner(), !movingOntoWater && !movingOntoObstacle);
		doMove(TryMoveHero::SUCCESS, lookForGuards, visitDest, LEAVING_TILE);
		gs->statistic.accumulatedValues[asker].movementPointsUsed += tmh.movePoints;
		return true;
	}
}

bool CGameHandler::teleportHero(ObjectInstanceID hid, ObjectInstanceID dstid, ui8 source, PlayerColor asker)
{
	const CGHeroInstance *h = getHero(hid);
	const CGTownInstance *t = getTown(dstid);

	if (!h || !t)
		COMPLAIN_RET("Invalid call to teleportHero!");

	const CGTownInstance *from = h->visitedTown;
	if (((h->getOwner() != t->getOwner())
		&& complain("Cannot teleport hero to another player"))

	|| (from->getFactionID() != t->getFactionID()
		&& complain("Source town and destination town should belong to the same faction"))

	|| ((!from || !from->hasBuilt(BuildingSubID::CASTLE_GATE))
		&& complain("Hero must be in town with Castle gate for teleporting"))

	|| (!t->hasBuilt(BuildingSubID::CASTLE_GATE)
		&& complain("Cannot teleport hero to town without Castle gate in it")))
			return false;

	int3 pos = h->convertFromVisitablePos(t->visitablePos());
	moveHero(hid,pos,EMovementMode::CASTLE_GATE);
	return true;
}

void CGameHandler::setOwner(const CGObjectInstance * obj, const PlayerColor owner)
{
	PlayerColor oldOwner = getOwner(obj->id);

	setObjPropertyID(obj->id, ObjProperty::OWNER, owner);

	std::set<PlayerColor> playerColors = {owner, oldOwner};
	checkVictoryLossConditions(playerColors);

	const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(obj);
	if (town) //town captured
	{
		gs->statistic.accumulatedValues[owner].lastCapturedTownDay = gs->getDate(Date::DAY);

		if (owner.isValidPlayer()) //new owner is real player
		{
			if (town->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
				setPortalDwelling(town, true, false);
		}
	}

	if ((obj->ID == Obj::CREATURE_GENERATOR1 || obj->ID == Obj::CREATURE_GENERATOR4))
	{
		if (owner.isValidPlayer())
		{
			for (const CGTownInstance * t : getPlayerState(owner)->getTowns())
			{
				if (t->hasBuilt(BuildingSubID::PORTAL_OF_SUMMONING))
					setPortalDwelling(t);//set initial creatures for all portals of summoning
			}
		}
	}
}

void CGameHandler::showBlockingDialog(const IObjectInterface * caller, BlockingDialog *iw)
{
	auto dialogQuery = std::make_shared<CBlockingDialogQuery>(this, caller, *iw);
	queries->addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(*iw);
}

void CGameHandler::showTeleportDialog(TeleportDialog *iw)
{
	auto dialogQuery = std::make_shared<CTeleportDialogQuery>(this, *iw);
	queries->addQuery(dialogQuery);
	iw->queryID = dialogQuery->queryID;
	sendToAllClients(*iw);
}

void CGameHandler::giveResource(PlayerColor player, GameResID which, int val) //TODO: cap according to Bersy's suggestion
{
	if (!val) return; //don't waste time on empty call

	TResources resources;
	resources[which] = val;
	giveResources(player, resources);
}

void CGameHandler::giveResources(PlayerColor player, TResources resources)
{
	SetResources sr;
	sr.abs = false;
	sr.player = player;
	sr.res = resources;
	sendAndApply(sr);
}

void CGameHandler::giveCreatures(const CArmedInstance *obj, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove)
{
	COMPLAIN_RET_IF(!creatures.stacksCount(), "Strange, giveCreatures called without args!");
	COMPLAIN_RET_IF(obj->stacksCount(), "Cannot give creatures from not-cleared object!");
	COMPLAIN_RET_IF(creatures.stacksCount() > GameConstants::ARMY_SIZE, "Too many stacks to give!");

	//first we move creatures to give to make them army of object-source
	for (auto & elem : creatures.Slots())
	{
		addToSlot(StackLocation(obj->id, obj->getSlotFor(elem.second->getCreature())), elem.second->getCreature(), elem.second->count);
	}

	tryJoiningArmy(obj, h, remove, true);
}

void CGameHandler::takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures)
{
	std::vector<CStackBasicDescriptor> cres = creatures;
	if (cres.size() <= 0)
		return;
	const CArmedInstance* obj = static_cast<const CArmedInstance*>(getObj(objid));

	for (CStackBasicDescriptor &sbd : cres)
	{
		TQuantity collected = 0;
		while(collected < sbd.count)
		{
			bool foundSth = false;
			for (auto i = obj->Slots().begin(); i != obj->Slots().end(); i++)
			{
				if (i->second->getType() == sbd.getType())
				{
					TQuantity take = std::min(sbd.count - collected, i->second->count); //collect as much cres as we can
					changeStackCount(StackLocation(obj->id, i->first), -take, false);
					collected += take;
					foundSth = true;
					break;
				}
			}

			if (!foundSth) //we went through the whole loop and haven't found appropriate cres
			{
				complain("Unexpected failure during taking creatures!");
				return;
			}
		}
	}
}

void CGameHandler::heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)
{
	if (obj->visitingHero != hero && obj->garrisonHero != hero)
	{
		HeroVisitCastle vc;
		vc.hid = hero->id;
		vc.tid = obj->id;
		vc.flags |= 1;
		sendAndApply(vc);
	}
	visitCastleObjects(obj, hero);

	if (obj->visitingHero && obj->garrisonHero)
		useScholarSkill(obj->visitingHero->id, obj->garrisonHero->id);
	checkVictoryLossConditionsForPlayer(hero->tempOwner); //transported artifact?
}

void CGameHandler::visitCastleObjects(const CGTownInstance * t, const CGHeroInstance * h)
{
	std::vector<const CGHeroInstance * > visitors;
	visitors.push_back(h);
	visitCastleObjects(t, visitors);
}

void CGameHandler::visitCastleObjects(const CGTownInstance * t, std::vector<const CGHeroInstance * > visitors)
{
	std::vector<BuildingID> buildingsToVisit;
	for (auto const & hero : visitors)
		giveSpells (t, hero);

	for (auto & building : t->rewardableBuildings)
	{
		if (!t->getTown()->buildings.at(building.first)->manualHeroVisit && t->hasBuilt(building.first))
			buildingsToVisit.push_back(building.first);
	}

	if (!buildingsToVisit.empty())
	{
		auto visitQuery = std::make_shared<TownBuildingVisitQuery>(this, t, visitors, buildingsToVisit);
		queries->addQuery(visitQuery);
	}
}

void CGameHandler::stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero)
{
	HeroVisitCastle vc;
	vc.hid = hero->id;
	vc.tid = obj->id;
	sendAndApply(vc);
}

void CGameHandler::removeArtifact(const ArtifactLocation & al)
{
	removeArtifact(al.artHolder, {al.slot});
}

void CGameHandler::removeArtifact(const ObjectInstanceID & srcId, const std::vector<ArtifactPosition> & slotsPack)
{
	BulkEraseArtifacts ea;
	ea.artHolder = srcId;
	ea.posPack.insert(ea.posPack.end(), slotsPack.begin(), slotsPack.end());
	sendAndApply(ea);
}

void CGameHandler::changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells)
{
	ChangeSpells cs;
	cs.hid = hero->id;
	cs.spells = spells;
	cs.learn = give;
	sendAndApply(cs);
}

void CGameHandler::setResearchedSpells(const CGTownInstance * town, int level, const std::vector<SpellID> & spells, bool accepted)
{
	SetResearchedSpells cs;
	cs.tid = town->id;
	cs.spells = spells;
	cs.level = level;
	cs.accepted = accepted;
	sendAndApply(cs);
}

void CGameHandler::giveHeroBonus(GiveBonus * bonus)
{
	sendAndApply(*bonus);
}

void CGameHandler::setMovePoints(SetMovePoints * smp)
{
	sendAndApply(*smp);
}

void CGameHandler::setMovePoints(ObjectInstanceID hid, int val, bool absolute)
{
	SetMovePoints smp;
	smp.hid = hid;
	smp.val = val;
	smp.absolute = absolute;
	sendAndApply(smp);
}

void CGameHandler::setManaPoints(ObjectInstanceID hid, int val)
{
	SetMana sm;
	sm.hid = hid;
	sm.val = val;
	sm.absolute = true;
	sendAndApply(sm);
}

void CGameHandler::giveHero(ObjectInstanceID id, PlayerColor player, ObjectInstanceID boatId)
{
	GiveHero gh;
	gh.id = id;
	gh.player = player;
	gh.boatId = boatId;
	sendAndApply(gh);

	//Reveal fow around new hero, especially released from Prison
	auto h = getHero(id);
	changeFogOfWar(h->getSightCenter(), h->getSightRadius(), player, ETileVisibility::REVEALED);
}

void CGameHandler::changeObjPos(ObjectInstanceID objid, int3 newPos, const PlayerColor & initiator)
{
	ChangeObjPos cop;
	cop.objid = objid;
	cop.nPos = newPos;
	cop.initiator = initiator;
	sendAndApply(cop);
}

void CGameHandler::useScholarSkill(ObjectInstanceID fromHero, ObjectInstanceID toHero)
{
	const CGHeroInstance * h1 = getHero(fromHero);
	const CGHeroInstance * h2 = getHero(toHero);
	int h1_scholarSpellLevel = h1->valOfBonuses(BonusType::LEARN_MEETING_SPELL_LIMIT);
	int h2_scholarSpellLevel = h2->valOfBonuses(BonusType::LEARN_MEETING_SPELL_LIMIT);

	if (h1_scholarSpellLevel < h2_scholarSpellLevel)
	{
		std::swap (h1,h2);//1st hero need to have higher scholar level for correct message
		std::swap(fromHero, toHero);
	}

	int ScholarSpellLevel = std::max(h1_scholarSpellLevel, h2_scholarSpellLevel);//heroes can trade up to this level
	if (!ScholarSpellLevel || !h1->hasSpellbook() || !h2->hasSpellbook())
		return;//no scholar skill or no spellbook

	int h1Lvl = std::min(ScholarSpellLevel, h1->maxSpellLevel());//heroes can receive these levels
	int h2Lvl = std::min(ScholarSpellLevel, h2->maxSpellLevel());

	ChangeSpells cs1;
	cs1.learn = true;
	cs1.hid = toHero;//giving spells to first hero
	for (auto it : h1->getSpellsInSpellbook())
		if (h2Lvl >= it.toSpell()->getLevel() && !h2->spellbookContainsSpell(it))//hero can learn it and don't have it yet
			cs1.spells.insert(it);//spell to learn

	ChangeSpells cs2;
	cs2.learn = true;
	cs2.hid = fromHero;

	for (auto it : h2->getSpellsInSpellbook())
		if (h1Lvl >= it.toSpell()->getLevel() && !h1->spellbookContainsSpell(it))
			cs2.spells.insert(it);

	if (!cs1.spells.empty() || !cs2.spells.empty())//create a message
	{
		SecondarySkill scholarSkill = SecondarySkill::SCHOLAR;

		int scholarSkillLevel = std::max(h1->getSecSkillLevel(scholarSkill), h2->getSecSkillLevel(scholarSkill));
		InfoWindow iw;
		iw.player = h1->tempOwner;
		iw.components.emplace_back(ComponentType::SEC_SKILL, scholarSkill, scholarSkillLevel);

		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 139);//"%s, who has studied magic extensively,
		iw.text.replaceTextID(h1->getNameTextID());

		if (!cs2.spells.empty())//if found new spell - apply
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 140);//learns
			int size = static_cast<int>(cs2.spells.size());
			for (auto it : cs2.spells)
			{
				iw.components.emplace_back(ComponentType::SPELL, it);
				iw.text.appendName(it);
				switch (size--)
				{
					case 2:
						iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);
					case 1:
						break;
					default:
						iw.text.appendRawString(", ");
				}
			}
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 142);//from %s
			iw.text.replaceTextID(h2->getNameTextID());
			sendAndApply(cs2);
		}

		if (!cs1.spells.empty() && !cs2.spells.empty())
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);//and
		}

		if (!cs1.spells.empty())
		{
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 147);//teaches
			int size = static_cast<int>(cs1.spells.size());
			for (auto it : cs1.spells)
			{
				iw.components.emplace_back(ComponentType::SPELL, it);
				iw.text.appendName(it);
				switch (size--)
				{
					case 2:
						iw.text.appendLocalString(EMetaText::GENERAL_TXT, 141);
					case 1:
						break;
					default:
						iw.text.appendRawString(", ");
				}
			}
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 148);//from %s
			iw.text.replaceTextID(h2->getNameTextID());
			sendAndApply(cs1);
		}
		sendAndApply(iw);
	}
}

void CGameHandler::heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)
{
	auto h1 = getHero(hero1);
	auto h2 = getHero(hero2);

	if (getPlayerRelations(h1->getOwner(), h2->getOwner()) != PlayerRelations::ENEMIES)
	{
		auto exchange = std::make_shared<CGarrisonDialogQuery>(this, h1, h2);
		ExchangeDialog hex;
		hex.queryID = exchange->queryID;
		hex.player = h1->getOwner();
		hex.hero1 = hero1;
		hex.hero2 = hero2;
		sendAndApply(hex);

		useScholarSkill(hero1,hero2);
		queries->addQuery(exchange);
	}
}

void CGameHandler::sendToAllClients(CPackForClient & pack)
{
	logNetwork->trace("\tSending to all clients: %s", typeid(pack).name());
	for (auto c : gameLobby().activeConnections)
		c->sendPack(pack);
}

void CGameHandler::sendAndApply(CPackForClient & pack)
{
	sendToAllClients(pack);
	gs->apply(pack);
	logNetwork->trace("\tApplied on gs: %s", typeid(pack).name());
}

void CGameHandler::sendAndApply(CGarrisonOperationPack & pack)
{
	sendAndApply(static_cast<CPackForClient &>(pack));
	checkVictoryLossConditionsForAll();
}

void CGameHandler::sendAndApply(SetResources & pack)
{
	sendAndApply(static_cast<CPackForClient &>(pack));
	checkVictoryLossConditionsForPlayer(pack.player);
}

void CGameHandler::sendAndApply(NewStructures & pack)
{
	sendAndApply(static_cast<CPackForClient &>(pack));
	checkVictoryLossConditionsForPlayer(getTown(pack.tid)->tempOwner);
}

bool CGameHandler::isPlayerOwns(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack, ObjectInstanceID id)
{
	return pack->player == getOwner(id) && hasPlayerAt(getOwner(id), connection);
}

void CGameHandler::throwNotAllowedAction(const std::shared_ptr<CConnection> & connection)
{
	playerMessages->sendSystemMessage(connection, MetaString::createFromTextID("vcmi.server.errors.notAllowed"));

	logNetwork->error("Player is not allowed to perform this action!");
	throw ExceptionNotAllowedAction();
}

void CGameHandler::wrongPlayerMessage(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack, PlayerColor expectedplayer)
{
	auto str = MetaString::createFromTextID("vcmi.server.errors.wrongIdentified");
	str.appendName(pack->player);
	str.appendName(expectedplayer);
	logNetwork->error(str.toString());

	playerMessages->sendSystemMessage(connection, str);
}

void CGameHandler::throwIfWrongOwner(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack, ObjectInstanceID id)
{
	if(!isPlayerOwns(connection, pack, id))
	{
		wrongPlayerMessage(connection, pack, getOwner(id));
		throwNotAllowedAction(connection);
	}
}

void CGameHandler::throwIfPlayerNotActive(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack)
{
	if (!turnOrder->isPlayerMakingTurn(pack->player))
		throwNotAllowedAction(connection);
}

void CGameHandler::throwIfWrongPlayer(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack)
{
	throwIfWrongPlayer(connection, pack, pack->player);
}

void CGameHandler::throwIfWrongPlayer(const std::shared_ptr<CConnection> & connection, const CPackForServer * pack, PlayerColor player)
{
	if(!hasPlayerAt(player, connection) || pack->player != player)
	{
		wrongPlayerMessage(connection, pack, player);
		throwNotAllowedAction(connection);
	}
}

void CGameHandler::throwAndComplain(const std::shared_ptr<CConnection> & connection, std::string txt)
{
	complain(txt);
	throwNotAllowedAction(connection);
}

void CGameHandler::save(const std::string & filename)
{
	logGlobal->info("Saving to %s", filename);
	const auto stem	= FileInfo::GetPathStem(filename);
	const auto savefname = stem.to_string() + ".vsgm1";
	ResourcePath savePath(stem.to_string(), EResType::SAVEGAME);
	CResourceHandler::get("local")->createResource(savefname);

	try
	{
		{
			CSaveFile save(*CResourceHandler::get("local")->getResourceName(savePath));
			saveCommonState(save);
			logGlobal->info("Saving server state");
			save << *this;
		}
		logGlobal->info("Game has been successfully saved!");
	}
	catch(std::exception &e)
	{
		logGlobal->error("Failed to save game: %s", e.what());
	}
}

bool CGameHandler::load(const std::string & filename)
{
	logGlobal->info("Loading from %s", filename);
	const auto stem	= FileInfo::GetPathStem(filename);

	reinitScripting();

	try
	{
		{
			CLoadFile lf(*CResourceHandler::get()->getResourceName(ResourcePath(stem.to_string(), EResType::SAVEGAME)), ESerializationVersion::MINIMAL);
			lf.serializer.cb = this;
			loadCommonState(lf);
			logGlobal->info("Loading server state");
			lf >> *this;
		}
		logGlobal->info("Game has been successfully loaded!");
	}
	catch(const ModIncompatibility & e)
	{
		logGlobal->error("Failed to load game: %s", e.what());
		MetaString errorMsg;
		if(!e.whatMissing().empty())
		{
			errorMsg.appendTextID("vcmi.server.errors.modsToEnable");
			errorMsg.appendRawString("\n");
			errorMsg.appendRawString(e.whatMissing());
		}
		if(!e.whatExcessive().empty())
		{
			errorMsg.appendTextID("vcmi.server.errors.modsToDisable");
			errorMsg.appendRawString("\n");
			errorMsg.appendRawString(e.whatExcessive());
		}
		gameLobby().announceMessage(errorMsg);
		return false;
	}
	catch(const IdentifierResolutionException & e)
	{
		logGlobal->error("Failed to load game: %s", e.what());
		MetaString errorMsg;
		errorMsg.appendTextID("vcmi.server.errors.unknownEntity");
		errorMsg.replaceRawString(e.identifierName);
		gameLobby().announceMessage(errorMsg);
		return false;
	}

	catch(const std::exception & e)
	{
		logGlobal->error("Failed to load game: %s", e.what());
		auto str = MetaString::createFromTextID("vcmi.broadcast.failedLoadGame");
		str.appendRawString(": ");
		str.appendRawString(e.what());
		gameLobby().announceMessage(str);
		return false;
	}
	gs->preInit(LIBRARY, this);
	gs->updateOnLoad(gameLobby().si.get());
	return true;
}

bool CGameHandler::bulkSplitStack(SlotID slotSrc, ObjectInstanceID srcOwner, si32 howMany)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if((!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		|| (howMany < 1 && complain("Invalid split parameter!")))
	{
		return false;
	}
	auto actualAmount = army->getStackCount(slotSrc);

	if(actualAmount <= howMany && complain(complainNotEnoughCreatures)) // '<=' because it's not intended just for moving a stack
		return false;

	auto freeSlots = creatureSet.getFreeSlots();

	if(freeSlots.empty() && complain("No empty stacks"))
		return false;

	BulkRebalanceStacks bulkRS;

	for(auto slot : freeSlots)
	{
		RebalanceStacks rs;
		rs.srcArmy = army->id;
		rs.dstArmy = army->id;
		rs.srcSlot = slotSrc;
		rs.dstSlot = slot;
		rs.count = howMany;

		bulkRS.moves.push_back(rs);
		actualAmount -= howMany;

		if(actualAmount <= howMany)
			break;
	}
	sendAndApply(bulkRS);
	return true;
}

bool CGameHandler::bulkMergeStacks(SlotID slotSrc, ObjectInstanceID srcOwner)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if(!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		return false;

	auto actualAmount = creatureSet.getStackCount(slotSrc);

	if(actualAmount < 1 && complain(complainNoCreatures))
		return false;

	auto currentCreature = creatureSet.getCreature(slotSrc);

	if(!currentCreature && complain(complainNoCreatures))
		return false;

	auto creatureSlots = creatureSet.getCreatureSlots(currentCreature, slotSrc);

	if(!creatureSlots.size())
		return false;

	BulkRebalanceStacks bulkRS;

	for(auto slot : creatureSlots)
	{
		RebalanceStacks rs;
		rs.srcArmy = army->id;
		rs.dstArmy = army->id;
		rs.srcSlot = slot;
		rs.dstSlot = slotSrc;
		rs.count = creatureSet.getStackCount(slot);
		bulkRS.moves.push_back(rs);
	}
	sendAndApply(bulkRS);
	return true;
}

bool CGameHandler::bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot)
{
	if(!srcSlot.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * armySrc = static_cast<const CArmedInstance*>(getObjInstance(srcArmy));
	const CCreatureSet & setSrc = *armySrc;

	if(!vstd::contains(setSrc.stacks, srcSlot) && complain(complainNoCreatures))
		return false;

	const CArmedInstance * armyDest = static_cast<const CArmedInstance*>(getObjInstance(destArmy));
	const CCreatureSet & setDest = *armyDest;
	auto freeSlots = setDest.getFreeSlotsQueue();

	typedef std::map<SlotID, std::pair<SlotID, TQuantity>> TRebalanceMap;
	TRebalanceMap moves;

	auto srcQueue = setSrc.getCreatureQueue(srcSlot); // Exclude srcSlot, it should be moved last
	auto slotsLeft = setSrc.stacksCount();
	auto destMap = setDest.getCreatureMap();
	TMapCreatureSlot::key_compare keyComp = destMap.key_comp();

	while(!srcQueue.empty())
	{
		auto pair = srcQueue.top();
		srcQueue.pop();

		auto currCreature = pair.first;
		auto currSlot = pair.second;
		const auto quantity = setSrc.getStackCount(currSlot);

		TMapCreatureSlot::iterator lb = destMap.lower_bound(currCreature);
		const bool alreadyExists = (lb != destMap.end() && !(keyComp(currCreature, lb->first)));

		if(!alreadyExists)
		{
			if(freeSlots.empty())
				continue;

			auto currFreeSlot = freeSlots.front();
			freeSlots.pop();
			destMap.insert(lb, TMapCreatureSlot::value_type(currCreature, currFreeSlot));
		}
		moves.insert(std::make_pair(currSlot, std::make_pair(destMap[currCreature], quantity)));
		slotsLeft--;
	}
	if(slotsLeft == 1)
	{
		auto lastCreature = setSrc.getCreature(srcSlot);
		auto slotToMove = SlotID();
		// Try to find a slot for last creature
		if(destMap.find(lastCreature) == destMap.end())
		{
			if(!freeSlots.empty())
				slotToMove = freeSlots.front();
		}
		else
		{
			slotToMove = destMap[lastCreature];
		}

		if(slotToMove != SlotID())
		{
			const bool needsLastStack = armySrc->needsLastStack();
			const auto quantity = setSrc.getStackCount(srcSlot) - (needsLastStack ? 1 : 0);

			if(quantity > 0) //0 may happen when we need last creature and we have exactly 1 amount of that creature - amount of "rest we can transfer" becomes 0
				moves.insert(std::make_pair(srcSlot, std::make_pair(slotToMove, quantity)));
		}
	}
	BulkRebalanceStacks bulkRS;

	for(auto & move : moves)
	{
		RebalanceStacks rs;
		rs.srcArmy = armySrc->id;
		rs.dstArmy = armyDest->id;
		rs.srcSlot = move.first;
		rs.dstSlot = move.second.first;
		rs.count = move.second.second;
		bulkRS.moves.push_back(rs);
	}
	sendAndApply(bulkRS);
	return true;
}

bool CGameHandler::bulkSmartSplitStack(SlotID slotSrc, ObjectInstanceID srcOwner)
{
	if(!slotSrc.validSlot() && complain(complainInvalidSlot))
		return false;

	const CArmedInstance * army = static_cast<const CArmedInstance*>(getObjInstance(srcOwner));
	const CCreatureSet & creatureSet = *army;

	if(!vstd::contains(creatureSet.stacks, slotSrc) && complain(complainNoCreatures))
		return false;

	auto actualAmount = creatureSet.getStackCount(slotSrc);

	if(actualAmount <= 1 && complain(complainNoCreatures))
		return false;

	auto freeSlot = creatureSet.getFreeSlot();
	auto currentCreature = creatureSet.getCreature(slotSrc);

	if(freeSlot == SlotID() && creatureSet.isCreatureBalanced(currentCreature))
		return true;

	auto creatureSlots = creatureSet.getCreatureSlots(currentCreature, SlotID(-1), 1); // Ignore slots where's only 1 creature, don't ignore slotSrc
	TQuantity totalCreatures = 0;

	for(auto slot : creatureSlots)
		totalCreatures += creatureSet.getStackCount(slot);

	if(totalCreatures <= 1 && complain("Total creatures number is invalid"))
		return false;

	if(freeSlot != SlotID())
		creatureSlots.push_back(freeSlot);

	if(creatureSlots.empty() && complain("No available slots for smart rebalancing"))
		return false;

	const auto totalCreatureSlots = creatureSlots.size();
	const auto rem = totalCreatures % totalCreatureSlots;
	const auto quotient = totalCreatures / totalCreatureSlots;

	// totalCreatures == rem * (quotient + 1) + (totalCreatureSlots - rem) * quotient;
	// Proof: r(q+1)+(s-r)q = rq+r+qs-rq = r+qs = total, where total/s = q+r/s

	BulkSmartRebalanceStacks bulkSRS;

	if(freeSlot != SlotID())
	{
		RebalanceStacks rs;
		rs.srcArmy = rs.dstArmy = army->id;
		rs.srcSlot = slotSrc;
		rs.dstSlot = freeSlot;
		rs.count = 1;
		bulkSRS.moves.push_back(rs);
	}
	auto currSlot = 0;
	auto check = 0;

	for(auto slot : creatureSlots)
	{
		ChangeStackCount csc;

		csc.army = army->id;
		csc.slot = slot;
		csc.count = (currSlot < rem)
			? quotient + 1
			: quotient;
		csc.absoluteValue = true;
		bulkSRS.changes.push_back(csc);
		currSlot++;
		check += csc.count;
	}

	if(check != totalCreatures)
	{
		complain((boost::format("Failure: totalCreatures=%d but check=%d") % totalCreatures % check).str());
		return false;
	}
	sendAndApply(bulkSRS);
	return true;
}

bool CGameHandler::arrangeStacks(ObjectInstanceID id1, ObjectInstanceID id2, ui8 what, SlotID p1, SlotID p2, si32 val, PlayerColor player)
{
	const CArmedInstance * s1 = static_cast<const CArmedInstance *>(getObj(id1));
	const CArmedInstance * s2 = static_cast<const CArmedInstance *>(getObj(id2));

	if (s1 == nullptr || s2 == nullptr)
	{
		complain("Cannot exchange stacks between non-existing objects!!\n");
		return false;
	}

	const CCreatureSet & S1 = *s1;
	const CCreatureSet & S2 = *s2;
	StackLocation sl1(s1->id, p1);
	StackLocation sl2(s2->id, p2);

	if (!sl1.slot.validSlot()  ||  !sl2.slot.validSlot())
	{
		complain(complainInvalidSlot);
		return false;
	}

	if (!isAllowedExchange(id1,id2))
	{
		complain("Cannot exchange stacks between these two objects!\n");
		return false;
	}

	// We can always put stacks into locked garrison, but not take them out of it
	auto notRemovable = [&](const CArmedInstance * army)
	{
		if (id1 != id2) // Stack arrangement inside locked garrison is allowed
		{
			auto g = dynamic_cast<const CGGarrison *>(army);
			if (g && !g->removableUnits)
			{
				complain("Stacks in this garrison are not removable!\n");
				return true;
			}
		}
		return false;
	};

	if (what==1) //swap
	{
		if (((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s1->getStackCount(p1))
		  || ((s2->tempOwner != player && s2->tempOwner != PlayerColor::UNFLAGGABLE) && s2->getStackCount(p2)))
		{
			complain("Can't take troops from another player!");
			return false;
		}

		if (sl1.army == sl2.army && sl1.slot == sl2.slot)
		{
			complain("Cannot swap stacks - slots are the same!");
			return false;
		}

		if (!s1->slotEmpty(p1) && !s2->slotEmpty(p2))
		{
			if (notRemovable(s1) || notRemovable(s2))
				return false;
		}
		if (s1->slotEmpty(p1) && notRemovable(s2))
			return false;
		else if (s2->slotEmpty(p2) && notRemovable(s1))
			return false;

		swapStacks(sl1, sl2);
	}
	else if (what==2)//merge
	{
		if ((s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot merge different creatures stacks!"))
		|| (((s1->tempOwner != player && s1->tempOwner != PlayerColor::UNFLAGGABLE) && s2->getStackCount(p2)) && complain("Can't take troops from another player!")))
			return false;

		if (s1->slotEmpty(p1) || s2->slotEmpty(p2))
		{
			complain("Cannot merge empty stack!");
			return false;
		}
		else if (notRemovable(s1))
			return false;

		moveStack(sl1, sl2);
	}
	else if (what==3) //split
	{
		const int countToMove = val - s2->getStackCount(p2);
		const int countLeftOnSrc = s1->getStackCount(p1) - countToMove;

		if (  (s1->tempOwner != player && countLeftOnSrc < s1->getStackCount(p1))
			|| (s2->tempOwner != player && val < s2->getStackCount(p2)))
		{
			complain("Can't move troops of another player!");
			return false;
		}

		//general conditions checking
		if ((!vstd::contains(S1.stacks,p1) && complain(complainNoCreatures))
			|| (val<1  && complain(complainNoCreatures)) )
		{
			return false;
		}


		if (vstd::contains(S2.stacks,p2))	 //dest. slot not free - it must be "rebalancing"...
		{
			int total = s1->getStackCount(p1) + s2->getStackCount(p2);
			if ((total < val   &&   complain("Cannot split that stack, not enough creatures!"))
				|| (s1->getCreature(p1) != s2->getCreature(p2) && complain("Cannot rebalance different creatures stacks!"))
			)
			{
				return false;
			}

			if (notRemovable(s1))
			{
				if (s1->getStackCount(p1) > countLeftOnSrc)
					return false;
			}
			else if (notRemovable(s2))
			{
				if (s2->getStackCount(p1) < countLeftOnSrc)
					return false;
			}

			moveStack(sl1, sl2, countToMove);
			//S2.slots[p2]->count = val;
			//S1.slots[p1]->count = total - val;
		}
		else //split one stack to the two
		{
			if (s1->getStackCount(p1) < val)//not enough creatures
			{
				complain(complainNotEnoughCreatures);
				return false;
			}

			if (notRemovable(s1))
				return false;

			moveStack(sl1, sl2, val);
		}

	}
	return true;
}

bool CGameHandler::hasPlayerAt(PlayerColor player, std::shared_ptr<CConnection> c) const
{
	return connections.count(player) && connections.at(player).count(c);
}

bool CGameHandler::hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const
{
	return connections.count(left) && connections.count(right) && connections.at(left) == connections.at(right);
}

bool CGameHandler::disbandCreature(ObjectInstanceID id, SlotID pos)
{
	const CArmedInstance * s1 = static_cast<const CArmedInstance *>(getObjInstance(id));
	if (!vstd::contains(s1->stacks,pos))
	{
		complain("Illegal call to disbandCreature - no such stack in army!");
		return false;
	}

	eraseStack(StackLocation(s1->id, pos));
	return true;
}

bool CGameHandler::buildStructure(ObjectInstanceID tid, BuildingID requestedID, bool force)
{
	const CGTownInstance * t = getTown(tid);
	if(!t)
		COMPLAIN_RETF("No such town (ID=%s)!", tid);
	if(!t->getTown()->buildings.count(requestedID))
		COMPLAIN_RETF("Town of faction %s does not have info about building ID=%s!", t->getFaction()->getNameTranslated() % requestedID);
	if(t->hasBuilt(requestedID))
		COMPLAIN_RETF("Building %s is already built in %s", t->getTown()->buildings.at(requestedID)->getNameTranslated() % t->getNameTranslated());

	const auto & requestedBuilding = t->getTown()->buildings.at(requestedID);

	//Vector with future list of built building and buildings in auto-mode that are not yet built.
	std::vector<const CBuilding*> remainingAutoBuildings;
	std::set<BuildingID> buildingsThatWillBe;

	//Check validity of request
	if(!force)
	{
		switch(requestedBuilding->mode)
		{
		case CBuilding::BUILD_NORMAL :
			if (canBuildStructure(t, requestedID) != EBuildingState::ALLOWED)
				COMPLAIN_RET("Cannot build that building!");
			break;

		case CBuilding::BUILD_AUTO   :
		case CBuilding::BUILD_SPECIAL:
			COMPLAIN_RET("This building can not be constructed normally!");

		case CBuilding::BUILD_GRAIL  :
			if(requestedBuilding->mode == CBuilding::BUILD_GRAIL) //needs grail
			{
				if(!t->visitingHero || !t->visitingHero->hasArt(ArtifactID::GRAIL))
					COMPLAIN_RET("Cannot build this without grail!")
				else
					removeArtifact(ArtifactLocation(t->visitingHero->id, t->visitingHero->getArtPos(ArtifactID::GRAIL, false)));
			}
			break;
		}
	}

	//Performs stuff that has to be done before new building is built
	auto processBeforeBuiltStructure = [t, this](const BuildingID buildingID)
	{
		if(buildingID.isDwelling())
		{
			int level = BuildingID::getLevelFromDwelling(buildingID);
			int upgradeNumber = BuildingID::getUpgradedFromDwelling(buildingID);

			if(upgradeNumber >= t->getTown()->creatures.at(level).size())
			{
				complain(boost::str(boost::format("Error encountered when building dwelling (bid=%s):"
													"no creature found (upgrade number %d, level %d!")
												% buildingID % upgradeNumber % level));
				return;
			}

			const CCreature * crea = t->getTown()->creatures.at(level).at(upgradeNumber).toCreature();

			SetAvailableCreatures ssi;
			ssi.tid = t->id;
			ssi.creatures = t->creatures;
			if (ssi.creatures[level].second.empty()) // first creature in a dwelling
				ssi.creatures[level].first = crea->getGrowth();
			ssi.creatures[level].second.push_back(crea->getId());
			sendAndApply(ssi);
		}
		if(t->getTown()->buildings.at(buildingID)->subId == BuildingSubID::PORTAL_OF_SUMMONING)
		{
			setPortalDwelling(t);
		}
	};

	//Performs stuff that has to be done after new building is built
	auto processAfterBuiltStructure = [t, this](const BuildingID buildingID)
	{
		auto isMageGuild = (buildingID <= BuildingID::MAGES_GUILD_5 && buildingID >= BuildingID::MAGES_GUILD_1);
		auto isLibrary = isMageGuild ? false
			: t->getTown()->buildings.at(buildingID)->subId == BuildingSubID::EBuildingSubID::LIBRARY;

		if(isMageGuild || isLibrary || (t->getFactionID() == ETownType::CONFLUX && buildingID == BuildingID::GRAIL))
		{
			if(t->visitingHero)
				giveSpells(t,t->visitingHero);
			if(t->garrisonHero)
				giveSpells(t,t->garrisonHero);
		}
	};

	//Checks if all requirements will be met with expected building list "buildingsThatWillBe"
	auto areRequirementsFulfilled = [&](const BuildingID & buildID)
	{
		return buildingsThatWillBe.count(buildID);
	};

	//Init the vectors
	for(auto & build : t->getTown()->buildings)
	{
		if(t->hasBuilt(build.first))
		{
			buildingsThatWillBe.insert(build.first);
		}
		else
		{
			if(build.second->mode == CBuilding::BUILD_AUTO) //not built auto building
				remainingAutoBuildings.push_back(build.second.get());
		}
	}

	//Prepare structure (list of building ids will be filled later)
	NewStructures ns;
	ns.tid = tid;
	ns.built = force ? t->built : (t->built+1);

	std::queue<const CBuilding*> buildingsToAdd;
	buildingsToAdd.push(requestedBuilding.get());

	while(!buildingsToAdd.empty())
	{
		auto b = buildingsToAdd.front();
		buildingsToAdd.pop();

		ns.bid.insert(b->bid);
		buildingsThatWillBe.insert(b->bid);
		remainingAutoBuildings -= b;

		for(auto autoBuilding : remainingAutoBuildings)
		{
			auto actualRequirements = t->genBuildingRequirements(autoBuilding->bid);

			if(actualRequirements.test(areRequirementsFulfilled))
				buildingsToAdd.push(autoBuilding);
		}
	}

	// FIXME: it's done before NewStructures applied because otherwise town window wont be properly updated on client. That should be actually fixed on client and not on server.
	for(auto builtID : ns.bid)
		processBeforeBuiltStructure(builtID);

	//Take cost
	if(!force)
	{
		giveResources(t->tempOwner, -requestedBuilding->resources);
		gs->statistic.accumulatedValues[t->tempOwner].spentResourcesForBuildings += requestedBuilding->resources;
	}

	//We know what has been built, apply changes. Do this as final step to properly update town window
	sendAndApply(ns);

	//Other post-built events. To some logic like giving spells to work gamestate changes for new building must be already in place!
	for(auto builtID : ns.bid)
		processAfterBuiltStructure(builtID);

	// now when everything is built - reveal tiles for lookout tower
	changeFogOfWar(t->getSightCenter(), t->getSightRadius(), t->getOwner(), ETileVisibility::REVEALED);

	if (!force)
	{
		//garrison hero first - consistent with original H3 Mana Vortex and Battle Scholar Academy levelup windows order
		std::vector<const CGHeroInstance *> visitors;
		if (t->garrisonHero)
			visitors.push_back(t->garrisonHero);
		if (t->visitingHero)
			visitors.push_back(t->visitingHero);

		if (!visitors.empty())
			visitCastleObjects(t, visitors);
	}

	checkVictoryLossConditionsForPlayer(t->tempOwner);
	return true;
}

bool CGameHandler::visitTownBuilding(ObjectInstanceID tid, BuildingID bid)
{
	const CGTownInstance * t = getTown(tid);

	if(!t->hasBuilt(bid))
		return false;

	auto subID = t->getTown()->buildings.at(bid)->subId;

	if(subID == BuildingSubID::EBuildingSubID::BANK)
	{
		TResources res;
		res[EGameResID::GOLD] = 2500;
		giveResources(t->getOwner(), res);

		setObjPropertyValue(t->id, ObjProperty::BONUS_VALUE_SECOND, 2500);
		return true;
	}

	if (t->rewardableBuildings.count(bid) && t->visitingHero && t->getTown()->buildings.at(bid)->manualHeroVisit)
	{
		std::vector<BuildingID> buildingsToVisit;
		std::vector<const CGHeroInstance*> visitors;
		buildingsToVisit.push_back(bid);
		visitors.push_back(t->visitingHero);
		auto visitQuery = std::make_shared<TownBuildingVisitQuery>(this, t, visitors, buildingsToVisit);
		queries->addQuery(visitQuery);
		return true;
	}

	return true;
}

bool CGameHandler::razeStructure (ObjectInstanceID tid, BuildingID bid)
{
///incomplete, simply erases target building
	const CGTownInstance * t = getTown(tid);
	if(!t->hasBuilt(bid))
		return false;
	RazeStructures rs;
	rs.tid = tid;
	rs.bid.insert(bid);
	rs.destroyed = t->destroyed + 1;
	sendAndApply(rs);
//TODO: Remove dwellers
// 	if (t->subID == 4 && bid == 17) //Veil of Darkness
// 	{
// 		RemoveBonus rb(RemoveBonus::TOWN);
// 		rb.whoID = t->id;
// 		rb.source = BonusSource::TOWN_STRUCTURE;
// 		rb.id = 17;
// 		sendAndApply(rb);
// 	}
	return true;
}

bool CGameHandler::spellResearch(ObjectInstanceID tid, SpellID spellAtSlot, bool accepted)
{
	CGTownInstance *t = gs->getTown(tid);

	if(!getSettings().getBoolean(EGameSettings::TOWNS_SPELL_RESEARCH) && complain("Spell research not allowed!"))
		return false;
	if (!t->spellResearchAllowed && complain("Spell research not allowed in this town!"))
		return false;

	int level = -1;
	for(int i = 0; i < t->spells.size(); i++)
		if(vstd::find_pos(t->spells[i], spellAtSlot) != -1)
			level = i;
	
	if(level == -1 && complain("Spell for replacement not found!"))
		return false;

	auto spells = t->spells.at(level);
	
	bool researchLimitExceeded = t->spellResearchCounterDay >= getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_PER_DAY).Vector()[level].Float();
	if(researchLimitExceeded && complain("Already researched today!"))
		return false;

	if(!accepted)
	{
		auto it = spells.begin() + t->spellsAtLevel(level, false);
		std::rotate(it, it + 1, spells.end()); // move to end
		setResearchedSpells(t, level, spells, accepted);
		return true;
	}

	auto costBase = TResources(getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_COST).Vector()[level]);
	auto costExponent = getSettings().getValue(EGameSettings::TOWNS_SPELL_RESEARCH_COST_EXPONENT_PER_RESEARCH).Vector()[level].Float();
	auto cost = costBase * std::pow(t->spellResearchAcceptedCounter + 1, costExponent);

	if(!getPlayerState(t->getOwner())->resources.canAfford(cost) && complain("Spell replacement cannot be afforded!"))
		return false;

	giveResources(t->getOwner(), -cost);

	std::swap(spells.at(t->spellsAtLevel(level, false)), spells.at(vstd::find_pos(spells, spellAtSlot)));
	auto it = spells.begin() + t->spellsAtLevel(level, false);
	std::rotate(it, it + 1, spells.end()); // move to end

	setResearchedSpells(t, level, spells, accepted);

	if(t->visitingHero)
		giveSpells(t, t->visitingHero);
	if(t->garrisonHero)
		giveSpells(t, t->garrisonHero);

	return true;
}

bool CGameHandler::recruitCreatures(ObjectInstanceID objid, ObjectInstanceID dstid, CreatureID crid, ui32 cram, si32 fromLvl, PlayerColor player)
{
	const CGDwelling * dwelling = dynamic_cast<const CGDwelling *>(getObj(objid));
	const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(getObj(objid));
	const CArmedInstance * army = dynamic_cast<const CArmedInstance *>(getObj(dstid));
	const CGHeroInstance * hero = dynamic_cast<const CGHeroInstance *>(getObj(dstid));
	const CCreature * c = crid.toCreature();

	const bool warMachine = c->warMachine != ArtifactID::NONE;

	//TODO: check if hero is actually visiting object

	COMPLAIN_RET_FALSE_IF(!dwelling || !army, "Cannot recruit: invalid object!");
	COMPLAIN_RET_FALSE_IF(dwelling->getOwner() != player && dwelling->getOwner() != PlayerColor::UNFLAGGABLE, "Cannot recruit: dwelling not owned!");

	if (town)
	{
		COMPLAIN_RET_FALSE_IF(town != army && !hero, "Cannot recruit: invalid destination!");
		COMPLAIN_RET_FALSE_IF(hero != town->garrisonHero && hero != town->visitingHero, "Cannot recruit: can only recruit to town or hero in town!!");
	}
	else
	{
		COMPLAIN_RET_FALSE_IF(getVisitingHero(dwelling) != hero, "Cannot recruit: can only recruit by visiting hero!");
		COMPLAIN_RET_FALSE_IF(!hero || hero->getOwner() != player, "Cannot recruit: can only recruit to owned hero!");
	}

	//verify
	bool found = false;
	int level = 0;

	for (; level < dwelling->creatures.size(); level++) //iterate through all levels
	{
		if ((fromLvl != -1) && (level !=fromLvl))
			continue;
		const auto &cur = dwelling->creatures.at(level); //current level info <amount, list of cr. ids>
		int i = 0;
		for (; i < cur.second.size(); i++) //look for crid among available creatures list on current level
			if (cur.second.at(i) == crid)
				break;

		if (i < cur.second.size())
		{
			found = true;
			cram = std::min(cram, cur.first); //reduce recruited amount up to available amount
			break;
		}
	}
	SlotID slot = army->getSlotFor(crid);

	if ((!found && complain("Cannot recruit: no such creatures!"))
		|| ((si32)cram  >  LIBRARY->creh->objects.at(crid)->maxAmount(getPlayerState(army->tempOwner)->resources) && complain("Cannot recruit: lack of resources!"))
		|| (cram<=0  &&  complain("Cannot recruit: cram <= 0!"))
		|| (!slot.validSlot()  && !warMachine && complain("Cannot recruit: no available slot!")))
	{
		return false;
	}

	//recruit
	TResources cost = (c->getFullRecruitCost() * cram);
	giveResources(army->tempOwner, -cost);
	gs->statistic.accumulatedValues[army->tempOwner].spentResourcesForArmy += cost;

	SetAvailableCreatures sac;
	sac.tid = objid;
	sac.creatures = dwelling->creatures;
	sac.creatures[level].first -= cram;
	sendAndApply(sac);

	if (warMachine)
	{
		ArtifactID artId = c->warMachine;
		const CArtifact * art = artId.toArtifact();

		COMPLAIN_RET_FALSE_IF(!hero, "Only hero can buy war machines");
		COMPLAIN_RET_FALSE_IF(artId == ArtifactID::CATAPULT, "Catapult cannot be recruited!");
		COMPLAIN_RET_FALSE_IF(nullptr == art, "Invalid war machine artifact");
		COMPLAIN_RET_FALSE_IF(hero->hasArt(artId),"Hero already has this machine!");

		bool hasFreeSlot = false;
		for(auto slot : art->getPossibleSlots().at(ArtBearer::HERO))
			if (hero->getArt(slot) == nullptr)
				hasFreeSlot = true;

		if (!hasFreeSlot)
		{
			auto slot = art->getPossibleSlots().at(ArtBearer::HERO).front();
			removeArtifact(ArtifactLocation(hero->id, slot));
		}
		return giveHeroNewArtifact(hero, artId, ArtifactPosition::FIRST_AVAILABLE);
	}
	else
	{
		addToSlot(StackLocation(army->id, slot), c, cram);
	}
	return true;
}

bool CGameHandler::upgradeCreature(ObjectInstanceID objid, SlotID pos, CreatureID upgID)
{
	const CArmedInstance * obj = static_cast<const CArmedInstance *>(getObjInstance(objid));
	if (!obj->hasStackAtSlot(pos))
	{
		COMPLAIN_RET("Cannot upgrade, no stack at slot " + std::to_string(pos));
	}
	UpgradeInfo upgradeInfo(obj->getStackPtr(pos)->getId());
	fillUpgradeInfo(obj, pos, upgradeInfo);
	PlayerColor player = obj->tempOwner;
	const PlayerState *p = getPlayerState(player);
	int crQuantity = obj->stacks.at(pos)->count;

	//check if upgrade is possible
	if (!upgradeInfo.hasUpgrades() && complain("That upgrade is not possible!"))
	{
		return false;
	}
	TResources totalCost = upgradeInfo.getUpgradeCostsFor(upgID) * crQuantity;

	//check if player has enough resources
	if (!p->resources.canAfford(totalCost))
		COMPLAIN_RET("Cannot upgrade, not enough resources!");

	//take resources
	giveResources(player, -totalCost);
	gs->statistic.accumulatedValues[player].spentResourcesForArmy += totalCost;

	//upgrade creature
	changeStackType(StackLocation(obj->id, pos), upgID.toCreature());
	return true;
}

bool CGameHandler::changeStackType(const StackLocation &sl, const CCreature *c)
{
	const CArmedInstance * obj = static_cast<const CArmedInstance *>(getObjInstance(sl.army));

	if (!obj->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to change type");

	SetStackType sst;
	sst.army = obj->id;
	sst.slot = sl.slot;
	sst.type = c->getId();
	sendAndApply(sst);
	return true;
}

void CGameHandler::moveArmy(const CArmedInstance *src, const CArmedInstance *dst, bool allowMerging)
{
	assert(src->canBeMergedWith(*dst, allowMerging));
	while(src->stacksCount())//while there are unmoved creatures
	{
		auto i = src->Slots().begin(); //iterator to stack to move
		StackLocation sl(src->id, i->first); //location of stack to move

		SlotID pos = dst->getSlotFor(i->second->getCreature());
		if (!pos.validSlot())
		{
			//try to merge two other stacks to make place
			std::pair<SlotID, SlotID> toMerge;
			if (dst->mergeableStacks(toMerge, i->first) && allowMerging)
			{
				moveStack(StackLocation(dst->id, toMerge.first), StackLocation(dst->id, toMerge.second)); //merge toMerge.first into toMerge.second
				assert(!dst->hasStackAtSlot(toMerge.first)); //we have now a new free slot
				moveStack(sl, StackLocation(dst->id, toMerge.first)); //move stack to freed slot
			}
			else
			{
				complain("Unexpected failure during an attempt to move army from " + src->nodeName() + " to " + dst->nodeName() + "!");
				return;
			}
		}
		else
		{
			moveStack(sl, StackLocation(dst->id, pos));
		}
	}
}

bool CGameHandler::swapGarrisonOnSiege(ObjectInstanceID tid)
{
	const CGTownInstance * town = getTown(tid);

	if(!town->garrisonHero == !town->visitingHero)
		return false;

	SetHeroesInTown intown;
	intown.tid = tid;

	if(town->garrisonHero) //garrison -> vising
	{
		intown.garrison = ObjectInstanceID();
		intown.visiting = town->garrisonHero->id;
	}
	else //visiting -> garrison
	{
		if(town->armedGarrison())
			town->mergeGarrisonOnSiege();

		intown.visiting = ObjectInstanceID();
		intown.garrison = town->visitingHero->id;
	}
	sendAndApply(intown);
	return true;
}

bool CGameHandler::garrisonSwap(ObjectInstanceID tid)
{
	const CGTownInstance * town = getTown(tid);
	if (!town->garrisonHero && town->visitingHero) //visiting => garrison, merge armies: town army => hero army
	{

		if (!town->visitingHero->canBeMergedWith(*town))
		{
			complain("Cannot make garrison swap, not enough free slots!");
			return false;
		}

		moveArmy(town, town->visitingHero, true);

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.visiting = ObjectInstanceID();
		intown.garrison = town->visitingHero->id;
		sendAndApply(intown);
		return true;
	}
	else if (town->garrisonHero && !town->visitingHero) //move hero out of the garrison
	{
		int mapCap = getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP);
		//check if moving hero out of town will break wandering heroes limit
		if (getHeroCount(town->garrisonHero->tempOwner,false) >= mapCap)
		{
			complain("Cannot move hero out of the garrison, there are already " + std::to_string(mapCap) + " wandering heroes!");
			return false;
		}

		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = ObjectInstanceID();
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(intown);
		return true;
	}
	else if (!!town->garrisonHero && town->visitingHero) //swap visiting and garrison hero
	{
		SetHeroesInTown intown;
		intown.tid = tid;
		intown.garrison = town->visitingHero->id;
		intown.visiting =  town->garrisonHero->id;
		sendAndApply(intown);
		return true;
	}
	else
	{
		complain("Cannot swap garrison hero!");
		return false;
	}
}

// With the amount of changes done to the function, it's more like transferArtifacts.
// Function moves artifact from src to dst. If dst is not a backpack and is already occupied, old dst art goes to backpack and is replaced.
bool CGameHandler::moveArtifact(const PlayerColor & player, const ArtifactLocation & src, const ArtifactLocation & dst)
{
	const auto srcArtSet = getArtSet(src);
	const auto dstArtSet = getArtSet(dst);
	assert(srcArtSet);
	assert(dstArtSet);

	// Make sure exchange is even possible between the two heroes.
	if(!isAllowedExchange(src.artHolder, dst.artHolder))
		COMPLAIN_RET("That heroes cannot make any exchange!");

	COMPLAIN_RET_FALSE_IF(!ArtifactUtils::checkIfSlotValid(*srcArtSet, src.slot), "moveArtifact: wrong artifact source slot");
	const auto srcArtifact = srcArtSet->getArt(src.slot);
	auto dstSlot = dst.slot;
	if(dstSlot == ArtifactPosition::FIRST_AVAILABLE)
		dstSlot = ArtifactUtils::getArtAnyPosition(dstArtSet, srcArtifact->getTypeId());
	if(!ArtifactUtils::checkIfSlotValid(*dstArtSet, dstSlot))
		return true;
	const auto dstArtifact = dstArtSet->getArt(dstSlot);
	const bool isDstSlotOccupied = dstArtSet->bearerType() == ArtBearer::ALTAR ? false : dstArtifact != nullptr;
	const bool isDstSlotBackpack = dstArtSet->bearerType() == ArtBearer::HERO ? ArtifactUtils::isSlotBackpack(dstSlot) : false;

	if(srcArtifact == nullptr)
		COMPLAIN_RET("No artifact to move!");
	if(isDstSlotOccupied && getOwner(src.artHolder) != getOwner(dst.artHolder) && !isDstSlotBackpack)
		COMPLAIN_RET("Can't touch artifact on hero of another player!");

	// Check if src/dest slots are appropriate for the artifacts exchanged.
	// Moving to the backpack is always allowed.
	if((!srcArtifact || !isDstSlotBackpack) && !srcArtifact->canBePutAt(dstArtSet, dstSlot, true))
		COMPLAIN_RET("Cannot move artifact!");

	auto srcSlotInfo = srcArtSet->getSlot(src.slot);
	auto dstSlotInfo = dstArtSet->getSlot(dstSlot);

	if((srcSlotInfo && srcSlotInfo->locked) || (dstSlotInfo && dstSlotInfo->locked))
		COMPLAIN_RET("Cannot move artifact locks.");

	if(isDstSlotBackpack && srcArtifact->getType()->isBig())
		COMPLAIN_RET("Cannot put big artifacts in backpack!");
	if(src.slot == ArtifactPosition::MACH4 || dstSlot == ArtifactPosition::MACH4)
		COMPLAIN_RET("Cannot move catapult!");
	if(isDstSlotBackpack && !ArtifactUtils::isBackpackFreeSlots(dstArtSet))
		COMPLAIN_RET("Backpack is full!");

	dstSlot = std::min(dstSlot, ArtifactPosition(ArtifactPosition::BACKPACK_START + dstArtSet->artifactsInBackpack.size()));

	if(src.slot == dstSlot && src.artHolder == dst.artHolder)
		COMPLAIN_RET("Won't move artifact: Dest same as source!");
	
	BulkMoveArtifacts ma(player, src.artHolder, dst.artHolder, false);
	ma.srcCreature = src.creature;
	ma.dstCreature = dst.creature;
	
	// Check if dst slot is occupied
	if(!isDstSlotBackpack && isDstSlotOccupied)
	{
		// Previous artifact must be swapped
		COMPLAIN_RET_FALSE_IF(!dstArtifact->canBePutAt(srcArtSet, src.slot, true), "Cannot swap artifacts!");
		ma.artsPack1.push_back(BulkMoveArtifacts::LinkedSlots(dstSlot, src.slot));
	}

	auto hero = getHero(dst.artHolder);
	if(ArtifactUtils::checkSpellbookIsNeeded(hero, srcArtifact->getTypeId(), dstSlot))
		giveHeroNewArtifact(hero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

	ma.artsPack0.push_back(BulkMoveArtifacts::LinkedSlots(src.slot, dstSlot));
	if(src.artHolder != dst.artHolder && !isDstSlotBackpack)
		ma.artsPack0.back().askAssemble = true;
	sendAndApply(ma);
	return true;
}

bool CGameHandler::bulkMoveArtifacts(const PlayerColor & player, ObjectInstanceID srcId, ObjectInstanceID dstId, bool swap, bool equipped, bool backpack)
{
	// Make sure exchange is even possible between the two heroes.
	if(!isAllowedExchange(srcId, dstId))
		COMPLAIN_RET("That heroes cannot make any exchange!");

	auto psrcSet = getArtSet(srcId);
	auto pdstSet = getArtSet(dstId);
	if((!psrcSet) || (!pdstSet))
		COMPLAIN_RET("bulkMoveArtifacts: wrong hero's ID");

	BulkMoveArtifacts ma(player, srcId, dstId, swap);
	auto & slotsSrcDst = ma.artsPack0;
	auto & slotsDstSrc = ma.artsPack1;

	// Temporary fitting set for artifacts. Used to select available slots before sending data.
	CArtifactFittingSet artFittingSet(pdstSet->bearerType());

	auto moveArtifact = [this, &artFittingSet, dstId](const CArtifactInstance * artifact,
		ArtifactPosition srcSlot, std::vector<BulkMoveArtifacts::LinkedSlots> & slots) -> void
	{
		assert(artifact);
		auto dstSlot = ArtifactUtils::getArtAnyPosition(&artFittingSet, artifact->getTypeId());
		if(dstSlot != ArtifactPosition::PRE_FIRST)
		{
			artFittingSet.putArtifact(dstSlot, static_cast<ConstTransitivePtr<CArtifactInstance>>(artifact));
			slots.push_back(BulkMoveArtifacts::LinkedSlots(srcSlot, dstSlot));

			// TODO Shouldn't be here. Possibly in callback after equipping the artifact
			if(auto dstHero = getHero(dstId))
			{
				if(ArtifactUtils::checkSpellbookIsNeeded(dstHero, artifact->getTypeId(), dstSlot))
					giveHeroNewArtifact(dstHero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
			}
		}
	};

	if(swap)
	{
		auto moveArtsWorn = [moveArtifact](const CArtifactSet * srcArtSet, std::vector<BulkMoveArtifacts::LinkedSlots> & slots)
		{
			for(auto & artifact : srcArtSet->artifactsWorn)
			{
				if(ArtifactUtils::isArtRemovable(artifact))
					moveArtifact(artifact.second.getArt(), artifact.first, slots);
			}
		};
		auto moveArtsInBackpack = [](const CArtifactSet * artSet,
			std::vector<BulkMoveArtifacts::LinkedSlots> & slots) -> void
		{
			for(auto & slotInfo : artSet->artifactsInBackpack)
			{
				auto slot = artSet->getArtPos(slotInfo.artifact);
				slots.push_back(BulkMoveArtifacts::LinkedSlots(slot, slot));
			}
		};
		if(equipped)
		{
			// Move over artifacts that are worn srcHero -> dstHero
			moveArtsWorn(psrcSet, slotsSrcDst);
			artFittingSet.artifactsWorn.clear();
			// Move over artifacts that are worn dstHero -> srcHero
			moveArtsWorn(pdstSet, slotsDstSrc);
		}
		if(backpack)
		{
			// Move over artifacts that are in backpack srcHero -> dstHero
			moveArtsInBackpack(psrcSet, slotsSrcDst);
			// Move over artifacts that are in backpack dstHero -> srcHero
			moveArtsInBackpack(pdstSet, slotsDstSrc);
		}
	}
	else
	{
		artFittingSet.artifactsInBackpack = pdstSet->artifactsInBackpack;
		artFittingSet.artifactsWorn = pdstSet->artifactsWorn;
		if(equipped)
		{
			// Move over artifacts that are worn
			for(auto & artInfo : psrcSet->artifactsWorn)
			{
				if(ArtifactUtils::isArtRemovable(artInfo))
				{
					moveArtifact(psrcSet->getArt(artInfo.first), artInfo.first, slotsSrcDst);
				}
			}
		}
		if(backpack)
		{
			// Move over artifacts that are in backpack
			for(auto & slotInfo : psrcSet->artifactsInBackpack)
			{
				moveArtifact(psrcSet->getArt(psrcSet->getArtPos(slotInfo.artifact)),
					psrcSet->getArtPos(slotInfo.artifact), slotsSrcDst);
			}
		}
	}
	sendAndApply(ma);
	return true;
}

bool CGameHandler::manageBackpackArtifacts(const PlayerColor & player, const ObjectInstanceID heroID, const ManageBackpackArtifacts::ManageCmd & sortType)
{
	const auto artSet = getArtSet(heroID);
	COMPLAIN_RET_FALSE_IF(artSet == nullptr, "manageBackpackArtifacts: wrong hero's ID");

	BulkMoveArtifacts bma(player, heroID, heroID, false);
	const auto makeSortBackpackRequest = [artSet, &bma](const std::function<int32_t(const ArtSlotInfo&)> & getSortId)
	{
		std::map<int32_t, std::vector<BulkMoveArtifacts::LinkedSlots>> packsSorted;
		ArtifactPosition backpackSlot = ArtifactPosition::BACKPACK_START;
		for(const auto & backpackSlotInfo : artSet->artifactsInBackpack)
			packsSorted.try_emplace(getSortId(backpackSlotInfo)).first->second.emplace_back(backpackSlot++, ArtifactPosition::PRE_FIRST);

		for(auto & [sortId, pack] : packsSorted)
		{
			// Each pack of artifacts is also sorted by ArtifactID. Scrolls by SpellID
			std::sort(pack.begin(), pack.end(), [artSet](const auto & slots0, const auto & slots1) -> bool
				{
					const auto art0 = artSet->getArt(slots0.srcPos);
					const auto art1 = artSet->getArt(slots1.srcPos);
					if(art0->isScroll() && art1->isScroll())
						return art0->getScrollSpellID() > art1->getScrollSpellID();
					return art0->getTypeId().num > art1->getTypeId().num;
				});
			bma.artsPack0.insert(bma.artsPack0.end(), pack.begin(), pack.end());
		}
		backpackSlot = ArtifactPosition::BACKPACK_START;
		for(auto & slots : bma.artsPack0)
			slots.dstPos = backpackSlot++;
	};
	
	if(sortType == ManageBackpackArtifacts::ManageCmd::SORT_BY_SLOT)
	{
		makeSortBackpackRequest([](const ArtSlotInfo & inf) -> int32_t
			{
				auto possibleSlots = inf.getArt()->getType()->getPossibleSlots();
				if (possibleSlots.find(ArtBearer::CREATURE) != possibleSlots.end() && !possibleSlots.at(ArtBearer::CREATURE).empty()) 
				{
					return -2;
				}
				else if (possibleSlots.find(ArtBearer::COMMANDER) != possibleSlots.end() && !possibleSlots.at(ArtBearer::COMMANDER).empty()) 
				{
					return -1;
				}
				else if (possibleSlots.find(ArtBearer::HERO) != possibleSlots.end() && !possibleSlots.at(ArtBearer::HERO).empty()) 
				{
					return inf.getArt()->getType()->getPossibleSlots().at(ArtBearer::HERO).front().num;
				}
				else 
				{
					// for grail
					return -3;
				}
			});
	}
	else if(sortType == ManageBackpackArtifacts::ManageCmd::SORT_BY_COST)
	{
		makeSortBackpackRequest([](const ArtSlotInfo & inf) -> int32_t
			{
				return inf.getArt()->getType()->getPrice();
			});
	}
	else if(sortType == ManageBackpackArtifacts::ManageCmd::SORT_BY_CLASS)
	{
		makeSortBackpackRequest([](const ArtSlotInfo & inf) -> int32_t
			{
				return inf.getArt()->getType()->aClass;
			});
	}
	else
	{
		const auto backpackEnd = ArtifactPosition(ArtifactPosition::BACKPACK_START + artSet->artifactsInBackpack.size() - 1);
		if(backpackEnd > ArtifactPosition::BACKPACK_START)
		{
			if(sortType == ManageBackpackArtifacts::ManageCmd::SCROLL_LEFT)
				bma.artsPack0.emplace_back(backpackEnd, ArtifactPosition::BACKPACK_START);
			else
				bma.artsPack0.emplace_back(ArtifactPosition::BACKPACK_START, backpackEnd);
		}
	}
	sendAndApply(bma);
	return true;
}

bool CGameHandler::saveArtifactsCostume(const PlayerColor & player, const ObjectInstanceID heroID, uint32_t costumeIdx)
{
	auto artSet = getArtSet(heroID);
	COMPLAIN_RET_FALSE_IF(artSet == nullptr, "saveArtifactsCostume: wrong hero's ID");

	ChangeArtifactsCostume costume(player, costumeIdx);
	for(const auto & slot : ArtifactUtils::commonWornSlots())
	{
		if(const auto slotInfo = artSet->getSlot(slot); slotInfo != nullptr && !slotInfo->locked)
			costume.costumeSet.emplace(slot, slotInfo->getArt()->getTypeId());
	}

	sendAndApply(costume);
	return true;
}

bool CGameHandler::switchArtifactsCostume(const PlayerColor & player, const ObjectInstanceID heroID, uint32_t costumeIdx)
{
	const auto artSet = getArtSet(heroID);
	COMPLAIN_RET_FALSE_IF(artSet == nullptr, "switchArtifactsCostume: wrong hero's ID");
	const auto playerState = getPlayerState(player);
	COMPLAIN_RET_FALSE_IF(playerState == nullptr, "switchArtifactsCostume: wrong player");

	if(auto costume = playerState->costumesArtifacts.find(costumeIdx); costume != playerState->costumesArtifacts.end())
	{
		CArtifactFittingSet artFittingSet(*artSet);
		BulkMoveArtifacts bma(player, heroID, heroID, false);
		auto costumeArtMap = costume->second;
		auto estimateBackpackSize = artSet->artifactsInBackpack.size();

		// First, find those artifacts that are already in place
		for(const auto & slot : ArtifactUtils::commonWornSlots())
		{
			if(const auto * slotInfo = artFittingSet.getSlot(slot); slotInfo != nullptr && !slotInfo->locked)
				if(const auto artPos = costumeArtMap.find(slot); artPos != costumeArtMap.end() && artPos->second == slotInfo->getArt()->getTypeId())
				{
					costumeArtMap.erase(artPos);
					artFittingSet.removeArtifact(slot);
				}
		}
		
		// Second, find the necessary artifacts for the costume
		for(const auto & artPos : costumeArtMap)
		{
			if(const auto slot = artFittingSet.getArtPos(artPos.second, false, false); slot != ArtifactPosition::PRE_FIRST)
			{
				bma.artsPack0.emplace_back(BulkMoveArtifacts::LinkedSlots
					{
						artSet->getArtPos(artFittingSet.getArt(slot)),
						artPos.first
					});
				artFittingSet.removeArtifact(slot);
				if(ArtifactUtils::isSlotBackpack(slot))
					estimateBackpackSize--;
			}
		}

		// Third, put unnecessary artifacts into backpack
		for(const auto & slot : ArtifactUtils::commonWornSlots())
			if(artFittingSet.getArt(slot))
			{
				bma.artsPack0.emplace_back(BulkMoveArtifacts::LinkedSlots{slot, ArtifactPosition::BACKPACK_START});
				estimateBackpackSize++;
			}
		
		const auto backpackCap = getSettings().getInteger(EGameSettings::HEROES_BACKPACK_CAP);
		if((backpackCap < 0 || estimateBackpackSize <= backpackCap) && !bma.artsPack0.empty())
			sendAndApply(bma);
	}
	return true;
}

/**
 * Assembles or disassembles a combination artifact.
 * @param heroID ID of hero holding the artifact(s).
 * @param artifactSlot The worn slot ID of the combination- or constituent artifact.
 * @param assemble True for assembly operation, false for disassembly.
 * @param assembleTo If assemble is true, this represents the artifact ID of the combination
 * artifact to assemble to. Otherwise it's not used.
 */
bool CGameHandler::assembleArtifacts(ObjectInstanceID heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)
{
	const CGHeroInstance * hero = getHero(heroID);
	const CArtifactInstance * destArtifact = hero->getArt(artifactSlot);

	if(!destArtifact)
		COMPLAIN_RET("assembleArtifacts: there is no such artifact instance!");

	const auto dstLoc = ArtifactLocation(hero->id, artifactSlot);
	if(assemble)
	{
		const CArtifact * combinedArt = assembleTo.toArtifact();
		if(!combinedArt->isCombined())
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to assemble is not a combined artifacts!");
		if(!vstd::contains(ArtifactUtils::assemblyPossibilities(hero, destArtifact->getTypeId()), combinedArt))
		{
			COMPLAIN_RET("assembleArtifacts: It's impossible to assemble requested artifact!");
		}
		if(!destArtifact->canBePutAt(hero, artifactSlot, true)
			&& !destArtifact->canBePutAt(hero, ArtifactPosition::BACKPACK_START, true))
		{
			COMPLAIN_RET("assembleArtifacts: It's impossible to give the artholder requested artifact!");
		}
		
		if(ArtifactUtils::checkSpellbookIsNeeded(hero, assembleTo, artifactSlot))
			giveHeroNewArtifact(hero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

		AssembledArtifact aa;
		aa.al = dstLoc;
		aa.artId = assembleTo;
		sendAndApply(aa);
	}
	else
	{
		if(!destArtifact->isCombined())
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble is not a combined artifact!");

		if(!destArtifact->hasParts())
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble is fused combined artifact!");

		if(ArtifactUtils::isSlotBackpack(artifactSlot)
			&& !ArtifactUtils::isBackpackFreeSlots(hero, destArtifact->getType()->getConstituents().size() - 1))
			COMPLAIN_RET("assembleArtifacts: Artifact being attempted to disassemble but backpack is full!");

		DisassembledArtifact da;
		da.al = dstLoc;
		sendAndApply(da);
	}

	return true;
}

bool CGameHandler::eraseArtifactByClient(const ArtifactLocation & al)
{
	const auto * hero = getHero(al.artHolder);
	if(hero == nullptr)
		COMPLAIN_RET("eraseArtifactByClient: wrong hero's ID");

	const auto * art = hero->getArt(al.slot);
	if(art == nullptr)
		COMPLAIN_RET("Cannot remove artifact!");

	if(art->canBePutAt(hero) || al.slot != ArtifactPosition::TRANSITION_POS)
		COMPLAIN_RET("Illegal artifact removal request");

	removeArtifact(al);
	return true;
}

bool CGameHandler::buyArtifact(ObjectInstanceID hid, ArtifactID aid)
{
	const CGHeroInstance * hero = getHero(hid);
	COMPLAIN_RET_FALSE_IF(nullptr == hero, "Invalid hero index");
	const CGTownInstance * town = hero->visitedTown;
	COMPLAIN_RET_FALSE_IF(nullptr == town, "Hero not in town");

	if (aid==ArtifactID::SPELLBOOK)
	{
		if ((!town->hasBuilt(BuildingID::MAGES_GUILD_1) && complain("Cannot buy a spellbook, no mage guild in the town!"))
		    || (getResource(hero->getOwner(), EGameResID::GOLD) < GameConstants::SPELLBOOK_GOLD_COST && complain("Cannot buy a spellbook, not enough gold!"))
		    || (hero->getArt(ArtifactPosition::SPELLBOOK) && complain("Cannot buy a spellbook, hero already has a one!"))
		   )
			return false;

		giveResource(hero->getOwner(),EGameResID::GOLD,-GameConstants::SPELLBOOK_GOLD_COST);
		giveHeroNewArtifact(hero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
		assert(hero->getArt(ArtifactPosition::SPELLBOOK));
		giveSpells(town,hero);
		return true;
	}
	else
	{
		const CArtifact * art = aid.toArtifact();
		COMPLAIN_RET_FALSE_IF(nullptr == art, "Invalid artifact index to buy");
		COMPLAIN_RET_FALSE_IF(art->getWarMachine() == CreatureID::NONE, "War machine artifact required");
		COMPLAIN_RET_FALSE_IF(hero->hasArt(aid),"Hero already has this machine!");
		const int price = art->getPrice();
		COMPLAIN_RET_FALSE_IF(getPlayerState(hero->getOwner())->resources[EGameResID::GOLD] < price, "Not enough gold!");

		if(town->isWarMachineAvailable(aid))
		{
			bool hasFreeSlot = false;
			for(auto slot : art->getPossibleSlots().at(ArtBearer::HERO))
				if (hero->getArt(slot) == nullptr)
					hasFreeSlot = true;

			if (!hasFreeSlot)
			{
				auto slot = art->getPossibleSlots().at(ArtBearer::HERO).front();
				removeArtifact(ArtifactLocation(hero->id, slot));
			}

			giveResource(hero->getOwner(),EGameResID::GOLD,-price);
			return giveHeroNewArtifact(hero, aid, ArtifactPosition::FIRST_AVAILABLE);
		}
		else
			COMPLAIN_RET("This machine is unavailable here!");
	}
}

bool CGameHandler::buyArtifact(const IMarket *m, const CGHeroInstance *h, GameResID rid, ArtifactID aid)
{
	if(!h)
		COMPLAIN_RET("Only hero can buy artifacts!");

	if (!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_ARTIFACT), aid))
		COMPLAIN_RET("That artifact is unavailable!");

	int b1;
	int b2;
	m->getOffer(rid, aid, b1, b2, EMarketMode::RESOURCE_ARTIFACT);

	if (getResource(h->tempOwner, rid) < b1)
		COMPLAIN_RET("You can't afford to buy this artifact!");

	giveResource(h->tempOwner, rid, -b1);

	SetAvailableArtifacts saa;
	if(dynamic_cast<const CGTownInstance *>(m))
	{
		saa.id = ObjectInstanceID::NONE;
		saa.arts = gs->getMap().townMerchantArtifacts;
	}
	else if(const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(m)) //black market
	{
		saa.id = bm->id;
		saa.arts = bm->artifacts;
	}
	else
		COMPLAIN_RET("Wrong marktet...");

	bool found = false;
	for (ArtifactID & art : saa.arts)
	{
		if (art == aid)
		{
			art = ArtifactID();
			found = true;
			break;
		}
	}

	if (!found)
		COMPLAIN_RET("Cannot find selected artifact on the list");

	sendAndApply(saa);
	giveHeroNewArtifact(h, aid, ArtifactPosition::FIRST_AVAILABLE);
	return true;
}

bool CGameHandler::sellArtifact(const IMarket *m, const CGHeroInstance *h, ArtifactInstanceID aid, GameResID rid)
{
	COMPLAIN_RET_FALSE_IF((!h), "Only hero can sell artifacts!");
	const CArtifactInstance *art = h->getArtByInstanceId(aid);
	COMPLAIN_RET_FALSE_IF((!art), "There is no artifact to sell!");
	COMPLAIN_RET_FALSE_IF((!art->getType()->isTradable()), "Cannot sell a war machine or spellbook!");

	int resVal = 0;
	int dump = 1;
	m->getOffer(art->getType()->getId(), rid, dump, resVal, EMarketMode::ARTIFACT_RESOURCE);

	removeArtifact(ArtifactLocation(h->id, h->getArtPos(art)));
	giveResource(h->tempOwner, rid, resVal);
	return true;
}

bool CGameHandler::buySecSkill(const IMarket *m, const CGHeroInstance *h, SecondarySkill skill)
{
	if (!h)
		COMPLAIN_RET("You need hero to buy a skill!");

	if (h->getSecSkillLevel(SecondarySkill(skill)))
		COMPLAIN_RET("Hero already know this skill");

	if (!h->canLearnSkill())
		COMPLAIN_RET("Hero can't learn any more skills");

	if (!h->canLearnSkill(skill))
		COMPLAIN_RET("The hero can't learn this skill!");

	if (!vstd::contains(m->availableItemsIds(EMarketMode::RESOURCE_SKILL), skill))
		COMPLAIN_RET("That skill is unavailable!");

	if (getResource(h->tempOwner, EGameResID::GOLD) < GameConstants::SKILL_GOLD_COST)//TODO: remove hardcoded resource\summ?
		COMPLAIN_RET("You can't afford to buy this skill");

	giveResource(h->tempOwner, EGameResID::GOLD, -GameConstants::SKILL_GOLD_COST);

	changeSecSkill(h, skill, 1, true);
	return true;
}

bool CGameHandler::tradeResources(const IMarket *market, ui32 amountToSell, PlayerColor player, GameResID toSell, GameResID toBuy)
{
	TResourceCap haveToSell = getPlayerState(player)->resources[toSell];

	vstd::amin(amountToSell, haveToSell); //can't trade more resources than have

	int b1; //base quantities for trade
	int b2;
	market->getOffer(toSell, toBuy, b1, b2, EMarketMode::RESOURCE_RESOURCE);
	int amountToBoy = amountToSell / b1; //how many base quantities we trade

	if (amountToSell % b1 != 0) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		COMPLAIN_RET("Invalid deal, not all offered units of resource were used.");
	}

	giveResource(player, toSell, -b1 * amountToBoy);
	giveResource(player, toBuy, b2 * amountToBoy);

	gs->statistic.accumulatedValues[player].tradeVolume[toSell] += -b1 * amountToBoy;
	gs->statistic.accumulatedValues[player].tradeVolume[toBuy] += b2 * amountToBoy;

	return true;
}

bool CGameHandler::sellCreatures(ui32 count, const IMarket *market, const CGHeroInstance * hero, SlotID slot, GameResID resourceID)
{
	if(!hero)
		COMPLAIN_RET("Only hero can sell creatures!");
	if (!vstd::contains(hero->Slots(), slot))
		COMPLAIN_RET("Hero doesn't have any creature in that slot!");

	const CStackInstance &s = hero->getStack(slot);

	if (s.count < (TQuantity)count //can't sell more creatures than have
		|| (hero->stacksCount() == 1 && hero->needsLastStack() && s.count == count)) //can't sell last stack
	{
		COMPLAIN_RET("Not enough creatures in army!");
	}

	int b1; //base quantities for trade
	int b2;
	market->getOffer(s.getId(), resourceID, b1, b2, EMarketMode::CREATURE_RESOURCE);
	int units = count / b1; //how many base quantities we trade

	if (count%b1) //all offered units of resource should be used, if not -> somewhere in calculations must be an error
	{
		//TODO: complain?
		assert(0);
	}

	changeStackCount(StackLocation(hero->id, slot), -(TQuantity)count);

	giveResource(hero->tempOwner, resourceID, b2 * units);

	return true;
}

bool CGameHandler::transformInUndead(const IMarket *market, const CGHeroInstance * hero, SlotID slot)
{
	const CArmedInstance *army = nullptr;
	if (hero)
		army = hero;
	else
		army = dynamic_cast<const CGTownInstance *>(market);

	if (!army)
		COMPLAIN_RET("Incorrect call to transform in undead!");
	if (!army->hasStackAtSlot(slot))
		COMPLAIN_RET("Army doesn't have any creature in that slot!");


	const CStackInstance &s = army->getStack(slot);

	//resulting creature - bone dragons or skeletons
	CreatureID resCreature = CreatureID::SKELETON;

	if ((s.hasBonusOfType(BonusType::DRAGON_NATURE)
			&& !(s.hasBonusOfType(BonusType::UNDEAD)))
			|| (s.getCreatureID() == CreatureID::HYDRA)
			|| (s.getCreatureID() == CreatureID::CHAOS_HYDRA))
		resCreature = CreatureID::BONE_DRAGON;
	changeStackType(StackLocation(army->id, slot), resCreature.toCreature());
	return true;
}

bool CGameHandler::sendResources(ui32 val, PlayerColor player, GameResID r1, PlayerColor r2)
{
	const PlayerState *p2 = getPlayerState(r2, false);
	if (!p2  ||  p2->status != EPlayerStatus::INGAME)
	{
		complain("Dest player must be in game!");
		return false;
	}

	TResourceCap curRes1 = getPlayerState(player)->resources[r1];

	vstd::amin(val, curRes1);

	giveResource(player, r1, -(int)val);
	giveResource(r2, r1, val);

	return true;
}

bool CGameHandler::setFormation(ObjectInstanceID hid, EArmyFormation formation)
{
	const CGHeroInstance *h = getHero(hid);
	if (!h)
	{
		logGlobal->error("Hero doesn't exist!");
		return false;
	}

	ChangeFormation cf;
	cf.hid = hid;
	cf.formation = formation;
	sendAndApply(cf);

	return true;
}

bool CGameHandler::queryReply(QueryID qid, std::optional<int32_t> answer, PlayerColor player)
{
	logGlobal->trace("Player %s attempts answering query %d with answer:", player, qid);
	if (answer)
		logGlobal->trace("%d", *answer);

	auto topQuery = queries->topQuery(player);

	COMPLAIN_RET_FALSE_IF(!topQuery, "This player doesn't have any queries!");

	if(topQuery->queryID != qid)
	{
		auto currentQuery = queries->getQuery(qid);

		if(currentQuery != nullptr && currentQuery->endsByPlayerAnswer())
			currentQuery->setReply(answer);

		COMPLAIN_RET("This player top query has different ID!"); //topQuery->queryID != qid
	}
	COMPLAIN_RET_FALSE_IF(!topQuery->endsByPlayerAnswer(), "This query cannot be ended by player's answer!");

	topQuery->setReply(answer);
	queries->popQuery(topQuery);
	return true;
}

bool CGameHandler::complain(const std::string &problem)
{
#ifndef ENABLE_GOLDMASTER
	MetaString str;
	str.appendTextID("vcmi.broadcast.serverProblem");
	str.appendRawString(": ");
	str.appendRawString(problem);
	playerMessages->broadcastSystemMessage(str);
#endif
	logGlobal->error(problem);
	return true;
}

void CGameHandler::showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits)
{
	//PlayerColor player = getOwner(hid);
	auto upperArmy = dynamic_cast<const CArmedInstance*>(getObj(upobj));
	auto lowerArmy = dynamic_cast<const CArmedInstance*>(getObj(hid));

	assert(lowerArmy);
	assert(upperArmy);

	auto garrisonQuery = std::make_shared<CGarrisonDialogQuery>(this, upperArmy, lowerArmy);
	queries->addQuery(garrisonQuery);

	GarrisonDialog gd;
	gd.hid = hid;
	gd.objid = upobj;
	gd.removableUnits = removableUnits;
	gd.queryID = garrisonQuery->queryID;
	sendAndApply(gd);
}

void CGameHandler::showObjectWindow(const CGObjectInstance * object, EOpenWindowMode window, const CGHeroInstance * visitor, bool addQuery)
{
	OpenWindow pack;
	pack.window = window;
	pack.object = object->id;
	pack.visitor = visitor->id;

	if (addQuery)
	{
		auto windowQuery = std::make_shared<OpenWindowQuery>(this, visitor, window);
		pack.queryID = windowQuery->queryID;
		queries->addQuery(windowQuery);
	}
	sendAndApply(pack);
}

bool CGameHandler::isAllowedExchange(ObjectInstanceID id1, ObjectInstanceID id2)
{
	if (id1 == id2)
		return true;

	const CGObjectInstance *o1 = getObj(id1);
	const CGObjectInstance *o2 = getObj(id2);
	if (!o1 || !o2)
		return true; //arranging stacks within an object should be always allowed

	if (o1 && o2)
	{
		if (o1->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o1);
			if (t->visitingHero == o2  ||  t->garrisonHero == o2)
				return true;
		}
		if (o2->ID == Obj::TOWN)
		{
			const CGTownInstance *t = static_cast<const CGTownInstance*>(o2);
			if (t->visitingHero == o1  ||  t->garrisonHero == o1)
				return true;
		}

		auto market = getMarket(id1);
		if(market == nullptr)
			market = getMarket(id2);
		if(market)
			return market->allowsTrade(EMarketMode::ARTIFACT_EXP);

		if (o1->ID == Obj::HERO && o2->ID == Obj::HERO)
		{
			const CGHeroInstance *h1 = static_cast<const CGHeroInstance*>(o1);
			const CGHeroInstance *h2 = static_cast<const CGHeroInstance*>(o2);

			// two heroes in same town (garrisoned and visiting)
			if (h1->visitedTown != nullptr && h2->visitedTown != nullptr && h1->visitedTown == h2->visitedTown)
				return true;
		}

		//Ongoing garrison exchange - usually picking from top garison (from o1 to o2), but who knows
		auto dialog = std::dynamic_pointer_cast<CGarrisonDialogQuery>(queries->topQuery(o1->tempOwner));
		if (!dialog)
		{
			dialog = std::dynamic_pointer_cast<CGarrisonDialogQuery>(queries->topQuery(o2->tempOwner));
		}
		if (dialog)
		{
			auto topArmy = dialog->exchangingArmies.at(0);
			auto bottomArmy = dialog->exchangingArmies.at(1);

			if ((topArmy == o1 && bottomArmy == o2) || (bottomArmy == o1 && topArmy == o2))
				return true;
		}
	}

	return false;
}

void CGameHandler::objectVisited(const CGObjectInstance * obj, const CGHeroInstance * h)
{
	using events::ObjectVisitStarted;

	logGlobal->debug("%s visits %s (%d)", h->nodeName(), obj->getObjectName(), obj->ID);

	if (getVisitingHero(obj) != nullptr)
	{
		logGlobal->error("Attempt to visit object that is being visited by another hero!");
		throw std::runtime_error("Can not visit object that is being visited");
	}

	std::shared_ptr<MapObjectVisitQuery> visitQuery;

	auto startVisit = [&](ObjectVisitStarted & event)
	{
		auto visitedObject = obj;

		if(obj->ID == Obj::HERO)
		{
			auto visitedHero = static_cast<const CGHeroInstance *>(obj);
			const auto visitedTown = visitedHero->visitedTown;

			if(visitedTown)
			{
				const bool isEnemy = visitedHero->getOwner() != h->getOwner();

				if(isEnemy && !visitedTown->isBattleOutsideTown(visitedHero))
					visitedObject = visitedTown;
			}
		}
		visitQuery = std::make_shared<MapObjectVisitQuery>(this, visitedObject, h);
		queries->addQuery(visitQuery); //TODO real visit pos

		HeroVisit hv;
		hv.objId = obj->id;
		hv.heroId = h->id;
		hv.player = h->tempOwner;
		hv.starting = true;
		sendAndApply(hv);

		obj->onHeroVisit(h);
	};

	ObjectVisitStarted::defaultExecute(serverEventBus.get(), startVisit, h->tempOwner, h->id, obj->id);

	if(visitQuery)
		queries->popIfTop(visitQuery); //visit ends here if no queries were created
}

void CGameHandler::objectVisitEnded(const CGHeroInstance *h, PlayerColor player)
{
	using events::ObjectVisitEnded;

	logGlobal->debug("%s visit ends.\n", h->nodeName());

	auto endVisit = [&](ObjectVisitEnded & event)
	{
		HeroVisit hv;
		hv.player = event.getPlayer();
		hv.heroId = event.getHero();
		hv.starting = false;
		sendAndApply(hv);
	};

	//TODO: ObjectVisitEnded should also have id of visited object,
	//but this requires object being deleted only by `removeAfterVisit()` but not `removeObject()`
	ObjectVisitEnded::defaultExecute(serverEventBus.get(), endVisit, player, h->id);
}

bool CGameHandler::buildBoat(ObjectInstanceID objid, PlayerColor playerID)
{
	const auto *obj = dynamic_cast<const IShipyard *>(getObj(objid));

	if (obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		complain("Cannot build boat in this shipyard!");
		return false;
	}

	TResources boatCost;
	obj->getBoatCost(boatCost);
	TResources available = getPlayerState(playerID)->resources;

	if (!available.canAfford(boatCost))
	{
		complain("Not enough resources to build a boat!");
		return false;
	}

	int3 tile = obj->bestLocation();
	if (!gs->getMap().isInTheMap(tile))
	{
		complain("Cannot find appropriate tile for a boat!");
		return false;
	}

	giveResources(playerID, -boatCost);
	createBoat(tile, obj->getBoatType(), playerID);
	return true;
}

void CGameHandler::checkVictoryLossConditions(const std::set<PlayerColor> & playerColors)
{
	for (auto playerColor : playerColors)
	{
		if (getPlayerState(playerColor, false))
			checkVictoryLossConditionsForPlayer(playerColor);
	}
}

void CGameHandler::checkVictoryLossConditionsForAll()
{
	std::set<PlayerColor> playerColors;
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		playerColors.insert(PlayerColor(i));
	}
	checkVictoryLossConditions(playerColors);
}

void CGameHandler::checkVictoryLossConditionsForPlayer(PlayerColor player)
{
	const PlayerState * p = getPlayerState(player);

	if(!p || p->status != EPlayerStatus::INGAME) return;

	auto victoryLossCheckResult = gs->checkForVictoryAndLoss(player);

	if (victoryLossCheckResult.victory() || victoryLossCheckResult.loss())
	{
		InfoWindow iw;
		getVictoryLossMessage(player, victoryLossCheckResult, iw);
		sendAndApply(iw);

		PlayerEndsGame peg;
		peg.player = player;
		peg.victoryLossCheckResult = victoryLossCheckResult;
		peg.statistic = StatisticDataSet(gameState()->statistic);
		addStatistics(peg.statistic); // add last turn befor win / loss
		sendAndApply(peg);

		if (victoryLossCheckResult.victory())
		{
			//one player won -> all enemies lost
			for (auto i = gs->players.cbegin(); i!=gs->players.cend(); i++)
			{
				if (i->first != player && getPlayerState(i->first)->status == EPlayerStatus::INGAME)
				{
					peg.player = i->first;
					peg.victoryLossCheckResult = getPlayerRelations(player, i->first) == PlayerRelations::ALLIES ?
								victoryLossCheckResult : victoryLossCheckResult.invert(); // ally of winner

					InfoWindow iw;
					getVictoryLossMessage(player, peg.victoryLossCheckResult, iw);
					iw.player = i->first;

					sendAndApply(iw);
					sendAndApply(peg);
				}
			}

			if(p->human)
			{
				gameLobby().setState(EServerState::SHUTDOWN);
			}
		}
		else
		{
			// give turn to next player(s)
			turnOrder->onPlayerEndsGame(player);

			//copy heroes vector to avoid iterator invalidation as removal change PlayerState
			auto hlp = p->getHeroes();
			for (auto h : hlp) //eliminate heroes
			{
				if (h)
					removeObject(h, player);
			}

			//player lost -> all his objects become unflagged (neutral)
			for (auto obj : gs->getMap().objects) //unflag objs
			{
				if (obj.get() && obj->tempOwner == player)
					setOwner(obj, PlayerColor::NEUTRAL);
			}

			//eliminating one player may cause victory of another:
			std::set<PlayerColor> playerColors;

			//do not copy player state (CBonusSystemNode) by value
			for (auto &p : gs->players) //players may have different colors, iterate over players and not integers
			{
				if (p.first != player)
					playerColors.insert(p.first);
			}

			//notify all players
			for (auto pc : playerColors)
			{
				if (getPlayerState(pc)->status == EPlayerStatus::INGAME)
				{
					InfoWindow iw;
					getVictoryLossMessage(player, victoryLossCheckResult.invert(), iw);
					iw.player = pc;
					sendAndApply(iw);
				}
			}
			checkVictoryLossConditions(playerColors);
		}
	}
}

void CGameHandler::getVictoryLossMessage(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult, InfoWindow & out) const
{
	out.player = player;
	out.text = victoryLossCheckResult.messageToSelf;
	out.text.replaceName(player);
	out.components.emplace_back(ComponentType::FLAG, player);
}

bool CGameHandler::dig(const CGHeroInstance *h)
{
	if (h->diggingStatus() != EDiggingStatus::CAN_DIG) //checks for terrain and movement
		COMPLAIN_RETF("Hero cannot dig (error code %d)!", static_cast<int>(h->diggingStatus()));

	createHole(h->visitablePos(), h->getOwner());

	//take MPs
	SetMovePoints smp;
	smp.hid = h->id;
	smp.val = 0;
	sendAndApply(smp);

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->tempOwner;
	if (gs->getMap().grailPos == h->visitablePos())
	{
		ArtifactID grail = ArtifactID::GRAIL;

		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 58); //"Congratulations! After spending many hours digging here, your hero has uncovered the " ...
		iw.text.appendName(grail); // ... " The Grail"
		iw.soundID = soundBase::ULTIMATEARTIFACT;
		giveHeroNewArtifact(h, grail, ArtifactPosition::FIRST_AVAILABLE); //give grail
		sendAndApply(iw);

		iw.soundID = soundBase::invalid;
		iw.components.emplace_back(ComponentType::ARTIFACT, grail);
		iw.text.clear();
		iw.text.appendTextID(grail.toArtifact()->getDescriptionTextID());
		sendAndApply(iw);
	}
	else
	{
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 59); //"Nothing here. \n Where could it be?"
		iw.soundID = soundBase::Dig;
		sendAndApply(iw);
	}

	return true;
}

void CGameHandler::visitObjectOnTile(const TerrainTile &t, const CGHeroInstance * h)
{
	if (!t.visitableObjects.empty())
	{
		//to prevent self-visiting heroes on space press
		if (t.visitableObjects.back() != h)
			objectVisited(t.visitableObjects.back(), h);
		else if (t.visitableObjects.size() > 1)
			objectVisited(*(t.visitableObjects.end()-2),h);
	}
}

bool CGameHandler::sacrificeCreatures(const IMarket * market, const CGHeroInstance * hero, const std::vector<SlotID> & slot, const std::vector<ui32> & count)
{
	if (!hero)
		COMPLAIN_RET("You need hero to sacrifice creature!");

	int expSum = 0;
	auto finish = [this, &hero, &expSum]()
	{
		giveExperience(hero, hero->calculateXp(expSum));
	};

	for(int i = 0; i < slot.size(); ++i)
	{
		int oldCount = hero->getStackCount(slot[i]);

		if(oldCount < (int)count[i])
		{
			finish();
			COMPLAIN_RET("Not enough creatures to sacrifice!")
		}
		else if(oldCount == count[i] && hero->stacksCount() == 1 && hero->needsLastStack())
		{
			finish();
			COMPLAIN_RET("Cannot sacrifice last creature!");
		}

		int crid = hero->getStack(slot[i]).getId();

		changeStackCount(StackLocation(hero->id, slot[i]), -(TQuantity)count[i]);

		int dump;
		int exp;
		market->getOffer(crid, 0, dump, exp, EMarketMode::CREATURE_EXP);
		exp *= count[i];
		expSum += exp;
	}

	finish();

	return true;
}

bool CGameHandler::sacrificeArtifact(const IMarket * market, const CGHeroInstance * hero, const std::vector<ArtifactInstanceID> & arts)
{
	if (!hero)
		COMPLAIN_RET("You need hero to sacrifice artifact!");
	if(hero->getAlignment() == EAlignment::EVIL)
		COMPLAIN_RET("Evil hero can't sacrifice artifact!");

	assert(market);
	const auto artSet = market->getArtifactsStorage();

	int expSum = 0;
	std::vector<ArtifactPosition> artPack;
	auto finish = [this, &hero, &expSum, &artPack, market]()
	{
		removeArtifact(market->getObjInstanceID(), artPack);
		giveExperience(hero, hero->calculateXp(expSum));
	};

	for(const auto & artInstId : arts)
	{
		if(auto art = artSet->getArtByInstanceId(artInstId))
		{
			if(art->getType()->isTradable())
			{
				int dmp;
				int expToGive;
				market->getOffer(art->getTypeId(), 0, dmp, expToGive, EMarketMode::ARTIFACT_EXP);
				expSum += expToGive;
				artPack.push_back(artSet->getArtPos(art));
			}
			else
			{
				COMPLAIN_RET("Cannot sacrifice not tradable artifact!");
			}
		}
		else
		{
			finish();
			COMPLAIN_RET("Cannot find artifact to sacrifice!");
		}
	}

	finish();

	return true;
}

bool CGameHandler::insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	auto army = dynamic_cast<const CArmedInstance*>(getObj(sl.army));

	if (army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Slot is already taken!");

	if (!sl.slot.validSlot())
		COMPLAIN_RET("Cannot insert stack to that slot!");

	InsertNewStack ins;
	ins.army = army->id;
	ins.slot = sl.slot;
	ins.type = c->getId();
	ins.count = count;
	sendAndApply(ins);
	return true;
}

bool CGameHandler::eraseStack(const StackLocation &sl, bool forceRemoval)
{
	auto army = dynamic_cast<const CArmedInstance*>(getObj(sl.army));

	if (!army->hasStackAtSlot(sl.slot))
		COMPLAIN_RET("Cannot find a stack to erase");

	if (army->stacksCount() == 1 //from the last stack
		&& army->needsLastStack() //that must be left
		&& !forceRemoval) //ignore above conditions if we are forcing removal
	{
		COMPLAIN_RET("Cannot erase the last stack!");
	}

	EraseStack es;
	es.army = army->id;
	es.slot = sl.slot;
	sendAndApply(es);
	return true;
}

bool CGameHandler::changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue)
{
	auto army = dynamic_cast<const CArmedInstance*>(getObj(sl.army));

	TQuantity currentCount = army->getStackCount(sl.slot);
	if ((absoluteValue && count < 0)
		|| (!absoluteValue && -count > currentCount))
	{
		COMPLAIN_RET("Cannot take more stacks than present!");
	}

	if ((currentCount == -count  &&  !absoluteValue)
	   || (!count && absoluteValue))
	{
		eraseStack(sl);
	}
	else
	{
		ChangeStackCount csc;
		csc.army = army->id;
		csc.slot = sl.slot;
		csc.count = count;
		csc.absoluteValue = absoluteValue;
		sendAndApply(csc);
	}
	return true;
}

bool CGameHandler::addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count)
{
	auto army = dynamic_cast<const CArmedInstance*>(getObj(sl.army));

	const CCreature *slotC = army->getCreature(sl.slot);
	if (!slotC) //slot is empty
		insertNewStack(sl, c, count);
	else if (c == slotC)
		changeStackCount(sl, count);
	else
	{
		COMPLAIN_RET("Cannot add " + c->getNamePluralTranslated() + " to slot " + boost::lexical_cast<std::string>(sl.slot) + "!");
	}
	return true;
}

void CGameHandler::tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging)
{
	if (removeObjWhenFinished)
		removeAfterVisit(src);

	if (!src->canBeMergedWith(*dst, allowMerging))
	{
		if (allowMerging) //do that, add all matching creatures.
		{
			bool cont = true;
			while (cont)
			{
				for (auto i = src->stacks.begin(); i != src->stacks.end(); i++)//while there are unmoved creatures
				{
					SlotID pos = dst->getSlotFor(i->second->getCreature());
					if (pos.validSlot())
					{
						moveStack(StackLocation(src->id, i->first), StackLocation(dst->id, pos));
						cont = true;
						break; //or iterator crashes
					}
					cont = false;
				}
			}
		}
		showGarrisonDialog(src->id, dst->id, true); //show garrison window and optionally remove ourselves from map when player ends
	}
	else //merge
	{
		moveArmy(src, dst, allowMerging);
	}
}

bool CGameHandler::moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count)
{
	auto srcArmy = dynamic_cast<const CArmedInstance*>(getObj(src.army));
	auto dstArmy = dynamic_cast<const CArmedInstance*>(getObj(dst.army));

	if (!srcArmy->hasStackAtSlot(src.slot))
		COMPLAIN_RET("No stack to move!");

	if (dstArmy->hasStackAtSlot(dst.slot) && dstArmy->getCreature(dst.slot) != srcArmy->getCreature(src.slot))
		COMPLAIN_RET("Cannot move: stack of different type at destination pos!");

	if (!dst.slot.validSlot())
		COMPLAIN_RET("Cannot move stack to that slot!");

	if (count == -1)
	{
		count = srcArmy->getStackCount(src.slot);
	}

	if (srcArmy != dstArmy  //moving away
		&&  count == srcArmy->getStackCount(src.slot) //all creatures
		&& srcArmy->stacksCount() == 1 //from the last stack
		&& srcArmy->needsLastStack()) //that must be left
	{
		COMPLAIN_RET("Cannot move away the last creature!");
	}

	RebalanceStacks rs;
	rs.srcArmy = srcArmy->id;
	rs.dstArmy = dstArmy->id;
	rs.srcSlot = src.slot;
	rs.dstSlot = dst.slot;
	rs.count = count;
	sendAndApply(rs);
	return true;
}

void CGameHandler::castSpell(const spells::Caster * caster, SpellID spellID, const int3 &pos)
{
	if (!spellID.hasValue())
		return;

	AdventureSpellCastParameters p;
	p.caster = caster;
	p.pos = pos;

	const CSpell * s = spellID.toSpell();
	s->adventureCast(spellEnv, p);
}

bool CGameHandler::swapStacks(const StackLocation & sl1, const StackLocation & sl2)
{
	auto army1 = dynamic_cast<const CArmedInstance*>(getObj(sl1.army));
	auto army2 = dynamic_cast<const CArmedInstance*>(getObj(sl2.army));

	if(!army1->hasStackAtSlot(sl1.slot))
	{
		return moveStack(sl2, sl1);
	}
	else if(!army2->hasStackAtSlot(sl2.slot))
	{
		return moveStack(sl1, sl2);
	}
	else
	{
		SwapStacks ss;
		ss.srcArmy = army1->id;
		ss.dstArmy = army2->id;
		ss.srcSlot = sl1.slot;
		ss.dstSlot = sl2.slot;
		sendAndApply(ss);
		return true;
	}
}

bool CGameHandler::putArtifact(const ArtifactLocation & al, const ArtifactInstanceID & id, std::optional<bool> askAssemble)
{
	const auto artInst = getArtInstance(id);
	assert(artInst && artInst->getType());
	ArtifactLocation dst(al.artHolder, ArtifactPosition::PRE_FIRST);
	dst.creature = al.creature;
	auto putTo = getArtSet(al);
	assert(putTo);

	if(al.slot == ArtifactPosition::FIRST_AVAILABLE)
	{
		dst.slot = ArtifactUtils::getArtAnyPosition(putTo, artInst->getTypeId());
	}
	else if(ArtifactUtils::isSlotBackpack(al.slot) && !al.creature.has_value())
	{
		dst.slot = ArtifactUtils::getArtBackpackPosition(putTo, artInst->getTypeId());
	}
	else
	{
		dst.slot = al.slot;
	}

	if(!askAssemble.has_value())
	{
		if(!dst.creature.has_value() && ArtifactUtils::isSlotEquipment(dst.slot))
			askAssemble = true;
		else
			askAssemble = false;
	}

	if(artInst->canBePutAt(putTo, dst.slot))
	{
		PutArtifact pa(id, dst, askAssemble.value());
		sendAndApply(pa);
		return true;
	}
	else
	{
		return false;
	}
}

bool CGameHandler::giveHeroNewArtifact(
	const CGHeroInstance * h, const CArtifact * artType, const SpellID & spellId, const ArtifactPosition & pos)
{
	assert(artType);

	NewArtifact na;
	na.artHolder = h->id;
	na.artId = artType->getId();
	na.spellId = spellId;
	na.pos = pos;

	if(pos == ArtifactPosition::FIRST_AVAILABLE)
	{
		na.pos = ArtifactUtils::getArtAnyPosition(h, artType->getId());
		if(!artType->canBePutAt(h, na.pos))
			COMPLAIN_RET("Cannot put artifact in that slot!");
	}
	else if(ArtifactUtils::isSlotBackpack(pos))
	{
		if(!artType->canBePutAt(h, ArtifactUtils::getArtBackpackPosition(h, artType->getId())))
			COMPLAIN_RET("Cannot put artifact in that slot!");
	}
	else
	{
		COMPLAIN_RET_FALSE_IF(!artType->canBePutAt(h, pos, false), "Cannot put artifact in that slot!");
	}
	sendAndApply(na);
	return true;
}

bool CGameHandler::giveHeroNewArtifact(const CGHeroInstance * h, const ArtifactID & artId, const ArtifactPosition & pos)
{
	return giveHeroNewArtifact(h, artId.toArtifact(), SpellID::NONE, pos);
}

bool CGameHandler::giveHeroNewScroll(const CGHeroInstance * h, const SpellID & spellId, const ArtifactPosition & pos)
{
	return giveHeroNewArtifact(h, ArtifactID(ArtifactID::SPELL_SCROLL).toArtifact(), spellId, pos);
}

void CGameHandler::spawnWanderingMonsters(CreatureID creatureID)
{
	std::vector<int3>::iterator tile;
	std::vector<int3> tiles;
	getFreeTiles(tiles);
	ui32 amount = (ui32)tiles.size() / 200; //Chance is 0.5% for each tile

	RandomGeneratorUtil::randomShuffle(tiles, getRandomGenerator());
	logGlobal->trace("Spawning wandering monsters. Found %d free tiles. Creature type: %d", tiles.size(), creatureID.num);
	const CCreature *cre = creatureID.toCreature();
	for (int i = 0; i < (int)amount; ++i)
	{
		tile = tiles.begin();
		logGlobal->trace("\tSpawning monster at %s", tile->toString());
		{
			auto count = cre->getRandomAmount(std::rand);

			createWanderingMonster(*tile, creatureID);
			auto monsterId = getTopObj(*tile)->id;

			setObjPropertyValue(monsterId, ObjProperty::MONSTER_COUNT, count);
			setObjPropertyValue(monsterId, ObjProperty::MONSTER_POWER, (si64)1000*count);
		}
		tiles.erase(tile); //not use it again
	}
}

void CGameHandler::synchronizeArtifactHandlerLists()
{
	UpdateArtHandlerLists uahl;
	uahl.allocatedArtifacts = gs->allocatedArtifacts;
	sendAndApply(uahl);
}

bool CGameHandler::isValidObject(const CGObjectInstance *obj) const
{
	return vstd::contains(gs->getMap().objects, obj);
}

bool CGameHandler::isBlockedByQueries(const CPackForServer *pack, PlayerColor player)
{
	if (dynamic_cast<const PlayerMessage *>(pack) != nullptr)
		return false;

	if (dynamic_cast<const SaveLocalState *>(pack) != nullptr)
		return false;

	auto query = queries->topQuery(player);
	if (query && query->blocksPack(pack))
	{
		complain(boost::str(boost::format(
			"\r\n| Player \"%s\" has to answer queries before attempting any further actions.\r\n| Top Query: \"%s\"\r\n")
			% boost::to_upper_copy<std::string>(player.toString())
			% query->toString()
		));
		return true;
	}

	return false;
}

void CGameHandler::removeAfterVisit(const CGObjectInstance *object)
{
	//If the object is being visited, there must be a matching query
	for (const auto &query : queries->allQueries())
	{
		if (auto someVistQuery = std::dynamic_pointer_cast<MapObjectVisitQuery>(query))
		{
			if (someVistQuery->visitedObject == object)
			{
				someVistQuery->removeObjectAfterVisit = true;
				return;
			}
		}
	}

	//If we haven't returned so far, there is no query and no visit, call was wrong
	assert("This function needs to be called during the object visit!");
}

void CGameHandler::changeFogOfWar(int3 center, ui32 radius, PlayerColor player, ETileVisibility mode)
{
	std::unordered_set<int3> tiles;

	if (mode == ETileVisibility::HIDDEN)
	{
		getTilesInRange(tiles, center, radius, ETileVisibility::REVEALED, player);
	}
	else
	{
		getTilesInRange(tiles, center, radius, ETileVisibility::HIDDEN, player);
	}
	changeFogOfWar(tiles, player, mode);
}

void CGameHandler::changeFogOfWar(const std::unordered_set<int3> &tiles, PlayerColor player, ETileVisibility mode)
{
	if (tiles.empty())
		return;

	FoWChange fow;
	fow.tiles = tiles;
	fow.player = player;
	fow.mode = mode;

	if (mode == ETileVisibility::HIDDEN)
	{
		// do not hide tiles observed by owned objects. May lead to disastrous AI problems
		// FIXME: this leads to a bug - shroud of darkness from Necropolis does can not override Skyship from Tower
		std::unordered_set<int3> observedTiles;
		auto p = getPlayerState(player);
		for (auto obj : p->getOwnedObjects())
			getTilesInRange(observedTiles, obj->getSightCenter(), obj->getSightRadius(), ETileVisibility::REVEALED, obj->getOwner());

		for (auto tile : observedTiles)
			vstd::erase_if_present (fow.tiles, tile);
	}

	if (!fow.tiles.empty())
		sendAndApply(fow);
}

const CGHeroInstance * CGameHandler::getVisitingHero(const CGObjectInstance *obj)
{
	assert(obj);

	for(const auto & query : queries->allQueries())
	{
		auto visit = std::dynamic_pointer_cast<const VisitQuery>(query);
		if (visit && visit->visitedObject == obj)
			return visit->visitingHero;
	}
	return nullptr;
}

const CGObjectInstance * CGameHandler::getVisitingObject(const CGHeroInstance *hero)
{
	assert(hero);

	for(const auto & query : queries->allQueries())
	{
		auto visit = std::dynamic_pointer_cast<const VisitQuery>(query);
		if (visit && visit->visitingHero == hero)
			return visit->visitedObject;
	}
	return nullptr;
}

bool CGameHandler::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	assert(obj);
	assert(hero);
	assert(getVisitingHero(obj) == hero);
	// Check top query of targeted player:
	// If top query is NOT visit to targeted object then we assume that
	// visitation query is covered by other query that must be answered first

	if (auto topQuery = queries->topQuery(hero->getOwner()))
		if (auto visit = std::dynamic_pointer_cast<const VisitQuery>(topQuery))
			return !(visit->visitedObject == obj && visit->visitingHero == hero);

	return true;
}

void CGameHandler::setObjPropertyValue(ObjectInstanceID objid, ObjProperty prop, int32_t value)
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.identifier = NumericID(value);
	sendAndApply(sob);
}

void CGameHandler::setObjPropertyID(ObjectInstanceID objid, ObjProperty prop, ObjPropertyID identifier)
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.identifier = identifier;
	sendAndApply(sob);
}

void CGameHandler::setRewardableObjectConfiguration(ObjectInstanceID objid, const Rewardable::Configuration & configuration)
{
	SetRewardableConfiguration srb;
	srb.objectID = objid;
	srb.configuration = configuration;
	sendAndApply(srb);
}

void CGameHandler::setRewardableObjectConfiguration(ObjectInstanceID townInstanceID, BuildingID buildingID, const Rewardable::Configuration & configuration)
{
	SetRewardableConfiguration srb;
	srb.objectID = townInstanceID;
	srb.buildingID = buildingID;
	srb.configuration = configuration;
	sendAndApply(srb);
}

void CGameHandler::showInfoDialog(InfoWindow * iw)
{
	sendAndApply(*iw);
}

vstd::RNG & CGameHandler::getRandomGenerator()
{
	return *randomNumberGenerator;
}

#if SCRIPTING_ENABLED
scripting::Pool * CGameHandler::getGlobalContextPool() const
{
	return serverScripts.get();
}

//scripting::Pool * CGameHandler::getContextPool() const
//{
//	return serverScripts.get();
//}
#endif


CGObjectInstance * CGameHandler::createNewObject(const int3 & visitablePosition, MapObjectID objectID, MapObjectSubID subID)
{
	TerrainId terrainType = ETerrainId::NONE;

	if (!gs->isInTheMap(visitablePosition))
		throw std::runtime_error("Attempt to create object outside map at " + visitablePosition.toString());

	const TerrainTile & t = gs->getMap().getTile(visitablePosition);
	terrainType = t.getTerrainID();

	auto handler = LIBRARY->objtypeh->getHandlerFor(objectID, subID);

	CGObjectInstance * o = handler->create(gs->callback, nullptr);
	handler->configureObject(o, getRandomGenerator());
	assert(o->ID == objectID);

	assert(!handler->getTemplates(terrainType).empty());
	if (handler->getTemplates().empty())
		throw std::runtime_error("Attempt to create object (" + std::to_string(objectID) + ", " + std::to_string(subID.getNum()) + ") with no templates!");

	if (!handler->getTemplates(terrainType).empty())
		o->appearance = handler->getTemplates(terrainType).front();
	else
		o->appearance = handler->getTemplates().front();

	if (o->isVisitable())
		o->setAnchorPos(visitablePosition + o->getVisitableOffset());
	else
		o->setAnchorPos(visitablePosition);

	return o;
}

void CGameHandler::createWanderingMonster(const int3 & visitablePosition, CreatureID creature)
{
	auto createdObject = createNewObject(visitablePosition, Obj::MONSTER, creature);

	auto * cre = dynamic_cast<CGCreature *>(createdObject);
	assert(cre);
	cre->notGrowingTeam = cre->neverFlees = false;
	cre->character = 2;
	cre->gainedArtifact = ArtifactID::NONE;
	cre->identifier = -1;
	cre->addToSlot(SlotID(0), new CStackInstance(creature, -1)); //add placeholder stack

	newObject(createdObject, PlayerColor::NEUTRAL);
}

void CGameHandler::createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator)
{
	auto createdObject = createNewObject(visitablePosition, Obj::BOAT, type);
	newObject(createdObject, initiator);
}

void CGameHandler::createHole(const int3 & visitablePosition, PlayerColor initiator)
{
	auto createdObject = createNewObject(visitablePosition, Obj::HOLE, 0);
	newObject(createdObject, initiator);
}

void CGameHandler::newObject(CGObjectInstance * object, PlayerColor initiator)
{
	object->initObj(gs->getRandomGenerator());

	NewObject no;
	no.newObject = object;
	no.initiator = initiator;
	sendAndApply(no);
}

void CGameHandler::startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town)
{
	battles->startBattle(army1, army2, tile, hero1, hero2, layout, town);
}

void CGameHandler::startBattle(const CArmedInstance *army1, const CArmedInstance *army2 )
{
	battles->startBattle(army1, army2);
}
