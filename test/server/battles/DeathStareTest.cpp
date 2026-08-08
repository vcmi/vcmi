/*
 * DeathStareTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_MapServiceTinyH3M.h"
#include "mock/TinyH3MBuilder.h"

#include "../../server/CGameHandler.h"
#include "../../server/IGameServer.h"
#include "../../server/battles/BattleProcessor.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

namespace
{

/// One death stare cast, as the clients see it.
struct StareCast
{
	BattleSpellCast announcement;
	int64_t damage = 0;
	uint32_t killed = 0;
	std::vector<std::string> logLines;
};

/// One scenario: who the gorgons stare at, and how many of them die per triggered stare.
struct DeathStareCase
{
	const char * name;
	int defendingCreature;
	uint32_t expectedKills;   ///< 0 when the target is immune and no stare ever lands
	std::vector<std::string> expectedLog;
};

/// Applies everything the game handler emits straight to the game state, and keeps the packs
/// that make up an acid breath so the test can inspect them.
class RecordingGameServer : public IGameServer
{
public:
	std::shared_ptr<CGameState> gameState;
	std::vector<StareCast> casts;

	void setState(EServerState value) override { state = value; }
	EServerState getState() const override { return state; }
	bool isPlayerHost(const PlayerColor &) const override { return true; }
	bool hasPlayerAt(PlayerColor, GameConnectionID) const override { return true; }
	bool hasBothPlayersAtSameConnection(PlayerColor, PlayerColor) const override { return true; }
	void sendPack(CPackForClient &, GameConnectionID) override {}

	void applyPack(CPackForClient & pack) override
	{
		record(pack);
		gameState->apply(pack);
	}

private:
	EServerState state = EServerState::GAMEPLAY;

	/// A cast arrives as an announcement, then the damage, then the log. Anything belonging to
	/// another spell or to the next attack closes the window again.
	void record(CPackForClient & pack)
	{
		if(dynamic_cast<const BattleAttack *>(&pack))
		{
			recording = false;
			return;
		}

		if(const auto * announcement = dynamic_cast<const BattleSpellCast *>(&pack))
		{
			recording = announcement->spellID == SpellID::DEATH_STARE;
			if(recording)
				casts.push_back(StareCast{*announcement, 0, 0, {}});
			return;
		}

		if(!recording || casts.empty())
			return;

		if(const auto * injured = dynamic_cast<const StacksInjured *>(&pack))
		{
			for(const auto & stack : injured->stacks)
			{
				casts.back().damage += stack.damageAmount;
				casts.back().killed += stack.killedAmount;
			}
			return;
		}

		if(const auto * log = dynamic_cast<const BattleLogMessage *>(&pack))
		{
			for(const auto & line : log->lines)
				casts.back().logLines.push_back(line.toString());
		}
	}

	bool recording = false;
};

}

/// Death stare kills creatures of the attacked stack outright: every gorgon of the attacking
/// stack rolls its own chance, and the total is capped at the share of the stack that could have
/// rolled it. This pins what the ability produces - the kills, the spell announcement that drives
/// the client animation, and the combat log - so that reimplementing it can be checked against
/// the behaviour it replaces rather than against a description of itself.
class DeathStareTest : public ::testing::Test, public MapListener, public ::testing::WithParamInterface<DeathStareCase>
{
public:
	/// Large enough that the per-creature roll and the cap on it are both exercised: at 10% each,
	/// this many gorgons kill up to ten creatures per stare.
	static constexpr int32_t gorgonCount = 100;
	/// Large enough to survive every attack of the run, so that no scenario ends early.
	static constexpr int32_t targetCount = 100000;
	/// How many attacks one scenario makes. The roll is biased against long streaks of failure,
	/// so a run this long always contains several triggers.
	static constexpr int attacks = 20;
	/// Fixes the rolls, which decide which of those attacks trigger.
	static constexpr int seed = 1337;
	/// How many of those attacks land a stare at all, for this seed.
	static constexpr size_t expectedTriggers = 20;

	static constexpr int targetHex = 5 * GameConstants::BFIELD_WIDTH + 7;
	/// The gorgon is double wide and occupies this hex and the one behind it.
	static constexpr int gorgonHex = targetHex + 1;

	void SetUp() override
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(LIBRARY);
	}

	void TearDown() override
	{
		gameHandler.reset();
		gameState.reset();
	}

	void mapLoaded(CMap * map) override
	{
		EXPECT_EQ(this->map, nullptr);
		this->map = map;
	}

	/// Two heroes with a token army each, so that a battle between them is valid.
	void startGame()
	{
		const CreatureID token(0);

		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("DeathStareTest")
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0)).heroGarrison({{token, 1}})
			.hero({7, 7, 0}, HeroTypeID(1), PlayerColor(1)).heroGarrison({{token, 1}});

		auto bytes = builder.build();
		mapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

		StartInfo si;
		si.mapname = "tiny";
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		std::unique_ptr<CMapHeader> header = mapService->loadMapHeader(ResourcePath(si.mapname));
		ASSERT_NE(header.get(), nullptr);

		for(int i = 0; i < static_cast<int>(header->players.size()); i++)
		{
			const PlayerInfo & pinfo = header->players[i];
			if(!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si.playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			pset.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			pset.name = "Player";
			pset.bonus = PlayerStartingBonus::GOLD; // no random starting artifact
			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(mapService.get(), &si, randomizer, progressTracker, false);
		ASSERT_NE(map, nullptr);

		server.gameState = gameState;
		gameHandler = std::make_unique<CGameHandler>(server, gameState);

		attackerHero = getHeroByOwner(PlayerColor(1));
		defenderHero = getHeroByOwner(PlayerColor(0));
		ASSERT_NE(attackerHero, nullptr);
		ASSERT_NE(defenderHero, nullptr);

		makeNeutral(attackerHero);
		makeNeutral(defenderHero);
	}

	CGHeroInstance * getHeroByOwner(PlayerColor owner) const
	{
		for(auto heroID : map->getHeroesOnMap())
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(map->getObject(heroID));
			if(hero && hero->tempOwner == owner)
				return hero;
		}
		return nullptr;
	}

	/// Strips everything that could scale damage on its own, so the acid damage is only what
	/// the creature itself brings.
	void makeNeutral(CGHeroInstance * hero)
	{
		for(const auto & bonus : hero->getHeroType()->specialty)
			hero->removeBonus(bonus);

		for(int i = 0; i < LIBRARY->skillh->size(); ++i)
			hero->setSecSkillLevel(SecondarySkill(i), 0, ChangeValueMode::ABSOLUTE);

		for(auto skill : {PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE})
			hero->setPrimarySkill(skill, 0, ChangeValueMode::ABSOLUTE);
	}

	void startBattle()
	{
		BattleSideArray<const CGHeroInstance *> heroes = {defenderHero, attackerHero};
		BattleSideArray<const CArmedInstance *> armies = {defenderHero, attackerHero};

		int3 tile(4, 4, 0);
		auto terrain = gameState->getTile(tile)->getTerrainID();
		BattleLayout layout = BattleLayout::createDefaultLayout(*gameState, defenderHero, attackerHero);

		BattleStart bs;
		bs.info = BattleInfo::setupBattle(gameState.get(), tile, terrain, BattleField(0), armies, heroes, layout, nullptr);
		bs.battleID = BattleID(0);
		gameHandler->sendAndApply(bs);

		ASSERT_EQ(gameState->currentBattles.size(), 1u);
		battle()->tacticDistance = 0;

		// the layout scatters obstacles at random, and a mine under a unit would show up as damage
		battle()->obstacles.clear();
	}

	BattleInfo * battle() const
	{
		return gameState->currentBattles.front().get();
	}

	CStack * addStack(BattleSide side, const CreatureID & creature, const BattleHex & position, int32_t count)
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

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	std::unique_ptr<CGameHandler> gameHandler;
	RecordingGameServer server;

	CMap * map = nullptr;
	CGHeroInstance * attackerHero = nullptr;
	CGHeroInstance * defenderHero = nullptr;
};

TEST_P(DeathStareTest, killsExpectedCreatures)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * target = addStack(BattleSide::ATTACKER, CreatureID(scenario.defendingCreature), BattleHex(targetHex), targetCount);
	CStack * gorgon = addStack(BattleSide::DEFENDER, CreatureID(103 /* mighty gorgon */), BattleHex(gorgonHex), gorgonCount);
	ASSERT_NE(target, nullptr);
	ASSERT_NE(gorgon, nullptr);

	// retaliation would shrink the staring stack, and every gorgon in it rolls its own chance
	gorgon->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BLOCKS_RETALIATION, BonusSource::OTHER, 0, BonusSourceID()));

	gameHandler->randomizer->setSeed(seed);

	for(int i = 0; i < attacks; ++i)
	{
		ASSERT_TRUE(target->alive()) << "target died on attack " << i;

		battle()->activeStack = gorgon->unitId();
		BattleAction action = BattleAction::makeMeleeAttack(gorgon, BattleHex(targetHex), gorgon->getPosition());
		ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(1), action));
	}

	ASSERT_FALSE(server.casts.empty()) << scenario.name << ": death stare never triggered";

	for(const auto & cast : server.casts)
	{
		// what the client turns into the death stare animation and sound
		EXPECT_EQ(cast.announcement.spellID, SpellID(SpellID::DEATH_STARE)) << scenario.name;
		EXPECT_FALSE(cast.announcement.castByHero) << scenario.name;
		EXPECT_FALSE(cast.announcement.activeCast) << scenario.name; // a passive ability, not an action
		EXPECT_EQ(cast.announcement.casterStack, static_cast<si32>(gorgon->unitId())) << scenario.name;
		EXPECT_TRUE(cast.announcement.resistedCres.empty()) << scenario.name;
		EXPECT_TRUE(cast.announcement.reflectedCres.empty()) << scenario.name;

		// the cap is what makes this exact rather than a distribution: at 10% each, this many
		// gorgons can never kill more than a tenth of their own number
		EXPECT_LE(cast.killed, scenario.expectedKills) << scenario.name;
		EXPECT_EQ(cast.damage, static_cast<int64_t>(cast.killed) * target->getMaxHealth()) << scenario.name;
	}

	// how often the ability fired. Fixed by the seed, so this only moves when the chance itself
	// moves, or when the way it is rolled does. An immune target is announced and animated all the
	// same - the roll happens before the spell, which is what filters the target back out
	EXPECT_EQ(server.casts.size(), expectedTriggers) << scenario.name;

	EXPECT_EQ(server.casts.front().killed, scenario.expectedKills) << scenario.name;
	EXPECT_EQ(server.casts.front().logLines, scenario.expectedLog) << scenario.name;

	if(scenario.expectedKills == 0)
		EXPECT_TRUE(server.casts.front().announcement.affectedCres.empty()) << scenario.name;
	else
		EXPECT_EQ(server.casts.front().announcement.affectedCres, std::vector<ui32>{target->unitId()}) << scenario.name;
}

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int skeleton = 56;   // undead, absolutely immune to the stare
constexpr int ironGolem = 33;  // non-living, likewise immune
}

// Every gorgon of the stack rolls its own 10% chance, and the total is capped at a tenth of the
// stack, so 100 gorgons kill at most 10 creatures per stare. The immune targets are here because
// the immunity lives in the spell's targetCondition, which a script dealing raw damage would skip -
// note that they still get the cast and its animation, they just survive it.
INSTANTIATE_TEST_SUITE_P(Scenarios, DeathStareTest, ::testing::Values(
	DeathStareCase{"livingTarget", pikeman,  10, {"10 Pikemen die under the terrible gaze of the Mighty Gorgons."}},
	DeathStareCase{"undeadTarget", skeleton,  0, {}},
	DeathStareCase{"golemTarget",  ironGolem, 0, {}}
),
	[](const ::testing::TestParamInfo<DeathStareCase> & info) { return info.param.name; });
