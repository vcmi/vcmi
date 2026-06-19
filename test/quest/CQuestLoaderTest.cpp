/*
 * CQuestLoaderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestAssertions.h"
#include "QuestScenarios.h"
#include "QuestTest.h"

#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/army/CStackBasicDescriptor.h"
#include "../../lib/mapping/MapDifficulty.h"

// What survives the .h3m → CQuest mapping pipeline. Each test loads a SOD
// scenario and asserts the resulting CQuest::mission matches the limiter the
// scenario asked for.

using namespace QuestScenarios;

namespace
{

// One row per single-seer scenario: (scenario factory, expected limiter).
struct LoaderCase
{
	const char *                          name;
	Scenario                            (*factory)();
	std::function<Rewardable::Limiter()>  expected;
	int                                   lastDay = -1;
};

Rewardable::Limiter limArtifact(ArtifactID id)
{
	Rewardable::Limiter l;
	l.artifacts.push_back(id);
	return l;
}

Rewardable::Limiter limCreatures(CreatureID id, int count)
{
	Rewardable::Limiter l;
	l.creatures.emplace_back(id, count);
	return l;
}

Rewardable::Limiter limResources(GameResID which, int amount)
{
	Rewardable::Limiter l;
	l.resources[which] = amount;
	return l;
}

} // namespace

class QuestLoaderTest : public QuestTest, public ::testing::WithParamInterface<LoaderCase> {};

TEST_P(QuestLoaderTest, LoadsExpectedMission)
{
	auto s = GetParam().factory();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	const auto * seer = expectAt<CGSeerHut>(s.questPos);

	ExpectedMission expected;
	expected.limiter = GetParam().expected();
	expected.lastDay = GetParam().lastDay;
	EXPECT_QUEST_MISSION(seer->getQuest(), expected);
}

INSTANTIATE_TEST_SUITE_P(All, QuestLoaderTest, ::testing::Values(
	LoaderCase{"SeerArtifact",          seerArtifact,                    [] { return limArtifact (kArtifactSash); }},
	LoaderCase{"SeerArtifactAssembled", seerArtifactAssembledInBackpack, [] { return limArtifact (kArtifactHelm); }},
	LoaderCase{"SeerArmy",              seerArmy,                        [] { return limCreatures(kCreatureGriffin, 5); }},
	LoaderCase{"SeerResources",         seerResources,
		[] {
			Rewardable::Limiter l;
			l.resources[GameResID::WOOD] = 5;
			l.resources[GameResID::GOLD] = 5000;
			return l;
		}},
	LoaderCase{"SeerHero",              seerHero,
		[] { Rewardable::Limiter l; l.heroes.push_back(kHeroTyris); return l; }},
	LoaderCase{"SeerPlayer",            seerPlayer,
		[] { Rewardable::Limiter l; l.players.push_back(PlayerColor(1));        return l; }},
	LoaderCase{"SeerTimeout",           seerTimeout,
		[] { return limResources(GameResID(GameResID::WOOD), 1); }, /*lastDay=*/7},
	LoaderCase{"QuestGuard",            questGuard,
		[] { return limResources(GameResID(GameResID::WOOD), 1000); }}
),
[](const ::testing::TestParamInfo<LoaderCase> & info) { return std::string(info.param.name); });

// Two-seer scenarios don't fit the single-row table cleanly — kept as TEST_F.

class QuestLoaderTwoSeerTest : public QuestTest {};

TEST_F(QuestLoaderTwoSeerTest, SeerLevel)
{
	auto s = seerLevel();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	ExpectedMission easyExp;
	easyExp.limiter.heroLevel = 3;
	ExpectedMission hardExp;
	hardExp.limiter.heroLevel = 10;

	EXPECT_QUEST_MISSION(expectAt<CGSeerHut>(s.questPos )->getQuest(), easyExp);
	EXPECT_QUEST_MISSION(expectAt<CGSeerHut>(s.questPos2)->getQuest(), hardExp);
}

TEST_F(QuestLoaderTwoSeerTest, SeerPrimarySkill)
{
	auto s = seerPrimarySkill();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));

	ExpectedMission easyExp;
	easyExp.limiter.primary = {3, 0, 0, 0};
	ExpectedMission hardExp;
	hardExp.limiter.primary = {10, 0, 0, 0};

	EXPECT_QUEST_MISSION(expectAt<CGSeerHut>(s.questPos )->getQuest(), easyExp);
	EXPECT_QUEST_MISSION(expectAt<CGSeerHut>(s.questPos2)->getQuest(), hardExp);
}

// Kill-quest tests assert the resolved ObjectInstanceID matches the placed object.

class QuestLoaderKillTest : public QuestTest {};

TEST_F(QuestLoaderKillTest, SeerKillCreature)
{
	auto s = seerKillCreature();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	const auto * seer    = expectAt<CGSeerHut> (s.questPos);
	const auto * monster = expectAt<CGCreature>(s.secondHeroPos);
	ASSERT_FALSE(seer->getQuest().mission.destroyedObjects.empty());
	EXPECT_EQ(seer->getQuest().mission.destroyedObjects.front(), monster->id);
}

TEST_F(QuestLoaderKillTest, SeerKillHero)
{
	auto s = seerKillHero();
	ASSERT_NO_FATAL_FAILURE(startWithMap(std::move(s.builder)));
	const auto * seer   = expectAt<CGSeerHut>     (s.questPos);
	const auto * target = expectAt<CGHeroInstance>(s.secondHeroPos);
	ASSERT_FALSE(seer->getQuest().mission.destroyedObjects.empty());
	EXPECT_EQ(seer->getQuest().mission.destroyedObjects.front(), target->id);
}

// Pure classification check: defineQuestName must map the limiter shape (plus the
// kill-target backups) to the right EQuestMission. No map fixture needed.

namespace
{
EQuestMission classify(const std::function<void(CQuest &)> & setup)
{
	CQuest q;
	setup(q);
	q.defineQuestName();
	return q.missionKind;
}
}

TEST(QuestKind, classifiedByLimiterShape)
{
	EXPECT_EQ(classify([](CQuest &){}),                                                                        EQuestMission::NONE);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.heroLevel = 5; }),                                            EQuestMission::LEVEL);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.primary[0] = 3; }),                                           EQuestMission::PRIMARY_SKILL);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.destroyedObjects.emplace_back(0); q.heroName = "Bob"; }),     EQuestMission::KILL_HERO);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.destroyedObjects.emplace_back(0); q.stackToKill = CreatureID(0); }), EQuestMission::KILL_CREATURE);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.artifacts.push_back(ArtifactID(0)); }),                       EQuestMission::ARTIFACT);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.creatures.emplace_back(CreatureID(0), 1); }),                 EQuestMission::ARMY);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.resources[GameResID::GOLD] = 1000; }),                        EQuestMission::RESOURCES);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.heroes.push_back(HeroTypeID(0)); }),                          EQuestMission::HERO);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.players.push_back(PlayerColor(0)); }),                        EQuestMission::PLAYER);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.daysPassed = 10; }),                                          EQuestMission::HOTA_REACH_DATE);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.heroClasses.push_back(HeroClassID(0)); }),                    EQuestMission::HOTA_HERO_CLASS);
	EXPECT_EQ(classify([](CQuest & q){ q.mission.allowedDifficulties = MapDifficultySet(1); }),               EQuestMission::HOTA_GAME_DIFFICULTY);
}
