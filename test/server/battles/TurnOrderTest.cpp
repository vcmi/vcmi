/*
 * TurnOrderTest.cpp, part of VCMI engine
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

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/TerrainHandler.h"
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/callback/GameRandomizer.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

namespace
{

/// One unit taking its turn, with everything that decides its place in the queue.
struct Activation
{
	int32_t round;
	BattleSide side;
	SlotID slot;
	int32_t initiative;
	std::string creature;
};

std::ostream & operator<<(std::ostream & out, const Activation & activation)
{
	return out << "round " << activation.round << ": side " << static_cast<int>(activation.side) << " slot " << activation.slot.getNum()
		<< " " << activation.creature << " initiative " << activation.initiative;
}

struct TurnOrderCase
{
	const char * name;
	TerrainId terrain;
};

// creatures
const CreatureID marksman(3);
const CreatureID monk(8);
const CreatureID pikeman(0);
const CreatureID archer(2);
const CreatureID skeleton(56);

const int3 monsterPos(6, 5, 0);

// heroes
const HeroTypeID orrin(0);
const HeroTypeID valeska(1); // specialty: archers, +1 speed
const HeroTypeID ingham(12); // specialty: monks, +1 speed

}

/// Units with equal speed on the same side should act in order of their army slots, and a hero
/// specialty that brings one creature up to the speed of another should not change that.
class TurnOrderTest : public BattleTestFixture, public ::testing::WithParamInterface<TurnOrderCase>
{
public:
	/// Unlike the default game of the fixture, heroes keep their specialties and have real armies
	using Army = std::vector<std::pair<CreatureID, uint16_t>>;

	void startGameWithArmies(bool againstMonster = false)
	{
		// same layout as in the reports: speed ties between marksmen and monks of the specialist
		startGameWithArmies(ingham, {{marksman, 16}, {monk, 6}, {monk, 7}, {marksman, 15}}, orrin, {{pikeman, 1}}, againstMonster);
	}

	void startGameWithArmies(HeroTypeID attackerHero, const Army & attackerArmy, HeroTypeID defenderHero, const Army & defenderArmy, bool againstMonster = false)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("TurnOrderTest")
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, attackerHero, PlayerColor(0)).heroGarrison(attackerArmy)
			.hero({7, 7, 0}, defenderHero, PlayerColor(1)).heroGarrison(defenderArmy);

		if(againstMonster)
			builder.monster(monsterPos, skeleton, 51);

		startWithMap(std::move(builder));

		server.gameState = gameState();
		gameHandler = std::make_shared<CGameHandler>(server, gameState());
		gameHandler->randomizer->setSeed(seed);

		attackerSideHero = findHeroByOwner(PlayerColor(0));
		defenderSideHero = findHeroByOwner(PlayerColor(1));
		ASSERT_NE(attackerSideHero, nullptr);
		ASSERT_NE(defenderSideHero, nullptr);
	}

	/// Plays given number of rounds with every unit defending, and records who acted when
	std::vector<Activation> playRounds(int rounds)
	{
		std::vector<Activation> result;
		const int32_t lastRound = battle()->getRound() + rounds - 1;

		while(battle()->getRound() <= lastRound)
		{
			const auto * active = battle()->battleActiveUnit();
			EXPECT_NE(active, nullptr);
			if(!active)
				break;

			result.push_back({battle()->getRound(), active->unitSide(), active->unitSlot(), active->getInitiative(0), active->creatureId().toCreature()->getJsonKey()});

			BattleAction action = BattleAction::makeDefend(active);
			if(!gameHandler->battles->makePlayerBattleAction(BattleID(0), battle()->sideToPlayer(active->unitSide()), action))
			{
				// a rejected action leaves the same unit active, so the round would never end
				ADD_FAILURE() << "defend action rejected for " << result.back();
				break;
			}
		}
		return result;
	}

	const CStack * findStack(BattleSide side, const CreatureID & creature) const
	{
		for(const auto * stack : battle()->battleGetAllStacks())
			if(stack->unitSide() == side && stack->creatureId() == creature)
				return stack;
		ADD_FAILURE() << "no such stack";
		return nullptr;
	}

	/// Checks that within each round units of the same side act from fastest to slowest, and in slot order on ties
	static void checkOrder(const std::vector<Activation> & activations)
	{
		for(const auto & activation : activations)
			std::cout << activation << "\n";

		for(size_t i = 1; i < activations.size(); ++i)
		{
			const auto & previous = activations[i - 1];
			const auto & current = activations[i];

			if(previous.round != current.round)
				continue;

			EXPECT_GE(previous.initiative, current.initiative) << "slower unit acted first: " << previous << " before " << current;

			if(previous.side != current.side)
				continue;

			if(previous.initiative == current.initiative)
			{
				EXPECT_LT(previous.slot, current.slot) << "equal speed units out of slot order: " << previous << " before " << current;
			}
		}
	}
};

TEST_P(TurnOrderTest, equalSpeedUnitsActInSlotOrder)
{
	startGameWithArmies();
	startBattle(GetParam().terrain);

	// what the queue shows before battle starts
	std::vector<battle::Units> predicted;
	battle()->battleGetTurnOrder(predicted, 0, 2);
	for(const auto & unit : predicted.at(0))
		std::cout << "predicted: side " << static_cast<int>(unit->unitSide()) << " slot " << unit->unitSlot().getNum()
			<< " " << unit->creatureId().toCreature()->getJsonKey() << " initiative " << unit->getInitiative(0) << "\n";

	beginCombat();

	checkOrder(playRounds(2));
}

TEST_P(TurnOrderTest, equalSpeedUnitsActInSlotOrderAfterTactics)
{
	startGameWithArmies();
	startBattle(GetParam().terrain);

	battle()->tacticDistance = 3;
	battle()->tacticsSide = BattleSide::ATTACKER;

	// player repositions one of the monks during tactics phase
	const CStack * movedMonk = nullptr;
	for(const auto * stack : battle()->battleGetAllStacks())
		if(stack->unitSide() == BattleSide::ATTACKER && stack->unitSlot() == SlotID(1))
			movedMonk = stack;
	ASSERT_NE(movedMonk, nullptr);

	const BattleHex destination = movedMonk->getPosition().cloneInDirection(BattleHex::RIGHT, false);
	BattleAction move = BattleAction::makeMove(movedMonk, destination);
	move.side = BattleSide::ATTACKER;
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), move));
	EXPECT_EQ(movedMonk->getPosition(), destination);

	BattleAction endTactics = BattleAction::makeEndOFTacticPhase(BattleSide::ATTACKER);
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(0), endTactics));
	ASSERT_EQ(battle()->tacticDistance, 0);

	checkOrder(playRounds(2));
}

/// As in the second report: archers of the specialist should not overtake faster marksmen of the other side.
/// Native terrain bonus and specialty are both +1 speed, and the native bonus used to be counted twice
TEST_P(TurnOrderTest, specialtyDoesNotOvertakeFasterEnemy)
{
	startGameWithArmies(orrin, {{marksman, 10}, {marksman, 10}}, valeska, {{archer, 31}});
	startBattle(GetParam().terrain);
	beginCombat();

	const int nativeBonus = GetParam().terrain == ETerrainId::GRASS ? 1 : 0;
	EXPECT_EQ(findStack(BattleSide::DEFENDER, archer)->getInitiative(), 4 + 1 + nativeBonus);
	EXPECT_EQ(findStack(BattleSide::ATTACKER, marksman)->getInitiative(), 6 + nativeBonus);

	checkOrder(playRounds(2));
}

/// Same scenario, with battle started through the battle processor on the map terrain
TEST_F(TurnOrderTest, specialtyDoesNotOvertakeFasterEnemyInRealBattle)
{
	startGameWithArmies(orrin, {{marksman, 10}, {marksman, 10}}, valeska, {{archer, 31}});
	gameHandler->battles->startBattle(attackerSideHero, defenderSideHero);

	ASSERT_EQ(gameState()->currentBattles.size(), 1u);
	ASSERT_EQ(battle()->getTerrainType(), ETerrainId::GRASS);
	EXPECT_EQ(findStack(BattleSide::DEFENDER, archer)->getInitiative(), 4 + 1 + 1);
	EXPECT_EQ(findStack(BattleSide::ATTACKER, marksman)->getInitiative(), 6 + 1);

	checkOrder(playRounds(2));
}

/// Same check, but battle is started the way the adventure map starts it, through the battle processor
TEST_F(TurnOrderTest, equalSpeedUnitsActInSlotOrderInRealBattle)
{
	startGameWithArmies();
	gameHandler->battles->startBattle(attackerSideHero, defenderSideHero);

	ASSERT_EQ(gameState()->currentBattles.size(), 1u);
	ASSERT_EQ(battle()->tacticDistance, 0);

	std::cout << "terrain " << battle()->getTerrainType().toEntity(LIBRARY)->getJsonKey() << "\n";
	checkOrder(playRounds(2));
}

/// Battle against neutral creatures, as in the screenshots of the report
TEST_F(TurnOrderTest, equalSpeedUnitsActInSlotOrderAgainstMonster)
{
	startGameWithArmies(true);

	const auto * monster = dynamic_cast<const CArmedInstance *>(findObjectAt(monsterPos));
	ASSERT_NE(monster, nullptr);
	gameHandler->battles->startBattle(attackerSideHero, monster);

	ASSERT_EQ(gameState()->currentBattles.size(), 1u);
	checkOrder(playRounds(2));
}

INSTANTIATE_TEST_SUITE_P(Terrains, TurnOrderTest, ::testing::Values(
	TurnOrderCase{"sand", ETerrainId::SAND},
	TurnOrderCase{"grass", ETerrainId::GRASS}
),
	[](const ::testing::TestParamInfo<TurnOrderCase> & info) { return info.param.name; });
