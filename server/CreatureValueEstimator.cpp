/*
 * CreatureValueEstimator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CreatureValueEstimator.h"

#include "../lib/CCreatureHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CSkillHandler.h"
#include "../lib/CStack.h"
#include "../lib/GameLibrary.h"
#include "../lib/LoadProgress.h"
#include "../lib/StartInfo.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/battle/BattleLayout.h"
#include "../lib/battle/CUnitState.h"
#include "../lib/battle/CombatValue.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/spells/ISpellMechanics.h"
#include "../lib/bonuses/Bonus.h"
#include "../lib/callback/GameRandomizer.h"
#include "../lib/entities/hero/CHero.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/modding/IdentifierStorage.h"
#include "../lib/modding/ModScope.h"
#include "../lib/networkPacks/PacksForClientBattle.h"
#include "../lib/networkPacks/SetStackEffect.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/MapEditUtils.h"
#include "../lib/mapping/MapFormat.h"

void CreatureValueEstimator::run()
{
	CreatureValueEstimator estimator;

	estimator.startGame();
	estimator.startBattle();
	estimator.collectSubjects();
	estimator.measureDamage();
	estimator.deriveValues();
	estimator.applyFormula();
	estimator.checkModelPaths();
	estimator.report();
}

/// The smallest game a battle can happen in: bare terrain and one hero per side. Built here rather
/// than generated, since a random map takes minutes and depends on templates of loaded mods
class CreatureValueEstimator::BattlegroundMapService final : public IMapService
{
public:
	std::unique_ptr<CMap> loadMap(const ResourcePath &, IGameInfoCallback * cb) const override
	{
		auto map = std::make_unique<CMap>(cb);

		map->version = EMapFormat::VCMI;
		map->creationDateTime = std::time(nullptr);
		map->width = mapSize;
		map->height = mapSize;
		map->mapLayers = {MapLayerId::SURFACE};
		map->name.appendRawString("Battleground");
		map->initTerrain();
		// sand is native to no faction, so nobody gains the native terrain bonus
		map->getEditManager()->getTerrainSelection().selectAll();
		map->getEditManager()->drawTerrain(ETerrainId::SAND, 0, &CRandomGenerator::getDefault());

		for(int index = 0; index < 2; ++index)
		{
			PlayerInfo & player = map->players.at(index);
			player.canHumanPlay = true;
			player.canComputerPlay = true;
			player.isFactionRandom = true;

			placeHero(*map, cb, PlayerColor(index), HeroTypeID(index), int3(2 + index * 4, 2, 0));
		}

		return map;
	}

	std::unique_ptr<CMapHeader> loadMapHeader(const ResourcePath &, bool = false) const override { return nullptr; }
	std::unique_ptr<CMap> loadMap(const ui8 *, int, const std::string &, const std::string &, const std::string &, IGameInfoCallback *) const override { return nullptr; }
	std::unique_ptr<CMapHeader> loadMapHeader(const ui8 *, int, const std::string &, const std::string &, const std::string &) const override { return nullptr; }
	void saveMap(const std::unique_ptr<CMap> &, boost::filesystem::path) const override {}

private:
	static constexpr int mapSize = 12;

	static void placeHero(CMap & map, IGameInfoCallback * cb, const PlayerColor & owner, const HeroTypeID & type, const int3 & position)
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, type.toHeroType()->heroClass->getIndex());
		auto hero = std::dynamic_pointer_cast<CGHeroInstance>(handler->create(cb, handler->getTemplates().front()));

		hero->ID = Obj::HERO;
		hero->setHeroType(type);
		hero->tempOwner = owner;
		hero->setAnchorPos(position + hero->getVisitableOffset());

		map.getEditManager()->insertObject(hero);
	}
};

void CreatureValueEstimator::startGame()
{
	logGlobal->info("Preparing a game to measure creatures in");

	StartInfo startInfo;
	startInfo.mode = EStartMode::NEW_GAME;
	startInfo.difficulty = static_cast<ui8>(EMapDifficulty::EASY);
	startInfo.mapname = "battleground";

	for(int index = 0; index < 2; ++index)
	{
		PlayerSettings & settings = startInfo.playerInfos[PlayerColor(index)];
		settings.color = PlayerColor(index);
		settings.name = "Player";
		settings.bonus = PlayerStartingBonus::GOLD; // no random starting artifact
	}

	mapService = std::make_unique<BattlegroundMapService>();

	gameState = std::make_shared<CGameState>();
	gameState->preInit(LIBRARY);

	GameRandomizer randomizer(*gameState);
	Load::ProgressAccumulator progressTracker;
	gameState->init(mapService.get(), &startInfo, randomizer, progressTracker, false);

	for(auto * hero : gameState->getMap().getObjects<CGHeroInstance>())
	{
		if(hero->getOwner() == PlayerColor(0) && attackerSideHero == nullptr)
			attackerSideHero = hero;
		if(hero->getOwner() == PlayerColor(1) && defenderSideHero == nullptr)
			defenderSideHero = hero;
	}

	if(attackerSideHero == nullptr || defenderSideHero == nullptr)
		throw std::runtime_error("Battleground map has no hero for one of its two players");

	stripHeroBonuses(attackerSideHero);
	stripHeroBonuses(defenderSideHero);
}

void CreatureValueEstimator::stripHeroBonuses(CGHeroInstance * hero)
{
	// starting army would join the battle next to the measured pair, carrying battle-wide bonuses
	hero->clearSlots();

	for(const auto & bonus : hero->getHeroType()->specialty)
		hero->removeBonus(bonus);

	for(int index = 0; index < LIBRARY->skillh->size(); ++index)
		hero->setSecSkillLevel(SecondarySkill(index), 0, ChangeValueMode::ABSOLUTE);

	for(auto skill : {PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE})
		hero->setPrimarySkill(skill, 0, ChangeValueMode::ABSOLUTE);
}

void CreatureValueEstimator::startBattle()
{
	BattleSideArray<const CGHeroInstance *> heroes = {attackerSideHero, defenderSideHero};
	BattleSideArray<const CArmedInstance *> armies = {attackerSideHero, defenderSideHero};

	const int3 tile = attackerSideHero->anchorPos();
	const auto terrain = gameState->getTile(tile)->getTerrainID();
	const BattleLayout layout = BattleLayout::createDefaultLayout(*gameState, attackerSideHero, defenderSideHero);

	// default battlefield is a clover field, whose luck bonus would skew measured damage
	const BattleField battlefield(LIBRARY->identifiers()->getIdentifier(ModScope::scopeBuiltin(), "battlefield.sand_shore").value());

	BattleStart pack;
	pack.info = BattleInfo::setupBattle(gameState.get(), tile, terrain, battlefield, armies, heroes, layout, nullptr);
	pack.battleID = BattleID(0);
	gameState->apply(pack);

	battle()->tacticDistance = 0;
	// obstacles are placed at random, and a mine under a unit would show up as damage
	battle()->obstacles.clear();
}

BattleInfo * CreatureValueEstimator::battle() const
{
	return gameState->currentBattles.front().get();
}

void CreatureValueEstimator::collectSubjects()
{
	std::vector<const CCreature *> modded;

	for(const auto & creature : LIBRARY->creh->objects)
	{
		if(creature->special)
			continue;

		if(creature->getModScope() == ModScope::scopeBuiltin())
			subjects.push_back(creature.get());
		else
			modded.push_back(creature.get());
	}

	baselineCount = subjects.size();
	vstd::concatenate(subjects, modded);

	if(baselineCount < 100)
		throw std::runtime_error("Creatures of the original game failed to load - nothing to measure against");

	logGlobal->info("Measuring %d creatures against a baseline of %d", static_cast<int>(subjects.size()), static_cast<int>(baselineCount));
}

CStack * CreatureValueEstimator::placeStack(BattleSide side, const CCreature * creature, const BattleHex & hex)
{
	return placeStack(side, creature, hex, std::max(1, stackHitPoints / std::max(1, creature->getBaseHitPoints())));
}

CStack * CreatureValueEstimator::placeStack(BattleSide side, const CCreature * creature, const BattleHex & hex, int count)
{
	battle::UnitInfo info;
	info.id = battle()->battleNextUnitId();
	info.count = count;
	info.type = creature->getId();
	info.side = side;
	info.position = hex;
	info.summoned = false;

	BattleUnitsChanged pack;
	pack.battleID = BattleID(0);
	pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
	info.save(pack.changedStacks.back().data);
	gameState->apply(pack);

	return battle()->getStack(info.id);
}

void CreatureValueEstimator::removeStack(const CStack * stack)
{
	BattleUnitsChanged pack;
	pack.battleID = BattleID(0);
	pack.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
	gameState->apply(pack);

	// battle keeps removed units around so they can be raised, but this code makes far too many
	std::erase_if(battle()->stacks, [stack](const std::unique_ptr<CStack> & entry) { return entry.get() == stack; });
}

void CreatureValueEstimator::measurePair(const CStack * attacker, const CStack * defender, double & dealt, double & retaliated) const
{
	// jousting scales with distance covered, which is at most one turn of movement
	const int charge = std::min<int>(CombatValue::startingDistance(), attacker->getMovementRange());

	DamageEstimation retaliation;
	const DamageEstimation damage = battle()->battleEstimateDamage(attacker, defender, charge, &retaliation);

	dealt = (damage.damage.min + damage.damage.max) / 2.0 / attacker->getCount();
	retaliated = (retaliation.damage.min + retaliation.damage.max) / 2.0 / defender->getCount();
}

void CreatureValueEstimator::measureDamage()
{
	const size_t count = subjects.size();

	dealtBySubject.assign(count, std::vector<double>(baselineCount, 0.0));
	retaliatedByOpponent.assign(count, std::vector<double>(baselineCount, 0.0));
	dealtOnSubject.assign(count, std::vector<double>(baselineCount, 0.0));
	retaliatedBySubject.assign(count, std::vector<double>(baselineCount, 0.0));

	for(size_t subject = 0; subject < count; ++subject)
	{
		const CStack * attacker = placeStack(BattleSide::ATTACKER, subjects[subject], BattleHex(attackerHex));

		for(size_t opponent = 0; opponent < baselineCount; ++opponent)
		{
			const CStack * defender = placeStack(BattleSide::DEFENDER, subjects[opponent], BattleHex(defenderHex));

			measurePair(attacker, defender, dealtBySubject[subject][opponent], retaliatedByOpponent[subject][opponent]);

			// baseline creatures are attackers in their own iteration, so only modded ones need both
			if(subject >= baselineCount)
				measurePair(defender, attacker, dealtOnSubject[subject][opponent], retaliatedBySubject[subject][opponent]);

			removeStack(defender);
		}

		removeStack(attacker);
	}

	for(size_t subject = 0; subject < baselineCount; ++subject)
	{
		for(size_t opponent = 0; opponent < baselineCount; ++opponent)
		{
			dealtOnSubject[subject][opponent] = dealtBySubject[opponent][subject];
			retaliatedBySubject[subject][opponent] = retaliatedByOpponent[opponent][subject];
		}
	}
}

void CreatureValueEstimator::deriveValues()
{
	const size_t count = subjects.size();
	entries.resize(count);

	std::vector<double> rawOutput(count, 0.0);
	std::vector<double> survival(count, 1.0);

	// a live unit is used so that abilities are read through the bonus system, as in a real battle
	for(size_t subject = 0; subject < count; ++subject)
	{
		const CStack * unit = placeStack(BattleSide::ATTACKER, subjects[subject], BattleHex(attackerHex));

		const auto sum = [](const std::vector<double> & row) { return std::accumulate(row.begin(), row.end(), 0.0); };
		const double ownTurn = sum(dealtBySubject[subject]) / baselineCount * CombatValue::attacksPerRound(*unit) * CombatValue::targetsPerAttack(*unit);
		const double retaliations = sum(retaliatedBySubject[subject]) / baselineCount * CombatValue::retaliationsPerRound(*unit);

		rawOutput[subject] = std::max(ownTurn + retaliations, 1e-6);
		survival[subject] = CombatValue::survivalMultiplier(*unit) * CombatValue::situationalSurvival(*unit, values.averageBattle());

		Entry & entry = entries[subject];
		entry.creature = subjects[subject];
		entry.baseline = subject < baselineCount;
		entry.output = rawOutput[subject] * CombatValue::offenseMultiplier(*unit) * CombatValue::situationalOffense(*unit, values.averageBattle());
		entry.uptime = CombatValue::uptimeOf(*unit);

		removeStack(unit);
	}

	for(size_t subject = 0; subject < count; ++subject)
	{
		// damage taken relative to what the same attackers deal on average
		double vulnerability = 0;
		for(size_t opponent = 0; opponent < baselineCount; ++opponent)
			vulnerability += (dealtOnSubject[subject][opponent] + retaliatedByOpponent[subject][opponent]) / rawOutput[opponent];

		vulnerability = std::max(vulnerability / baselineCount, 1e-6);

		const double hitPoints = subjects[subject]->getBaseHitPoints()
			+ CombatValue::regeneratedHitPoints(*subjects[subject], CombatValue::referenceCount(subjects[subject]));

		Entry & entry = entries[subject];
		entry.effectiveHitPoints = hitPoints / vulnerability * survival[subject];
		entry.fightValue = CombatValue::combine(entry.output, entry.effectiveHitPoints, 1.0);
		entry.aiValue = entry.fightValue * entry.uptime;
	}

	// computed values have an arbitrary scale, so they are pinned to H3 values
	std::vector<double> fightScales;
	std::vector<double> aiScales;
	for(const auto & entry : entries)
	{
		if(!entry.baseline)
			continue;

		if(entry.fightValue > 0 && entry.creature->getFightValue() > 0)
			fightScales.push_back(entry.creature->getFightValue() / entry.fightValue);
		if(entry.aiValue > 0 && entry.creature->getAIValue() > 0)
			aiScales.push_back(entry.creature->getAIValue() / entry.aiValue);
	}

	if(fightScales.empty() || aiScales.empty())
		throw std::runtime_error("No creature of the original game declares a value to pin the scale to");

	const double scaleFight = CombatValue::median(fightScales);
	const double scaleAi = CombatValue::median(aiScales);

	for(auto & entry : entries)
	{
		entry.fightValue *= scaleFight;
		entry.aiValue *= scaleAi;
	}
}

void CreatureValueEstimator::applyFormula()
{
	for(auto & entry : entries)
	{
		// stack size that CombatValue itself assumes, so the two results are comparable
		const CStack * unit = placeStack(BattleSide::ATTACKER, entry.creature, BattleHex(attackerHex), CombatValue::referenceCount(entry.creature));

		// CombatValue returns an AI value, which is a fight value weighted by uptime
		entry.formulaAiValue = values.getAIValue(unit);
		entry.formulaFightValue = entry.formulaAiValue / CombatValue::uptimeOf(*unit);

		removeStack(unit);
	}

	// CombatValue pins only AI value to H3 scale; fight value is a diagnostic, pinned here instead
	std::vector<double> scales;
	for(const auto & entry : entries)
		if(entry.baseline && entry.formulaFightValue > 0 && entry.creature->getFightValue() > 0)
			scales.push_back(entry.creature->getFightValue() / entry.formulaFightValue);

	if(scales.empty())
		throw std::runtime_error("No creature of the original game declares a fight value to pin the scale to");

	const double scale = CombatValue::median(scales);

	for(auto & entry : entries)
		entry.formulaFightValue *= scale;
}

void CreatureValueEstimator::checkModelPaths()
{
	std::vector<double> ratios;
	int closer = 0;

	// heroes were stripped of their armies, so this is the only unit to approach
	const CStack * enemy = placeStack(BattleSide::DEFENDER, subjects.front(), BattleHex(defenderHex));

	for(const auto & entry : entries)
	{
		if(!entry.baseline)
			continue;

		// stack size that CombatValue itself assumes, so the two query paths are comparable
		const CStack * unit = placeStack(BattleSide::ATTACKER, entry.creature, BattleHex(attackerHex), CombatValue::referenceCount(entry.creature));

		const int64_t fromConfiguration = values.getAIValue(entry.creature);
		const int64_t asUnit = values.getAIValue(unit);
		const int64_t inBattle = values.getAIValue(unit, *battle());

		if(fromConfiguration > 0)
			ratios.push_back(static_cast<double>(asUnit) / fromConfiguration);

		// regeneration is the one effect whose worth per creature could depend on how many there are
		if(entry.creature->hasBonusOfType(BonusType::HP_REGENERATION))
		{
			const CStack * few = placeStack(BattleSide::ATTACKER, entry.creature, BattleHex(attackerHex + 1), 5);
			const CStack * many = placeStack(BattleSide::ATTACKER, entry.creature, BattleHex(attackerHex + 2), 200);

			if(values.getAIValue(few) != values.getAIValue(many))
				logGlobal->error("%s is worth a different amount per creature in a small stack than in a large one",
					entry.creature->getJsonKey());

			removeStack(few);
			removeStack(many);
		}

		// enemy stands closer than battle sides normally start, so approaching creatures gain here
		if(inBattle > asUnit)
			++closer;

		removeStack(unit);
	}

	removeStack(enemy);

	if(ratios.empty())
		throw std::runtime_error("No creature of the original game declares a value to check the model against");

	const auto range = std::ranges::minmax(ratios);

	logGlobal->info("Model paths: a creature answered for as a unit is worth %f of what it is worth answered for out of its configuration (%f to %f); %d of %d are worth more once the enemy is in reach",
		CombatValue::median(ratios), range.min, range.max, closer, static_cast<int>(ratios.size()));

	// only creatures that cross the field in one turn are unaffected, and there are few of those
	if(closer == 0)
		logGlobal->error("No creature gained from the enemy being in reach - the battle has nothing to close on");
}

void CreatureValueEstimator::report() const
{
	logGlobal->info("creature,fightValue,derivedFightValue,formulaFightValue,aiValue,derivedAiValue,formulaAiValue");

	int agreed = 0;
	int differed = 0;
	int disagreed = 0;
	int unjudged = 0;

	// how far apart the two values are, whichever way round they sit
	const auto divergence = [](double derived, double declared)
	{
		if(derived <= 0 || declared <= 0)
			return std::numeric_limits<double>::infinity();

		return std::max(derived / declared, declared / derived);
	};

	for(const auto & entry : entries)
	{
		const int64_t derivedFight = std::llround(entry.fightValue);
		const int64_t derivedAi = std::llround(entry.aiValue);

		const std::string line = boost::str(boost::format("%s,%d,%d,%d,%d,%d,%d")
			% entry.creature->getJsonKey()
			% entry.creature->getFightValue()
			% derivedFight
			% std::llround(entry.formulaFightValue)
			% entry.creature->getAIValue()
			% derivedAi
			% std::llround(entry.formulaAiValue));

		// creature with no configured value can only be reported, not judged
		if(entry.creature->getFightValue() == 0 || entry.creature->getAIValue() == 0)
		{
			logGlobal->warn("%s", line);
			++unjudged;
			continue;
		}

		const double error = std::max(divergence(entry.fightValue, entry.creature->getFightValue()),
			divergence(entry.aiValue, entry.creature->getAIValue()));

		if(error <= agreementTolerance)
		{
			logGlobal->info("%s", line);
			++agreed;
		}
		else if(error <= disagreementTolerance)
		{
			logGlobal->warn("%s", line);
			++differed;
		}
		else
		{
			logGlobal->error("%s", line);
			++disagreed;
		}
	}

	logGlobal->info("%d creatures within a tenth of their declared value, %d further off, %d beyond half or double it, %d declaring no value to judge them by",
		agreed, differed, disagreed, unjudged);
}

/// Nothing watches this battle, so a cast only has to reach the game state
class CreatureValueEstimator::LocalSpellEnvironment final : public ServerCallback
{
public:
	explicit LocalSpellEnvironment(CGameState * gameState)
		: gameState(gameState)
	{}

	void complain(const std::string & problem) override { logGlobal->warn("Spell cast complained: %s", problem); }
	bool describeChanges() const override { return false; }
	vstd::RNG * getRNG() override { return &CRandomGenerator::getDefault(); }

	bool rollCombatAbility(const IBattleInfoCallback &, const battle::Unit &, int percentageChance) override
	{
		return percentageChance >= 100;
	}

	void apply(CPackForClient & pack) override { gameState->apply(pack); }
	void apply(BattleLogMessage & pack) override { gameState->apply(pack); }
	void apply(BattleStackMoved & pack) override { gameState->apply(pack); }
	void apply(BattleUnitsChanged & pack) override { gameState->apply(pack); }
	void apply(SetStackEffect & pack) override { gameState->apply(pack); }
	void apply(StacksInjured & pack) override { gameState->apply(pack); }
	void apply(BattleObstaclesChanged & pack) override { gameState->apply(pack); }
	void apply(CatapultAttack & pack) override { gameState->apply(pack); }

private:
	CGameState * gameState;
};

std::vector<const CCreature *> CreatureValueEstimator::archetypes()
{
	const CCreature * walker = nullptr;
	const CCreature * shooter = nullptr;
	const CCreature * flyer = nullptr;

	for(const auto & creature : LIBRARY->creh->objects)
	{
		if(creature->special || creature->getModScope() != ModScope::scopeBuiltin())
			continue;

		if(creature->hasBonusOfType(BonusType::SHOOTER))
			shooter = shooter ? shooter : creature.get();
		else if(creature->hasBonusOfType(BonusType::FLYING))
			flyer = flyer ? flyer : creature.get();
		else
			walker = walker ? walker : creature.get();
	}

	std::vector<const CCreature *> chosen;
	for(const auto * creature : {walker, shooter, flyer})
		if(creature)
			chosen.push_back(creature);

	return chosen;
}

void CreatureValueEstimator::measureSpells()
{
	static constexpr std::array<const char *, 4> masteryNames = {"none", "basic", "advanced", "expert"};

	std::vector<SecondarySkill> schools;
	for(auto skill : {SecondarySkill::AIR_MAGIC, SecondarySkill::FIRE_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC})
		schools.emplace_back(skill);

	// the hero was stripped bare to keep it out of the damage measurements, but a spell needs some
	// power behind it to have any effect worth reading
	attackerSideHero->setPrimarySkill(PrimarySkill::SPELL_POWER, spellPower, ChangeValueMode::ABSOLUTE);

	LocalSpellEnvironment environment(gameState.get());
	std::vector<std::string> unseen;
	std::vector<std::string> unreached;
	int measured = 0;

	logGlobal->info("spell,mastery,creature,before,after,ratio,bonusesAdded");

	for(const auto & spell : LIBRARY->spellh->objects)
	{
		// only spells that alter a unit can be judged by what they do to its worth; damage is already
		// what the model is built on, and the special ones belong to obstacles and town moats
		if(!spell->isCombat() || spell->isOffensive() || spell->isDamage() || spell->isSpecial())
			continue;

		bool everSeen = false;
		bool everApplied = false;

		for(int mastery = 0; mastery < static_cast<int>(masteryNames.size()); ++mastery)
		{
			for(auto skill : schools)
				attackerSideHero->setSecSkillLevel(skill, mastery, ChangeValueMode::ABSOLUTE);

			spells::BattleCast probe(battle(), attackerSideHero, spells::Mode::HERO, spell.get());
			const auto probeMechanics = spell->battleMechanics(&probe);

			// the target built below names one point, so a spell wanting a second one - a hex to
			// teleport to, a unit to sacrifice - can not be cast here
			const auto aimTypes = probeMechanics->getTargetTypes();
			if(aimTypes.size() != 1)
				continue;

			// a hostile spell only reaches the other side of the battle
			const bool hostile = probeMechanics->isNegativeSpell();
			const BattleSide side = hostile ? BattleSide::DEFENDER : BattleSide::ATTACKER;
			const BattleHex hex(hostile ? defenderHex : attackerHex);

			for(const auto * creature : archetypes())
			{
				CStack * unit = placeStack(side, creature, hex, CombatValue::referenceCount(creature));

				const int64_t before = values.getAIValue(unit);
				const auto bonusesBefore = unit->getAllBonuses(Selector::all)->size();

				spells::BattleCast cast(battle(), attackerSideHero, spells::Mode::HERO, spell.get());

				// a spell reaches its target through the aim point it asks for, and ignores any other
				spells::Target target(1, aimTypes.front() == spells::AimType::LOCATION
					? spells::Destination(unit->getPosition())
					: spells::Destination(unit));

				cast.castEval(&environment, target);

				const int64_t after = values.getAIValue(unit);
				const auto added = unit->getAllBonuses(Selector::all)->size() - bonusesBefore;

				if(added > 0)
				{
					everApplied = true;
					++measured;
					if(after != before)
						everSeen = true;

					logGlobal->info("%s,%s,%s,%d,%d,%f,%d", spell->getJsonKey(), masteryNames[mastery],
						creature->getJsonKey(), before, after, static_cast<double>(after) / std::max<int64_t>(1, before), added);
				}

				removeStack(unit);

				// summoning and cloning leave stacks behind that would take part in the next cast
				for(const auto * leftover : battle()->battleGetAllStacks(true))
					removeStack(leftover);
			}
		}

		// a spell that changes a unit without changing what it is worth is one the AI can not see
		if(everApplied && !everSeen)
			unseen.push_back(spell->getJsonKey());

		if(!everApplied)
			unreached.push_back(spell->getJsonKey());
	}

	for(auto skill : schools)
		attackerSideHero->setSecSkillLevel(skill, 0, ChangeValueMode::ABSOLUTE);

	if(!unreached.empty())
		logGlobal->info("%d spells never reached a single unit and are not measured here: %s",
			static_cast<int>(unreached.size()), boost::algorithm::join(unreached, ", "));

	if(measured == 0)
		logGlobal->error("No spell reached a unit at all - nothing was measured");
	else if(unseen.empty())
		logGlobal->info("Each of the %d spell effects applied also changed what its bearer is worth", measured);
	else
		logGlobal->error("%d spells change a unit without changing what it is worth: %s",
			static_cast<int>(unseen.size()), boost::algorithm::join(unseen, ", "));
}

void CreatureValueEstimator::runSpells()
{
	CreatureValueEstimator estimator;

	estimator.startGame();
	estimator.startBattle();
	estimator.measureSpells();
}
