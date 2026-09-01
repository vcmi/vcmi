/*
 * BattleTestFixture.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "mock/TinyH3MBuilder.h"

#include "../../../server/CGameHandler.h"
#include "../../../server/battles/BattleProcessor.h"

#include "../../../lib/CSkillHandler.h"
#include "../../../lib/CStack.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/StartInfo.h"
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/battle/BattleLayout.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/callback/GameRandomizer.h"
#include "../../../lib/entities/hero/CHero.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/Problem.h"

void RecordingGameServer::applyPack(CPackForClient & pack)
{
	record(pack);
	gameState->apply(pack);
}

/// A cast arrives as an announcement, then the damage it did, then the log it printed. Anything
/// belonging to the next attack closes the window again.
void RecordingGameServer::record(CPackForClient & pack)
{
	if(dynamic_cast<const BattleAttack *>(&pack))
	{
		recording = false;
		return;
	}

	if(const auto * announcement = dynamic_cast<const BattleSpellCast *>(&pack))
	{
		casts.push_back(RecordedCast{*announcement, 0, 0, {}});
		recording = true;
		return;
	}

	if(!recording)
		return;

	if(const auto * log = dynamic_cast<const BattleLogMessage *>(&pack))
	{
		for(const auto & line : log->lines)
			casts.back().logLines.push_back(line.toString(LIBRARY->staticTexts()));
		return;
	}

	if(const auto * injured = dynamic_cast<const StacksInjured *>(&pack))
	{
		for(const auto & stack : injured->stacks)
		{
			casts.back().damage += stack.damageAmount;
			casts.back().killed += stack.killedAmount;
		}
	}
}

std::vector<RecordedCast> RecordingGameServer::castsOf(const SpellID & spell) const
{
	std::vector<RecordedCast> result;

	for(const auto & cast : casts)
		if(cast.announcement.spellID == spell)
			result.push_back(cast);

	return result;
}

void BattleTestFixture::TearDown()
{
	// the handler holds on to the game state, so it has to go before the state does
	gameHandler.reset();
	TinyMapGameTest::TearDown();
}

Services * BattleTestFixture::gameServices()
{
	return LIBRARY;
}

void BattleTestFixture::configurePlayer(PlayerSettings & settings) const
{
	settings.bonus = PlayerStartingBonus::GOLD; // no random starting artifact
}

void BattleTestFixture::startGame()
{
	const CreatureID token(0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("BattleTest")
		.playerActive(PlayerColor(0))
		.playerActive(PlayerColor(1))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0)).heroGarrison({{token, 1}})
		.hero({7, 7, 0}, HeroTypeID(1), PlayerColor(1)).heroGarrison({{token, 1}});

	startWithMap(std::move(builder));

	server.gameState = gameState();
	gameHandler = std::make_shared<CGameHandler>(server, gameState());

	// a battle rolls for luck, morale and every chance-based ability, so nothing in it is
	// reproducible until the rolls are pinned down
	gameHandler->randomizer->setSeed(seed);

	attackerSideHero = findHeroByOwner(PlayerColor(0));
	defenderSideHero = findHeroByOwner(PlayerColor(1));
	ASSERT_NE(attackerSideHero, nullptr);
	ASSERT_NE(defenderSideHero, nullptr);

	makeNeutral(attackerSideHero);
	makeNeutral(defenderSideHero);
}

/// Strips everything that could scale damage on its own - starting skills, primary stats and the
/// innate specialty - so only what a scenario grants explicitly is left.
void BattleTestFixture::makeNeutral(CGHeroInstance * hero)
{
	for(const auto & bonus : hero->getHeroType()->specialty)
		hero->removeBonus(bonus);

	for(int i = 0; i < LIBRARY->skillh->size(); ++i)
		hero->setSecSkillLevel(SecondarySkill(i), 0, ChangeValueMode::ABSOLUTE);

	for(auto skill : {PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE})
		hero->setPrimarySkill(skill, 0, ChangeValueMode::ABSOLUTE);
}

void BattleTestFixture::startBattle()
{
	BattleSideArray<const CGHeroInstance *> heroes = {attackerSideHero, defenderSideHero};
	BattleSideArray<const CArmedInstance *> armies = {attackerSideHero, defenderSideHero};

	int3 tile(4, 4, 0);
	auto terrain = gameState()->getTile(tile)->getTerrainID();
	BattleLayout layout = BattleLayout::createDefaultLayout(*gameState(), attackerSideHero, defenderSideHero);

	// a battlefield grants bonuses of its own, and the default one is a clover field, whose luck
	// would turn some attacks into lucky strikes and double the damage a scenario measures
	// a string rather than a literal, which would pick the overload that takes a scoped name
	const std::string battlefieldName = "core:sand_shore";
	BattleField battlefield(*LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "battlefield", battlefieldName));

	BattleStart bs;
	bs.info = BattleInfo::setupBattle(gameState().get(), tile, terrain, battlefield, armies, heroes, layout, nullptr);
	bs.battleID = BattleID(0);
	gameHandler->sendAndApply(bs);

	ASSERT_EQ(gameState()->currentBattles.size(), 1u);

	battle()->tacticDistance = 0;

	// the layout scatters obstacles at random, and a mine under a unit would show up as damage
	battle()->obstacles.clear();
}

void BattleTestFixture::beginCombat()
{
	// ending the tactics phase is what fires the battle-start triggers, so the battle is put back
	// into one for as long as it takes to end it
	battle()->tacticDistance = 1;
	battle()->tacticsSide = BattleSide::ATTACKER;

	BattleAction action = BattleAction::makeEndOFTacticPhase(BattleSide::ATTACKER);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), action));
	ASSERT_EQ(battle()->tacticDistance, 0);
}

BattleInfo * BattleTestFixture::battle() const
{
	return gameState()->currentBattles.front().get();
}

CStack * BattleTestFixture::addStack(BattleSide side, const CreatureID & creature, const BattleHex & position, int32_t count)
{
	battle::UnitInfo info;
	info.id = battle()->battleNextUnitId();
	info.count = count;
	info.type = creature;
	info.side = side;
	info.position = position;
	info.summoned = false;

	BattleUnitsChanged pack;
	pack.battleID = BattleID(0);
	pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
	info.save(pack.changedStacks.back().data);
	gameHandler->sendAndApply(pack);

	return battle()->getStack(info.id);
}

void BattleTestFixture::giveArtifact(const CGHeroInstance * hero, ArtifactID artifact, ArtifactPosition position)
{
	NewArtifact na;
	na.artHolder = hero->id;
	na.artId = artifact;
	na.pos = position;
	gameHandler->sendAndApply(na);
}

bool BattleTestFixture::castOn(const CGHeroInstance * hero, SpellID spellID, const CStack * target) const
{
	const CSpell * spell = spellID.toSpell();
	spells::BattleCast cast(battle(), hero, spells::Mode::HERO, spell);
	spells::Target destination;
	destination.emplace_back(target);

	spells::detail::ProblemImpl problem;

	auto mechanics = spell->battleMechanics(&cast);
	if(!mechanics->canBeCast(problem) || !mechanics->canBeCastAt(destination, problem))
		return false;

	cast.cast(gameHandler->spellEnv.get(), destination);
	return true;
}

bool BattleTestFixture::attack(const CStack * attacker, const BattleHex & targetHex)
{
	battle()->activeStack = attacker->unitId();

	BattleAction action = BattleAction::makeMeleeAttack(attacker, targetHex, attacker->getPosition());
	return gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(attacker->unitSide()), action);
}

void BattleTestFixture::endRound()
{
	const int32_t startingRound = battle()->getRound();

	// every unit defending is the shortest way through a round; the flow processor hands the turn
	// on by itself, so the loop only has to keep answering whoever it activates
	while(battle()->getRound() == startingRound)
	{
		const auto * active = battle()->battleActiveUnit();
		ASSERT_NE(active, nullptr);

		BattleAction action = BattleAction::makeDefend(active);
		ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(active->unitSide()), action));
	}
}

void BattleTestFixture::blockRetaliation(CStack * stack)
{
	stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BLOCKS_RETALIATION, BonusSource::OTHER, 0, BonusSourceID()));
}

void BattleTestFixture::forceMaximumDamage(CStack * stack)
{
	stack->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::ALWAYS_MAXIMUM_DAMAGE, BonusSource::OTHER, 0, BonusSourceID()));
}

CreatureID BattleTestFixture::creatureByName(const std::string & name)
{
	auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "creature", name);
	EXPECT_TRUE(identifier.has_value()) << "unknown creature " << name;

	return identifier ? CreatureID(*identifier) : CreatureID::NONE;
}
