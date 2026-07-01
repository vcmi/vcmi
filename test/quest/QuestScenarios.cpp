/*
 * QuestScenarios.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestScenarios.h"

namespace QuestScenarios
{

using TinyH3M::TinyH3MBuilder;

namespace
{

// Fresh, named SOD builder with red active. Every scenario starts from this
// skeleton; per-scenario differences live below it.
TinyH3MBuilder fresh(const std::string & name)
{
	TinyH3MBuilder b(EMapFormat::SOD);
	b.size(36, /*twoLevel*/ false)
	 .name(name)
	 .playerActive(PlayerColor(0));
	return b;
}

} // namespace

Scenario seerArtifact()
{
	Scenario s;
	s.builder = fresh("SeerArtifact");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroBackpack({kArtifactSash})
		.seerHut(s.questPos,
			TinyH3MBuilder::missionArtifacts({kArtifactSash}),
			TinyH3MBuilder::rewardExperience(1000));
	return s;
}

Scenario seerArtifactAssembledInBackpack()
{
	Scenario s;
	s.builder = fresh("SeerArtifactAssembledInBackpack");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroBackpack({kArtifactAngelicAlly})
		.seerHut(s.questPos,
			TinyH3MBuilder::missionArtifacts({kArtifactHelm}),
			TinyH3MBuilder::rewardExperience(1000));
	return s;
}

Scenario seerArtifactAssembledEquipped()
{
	Scenario s;
	s.builder = fresh("SeerArtifactAssembledEquipped");
	// Angelic Alliance worn in its sword-slot. Component slot for the Sword of
	// Judgement is RIGHT_HAND; equipping the assembly there makes the engine
	// expand it across all the component slots automatically.
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroEquipped({{ArtifactPosition::RIGHT_HAND, kArtifactAngelicAlly}})
		.seerHut(s.questPos,
			TinyH3MBuilder::missionArtifacts({kArtifactHelm}),
			TinyH3MBuilder::rewardExperience(1000));
	return s;
}

Scenario seerArmy()
{
	Scenario s;
	s.builder = fresh("SeerArmy");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroGarrison({{kCreatureGriffin,      10},
			               {kCreatureRoyalGriffin,  5}})
		.seerHut(s.questPos,
			TinyH3MBuilder::missionArmy({{kCreatureGriffin, 5}}),
			TinyH3MBuilder::rewardExperience(500));
	return s;
}

Scenario seerResources()
{
	Scenario s;
	s.builder = fresh("SeerResources");
	// Bring 5000 gold + 5 wood. Player starting resources are zero — the test
	// is expected to grantResources(RED, GOLD, 7000) + (WOOD, 10) post-load.
	std::array<uint32_t, 7> req{};
	req[GameResID::WOOD] = 5;
	req[GameResID::GOLD] = 5000;
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.seerHut(s.questPos,
			TinyH3MBuilder::missionResources(req),
			TinyH3MBuilder::rewardResource(GameResID(GameResID::MERCURY), 5));
	return s;
}

Scenario seerHero()
{
	Scenario s;
	s.secondHeroPos = {6, 5, 0};
	s.builder = fresh("SeerHero");
	s.builder
		.playerActive(PlayerColor(1))  // a second hero slot, owned by blue here
		.hero(s.heroPos,       kHeroChristian, PlayerColor(0))
		.hero(s.secondHeroPos, kHeroTyris,     PlayerColor(0))
		.seerHut(s.questPos,
			TinyH3MBuilder::missionHero(kHeroTyris),
			TinyH3MBuilder::rewardExperience(500));
	return s;
}

Scenario seerPlayer()
{
	Scenario s;
	s.secondHeroPos = {20, 20, 0};
	s.builder = fresh("SeerPlayer");
	s.builder
		.playerActive(PlayerColor(1))
		.hero(s.heroPos,       kHeroChristian, PlayerColor(0))
		.hero(s.secondHeroPos, kHeroTyris,     PlayerColor(1))
		.seerHut(s.questPos,
			TinyH3MBuilder::missionPlayer(PlayerColor(1)),
			TinyH3MBuilder::rewardExperience(500));
	return s;
}

Scenario seerLevel()
{
	Scenario s;
	s.questPos2 = {15, 15, 0};   // hard seer (level >= 10)
	s.builder = fresh("SeerLevel");
	// 10000 XP puts most heroes around level 5 in vanilla H3 progression.
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroExperience(10000)
		.seerHut(s.questPos,  TinyH3MBuilder::missionLevel(3),  TinyH3MBuilder::rewardExperience(100))
		.seerHut(s.questPos2, TinyH3MBuilder::missionLevel(10), TinyH3MBuilder::rewardExperience(100));
	return s;
}

Scenario seerPrimarySkill()
{
	Scenario s;
	s.questPos2 = {15, 15, 0};   // hard seer (attack >= 10)
	s.builder = fresh("SeerPrimarySkill");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroPrimary(/*attack*/ 5, /*defense*/ 0, /*sp*/ 0, /*knowledge*/ 0)
		.seerHut(s.questPos,
			TinyH3MBuilder::missionPrimarySkills(3, 0, 0, 0),
			TinyH3MBuilder::rewardExperience(100))
		.seerHut(s.questPos2,
			TinyH3MBuilder::missionPrimarySkills(10, 0, 0, 0),
			TinyH3MBuilder::rewardExperience(100));
	return s;
}

Scenario seerKillCreature()
{
	Scenario s;
	s.secondHeroPos = {12, 12, 0}; // monster position; "secondary" field is reused for findObjectAt convenience
	s.builder = fresh("SeerKillCreature");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.monster(s.secondHeroPos, kCreatureImp, /*count*/ 1);
	auto target = s.builder.lastHandle();
	s.builder
		.seerHut(s.questPos,
			TinyH3MBuilder::missionKillCreature(target),
			TinyH3MBuilder::rewardExperience(500));
	return s;
}

Scenario seerKillHero()
{
	Scenario s;
	s.secondHeroPos = {20, 20, 0};
	s.builder = fresh("SeerKillHero");
	s.builder
		.playerActive(PlayerColor(1))
		.hero(s.heroPos,       kHeroChristian, PlayerColor(0))
		.hero(s.secondHeroPos, kHeroTyris,     PlayerColor(1));
	auto target = s.builder.lastHandle();
	s.builder
		.seerHut(s.questPos,
			TinyH3MBuilder::missionKillHero(target),
			TinyH3MBuilder::rewardExperience(500));
	return s;
}

Scenario seerTimeout()
{
	Scenario s;
	s.builder = fresh("SeerTimeout");
	// Trivial limiter (1 wood) so the test only exercises the lastDay
	// expiry path, not the limiter-evaluation path.
	std::array<uint32_t, 7> req{};
	req[GameResID::WOOD] = 1;
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.seerHut(s.questPos,
			TinyH3MBuilder::missionResources(req).withLastDay(7),
			TinyH3MBuilder::rewardExperience(100));
	return s;
}

Scenario seerEmptyArmyToggle()
{
	Scenario s;
	s.builder = fresh("SeerEmptyArmyToggle");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
			.heroGarrison({{kCreatureGriffin, 1}})
		.seerHut(s.questPos,
			TinyH3MBuilder::missionArmy({{kCreatureGriffin, 1}}),
			TinyH3MBuilder::rewardNothing());
	return s;
}

Scenario questGuard()
{
	Scenario s;
	s.builder = fresh("QuestGuard");
	std::array<uint32_t, 7> req{};
	req[GameResID::WOOD] = 1000;
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.questGuard(s.questPos, TinyH3MBuilder::missionResources(req));
	return s;
}

Scenario questKeymasterTent()
{
	Scenario s;
	s.builder = fresh("QuestKeymasterTent");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.keymaster(s.questPos, /*color*/ 0);
	return s;
}

Scenario questBorderGuard()
{
	Scenario s;
	s.questPos2 = {15, 15, 0};      // border guard, same colour
	s.builder = fresh("QuestBorderGuard");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.keymaster  (s.questPos,  /*color*/ 0)
		.borderGuard(s.questPos2, /*color*/ 0);
	return s;
}

Scenario questBorderGate()
{
	Scenario s;
	s.questPos2 = {15, 15, 0};
	s.builder = fresh("QuestBorderGate");
	s.builder
		.hero(s.heroPos, kHeroChristian, PlayerColor(0))
		.keymaster (s.questPos,  /*color*/ 0)
		.borderGate(s.questPos2, /*color*/ 0);
	return s;
}

} // namespace QuestScenarios
