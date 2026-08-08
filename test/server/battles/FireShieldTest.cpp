/*
 * FireShieldTest.cpp, part of VCMI engine
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
#include "../../lib/spells/Problem.h"

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
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/spells/ISpellMechanics.h"

namespace
{

/// One scenario: which creatures meet, what the casting hero knows and carries,
/// and how much health the attacker must lose to the fire shield.
struct FireShieldCase
{
	const char * name;
	int shieldedCreature;  ///< attacker-side unit, receives the Fire Shield spell
	int attackingCreature; ///< defender-side unit, strikes it and takes the reflected damage
	int skill;             ///< secondary skill given to the casting hero, -1 for none
	int mastery;           ///< 1 basic, 2 advanced, 3 expert
	int artifact;          ///< artifact equipped on the casting hero, -1 for none
	int64_t expectedDamage;
};

/// Applies everything the game handler emits straight to the game state, which is
/// the whole of "being a server" that a battle needs.
class LocalGameServer : public IGameServer
{
public:
	std::shared_ptr<CGameState> gameState;

	void setState(EServerState value) override { state = value; }
	EServerState getState() const override { return state; }
	bool isPlayerHost(const PlayerColor &) const override { return true; }
	bool hasPlayerAt(PlayerColor, GameConnectionID) const override { return true; }
	bool hasBothPlayersAtSameConnection(PlayerColor, PlayerColor) const override { return true; }
	void sendPack(CPackForClient &, GameConnectionID) override {}

	void applyPack(CPackForClient & pack) override
	{
		gameState->apply(pack);
	}

private:
	EServerState state = EServerState::GAMEPLAY;
};

}

/// Fire shield reflects part of the damage a melee attacker deals back at it. The amount
/// passes through the spell damage pipeline of the shielded unit's hero, so hero skills and
/// artifacts scale it, and the attacker's own fire resistances cut it down again.
///
/// Every scenario is the same battle: the casting hero shields its own unit, curses the
/// enemy unit so that it always deals its minimum damage, and lets that enemy attack. The
/// health the attacker loses is the reflected damage.
class FireShieldTest : public ::testing::Test, public MapListener, public ::testing::WithParamInterface<FireShieldCase>
{
public:
	/// The shielded stack is oversized on purpose: the reflected amount is capped by its
	/// remaining health, and no scenario is meant to hit that cap.
	static constexpr int32_t shieldedCount = 5000;
	static constexpr int32_t attackingCount = 1000;
	/// Adjacent free hexes in the middle of the field, away from the armies placed by the layout.
	static constexpr int shieldedHex = 5 * GameConstants::BFIELD_WIDTH + 7;
	static constexpr int attackingHex = shieldedHex + 1;

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
			.name("FireShieldTest")
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

		caster = getHeroByOwner(PlayerColor(0));
		enemy = getHeroByOwner(PlayerColor(1));
		ASSERT_NE(caster, nullptr);
		ASSERT_NE(enemy, nullptr);

		makeNeutral(caster);
		makeNeutral(enemy);
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

	/// Strips everything that could scale damage on its own - starting skills, primary stats
	/// and the innate specialty - so only what a scenario grants explicitly is left.
	void makeNeutral(CGHeroInstance * hero)
	{
		for(const auto & bonus : hero->getHeroType()->specialty)
			hero->removeBonus(bonus);

		for(int i = 0; i < LIBRARY->skillh->size(); ++i)
			hero->setSecSkillLevel(SecondarySkill(i), 0, ChangeValueMode::ABSOLUTE);

		for(auto skill : {PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE})
			hero->setPrimarySkill(skill, 0, ChangeValueMode::ABSOLUTE);
	}

	void giveArtifact(CGHeroInstance * hero, ArtifactID artifact, ArtifactPosition position)
	{
		NewArtifact na;
		na.artHolder = hero->id;
		na.artId = artifact;
		na.pos = position;
		gameHandler->sendAndApply(na);
	}

	void startBattle()
	{
		BattleSideArray<const CGHeroInstance *> heroes = {caster, enemy};
		BattleSideArray<const CArmedInstance *> armies = {caster, enemy};

		int3 tile(4, 4, 0);
		auto terrain = gameState->getTile(tile)->getTerrainID();
		BattleLayout layout = BattleLayout::createDefaultLayout(*gameState, caster, enemy);

		BattleStart bs;
		bs.info = BattleInfo::setupBattle(gameState.get(), tile, terrain, BattleField(0), armies, heroes, layout, nullptr);
		bs.battleID = BattleID(0);
		gameHandler->sendAndApply(bs);

		ASSERT_EQ(gameState->currentBattles.size(), 1u);
		battle()->tacticDistance = 0;
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

	/// Casts a hero spell at a unit, reporting whether the game allowed it at all.
	bool castOn(const CGHeroInstance * hero, SpellID spellID, const CStack * target)
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

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<MapServiceTinyH3M> mapService;
	std::unique_ptr<CGameHandler> gameHandler;
	LocalGameServer server;

	CMap * map = nullptr;
	CGHeroInstance * caster = nullptr;
	CGHeroInstance * enemy = nullptr;
};

TEST_P(FireShieldTest, reflectsExpectedDamage)
{
	const auto & scenario = GetParam();

	startGame();

	giveArtifact(caster, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	caster->addSpellToSpellbook(SpellID::FIRE_SHIELD);
	caster->mana = 9999;

	giveArtifact(enemy, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);
	enemy->addSpellToSpellbook(SpellID(SpellID::BLESS));
	enemy->mana = 9999;

	if(scenario.skill >= 0)
		caster->setSecSkillLevel(SecondarySkill(scenario.skill), scenario.mastery, ChangeValueMode::ABSOLUTE);

	if(scenario.artifact >= 0)
		giveArtifact(caster, ArtifactID(scenario.artifact), ArtifactPosition::MISC1);

	startBattle();

	CStack * shielded = addStack(BattleSide::ATTACKER, CreatureID(scenario.shieldedCreature), BattleHex(shieldedHex), shieldedCount);
	CStack * attacking = addStack(BattleSide::DEFENDER, CreatureID(scenario.attackingCreature), BattleHex(attackingHex), attackingCount);
	ASSERT_NE(shielded, nullptr);
	ASSERT_NE(attacking, nullptr);

	// retaliation would injure the attacker as well, hiding the reflected damage
	attacking->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BLOCKS_RETALIATION, BonusSource::OTHER, 0, BonusSourceID()));

	// a fire-immune target refuses the spell, which is what leaves its own shield untouched
	castOn(caster, SpellID::FIRE_SHIELD, shielded);

	// bless collapses the damage range onto its maximum, making the reflected amount exact.
	// It comes from the enemy hero, whose kit no scenario touches, so nothing granted to the
	// casting hero can reach the damage the attack itself deals
	ASSERT_TRUE(castOn(enemy, SpellID(SpellID::BLESS), attacking));

	const int64_t healthBefore = attacking->getAvailableHealth();

	battle()->activeStack = attacking->unitId();
	BattleAction action = BattleAction::makeMeleeAttack(attacking, BattleHex(shieldedHex), attacking->getPosition());
	ASSERT_TRUE(gameHandler->battles->makePlayerBattleAction(BattleID(0), PlayerColor(1), action));

	EXPECT_EQ(healthBefore - attacking->getAvailableHealth(), scenario.expectedDamage) << scenario.name;
}

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int stoneGolem = 33;
constexpr int efreet = 52;
constexpr int efreetSultan = 53;
constexpr int fireElemental = 114;
constexpr int waterElemental = 115;
constexpr int goldGolem = 116;
constexpr int diamondGolem = 117;

// what the casting hero may be given
constexpr int noSkill = -1;
constexpr int fireMagic = 14;
constexpr int sorcery = 25;
constexpr int noArtifact = -1;
constexpr int orbOfFire = 81; // Orb of Tempestuous Fire, +50% to fire spell damage
constexpr int orbOfVulnerability = 93; // negates natural immunities battle-wide

constexpr int basic = 1;
constexpr int advanced = 2;
constexpr int expert = 3;
}

// 1000 pikemen blessed to their maximum damage reflect a base of 3000 off a plain target, so
// the undisturbed reflection is the shield percentage of that: 600 at base mastery, 750
// advanced, 900 expert. Sorcery and the orb scale that, the attacker's own fire resistance
// cuts it down.
INSTANTIATE_TEST_SUITE_P(Scenarios, FireShieldTest, ::testing::Values(
	// the spell alone, and the mastery that decides its percentage
	FireShieldCase{"plain",              pikeman, pikeman, noSkill,   0,        noArtifact,  600},
	FireShieldCase{"fireMagicBasic",     pikeman, pikeman, fireMagic, basic,    noArtifact,  600},
	FireShieldCase{"fireMagicAdvanced",  pikeman, pikeman, fireMagic, advanced, noArtifact,  750},
	FireShieldCase{"fireMagicExpert",    pikeman, pikeman, fireMagic, expert,   noArtifact,  900},

	// sorcery scales the reflected damage like any other spell damage
	FireShieldCase{"sorceryBasic",       pikeman, pikeman, sorcery,   basic,    noArtifact,  630},
	FireShieldCase{"sorceryAdvanced",    pikeman, pikeman, sorcery,   advanced, noArtifact,  660},
	FireShieldCase{"sorceryExpert",      pikeman, pikeman, sorcery,   expert,   noArtifact,  690},

	// the orb adds its fire school bonus on top of both
	FireShieldCase{"orb",                pikeman, pikeman, noSkill,   0,        orbOfFire,   900},
	FireShieldCase{"orbAndFireMagic",    pikeman, pikeman, fireMagic, expert,   orbOfFire,  1350},
	FireShieldCase{"orbAndSorcery",      pikeman, pikeman, sorcery,   expert,   orbOfFire,  1035},

	// a fire-immune attacker takes nothing at all
	FireShieldCase{"immuneEfreet",       pikeman, efreet,        noSkill, 0, noArtifact, 0},
	FireShieldCase{"immuneElemental",    pikeman, fireElemental, noSkill, 0, noArtifact, 0},

	// golems reduce magic damage, so they take a fraction of what they reflect back
	FireShieldCase{"resistantStoneGolem",   pikeman, stoneGolem,   noSkill, 0, noArtifact, 300},
	FireShieldCase{"resistantGoldGolem",    pikeman, goldGolem,    noSkill, 0, noArtifact, 390},
	FireShieldCase{"resistantDiamondGolem", pikeman, diamondGolem, noSkill, 0, noArtifact, 196},

	// water elementals take double damage from this spell. 3218 rather than 3220 because the
	// damage calculator works in doubles and 7000 * 1.15 lands just below 8050
	FireShieldCase{"vulnerableWaterElemental", pikeman, waterElemental, noSkill, 0, noArtifact, 3218},

	// an efreet sultan already burns its attackers; the spell it is immune to adds nothing,
	// so expert mastery still reflects only the creature's own 20%
	FireShieldCase{"efreetSultanDoesNotStack", efreetSultan, pikeman, fireMagic, expert, noArtifact, 600},

	// with the Orb of Vulnerability the sultan loses that immunity and does accept the spell.
	// The two shields then add up - 20% from the creature plus the spell's 20% or 30% - which
	// records what the engine does today rather than a checked H3 rule
	FireShieldCase{"efreetSultanVulnerable",       efreetSultan, pikeman, noSkill,   0,      orbOfVulnerability, 1200},
	FireShieldCase{"efreetSultanVulnerableExpert", efreetSultan, pikeman, fireMagic, expert, orbOfVulnerability, 1500}
),
	[](const ::testing::TestParamInfo<FireShieldCase> & info) { return info.param.name; });
