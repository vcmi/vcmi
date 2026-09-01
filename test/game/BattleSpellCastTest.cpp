/*
 * BattleSpellCastTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_Services.h"
#include "mock/mock_MapService.h"
#include "mock/mock_IGameEventCallback.h"
#include "mock/mock_spells_Problem.h"
#include "mock/TinyH3MBuilder.h"
#include "mock/mock_MapServiceTinyH3M.h"

#include "../../lib/spells/CSpellHandler.h"

#include <cmath>

#include "../../lib/VCMIDirs.h"
#include "../../lib/json/JsonBonus.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/networkPacks/SetStackEffect.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/StartInfo.h"
#include "../../lib/TerrainHandler.h"

#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/CStack.h"

#include "../../lib/filesystem/ResourcePath.h"

#include "../../lib/mapping/CMap.h"

#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/AbilityCaster.h"
#include "../../lib/spells/CSpell.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/army/CStackInstance.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/bonuses/BonusCustomTypes.h"
#include "../../lib/bonuses/BonusSelector.h"

#include <vcmi/spells/Magic.h>

class BattleSpellCastTest : public ::testing::Test, public SpellCastEnvironment, public MapListener
{
public:
	BattleSpellCastTest()
		: gameEventCallback(std::make_shared<GameEventCallbackMock>(this)),
		mapService("test/MiniTest/", this),
		map(nullptr)
	{

	}

	void SetUp() override
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(&services);
	}

	void TearDown() override
	{
		gameState.reset();
	}

	bool describeChanges() const override
	{
		return true;
	}

	void apply(CPackForClient & pack) override
	{
		gameState->apply(pack);
	}

	void apply(BattleLogMessage & pack) override
	{
		gameState->apply(pack);
	}

	void apply(BattleStackMoved & pack) override
	{
		gameState->apply(pack);
	}

	void apply(BattleUnitsChanged & pack) override
	{
		gameState->apply(pack);
	}

	void apply(SetStackEffect & pack) override
	{
		gameState->apply(pack);
	}

	void apply(StacksInjured & pack) override
	{
		gameState->apply(pack);
	}

	void apply(BattleObstaclesChanged & pack) override
	{
		gameState->apply(pack);
	}

	void apply(CatapultAttack & pack) override
	{
		gameState->apply(pack);
	}

	void complain(const std::string & problem) override
	{
		FAIL() << "Server-side assertion: " << problem;
	};

	vstd::RNG * getRNG() override
	{
		return &randomGenerator;//todo: mock this
	}

	bool rollCombatAbility(const IBattleInfoCallback &, const battle::Unit &, int percentageChance) override
	{
		return randomGenerator.nextInt(1, 100) <= percentageChance;
	}

	const CMap * getMap() const override
	{
		return map;
	}
	const IGameInfoCallback * getCb() const override
	{
		return gameState.get();
	}

	void createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator) override
	{
	}

	bool moveHero(ObjectInstanceID hid, int3 dst, EMovementMode movementMode) override
	{
		return false;
	}

	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits, const MetaString & customTitle) override
	{
	}

	void genericQuery(Query * request, PlayerColor color, std::function<void(std::optional<int32_t>)> callback) override
	{
		//todo:
	}

	void mapLoaded(CMap * map) override
	{
		EXPECT_EQ(this->map, nullptr);
		this->map = map;
	}

	void startTestGame()
	{
		StartInfo si;
		si.mapname = "anything";//does not matter, map service mocked
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		std::unique_ptr<CMapHeader> header = mapService.loadMapHeader(ResourcePath(si.mapname));

		ASSERT_NE(header.get(), nullptr);

		//FIXME: this has been copied from CPreGame, but should be part of StartInfo
		for(int i = 0; i < header->players.size(); i++)
		{
			const PlayerInfo & pinfo = header->players[i];

			//neither computer nor human can play - no player
			if (!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si.playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			pset.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(i));
			pset.name = "Player";
			// Avoid order-dependent random starting artifacts in tests that set hero stats directly.
			pset.bonus = PlayerStartingBonus::GOLD;

			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();

			if(pset.hero != HeroTypeID::RANDOM && pinfo.hasCustomMainHero())
			{
				pset.hero = pinfo.mainCustomHeroId;
				pset.heroNameTextId = pinfo.mainCustomHeroNameTextId;
				pset.heroPortrait = HeroTypeID(pinfo.mainCustomHeroPortrait);
			}
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(&mapService, &si, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr);
		ASSERT_EQ(map->getHeroesOnMap().size(), 2);
	}

	void startTestBattle(const CGHeroInstance * attacker, const CGHeroInstance * defender)
	{
		BattleSideArray<const CGHeroInstance *> heroes = {attacker, defender};
		BattleSideArray<const CArmedInstance *> armedInstancies = {attacker, defender};

		int3 tile(4,4,0);

		const auto & t = *gameState->getTile(tile);

		auto terrain = t.getTerrainID();
		BattleField terType(0);
		BattleLayout layout = BattleLayout::createDefaultLayout(*gameState, attacker, defender);

		//send info about battles

		auto battle = BattleInfo::setupBattle(gameState.get(), tile, terrain, terType, armedInstancies, heroes, layout, nullptr);

		BattleStart bs;
		bs.info = std::move(battle);
		bs.battleID = BattleID(0);
		ASSERT_EQ(gameState->currentBattles.size(), 0);
		gameEventCallback->sendAndApply(bs);
		ASSERT_EQ(gameState->currentBattles.size(), 1);

		// Skip the tactics phase (some heroes have the Tactics skill); an ongoing
		// tactics phase blocks hero spellcasting via battleCanCastSpell.
		gameState->currentBattles.front()->tacticDistance = 0;
	}

	CGHeroInstance * getHeroByOwner(PlayerColor owner) const
	{
		for(auto heroID : map->getHeroesOnMap())
		{
			auto hero = dynamic_cast<CGHeroInstance *>(map->getObject(heroID));
			if(hero && hero->tempOwner == owner)
				return hero;
		}

		return nullptr;
	}

	std::shared_ptr<CGameState> gameState;

	std::shared_ptr<GameEventCallbackMock> gameEventCallback;

	MapServiceMock mapService;
	std::unique_ptr<MapServiceTinyH3M> tinyMapService;
	ServicesMock services;

	CMap * map;
	CRandomGenerator randomGenerator;

	void startTinyGame(TinyH3M::TinyH3MBuilder & builder)
	{
		auto bytes = builder.build();
		tinyMapService = std::make_unique<MapServiceTinyH3M>(std::move(bytes), this);

		StartInfo si;
		si.mapname = "tiny";
		si.difficulty = 0;
		si.mode = EStartMode::NEW_GAME;

		std::unique_ptr<CMapHeader> header = tinyMapService->loadMapHeader(ResourcePath(si.mapname));
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
			pset.bonus = PlayerStartingBonus::GOLD;
			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();
		}

		GameRandomizer randomizer(*gameState);
		Load::ProgressAccumulator progressTracker;
		gameState->init(tinyMapService.get(), &si, randomizer, progressTracker, false);

		ASSERT_NE(map, nullptr);
	}

	// ---- battle spell-cast helpers --------------------------------------

	/// Discard any current game and battle so a scenario can be set up from
	/// scratch within a single test (used to run with/without-specialty in
	/// independent battles that cannot cross-contaminate).
	void resetGame()
	{
		gameState = std::make_shared<CGameState>();
		gameState->preInit(&services);
		map = nullptr;
		tinyMapService.reset();
	}

	/// Build a two-hero tiny game (the specialist under test owns P0, a throwaway
	/// opponent owns P1) and start a battle. The specialist keeps its real, config
	/// loaded kit (spell, spellbook, skills); the opponent only exists to form a
	/// valid battle.
	void startDuel(int specialistHeroIdx, int heroLevel, EMapFormat format = EMapFormat::SOD)
	{
		const CreatureID token(0); // pikeman token army so the battle is valid
		const int opponentIdx = specialistHeroIdx == 0 ? 1 : 0;

		TinyH3M::TinyH3MBuilder builder(format);
		builder
			.size(36, false)
			.name("SpecialtyTest")
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, HeroTypeID(specialistHeroIdx), PlayerColor(0)).heroGarrison({{token, 1}})
			.hero({7, 7, 0}, HeroTypeID(opponentIdx),       PlayerColor(1)).heroGarrison({{token, 1}});

		startTinyGame(builder);

		specialist = getHeroByOwner(PlayerColor(0));
		opponent   = getHeroByOwner(PlayerColor(1));
		ASSERT_NE(specialist, nullptr);
		ASSERT_NE(opponent, nullptr);

		specialist->level = heroLevel;

		startTestBattle(specialist, opponent);
	}

	/// Strip a hero's innate specialty so the same hero can serve as its own
	/// specialty-free baseline. The specialty bonuses are copied onto the hero
	/// instance at init (CGHeroInstance::initHero), so they can be removed here.
	void removeSpecialty(CGHeroInstance * hero)
	{
		for(const auto & b : hero->getHeroType()->specialty)
			hero->removeBonus(b);
	}

	/// Place a single specialist hero (owns P0) with the given army into a fresh
	/// game, without starting a battle. Used by the non-battle specialty suites,
	/// which read the resulting hero / army-stack stats directly.
	CGHeroInstance * placeHero(int heroIdx, int heroLevel, const std::vector<std::pair<CreatureID, uint16_t>> & garrison)
	{
		const int opponentIdx = heroIdx == 0 ? 1 : 0;

		resetGame();
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.name("SpecialtyTest")
			.playerActive(PlayerColor(0))
			.playerActive(PlayerColor(1))
			.hero({5, 5, 0}, HeroTypeID(heroIdx),     PlayerColor(0)).heroGarrison(garrison)
			.hero({7, 7, 0}, HeroTypeID(opponentIdx), PlayerColor(1)).heroGarrison({{CreatureID(0), 1}});

		startTinyGame(builder);
		auto * hero = getHeroByOwner(PlayerColor(0));
		hero->level = heroLevel;
		return hero;
	}

	/// Give a hero a full spellbook entry, identical magic stats and mana so
	/// that only its innate specialty distinguishes it from another caster.
	void configureCaster(CGHeroInstance * hero, SpellID spell, int spellpower)
	{
		hero->setPrimarySkill(PrimarySkill::SPELL_POWER, spellpower, ChangeValueMode::ABSOLUTE);
		hero->setPrimarySkill(PrimarySkill::KNOWLEDGE, 20, ChangeValueMode::ABSOLUTE);
		for(auto school : {SecondarySkill::AIR_MAGIC, SecondarySkill::FIRE_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC})
			hero->setSecSkillLevel(school, 3, ChangeValueMode::ABSOLUTE);

		if(!hero->hasSpellbook())
		{
			NewArtifact na;
			na.artHolder = hero->id;
			na.artId = ArtifactID::SPELLBOOK;
			na.pos = ArtifactPosition::SPELLBOOK;
			gameEventCallback->sendAndApply(na);
		}

		hero->addSpellToSpellbook(spell);
		hero->mana = 99999;
	}

	/// Append a stack to an ongoing battle and return the created unit.
	CStack * addStack(BattleSide side, const CreatureID & cid, int count)
	{
		auto * battle = gameState->currentBattles.front().get();

		battle::UnitInfo info;
		info.id = battle->battleNextUnitId();
		info.count = count;
		info.type = cid;
		info.side = side;
		info.position = battle->getAvailableHex(info.type.toEntity(LIBRARY), info.side);
		info.summoned = false;

		BattleUnitsChanged pack;
		pack.battleID = BattleID(0);
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		gameEventCallback->sendAndApply(pack);

		return battle->getStack(info.id);
	}

	/// Cast a spell from a hero caster aimed at a unit. Targeting adapts to the
	/// spell's aim type: creature spells target the unit, location spells target
	/// the unit's hex, mass spells take no target.
	void castOn(CGHeroInstance * caster, SpellID spellId, const CStack * target)
	{
		using namespace ::testing;

		auto * battle = gameState->currentBattles.front().get();
		const CSpell * spell = spellId.toSpell();
		ASSERT_NE(spell, nullptr);

		spells::BattleCast cast(battle, caster, spells::Mode::HERO, spell);
		spells::Target tgt;
		switch(spell->getTargetType())
		{
			case spells::AimType::CREATURE:
				tgt.emplace_back(target);
				break;
			case spells::AimType::LOCATION:
				tgt.emplace_back(target->getPosition());
				break;
			default: // NOTHING - mass spell
				break;
		}

		spells::ProblemMock problemMock;
		EXPECT_CALL(problemMock, add(_)).Times(AnyNumber());

		auto m = spell->battleMechanics(&cast);
		ASSERT_TRUE(m->canBeCast(problemMock));
		ASSERT_TRUE(m->canBeCastAt(tgt, problemMock));
		cast.cast(this, tgt);
	}

	/// Whether a hero caster could legally cast a spell at a unit right now.
	bool canCastAt(CGHeroInstance * caster, SpellID spellId, const CStack * target)
	{
		using namespace ::testing;
		auto * battle = gameState->currentBattles.front().get();
		const CSpell * spell = spellId.toSpell();
		spells::BattleCast cast(battle, caster, spells::Mode::HERO, spell);
		spells::Target tgt;
		tgt.emplace_back(target);
		spells::ProblemMock problemMock;
		EXPECT_CALL(problemMock, add(_)).Times(AnyNumber());
		auto m = spell->battleMechanics(&cast);
		return m->canBeCast(problemMock) && m->canBeCastAt(tgt, problemMock);
	}

	CGHeroInstance * specialist = nullptr;
	CGHeroInstance * opponent = nullptr;
};

//Issue #2765, Ghost Dragons can cast Age on Catapults
TEST_F(BattleSpellCastTest, issue2765)
{
	startTestGame();

	auto attacker = getHeroByOwner(PlayerColor(0));
	auto defender = getHeroByOwner(PlayerColor(1));

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);
	ASSERT_NE(attacker->tempOwner, defender->tempOwner);

	{
		NewArtifact na;
		na.artHolder = defender->id;
		na.artId = ArtifactID::BALLISTA;
		na.pos = ArtifactPosition::MACH1;
		gameEventCallback->sendAndApply(na);
	}

	startTestBattle(attacker, defender);

	{
		battle::UnitInfo info;
		info.id = gameState->currentBattles.front()->battleNextUnitId();
		info.count = 1;
		info.type = CreatureID(69);
		info.side = BattleSide::ATTACKER;
		info.position = gameState->currentBattles.front()->getAvailableHex(info.type.toEntity(LIBRARY), info.side);
		info.summoned = false;

		BattleUnitsChanged pack;
		pack.battleID = BattleID(0);
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		gameEventCallback->sendAndApply(pack);
	}

	const CStack * att = nullptr;
	const CStack * def = nullptr;

	for(const auto & s : gameState->currentBattles.front()->stacks)
	{
		if(s->isBallista() && s->unitSide() == BattleSide::DEFENDER)
			def = s.get();
		else if(s->unitType()->getId() == CreatureID(69) && s->unitSide() == BattleSide::ATTACKER)
			att = s.get();
	}
	ASSERT_NE(att, nullptr);
	ASSERT_NE(def, nullptr);
	ASSERT_NE(att, def);

	EXPECT_NE(att->getMyHero(), defender);
	EXPECT_NE(def->getMyHero(), attacker);

	EXPECT_EQ(att->getMyHero(), attacker) << att->nodeName();
	EXPECT_EQ(def->getMyHero(), defender) << def->nodeName();

	{
		using namespace ::testing;

		spells::ProblemMock problemMock;
//		EXPECT_CALL(problemMock, add(_));

		const CSpell * age = SpellID(SpellID::AGE).toSpell();
		ASSERT_NE(age, nullptr);

		spells::AbilityCaster caster(att, 3);

		//here tested ballista, but this applied to all war machines
		spells::BattleCast cast(gameState->currentBattles.front().get(), &caster, spells::Mode::PASSIVE, age);

		spells::Target target;
		target.emplace_back(def);

		auto m = age->battleMechanics(&cast);

		EXPECT_FALSE(m->canBeCastAt(target, problemMock));

		EXPECT_TRUE(cast.castIfPossible(this, target));//should be possible, but with no effect (change to aimed cast check?)

		EXPECT_TRUE(def->activeSpells().empty());
	}
}

TEST_F(BattleSpellCastTest, battleResurrection)
{
	startTestGame();

	auto attacker = getHeroByOwner(PlayerColor(0));
	auto defender = getHeroByOwner(PlayerColor(1));

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);
	ASSERT_NE(attacker->tempOwner, defender->tempOwner);

	attacker->setSecSkillLevel(SecondarySkill::EARTH_MAGIC, 3, ChangeValueMode::ABSOLUTE);
	attacker->addSpellToSpellbook(SpellID::RESURRECTION);
	attacker->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, ChangeValueMode::ABSOLUTE);
	attacker->setPrimarySkill(PrimarySkill::KNOWLEDGE, 20, ChangeValueMode::ABSOLUTE);
	attacker->mana = attacker->manaLimit();

	{
		NewArtifact na;
		na.artHolder = attacker->id;
		na.artId = ArtifactID::SPELLBOOK;
		na.pos = ArtifactPosition::SPELLBOOK;
		gameEventCallback->sendAndApply(na);
	}

	startTestBattle(attacker, defender);

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(13), 10);
	addStack(BattleSide::DEFENDER, CreatureID(13), 10);

	ASSERT_NE(unit, nullptr);

	int64_t damage = unit->getMaxHealth() + 1;

	unit->damage(damage);

	EXPECT_EQ(unit->getCount(), 9);

	{
		using namespace ::testing;

		spells::ProblemMock problemMock;
		EXPECT_CALL(problemMock, add(_)).Times(AnyNumber()); //todo: do smth with problems of optional effects

		const CSpell * spell = SpellID(SpellID::RESURRECTION).toSpell();
		ASSERT_NE(spell, nullptr);

			spells::BattleCast cast(gameState->currentBattles.front().get(), attacker, spells::Mode::HERO, spell);

		spells::Target target;
		target.emplace_back(unit);

		auto m = spell->battleMechanics(&cast);

		EXPECT_TRUE(m->canBeCast(problemMock));

		EXPECT_TRUE(m->canBeCastAt(target, problemMock));

		cast.cast(this, target);
//
//		std::vector<std::string> expLog;
//
//		EXPECT_THAT(problemMock.log, ContainerEq(expLog));
	}

	EXPECT_EQ(unit->health.getCount(), 10);
	EXPECT_EQ(unit->health.getResurrected(), 0);
}

TEST_F(BattleSpellCastTest, battleInterference)
{
	static const char skillText[] = R"(
	{
		"type" : "PRIMARY_SKILL",
		"subtype" : "spellpower",
		"val" : -10,
		"valueType" : "PERCENT_TO_ALL",
		"propagator" : "BATTLE_WIDE",
		"sourceType" : "SECONDARY_SKILL",
		"sourceID" : "wisdom",
		"propagationUpdater" : "BONUS_OWNER_UPDATER",
		"limiters" : [ "OPPOSITE_SIDE" ]
	}
	)";

	static const char specialtyText[] = R"(
	{
		"type" : "PRIMARY_SKILL",
		"subtype" : "spellpower",
		"val" : 5,
		"sourceType" : "HERO_SPECIAL",
		"sourceID" : "lordHaart",
		"targetSourceType" : "SECONDARY_SKILL",
		"valueType" : "PERCENT_TO_TARGET_TYPE",
		"propagator" : "BATTLE_WIDE",
		"propagationUpdater" : [ "TIMES_HERO_LEVEL", "BONUS_OWNER_UPDATER" ],
		"limiters" : [ "OPPOSITE_SIDE" ]
	}
	)";
	JsonNode skillJson(skillText, std::size(skillText), "testBattleInterferenceSkillText");
	JsonNode specialtyJson(specialtyText, std::size(specialtyText), "testBattleInterferenceSpecialtyTextA");

	skillJson.setModScope(ModScope::scopeGame());
	specialtyJson.setModScope(ModScope::scopeGame());

	auto skillBonus = JsonUtils::parseBonus(skillJson);
	auto specialtyBonus = JsonUtils::parseBonus(specialtyJson);

	startTestGame();

	auto attacker = getHeroByOwner(PlayerColor(0));
	auto defender = getHeroByOwner(PlayerColor(1));

	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);
	ASSERT_NE(attacker->tempOwner, defender->tempOwner);

	attacker->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, ChangeValueMode::ABSOLUTE);
	attacker->addNewBonus(skillBonus);
	attacker->addNewBonus(specialtyBonus);
	attacker->level = 20;

	defender->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, ChangeValueMode::ABSOLUTE);
	defender->level = 10;

	ASSERT_EQ(attacker->getPrimSkillLevel(PrimarySkill::SPELL_POWER), 100);
	ASSERT_EQ(defender->getPrimSkillLevel(PrimarySkill::SPELL_POWER), 100);

	startTestBattle(attacker, defender);

	EXPECT_EQ(attacker->getPrimSkillLevel(PrimarySkill::SPELL_POWER), 100);
	EXPECT_EQ(defender->getPrimSkillLevel(PrimarySkill::SPELL_POWER), 80);
}

// Chain Lightning must chain to distinct nearest units (not hit the same unit repeatedly)
// and halve its damage on each hop (chainFactor). Regression after spell effects moved to Lua:
// the chain re-selected the aimed unit because target de-duplication keyed on Lua userdata
// identity, which differs per push for the same underlying unit.
TEST_F(BattleSpellCastTest, chainLightningChainsToDistinctUnits)
{
	using namespace ::testing;

	const CreatureID pikeman(0); // single-hex, no magic resistance
	const uint16_t stackCount = 1000; // large enough that no stack is wiped, so HP delta == damage

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("ChainLightningTest")
		.playerActive(PlayerColor(0))
		.playerActive(PlayerColor(1))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
			.heroPrimary(0, 0, 20, 20)
			.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
			.heroSpells({SpellID(SpellID::CHAIN_LIGHTNING)})
			.heroGarrison({{pikeman, 1}}) // token army so the battle is valid
		.hero({7, 7, 0}, HeroTypeID(1), PlayerColor(1))
			.heroGarrison({{pikeman, stackCount}, {pikeman, stackCount}, {pikeman, stackCount},
			               {pikeman, stackCount}, {pikeman, stackCount}});

	startTinyGame(builder);

	auto attacker = getHeroByOwner(PlayerColor(0));
	auto defender = getHeroByOwner(PlayerColor(1));
	ASSERT_NE(attacker, nullptr);
	ASSERT_NE(defender, nullptr);

	attacker->mana = 999;

	startTestBattle(attacker, defender);

	auto * battle = gameState->currentBattles.front().get();

	std::vector<const CStack *> defenderStacks;
	for(const auto & s : battle->stacks)
		if(s->unitSide() == BattleSide::DEFENDER && s->unitType()->getId() == pikeman)
			defenderStacks.push_back(s.get());
	ASSERT_EQ(defenderStacks.size(), 5u);

	std::map<uint32_t, int64_t> healthBefore;
	for(const auto * s : defenderStacks)
		healthBefore[s->unitId()] = s->getAvailableHealth();

	const CSpell * spell = SpellID(SpellID::CHAIN_LIGHTNING).toSpell();
	ASSERT_NE(spell, nullptr);

	spells::BattleCast cast(battle, attacker, spells::Mode::HERO, spell);
	spells::Target target;
	target.emplace_back(defenderStacks.front());

	spells::ProblemMock problemMock;
	EXPECT_CALL(problemMock, add(_)).Times(AnyNumber());

	auto m = spell->battleMechanics(&cast);
	ASSERT_TRUE(m->canBeCast(problemMock));
	ASSERT_TRUE(m->canBeCastAt(target, problemMock));

	cast.cast(this, target);

	std::vector<int64_t> damages;
	for(const auto * s : defenderStacks)
	{
		int64_t delta = healthBefore.at(s->unitId()) - s->getAvailableHealth();
		if(delta > 0)
			damages.push_back(delta);
	}

	// Base Chain Lightning hits 4 distinct units. The regression would damage only 1.
	ASSERT_EQ(damages.size(), 4u) << "Chain Lightning must hit 4 distinct units";

	// Identical units, so each hop deals chainFactor (0.5) of the previous, floored.
	std::sort(damages.begin(), damages.end(), std::greater<int64_t>());
	const int64_t base = damages[0];
	EXPECT_EQ(damages[1], static_cast<int64_t>(std::floor(0.5 * base)));
	EXPECT_EQ(damages[2], static_cast<int64_t>(std::floor(0.25 * base)));
	EXPECT_EQ(damages[3], static_cast<int64_t>(std::floor(0.125 * base)));
}

// ===========================================================================
// Spell-magnitude specialties (SPECIAL_SPELL_SCALING, SPECIFIC_SPELL_DAMAGE)
//
// A specialist casts a damaging spell once with its innate specialty and once
// after the specialty is stripped (same hero, spell power, school mastery and
// level). The specialty must scale the effect by the factor the engine applies
// in CGHeroInstance::getSpellBonus.
//
// SPECIFIC_SPELL_DAMAGE is a flat base% regardless of level. The per-level
// specialty (SPECIAL_SPELL_SCALING, generated by the spellScalingPercentage
// shortcut) gives H3-correct  base% * floor(heroLevel / targetLevel)  ("base%
// for every <targetLevel> hero levels"), including cases where targetLevel does
// not divide heroLevel.
// ===========================================================================

enum class MagnitudeKind { Damage, Heal };

struct MagnitudeCase
{
	const char *  name;
	int           specialistHero;
	int           spellIndex;
	int           targetCreature; // no-resistance creature of known tier
	MagnitudeKind kind;
	int           heroLevel;
};

class SpellMagnitudeSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<MagnitudeCase>
{
public:
	/// Set up a fresh battle, cast the case's spell once and return the size of the
	/// health swing on the target (damage dealt, or health restored for heal cases).
	/// With `withSpecialty == false` the hero's specialty is stripped first, giving
	/// the specialty-free baseline for the exact same hero, spell and target.
	int64_t measureEffect(const MagnitudeCase & c, bool withSpecialty)
	{
		const SpellID spell(c.spellIndex);
		const CreatureID target(c.targetCreature);
		const uint16_t count = 5000;

		resetGame();
		startDuel(c.specialistHero, c.heroLevel);
		configureCaster(specialist, spell, /*spellpower*/ 10);

		const BattleSide side = (c.kind == MagnitudeKind::Damage) ? BattleSide::DEFENDER : BattleSide::ATTACKER;
		CStack * tgt = addStack(side, target, count);

		if(c.kind == MagnitudeKind::Heal)
		{
			int64_t wound = 4000 * tgt->getMaxHealth(); // large dead pool so the raise isn't count-capped
			tgt->damage(wound);
		}

		if(!withSpecialty)
			removeSpecialty(specialist);

		const int64_t before = tgt->getAvailableHealth();
		castOn(specialist, spell, tgt);
		const int64_t after = tgt->getAvailableHealth();

		return c.kind == MagnitudeKind::Damage ? before - after : after - before;
	}
};

TEST_P(SpellMagnitudeSpecialty, matchesBaseline)
{
	const auto & c = GetParam();

	// Reproduce the specialty factor from config using the H3-correct floor formula.
	const SpellID spell(c.spellIndex);
	std::shared_ptr<Bonus> spec;
	for(const auto & b : LIBRARY->heroh->objects.at(c.specialistHero)->specialty)
		if(b->subtype == BonusSubtypeID(spell))
			spec = b;
	ASSERT_NE(spec, nullptr) << c.name << ": hero has no specialty for this spell";

	const int targetLevel = CreatureID(c.targetCreature).toCreature()->getLevel();
	const bool perLevel = spec->type == BonusType::SPECIAL_SPELL_LEV || spec->type == BonusType::SPECIAL_SPELL_SCALING;
	const int percent = perLevel
		? spec->val * (c.heroLevel / targetLevel) // base% * floor(heroLevel / targetLevel)
		: spec->val;                              // SPECIFIC_SPELL_DAMAGE: flat base%

	const int64_t specValue = measureEffect(c, /*withSpecialty*/ true);
	const int64_t baseValue = measureEffect(c, /*withSpecialty*/ false);

	ASSERT_GT(baseValue, 0) << c.name << ": baseline had no measurable effect";

	const int64_t expected = baseValue * (100 + percent) / 100;

	EXPECT_EQ(specValue, expected)
		<< c.name << " (spell " << c.spellIndex << ", target level " << targetLevel
		<< ", +" << percent << "%) specialist=" << specValue << " baseline=" << baseValue;
}

INSTANTIATE_TEST_SUITE_P(Heroes, SpellMagnitudeSpecialty, ::testing::Values(
	// Level-1 targets: floor(heroLevel / 1) == heroLevel.
	MagnitudeCase{"solmyr_chainLightning", 45, 19, 0, MagnitudeKind::Damage,  8},
	MagnitudeCase{"alagar_iceBolt",        30, 16, 0, MagnitudeKind::Damage, 13},
	MagnitudeCase{"ciele_magicArrow",     138, 15, 0, MagnitudeKind::Damage,  6}, // SPECIFIC_SPELL_DAMAGE, flat
	MagnitudeCase{"xarfax_fireball",       63, 21, 0, MagnitudeKind::Damage,  9}, // location-targeted
	MagnitudeCase{"xyron_inferno",         57, 22, 0, MagnitudeKind::Damage, 11},
	MagnitudeCase{"deemer_meteorShower",   93, 23, 0, MagnitudeKind::Damage,  7},
	MagnitudeCase{"aislinn_meteorShower",  73, 23, 0, MagnitudeKind::Damage,  5},
	// Higher-tier targets with heroLevel not divisible by target level, exercising the
	// H3-correct rounding base% * floor(heroLevel / targetLevel).
	MagnitudeCase{"solmyr_chainLightning_cavalier", 45, 19, 10, MagnitudeKind::Damage, 10}, // level 6
	MagnitudeCase{"alagar_iceBolt_archer",          30, 16,  2, MagnitudeKind::Damage,  7}, // level 2
	// Per-level scaling applied to healing / raising: heal.lua scales the raise via applySpellBonus.
	MagnitudeCase{"alamar_resurrection", 88, 38,  0, MagnitudeKind::Heal,  9}, // orc, level 3
	MagnitudeCase{"thant_animateDead",   76, 39, 56, MagnitudeKind::Heal,  8}  // medusa, level 4
),
	[](const ::testing::TestParamInfo<MagnitudeCase> & info) { return info.param.name; });

// ===========================================================================
// Enchant specialties (SPECIAL_PECULIAR_ENCHANT, SPECIAL_ADD_VALUE_ENCHANT,
// SPECIAL_FIXED_VALUE_ENCHANT)
//
// The specialty nudges the value of the bonus an enchantment spell places on the
// affected unit (see scripts/timed.lua:applyHeroSpecialty). Each case names the
// exact bonus the spell provides (e.g. STACKS_SPEED for Haste); the test reads
// that bonus's value coming from this spell's effect, with and without the
// specialty, and checks the shift. Positive spells land on the caster's own
// stack, negative ones on the enemy.
// ===========================================================================

struct EnchantCase
{
	const char *   name;
	int            hero;
	int            spellIndex;
	int            targetCreature; // tier drives the peculiar-enchant power bracket
	BonusType      bonusType;      // the specific bonus the spell places on the unit
	BonusSubtypeID subtype;
};

class EnchantSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<EnchantCase>
{
public:
	/// Cast the enchant in a fresh battle and return the value of the named bonus
	/// this spell placed on the target.
	int64_t measureEnchant(const EnchantCase & c, bool withSpecialty)
	{
		const SpellID spell(c.spellIndex);
		const CreatureID target(c.targetCreature);

		resetGame();
		startDuel(c.hero, /*heroLevel*/ 10);
		configureCaster(specialist, spell, /*spellpower*/ 10);

		const BattleSide side = spell.toSpell()->isNegative() ? BattleSide::DEFENDER : BattleSide::ATTACKER;
		CStack * tgt = addStack(side, target, 100);

		if(!withSpecialty)
			removeSpecialty(specialist);

		castOn(specialist, spell, tgt);

		auto sel = Selector::typeSubtype(c.bonusType, c.subtype)
			.And(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spell)));
		return tgt->getBonuses(sel)->totalValue();
	}
};

TEST_P(EnchantSpecialty, matchesBaseline)
{
	const auto & c = GetParam();
	const SpellID spell(c.spellIndex);

	// Read the specialty (type + parameter) straight from config, plus target tier
	// and spell polarity, to reproduce the shift scripts/timed.lua applies.
	std::shared_ptr<Bonus> specialtyBonus;
	for(const auto & b : LIBRARY->heroh->objects.at(c.hero)->specialty)
		if(b->subtype == BonusSubtypeID(spell))
			specialtyBonus = b;
	ASSERT_NE(specialtyBonus, nullptr) << c.name << ": hero has no specialty for this spell";

	const bool vectorParam = specialtyBonus->parameters && specialtyBonus->parameters->isVector();
	const int param = (specialtyBonus->parameters && !vectorParam) ? specialtyBonus->parameters->toNumber() : 0;
	const int tier = CreatureID(c.targetCreature).toCreature()->getLevel();
	const bool negative = spell.toSpell()->isNegative();

	const int64_t baseVal = measureEnchant(c, /*withSpecialty*/ false);
	const int64_t specVal = measureEnchant(c, /*withSpecialty*/ true);

	ASSERT_NE(baseVal, 0) << c.name << ": spell placed no such bonus on the target";

	int64_t expected = 0;
	switch(specialtyBonus->type)
	{
		case BonusType::SPECIAL_PECULIAR_ENCHANT:
		{
			int power = 0;
			if(vectorParam)
			{
				const auto & vec = specialtyBonus->parameters->toVector();
				int idx = std::clamp<int>(tier - 1, 0, static_cast<int>(vec.size()) - 1);
				power = vec.empty() ? 0 : vec[idx]; // explicit sign, no polarity flip
			}
			else
			{
				if(param == 0)
					power = tier <= 2 ? 3 : tier <= 4 ? 2 : tier <= 6 ? 1 : 0;
				if(negative)
					power = -power; // legacy addInfo flips with spell polarity
			}
			expected = baseVal + power;
			break;
		}
		case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
			expected = baseVal + param;
			break;
		case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
			expected = param; // bonus value is overwritten
			break;
		default:
			FAIL() << c.name << ": unexpected specialty type";
	}

	EXPECT_EQ(specVal, expected)
		<< c.name << " (spell " << c.spellIndex << ", tier " << tier << ", param " << param << ")"
		<< " specialist=" << specVal << " baseline=" << baseVal;
}

static const BonusSubtypeID subtypeAttack = BonusSubtypeID(PrimarySkill::ATTACK);
static const BonusSubtypeID subtypeDefence = BonusSubtypeID(PrimarySkill::DEFENSE);

INSTANTIATE_TEST_SUITE_P(Heroes, EnchantSpecialty, ::testing::Values(
	// SPECIAL_PECULIAR_ENCHANT - target tier spread covers the 3/2/1/0 power brackets
	EnchantCase{"cuthbert_weakness",   10, 45,  0, BonusType::PRIMARY_SKILL, subtypeAttack},  // tier 1, negative -> -3
	EnchantCase{"loynis_prayer",       14, 48,  4, BonusType::STACKS_SPEED,  BonusSubtypeID()}, // tier 3 -> +2
	EnchantCase{"darkstorn_stoneSkin", 95, 46,  8, BonusType::PRIMARY_SKILL, subtypeDefence},  // tier 5 -> +1
	EnchantCase{"cyra_haste",          46, 53, 12, BonusType::STACKS_SPEED,  BonusSubtypeID()}, // tier 7 -> 0
	EnchantCase{"brissa_haste",       137, 53,  0, BonusType::STACKS_SPEED,  BonusSubtypeID()}, // tier 1 -> +3
	EnchantCase{"labetha_stoneSkin",  139, 46,  0, BonusType::PRIMARY_SKILL, subtypeDefence},
	EnchantCase{"inteus_bloodlust",   140, 43,  0, BonusType::PRIMARY_SKILL, subtypeAttack},
	EnchantCase{"xsi_stoneSkin",       77, 46,  2, BonusType::PRIMARY_SKILL, subtypeDefence},  // tier 2 -> +3
	EnchantCase{"terek_haste",        107, 53,  6, BonusType::STACKS_SPEED,  BonusSubtypeID()}, // tier 4 -> +2
	EnchantCase{"zubin_precision",    108, 44,  2, BonusType::PRIMARY_SKILL, subtypeAttack},   // precision needs a shooter target
	EnchantCase{"olema_weakness",      59, 45,  4, BonusType::PRIMARY_SKILL, subtypeAttack},   // tier 3, negative -> -2
	EnchantCase{"ash_bloodlust",       61, 43,  0, BonusType::PRIMARY_SKILL, subtypeAttack},
	EnchantCase{"mirlanda_weakness",  120, 45,  8, BonusType::PRIMARY_SKILL, subtypeAttack},   // tier 5, negative -> -1
	EnchantCase{"merist_stoneSkin",   124, 46,  0, BonusType::PRIMARY_SKILL, subtypeDefence},
	EnchantCase{"coronius_slayer",     24, 55,  0, BonusType::SLAYER,        BonusSubtypeID()}, // tier 1 -> +4 attack (addInfo array)
	EnchantCase{"aenain_disruptingRay", 141, 47, 0, BonusType::PRIMARY_SKILL, subtypeDefence}, // addInfo [-2], no polarity flip
	// SPECIAL_FIXED_VALUE_ENCHANT
	EnchantCase{"daremyth_fortune",     43, 51, 0, BonusType::LUCK, BonusSubtypeID()},
	EnchantCase{"melodia_fortune",      29, 51, 4, BonusType::LUCK, BonusSubtypeID()}
),
	[](const ::testing::TestParamInfo<EnchantCase> & info) { return info.param.name; });

// ===========================================================================
// Per-target-level spell specialties: the bonus scales by 3% for every
// <target creature level> hero levels, i.e. 3 * floor(heroLevel / targetLevel).
// Both engine paths under test (SPECIAL_SPELL_SCALING for Astral's Hypnotize,
// TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL for Adela's Bless) round H3-correctly,
// including target levels that do not divide the hero level.
// ===========================================================================

struct SpellLevScaleCase
{
	const char * name;
	int          targetCreature;
	int          heroLevel;
};

// Astral's Hypnotize specialty raises the maximum-health threshold a stack can be
// hypnotised at by 3% for every <target creature level> hero levels. Base cap for
// Expert Air Magic is 50 + spellpower*25. The test gates on castability: a stack whose
// total health sits exactly at the scaled cap is castable, one health point above is not.
class AstralHypnotizeSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<SpellLevScaleCase>
{
public:
	/// Can Astral hypnotise a stack of the case's creature whose total health is set to
	/// exactly `health`? Rebuilds the battle each call so the two boundary probes are independent.
	bool canHypnotiseAtHealth(const SpellLevScaleCase & c, int64_t health)
	{
		const int astralIdx = 40;
		const SpellID hypnotize(60);

		resetGame();
		startDuel(astralIdx, c.heroLevel);
		configureCaster(specialist, hypnotize, /*spellpower*/ 10);

		const CreatureID cid(c.targetCreature);
		const int hp = cid.toCreature()->getMaxHealth();
		const int count = static_cast<int>(health / hp) + 3; // headroom so the stack survives the wound
		CStack * tgt = addStack(BattleSide::DEFENDER, cid, count);
		int64_t wound = static_cast<int64_t>(count) * hp - health; // leave exactly `health` available
		tgt->damage(wound);

		return canCastAt(specialist, hypnotize, tgt);
	}
};

TEST_P(AstralHypnotizeSpecialty, healthThresholdGatesCasting)
{
	const auto & c = GetParam();

	const int targetLevel = CreatureID(c.targetCreature).toCreature()->getLevel();
	const int64_t baseCap = 50 + 10 * 25; // Expert Air Magic at spellpower 10
	const int percent = 3 * (c.heroLevel / targetLevel); // 3% * floor(heroLevel / targetLevel)
	const int64_t cap = baseCap * (100 + percent) / 100;

	EXPECT_TRUE(canHypnotiseAtHealth(c, cap))
		<< c.name << ": health " << cap << " at cap (+" << percent << "%) should be castable";
	EXPECT_FALSE(canHypnotiseAtHealth(c, cap + 1))
		<< c.name << ": health " << (cap + 1) << " above cap (+" << percent << "%) should be blocked";
}

INSTANTIATE_TEST_SUITE_P(Astral, AstralHypnotizeSpecialty, ::testing::Values(
	SpellLevScaleCase{"pikeman_L5",    0,  5}, // level 1: +15%
	SpellLevScaleCase{"pikeman_L12",   0, 12}, // level 1: +36%
	SpellLevScaleCase{"griffin_L12",   4, 12}, // level 3, divisible: +12%
	SpellLevScaleCase{"griffin_L10",   4, 10}, // level 3, not divisible: floor(10/3)=3 -> +9%
	SpellLevScaleCase{"swordsman_L10", 6, 10}, // level 4, not divisible: floor(10/4)=2 -> +6%
	SpellLevScaleCase{"cavalier_L15", 10, 15}  // level 6, not divisible: floor(15/6)=2 -> +6%
),
	[](const ::testing::TestParamInfo<SpellLevScaleCase> & info) { return info.param.name; });

// Adela's Bless specialty (GENERAL_DAMAGE_PREMY): blessed units deal more damage.
class AdelaBlessSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<SpellLevScaleCase>
{
};

TEST_P(AdelaBlessSpecialty, damagePremyScalesWithLevel)
{
	const auto & c = GetParam();
	const int adelaIdx = 9;
	const SpellID bless(41);

	resetGame();
	startDuel(adelaIdx, c.heroLevel);
	configureCaster(specialist, bless, /*spellpower*/ 10);

	CStack * tgt = addStack(BattleSide::ATTACKER, CreatureID(c.targetCreature), 100);
	const int targetLevel = tgt->creatureLevel();

	// The limiter needs an active Bless on the stack: unblessed grants nothing.
	EXPECT_EQ(tgt->valOfBonuses(BonusType::GENERAL_DAMAGE_PREMY), 0);

	castOn(specialist, bless, tgt);

	const int expected = 3 * (c.heroLevel / targetLevel); // 3 * floor(heroLevel / targetLevel)
	EXPECT_EQ(tgt->valOfBonuses(BonusType::GENERAL_DAMAGE_PREMY), expected)
		<< c.name << " (target level " << targetLevel << ")";
}

INSTANTIATE_TEST_SUITE_P(Adela, AdelaBlessSpecialty, ::testing::Values(
	SpellLevScaleCase{"pikeman_L12",   0, 12}, // level 1: +36
	SpellLevScaleCase{"pikeman_L7",    0,  7}, // level 1: +21
	SpellLevScaleCase{"cavalier_L12", 10, 12}, // level 6, divisible: +6
	SpellLevScaleCase{"cavalier_L15", 10, 15}, // level 6, not divisible: floor(15/6)=2 -> +6
	SpellLevScaleCase{"griffin_L10",   4, 10}, // level 3, not divisible: floor(10/3)=3 -> +9
	SpellLevScaleCase{"swordsman_L10", 6, 10}  // level 4, not divisible: floor(10/4)=2 -> +6
),
	[](const ::testing::TestParamInfo<SpellLevScaleCase> & info) { return info.param.name; });

// ===========================================================================
// spellScalingPercentage on buff / debuff spells (vcmi-test mod heroes).
//
// Each hero's only distinction is a spellScalingPercentage specialty: the cast
// spell's buff/debuff magnitude grows by val% for every <target creature level>
// hero levels, i.e. val * floor(heroLevel / targetLevel). This is the timed-effect
// path (scripts/timed.lua:applySpellScaling) of SPECIAL_SPELL_SCALING; the damage
// and heal paths of the same bonus are covered by SpellMagnitudeSpecialty above.
// ===========================================================================

struct ScaleCase
{
	const char *   name;
	const char *   heroIdent;
	const char *   spellIdent;
	int            targetCreature; // creature whose level drives the per-step divisor
	int            heroLevel;
	BonusType      bonusType;      // the bonus the spell places on the unit
	BonusSubtypeID subtype;
};

class SpellScalingSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<ScaleCase>
{
public:
	/// Cast the spell and read back the value of the bonus it placed on the target.
	/// With withSpecialty == false the specialty is stripped for the specialty-free baseline.
	int64_t measure(const ScaleCase & c, bool withSpecialty)
	{
		const int heroIdx = HeroTypeID::decode(c.heroIdent);
		const SpellID spell(SpellID::decode(c.spellIdent));

		resetGame();
		startDuel(heroIdx, c.heroLevel, EMapFormat::HOTA); // mod heroes exceed the SOD hero index range
		configureCaster(specialist, spell, /*spellpower*/ 10);

		const bool negative = spell.toSpell()->isNegative();
		CStack * tgt = addStack(negative ? BattleSide::DEFENDER : BattleSide::ATTACKER, CreatureID(c.targetCreature), 100);

		if(!withSpecialty)
			removeSpecialty(specialist);

		castOn(specialist, spell, tgt);
		auto sel = Selector::typeSubtype(c.bonusType, c.subtype)
			.And(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spell)));
		return tgt->getBonuses(sel)->totalValue();
	}
};

TEST_P(SpellScalingSpecialty, matchesBaseline)
{
	const auto & c = GetParam();
	const SpellID spell(SpellID::decode(c.spellIdent));

	std::shared_ptr<Bonus> spec;
	for(const auto & b : LIBRARY->heroh->objects.at(HeroTypeID::decode(c.heroIdent))->specialty)
		if(b->type == BonusType::SPECIAL_SPELL_SCALING && b->subtype == BonusSubtypeID(spell))
			spec = b;
	ASSERT_NE(spec, nullptr) << c.name << ": hero has no spell-scaling specialty for this spell";

	const int targetLevel = CreatureID(c.targetCreature).toCreature()->getLevel();
	const int percent = spec->val * (c.heroLevel / targetLevel); // val% * floor(heroLevel / targetLevel)

	const int64_t baseValue = measure(c, /*withSpecialty*/ false);
	const int64_t specValue = measure(c, /*withSpecialty*/ true);

	ASSERT_GT(baseValue, 0) << c.name << ": baseline had no measurable effect";

	const int64_t expected = baseValue * (100 + percent) / 100;
	EXPECT_EQ(specValue, expected)
		<< c.name << " (target level " << targetLevel << ", +" << percent << "%)"
		<< " specialist=" << specValue << " baseline=" << baseValue;
}

INSTANTIATE_TEST_SUITE_P(Heroes, SpellScalingSpecialty, ::testing::Values(
	// griffin = level 3, hero level 10 -> floor(10/3)=3 -> +30%
	ScaleCase{"shield",        "vcmi-test:shieldSpecialist",        "shield",        4, 10, BonusType::GENERAL_DAMAGE_REDUCTION, BonusSubtypeID(BonusCustomSubtype::damageTypeMelee)},
	ScaleCase{"airShield",     "vcmi-test:airShieldSpecialist",     "airShield",     4, 10, BonusType::GENERAL_DAMAGE_REDUCTION, BonusSubtypeID(BonusCustomSubtype::damageTypeRanged)},
	// archer = level 2, hero level 7 -> floor(7/2)=3 -> +30%; forgetfulness needs a shooter target
	ScaleCase{"frenzy",        "vcmi-test:frenzySpecialist",        "frenzy",        2,  7, BonusType::IN_FRENZY,  BonusSubtypeID()},
	ScaleCase{"forgetfulness", "vcmi-test:forgetfulnessSpecialist", "forgetfulness", 2,  7, BonusType::FORGETFULL, BonusSubtypeID()}
),
	[](const ::testing::TestParamInfo<ScaleCase> & info) { return info.param.name; });

// ===========================================================================
// Non-battle specialties (read directly off the hero / its army stacks).
// ===========================================================================

// H3's creature specialty rounds its percentage bonus up (only place that does);
// every other bonus source rounds down. These mirror lib/bonuses/BonusList.cpp.
static int applyPercentUp(int base, int percent)
{
	if(base >= 0)
		return (static_cast<int64_t>(base) * (100 + percent) + 99) / 100;
	return (static_cast<int64_t>(base) * (100 + percent) - 99) / 100;
}
static int applyPercentDown(int base, int percent)
{
	return (static_cast<int64_t>(base) * (100 + percent)) / 100;
}

// --- creature specialty (level-scaled attack/defense + flat speed) ---
//
// Unlike the spell specialties above, creature specialties use
// TimesHeroLevelUpdater(creatureLevel), which computes growth * floor(heroLevel /
// creatureLevel) - the division happens before the multiply, so the H3-correct
// rounding holds and non-divisible hero levels are handled correctly.

struct CreatureCase
{
	const char * name;
	int          heroIdx;
	int          creatureIdx;   // creature placed in the specialist's army
	int          creatureLevel; // divisor used by the specialty (explicit level, or the creature's own)
	int          heroLevel;
};

class CreatureSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<CreatureCase>
{
};

TEST_P(CreatureSpecialty, scalesStackStats)
{
	const auto & c = GetParam();

	CGHeroInstance * hero = placeHero(c.heroIdx, c.heroLevel, {{CreatureID(c.creatureIdx), 100}});
	// Zero the hero primaries so the stack's attack/defense come only from the
	// creature (CREATURE_ABILITY source), which is what the specialty scales.
	hero->setPrimarySkill(PrimarySkill::ATTACK, 0, ChangeValueMode::ABSOLUTE);
	hero->setPrimarySkill(PrimarySkill::DEFENSE, 0, ChangeValueMode::ABSOLUTE);

	const CStackInstance & stack = hero->getStack(SlotID(0));
	const int attWith = stack.getAttack(false);
	const int defWith = stack.getDefense(false);
	const int speedWith = stack.getMovementRange();

	removeSpecialty(hero);
	const int attWithout = stack.getAttack(false);
	const int defWithout = stack.getDefense(false);
	const int speedWithout = stack.getMovementRange();

	const int percent = 5 * (c.heroLevel / c.creatureLevel); // growth 5, TimesHeroLevel(creatureLevel)

	EXPECT_EQ(attWith, applyPercentUp(attWithout, percent)) << c.name << " attack";
	EXPECT_EQ(defWith, applyPercentUp(defWithout, percent)) << c.name << " defense";
	EXPECT_EQ(speedWith - speedWithout, 1) << c.name << " speed";
}

INSTANTIATE_TEST_SUITE_P(Heroes, CreatureSpecialty, ::testing::Values(
	CreatureCase{"valeska_archer",       1,   2, 2, 10}, // level 2 creature, divisible
	CreatureCase{"valeska_archer_L9",    1,   2, 2,  9}, // level 2, not divisible: 5*floor(9/2)=20
	CreatureCase{"shakti_troglodyte",   87,  70, 1,  7}, // level 1 creature
	CreatureCase{"tyris_cavalier",       7,  10, 6, 13}, // level 6 creature, not divisible: 5*floor(13/6)=10
	CreatureCase{"valeska_marksman",     1,   3, 2, 10}  // upgrade of archer -> includeUpgrades
	// NOTE: the only explicit-creatureLevel specialists are all ballista (war machine)
	// heroes; war machines are not received as normal army stacks, so they are not
	// covered here. The explicit level is not a distinct code path from the implicit one.
),
	[](const ::testing::TestParamInfo<CreatureCase> & info) { return info.param.name; });

// --- secondary-skill specialty (level-scaled % on the skill's own bonus) ---
//
// The specialty adds PERCENT_TO_TARGET_TYPE of 5*heroLevel over the SECONDARY_SKILL
// source, scaling the value of the skill's own bonus. The exact bonus (type +
// subtype) the specialty scales is given per case rather than read from config, so
// the test pins down what it checks. Adelaide serves as the specialty-free control:
// her only secondary skill (Wisdom) never carries a specialty, so giving her the
// tested skill at expert reproduces the plain, unscaled skill bonus.

struct SkillCase
{
	const char *   name;
	int            heroIdx;
	int            skillIdx;
	BonusType      bonusType;
	BonusSubtypeID subtype;
	int            baseValue; // 0 for flat (BASE_NUMBER) effects; 100 for percentage effects
	int            heroLevel;
};

class SecondarySkillSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<SkillCase>
{
public:
	/// The amount the case's skill (at expert) adds to the named bonus for a hero:
	/// value with the skill minus value without it. Taking the per-hero difference
	/// cancels any innate, non-skill contribution to the same bonus (e.g. a hero's
	/// base mana regeneration or spell damage), leaving only the skill's part - which
	/// is what the specialty scales. Percentage effects (baseValue 100) are read
	/// against a fixed base so their contribution is measurable at all.
	int64_t skillContribution(int heroIdx, const SkillCase & c)
	{
		const auto sel = Selector::typeSubtype(c.bonusType, c.subtype);
		CGHeroInstance * hero = placeHero(heroIdx, c.heroLevel, {{CreatureID(0), 1}});

		hero->setSecSkillLevel(SecondarySkill(c.skillIdx), 0, ChangeValueMode::ABSOLUTE); // absent
		const int64_t without = hero->valOfBonuses(sel, "", c.baseValue);
		hero->setSecSkillLevel(SecondarySkill(c.skillIdx), 3, ChangeValueMode::ABSOLUTE); // expert
		const int64_t with = hero->valOfBonuses(sel, "", c.baseValue);
		return with - without;
	}
};

TEST_P(SecondarySkillSpecialty, scalesSkillBonus)
{
	const auto & c = GetParam();
	const int adelaideIdx = 11; // only skill is Wisdom, which never has a specialty

	const int64_t withSpec    = skillContribution(c.heroIdx, c);
	const int64_t withoutSpec = skillContribution(adelaideIdx, c);

	ASSERT_GT(withoutSpec, 0) << c.name << ": skill provides no measurable bonus";
	// mysticism specialty has a hidden +1 flat mana regeneration
	if (c.skillIdx == 8)
	{
		EXPECT_EQ(withSpec, applyPercentDown(withoutSpec, 5 * c.heroLevel) + 1) << c.name;
	} else {
		EXPECT_EQ(withSpec, applyPercentDown(withoutSpec, 5 * c.heroLevel)) << c.name;
	}	
}

INSTANTIATE_TEST_SUITE_P(Heroes, SecondarySkillSpecialty, ::testing::Values(
	SkillCase{"orrin_archery",         0,  1, BonusType::PERCENTAGE_DAMAGE_BOOST,     BonusSubtypeID(BonusCustomSubtype::damageTypeRanged), 0,  7},
	SkillCase{"gundula_offence",     109, 22, BonusType::PERCENTAGE_DAMAGE_BOOST,     BonusSubtypeID(BonusCustomSubtype::damageTypeMelee),  0, 13},
	SkillCase{"neela_armorer",        35, 23, BonusType::GENERAL_DAMAGE_REDUCTION,    BonusSubtypeID(BonusCustomSubtype::damageTypeAll),    0,  9},
	SkillCase{"lordHaart_estates",     4, 13, BonusType::GENERATE_RESOURCE,           BonusSubtypeID(GameResID(GameResID::GOLD)),           0, 11},
	SkillCase{"isra_necromancy",      69, 12, BonusType::UNDEAD_RAISE_PERCENTAGE,     BonusSubtypeID(),                                     0,  8},
	SkillCase{"thorgrim_resistance",  20, 26, BonusType::MAGIC_RESISTANCE,            BonusSubtypeID(),                                     0, 14},
	// firstAid: specialty scales SPECIFIC_SPELL_POWER of the First Aid Tent spell, whose
	// SpellID is not in the base enum (mod/creature-ability spell) - skipped for now.
	// SkillCase{"gem_firstAid",      27, 27, BonusType::SPECIFIC_SPELL_POWER,        BonusSubtypeID(<firstAid spell>), 0,  6},
	SkillCase{"axsis_mysticism",      58,  8, BonusType::MANA_REGENERATION,           BonusSubtypeID(),                                     0, 12},
	SkillCase{"gird_sorcery",        104, 25, BonusType::SPELL_DAMAGE,                BonusSubtypeID(SpellSchool::ANY),                     0, 10},
	SkillCase{"geon_eagleEye",        92, 11, BonusType::LEARN_BATTLE_SPELL_CHANCE,   BonusSubtypeID(),                                     0, 16},
	// percentage-valued skills: measured against a fixed base (see measure()).
	SkillCase{"kyrre_logistics",      23,  2, BonusType::MOVEMENT,                    BonusSubtypeID(BonusCustomSubtype::heroMovementLand), 100, 20},
	SkillCase{"ayden_intelligence",   56, 24, BonusType::MANA_PER_KNOWLEDGE_PERCENTAGE, BonusSubtypeID(),                                  100, 18}
),
	[](const ::testing::TestParamInfo<SkillCase> & info) { return info.param.name; });

// --- movement specialty (GROWS_WITH_LEVEL) ---

class MovementSpecialty : public BattleSpellCastTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(MovementSpecialty, growsWithLevel)
{
	const int level = GetParam();
	const int sylviaIdx = 3;
	const int navigationIdx = 5;

	CGHeroInstance * hero = placeHero(sylviaIdx, level, {{CreatureID(0), 1}});
	hero->setSecSkillLevel(SecondarySkill(navigationIdx), 3, ChangeValueMode::ABSOLUTE); // limiter needs navigation

	BonusSubtypeID seaMovement;
	for(const auto & b : hero->getHeroType()->specialty)
		if(b->type == BonusType::MOVEMENT)
			seaMovement = b->subtype;

	const int64_t withSpec = hero->valOfBonuses(BonusType::MOVEMENT, seaMovement);
	removeSpecialty(hero);
	const int64_t withoutSpec = hero->valOfBonuses(BonusType::MOVEMENT, seaMovement);

	// valPer20 = 1500, stepSize = 1 -> divideAndCeil(1500 * level, 20)
	EXPECT_EQ(withSpec - withoutSpec, vstd::divideAndCeil(1500 * level, 20));
}

INSTANTIATE_TEST_SUITE_P(Sylvia, MovementSpecialty, ::testing::Values(5, 20, 39));
