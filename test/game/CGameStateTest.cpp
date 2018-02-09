/*
 * CGameStateTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_MapService.h"
#include "mock/mock_IGameCallback.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/CGameState.h"
#include "../../lib/NetPacks.h"
#include "../../lib/StartInfo.h"

#include "../../lib/battle/BattleInfo.h"
#include "../../lib/CStack.h"

#include "../../lib/filesystem/ResourceID.h"

#include "../../lib/mapping/CMap.h"

#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"

class CGameStateTest : public ::testing::Test, public SpellCastEnvironment, public MapListener
{
public:
	CGameStateTest()
		: gameCallback(new GameCallbackMock(this)),
		mapService("test/MiniTest/", this),
		map(nullptr)
	{
		IObjectInterface::cb = gameCallback.get();
	}

	virtual ~CGameStateTest()
	{
		IObjectInterface::cb = nullptr;
	}

	void sendAndApply(CPackForClient * info) const override
	{
		gameState->apply(info);
	}

	void complain(const std::string & problem) const
	{
		FAIL() << "Server-side assertion:" << problem;
	};

	CRandomGenerator & getRandomGenerator() const override
	{
		return gameState->getRandomGenerator();//todo: mock this
	}

	const CMap * getMap() const override
	{
		return map;
	}
	const CGameInfoCallback * getCb() const override
	{
		return gameState.get();
	}

	bool moveHero(ObjectInstanceID hid, int3 dst, bool teleporting) const override
	{
		return false;
	}

	void genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode &)> callback) const
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
		si.mapfileChecksum = 0;
		si.mode = StartInfo::NEW_GAME;
		si.seedToBeUsed = 42;

		std::unique_ptr<CMapHeader> header = mapService.loadMapHeader(ResourceID(si.mapname));

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
			pset.connectedPlayerIDs.insert(i);
			pset.name = "Player";

			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();

			if(pset.hero != PlayerSettings::RANDOM && pinfo.hasCustomMainHero())
			{
				pset.hero = pinfo.mainCustomHeroId;
				pset.heroName = pinfo.mainCustomHeroName;
				pset.heroPortrait = pinfo.mainCustomHeroPortrait;
			}

			pset.handicap = PlayerSettings::NO_HANDICAP;
		}

		gameState = std::make_shared<CGameState>();
		gameCallback->setGameState(gameState.get());
		gameState->init(&mapService, &si, false);

		ASSERT_NE(map, nullptr);
		ASSERT_EQ(map->heroesOnMap.size(), 2);
	}


	void startTestBattle(const CGHeroInstance * attacker, const CGHeroInstance * defender)
	{
		const CGHeroInstance * heroes[2] = {attacker, defender};
		const CArmedInstance * armedInstancies[2] = {attacker, defender};

		int3 tile(4,4,0);

		const auto t = gameCallback->getTile(tile);

		ETerrainType terrain = t->terType;
		BFieldType terType = BFieldType::GRASS_HILLS;

		//send info about battles

		BattleInfo * battle = BattleInfo::setupBattle(tile, terrain, terType, armedInstancies, heroes, false, nullptr);

		BattleStart bs;
		bs.info = battle;
		ASSERT_EQ(gameState->curB, nullptr);
		gameCallback->sendAndApply(&bs);
		ASSERT_EQ(gameState->curB, battle);
	}

	std::shared_ptr<CGameState> gameState;

	std::shared_ptr<GameCallbackMock> gameCallback;

	MapServiceMock mapService;

	CMap * map;
};

//Issue #2765, Ghost Dragons can cast Age on Catapults
TEST_F(CGameStateTest, issue2765)
{
	startTestGame();

	CGHeroInstance * attacker = map->heroesOnMap[0];
	CGHeroInstance * defender = map->heroesOnMap[1];

	ASSERT_NE(attacker->tempOwner, defender->tempOwner);

	{
		CArtifactInstance * a = new CArtifactInstance();
		a->artType = const_cast<CArtifact *>(ArtifactID(ArtifactID::BALLISTA).toArtifact());

		NewArtifact na;
		na.art = a;
		gameCallback->sendAndApply(&na);

		PutArtifact pack;
		pack.al = ArtifactLocation(defender, ArtifactPosition::MACH1);
		pack.art = a;
		gameCallback->sendAndApply(&pack);
	}

	startTestBattle(attacker, defender);

	{
		battle::UnitInfo info;
		info.id = gameState->curB->battleNextUnitId();
		info.count = 1;
		info.type = CreatureID(69);
		info.side = BattleSide::ATTACKER;
		info.position = gameState->curB->getAvaliableHex(info.type, info.side);
		info.summoned = false;

		BattleUnitsChanged pack;
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		gameCallback->sendAndApply(&pack);
	}

	const CStack * att = nullptr;
	const CStack * def = nullptr;

	for(const CStack * s : gameState->curB->stacks)
	{
		if(s->type->idNumber == CreatureID::BALLISTA)
			def = s;
		else if(s->type->idNumber == CreatureID(69))
			att = s;
	}
	ASSERT_NE(att, nullptr);
	ASSERT_NE(def, nullptr);

	ASSERT_EQ(att->getMyHero(), attacker);
	ASSERT_EQ(def->getMyHero(), defender);

	{
		const CSpell * age = SpellID(SpellID::AGE).toSpell();
		ASSERT_NE(age, nullptr);
		//here tested ballista, but this applied to all war machines
		spells::BattleCast cast(gameState->curB, att, spells::Mode::AFTER_ATTACK, age);
		cast.aimToUnit(def);
		cast.setSpellLevel(3);

		EXPECT_FALSE(age->canBeCastAt(gameState->curB, spells::Mode::AFTER_ATTACK, att, def->getPosition()));

		EXPECT_TRUE(cast.castIfPossible(this));//should be possible, but with no effect (change to aimed cast check?)

		EXPECT_TRUE(def->activeSpells().empty());
	}

}

TEST_F(CGameStateTest, battleResurrection)
{
	startTestGame();

	CGHeroInstance * attacker = map->heroesOnMap[0];
	CGHeroInstance * defender = map->heroesOnMap[1];

	ASSERT_NE(attacker->tempOwner, defender->tempOwner);

	attacker->setSecSkillLevel(SecondarySkill::EARTH_MAGIC, 3, true);
	attacker->spells.insert(SpellID::RESURRECTION);
	attacker->setPrimarySkill(PrimarySkill::SPELL_POWER, 100, true);
	attacker->setPrimarySkill(PrimarySkill::KNOWLEDGE, 20, true);
	attacker->mana = attacker->manaLimit();

	{
		CArtifactInstance * a = new CArtifactInstance();
		a->artType = const_cast<CArtifact *>(ArtifactID(ArtifactID::SPELLBOOK).toArtifact());

		NewArtifact na;
		na.art = a;
		gameCallback->sendAndApply(&na);

		PutArtifact pack;
		pack.al = ArtifactLocation(attacker, ArtifactPosition::SPELLBOOK);
		pack.art = a;
		gameCallback->sendAndApply(&pack);
	}

	startTestBattle(attacker, defender);

	uint32_t unitId = gameState->curB->battleNextUnitId();

	{
		battle::UnitInfo info;
		info.id = unitId;
		info.count = 10;
		info.type = CreatureID(13);
		info.side = BattleSide::ATTACKER;
		info.position = gameState->curB->getAvaliableHex(info.type, info.side);
		info.summoned = false;

		BattleUnitsChanged pack;
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		gameCallback->sendAndApply(&pack);
	}

	{
		battle::UnitInfo info;
		info.id = gameState->curB->battleNextUnitId();
		info.count = 10;
		info.type = CreatureID(13);
		info.side = BattleSide::DEFENDER;
		info.position = gameState->curB->getAvaliableHex(info.type, info.side);
		info.summoned = false;

		BattleUnitsChanged pack;
		pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
		info.save(pack.changedStacks.back().data);
		gameCallback->sendAndApply(&pack);
	}

	CStack * unit = gameState->curB->getStack(unitId);

	ASSERT_NE(unit, nullptr);

	int64_t damage = unit->MaxHealth() + 1;

	unit->damage(damage);

	EXPECT_EQ(unit->getCount(), 9);

	{
		const CSpell * spell = SpellID(SpellID::RESURRECTION).toSpell();
		ASSERT_NE(spell, nullptr);

		spells::BattleCast cast(gameState->curB, attacker, spells::Mode::HERO, spell);
		cast.aimToUnit(unit);

		EXPECT_TRUE(spell->canBeCast(gameState->curB, spells::Mode::HERO, attacker));

		EXPECT_TRUE(spell->canBeCastAt(gameState->curB, spells::Mode::HERO, attacker, unit->getPosition()));

		cast.cast(this);
	}

	EXPECT_EQ(unit->health.getCount(), 10);
	EXPECT_EQ(unit->health.getResurrected(), 0);
}
