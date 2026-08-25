/*
 * CRewardableObjectTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/mock_IGameEventCallback.h"
#include "../mock/mock_IGameInfoCallback.h"
#include "../mock/TinyMapGameTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/callback/CGameInfoCallback.h"
#include "../../lib/mapObjects/CRewardableObject.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

namespace
{

class VisitorPackCapture : public GameEventCallbackMock
{
public:
	VisitorPackCapture()
		: GameEventCallbackMock(nullptr)
	{
	}

	std::vector<ChangeObjectVisitors::VisitMode> modes;

	void setObjPropertyValue(ObjectInstanceID, ObjProperty, int32_t) override
	{
	}

	void sendAndApply(CPackForClient & pack) override
	{
		const auto * visitors = dynamic_cast<ChangeObjectVisitors *>(&pack);
		ASSERT_NE(visitors, nullptr);
		modes.push_back(visitors->mode);
	}
};

class GameInfoCallbackForTest : public CGameInfoCallback
{
public:
	explicit GameInfoCallbackForTest(PlayerColor player)
		: player(player)
	{
	}

	CGameState & gameState() override
	{
		throw std::logic_error("Unexpected gameState access");
	}

	const CGameState & gameState() const override
	{
		throw std::logic_error("Unexpected gameState access");
	}

	std::optional<PlayerColor> getPlayerID() const override
	{
		return player;
	}

private:
	PlayerColor player;
};

class TeamGlobalVisitTest : public TinyMapGameTest
{
};

}

TEST(CRewardableObject, rewardValuesRespectScoutingVisibility)
{
	const PlayerColor visitor(0);
	struct ObjectCallback : IGameInfoCallbackMock
	{
		const IGameSettings & getSettings() const override { return *LIBRARY->engineSettings(); }
	} objectCallback;
	TeamState team;
	CRewardableObject object(&objectCallback);
	GameInfoCallbackForTest callback(visitor);
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.info.emplace_back();
	object.configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.configuration.info.back().reward.resources[GameResID::GOLD] = 1000;
	object.configuration.info.back().reward.grantedArtifacts.push_back(ArtifactID::SPELLBOOK);
	object.configuration.showScoutedPreview = true;

	EXPECT_CALL(objectCallback, getPlayerTeam(visitor)).WillRepeatedly(::testing::Return(&team));

	auto info = callback.getRewardableObjectInfo(&object);
	ASSERT_TRUE(info);
	EXPECT_FALSE(info->scouted);
	EXPECT_EQ(info->rewardAvailable, std::nullopt);
	EXPECT_FALSE(info->rewardValueKnown);
	EXPECT_EQ(info->guardStrength, 0);
	EXPECT_TRUE(info->rewards.empty());
	EXPECT_TRUE(object.getPopupComponents(visitor).empty());

	team.scoutedObjects.insert(object.id);
	info = callback.getRewardableObjectInfo(&object);
	ASSERT_TRUE(info);
	EXPECT_TRUE(info->scouted);
	EXPECT_EQ(info->rewardAvailable, true);
	EXPECT_TRUE(info->rewardValueKnown);
	ASSERT_EQ(info->rewards.size(), 1);
	EXPECT_EQ(info->rewards.front().resources[GameResID::GOLD], 1000);
	EXPECT_EQ(info->rewards.front().grantedArtifacts, std::vector<ArtifactID>{ArtifactID::SPELLBOOK});

	object.configuration.info.back().reward.guards.emplace_back(CreatureID(CreatureID::decode("imp")), 1);
	info = callback.getRewardableObjectInfo(&object);
	ASSERT_TRUE(info);
	EXPECT_EQ(info->rewardAvailable, std::nullopt);
	EXPECT_FALSE(info->rewardValueKnown);
	EXPECT_TRUE(info->rewards.empty());
}

TEST(CRewardableObject, onceVisitStateIsSharedOnlyWithClearingPlayersTeam)
{
	const PlayerColor player(0);
	const PlayerColor ally(1);
	const PlayerColor enemy(2);

	IGameInfoCallbackMock callback;
	PlayerState playerState(&callback);
	PlayerState allyState(&callback);
	PlayerState enemyState(&callback);
	TeamState visitorsTeam;
	TeamState enemyTeam;
	CRewardableObject object(&callback);
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.visitMode = Rewardable::VISIT_ONCE;
	object.configuration.info.emplace_back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	visitorsTeam.players.insert(player);
	visitorsTeam.players.insert(ally);
	enemyTeam.players.insert(enemy);
	visitorsTeam.scoutedObjects.insert(object.id);
	allyState.visitedObjects.insert(object.id);

	EXPECT_CALL(callback, getPlayerTeam(player)).WillRepeatedly(::testing::Return(&visitorsTeam));
	EXPECT_CALL(callback, getPlayerTeam(enemy)).WillRepeatedly(::testing::Return(&enemyTeam));
	EXPECT_CALL(callback, getPlayerState(player, ::testing::_)).WillRepeatedly(::testing::Return(&playerState));
	EXPECT_CALL(callback, getPlayerState(ally, ::testing::_)).WillRepeatedly(::testing::Return(&allyState));
	EXPECT_CALL(callback, getPlayerState(enemy, ::testing::_)).WillRepeatedly(::testing::Return(&enemyState));

	EXPECT_TRUE(object.wasVisited(player));
	EXPECT_FALSE(object.wasVisited(enemy));

	allyState.visitedObjects.erase(object.id);
	enemyState.visitedObjects.insert(object.id);

	EXPECT_FALSE(object.wasVisited(player));
	EXPECT_TRUE(object.wasVisited(enemy));

	GameInfoCallbackForTest infoCallback(player);
	const auto info = infoCallback.getRewardableObjectInfo(&object);
	ASSERT_TRUE(info);
	EXPECT_EQ(info->rewardAvailable, true);
}

TEST(CRewardableObject, checkingEnemyClearedOnceObjectMarksItVisited)
{
	const PlayerColor visitor(0);
	IGameInfoCallbackMock callback;
	PlayerState player(&callback);
	TeamState team;
	CRewardableObject object(&callback);
	CGHeroInstance hero(&callback);
	VisitorPackCapture gameEvents;
	object.id = ObjectInstanceID(42);
	object.configuration.visitMode = Rewardable::VISIT_ONCE;
	object.configuration.info.emplace_back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.configuration.info.back().reward.guards.emplace_back(CreatureID(CreatureID::decode("imp")), 1);
	object.setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	team.players.insert(visitor);
	team.scoutedObjects.insert(object.id);
	hero.id = ObjectInstanceID(7);
	hero.tempOwner = visitor;

	EXPECT_CALL(callback, getPlayerTeam(visitor)).WillRepeatedly(::testing::Return(&team));
	EXPECT_CALL(callback, getPlayerState(visitor, ::testing::_)).WillRepeatedly(::testing::Return(&player));

	object.onHeroVisit(gameEvents, &hero);

	ASSERT_EQ(gameEvents.modes.size(), 1);
	EXPECT_EQ(gameEvents.modes.front(), ChangeObjectVisitors::VISITOR_ADD_HERO);
}

TEST(CRewardableObject, globallyVisitedObjectInfoIsScoutedAndUnavailable)
{
	const PlayerColor ally(1);
	IGameInfoCallbackMock objectCallback;
	PlayerState allyState(&objectCallback);
	TeamState team;
	CRewardableObject object(&objectCallback);
	CGHeroInstance hero(&objectCallback);
	GameInfoCallbackForTest callback(ally);
	object.ID = Obj::CARTOGRAPHER;
	object.subID = MapObjectSubID(1);
	object.id = ObjectInstanceID(42);
	object.configuration.visitMode = Rewardable::VISIT_PLAYER_GLOBAL;
	object.configuration.info.emplace_back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	hero.tempOwner = ally;
	allyState.visitedObjectsGlobal.insert({object.ID, object.subID});

	EXPECT_CALL(objectCallback, getPlayerState(ally, ::testing::_)).WillRepeatedly(::testing::Return(&allyState));
	EXPECT_CALL(objectCallback, getPlayerTeam(ally)).WillRepeatedly(::testing::Return(&team));

	EXPECT_TRUE(object.wasVisited(ally));
	EXPECT_TRUE(object.wasScouted(ally));
	auto info = callback.getRewardableObjectInfo(&object, &hero);
	ASSERT_TRUE(info);
	EXPECT_TRUE(info->scouted);
	EXPECT_EQ(info->rewardAvailable, false);
	EXPECT_TRUE(info->rewards.empty());

	object.subID = MapObjectSubID(2);
	info = callback.getRewardableObjectInfo(&object, &hero);
	ASSERT_TRUE(info);
	EXPECT_FALSE(info->scouted);
	EXPECT_EQ(info->rewardAvailable, std::nullopt);
}

TEST_F(TeamGlobalVisitTest, teamGlobalVisitIsReadFromVisitingPlayersState)
{
	const PlayerColor visitor(0);
	const PlayerColor ally(1);
	const PlayerColor enemy(2);
	const MapObjectSubID rewardableObjectType(1);
	startWithMap(TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false)
		.name("TeamGlobalVisit")
		.playerActive(visitor)
		.playerActive(ally)
		.playerActive(enemy)
		.hero({5, 5, 0}, HeroTypeID(0), visitor)
		.hero({7, 7, 0}, HeroTypeID(1), ally)
		.hero({11, 11, 0}, HeroTypeID(2), enemy));

	auto & visitorState = gameState()->players.at(visitor);
	auto & allyState = gameState()->players.at(ally);
	auto & enemyState = gameState()->players.at(enemy);
	const TeamID visitorTeam = visitorState.team;
	const TeamID allyTeam = allyState.team;
	allyState.team = visitorTeam;
	gameState()->teams.at(visitorTeam).players.insert(ally);
	if(allyTeam != visitorTeam) gameState()->teams.at(allyTeam).players.erase(ally);

	auto callback = makeCallback(visitor);
	auto teamGlobalRewardableObject = std::make_shared<CRewardableObject>(callback.get());
	teamGlobalRewardableObject->ID = Obj::EVENT;
	teamGlobalRewardableObject->subID = rewardableObjectType;
	teamGlobalRewardableObject->configuration.visitMode = Rewardable::VISIT_TEAM_GLOBAL;
	teamGlobalRewardableObject->appearance = findHeroByOwner(visitor)->appearance;
	teamGlobalRewardableObject->instanceName = "teamGlobalRewardableObject";
	teamGlobalRewardableObject->setAnchorPos({9, 9, 0});
	map()->addNewObject(teamGlobalRewardableObject);

	ChangeObjectVisitors visit(ChangeObjectVisitors::VISITOR_ADD_HERO, teamGlobalRewardableObject->id, findHeroByOwner(visitor)->id);
	gameState()->apply(visit);

	EXPECT_EQ(visitorState.visitedObjectsGlobal.count({Obj::EVENT, rewardableObjectType}), 1);
	EXPECT_EQ(allyState.visitedObjectsGlobal.count({Obj::EVENT, rewardableObjectType}), 0);
	EXPECT_EQ(enemyState.visitedObjectsGlobal.count({Obj::EVENT, rewardableObjectType}), 0);
	EXPECT_TRUE(teamGlobalRewardableObject->wasVisited(visitor));
	EXPECT_TRUE(teamGlobalRewardableObject->wasVisited(ally));
	teamGlobalRewardableObject->subID = MapObjectSubID(2);
	EXPECT_FALSE(teamGlobalRewardableObject->wasVisited(ally));

	teamGlobalRewardableObject->subID = rewardableObjectType;
	ChangeObjectVisitors clearVisit(ChangeObjectVisitors::VISITOR_CLEAR, teamGlobalRewardableObject->id);
	gameState()->apply(clearVisit);
	EXPECT_EQ(visitorState.visitedObjectsGlobal.count({Obj::EVENT, rewardableObjectType}), 0);
	EXPECT_FALSE(teamGlobalRewardableObject->wasVisited(visitor));
	EXPECT_FALSE(teamGlobalRewardableObject->wasVisited(ally));
}

TEST(CRewardableObject, failedRewardLimiterScoutsWithoutMarkingVisited)
{
	const PlayerColor visitor(0);
	IGameInfoCallbackMock callback;
	PlayerState player(&callback);
	TeamState team;
	CRewardableObject object(&callback);
	CGHeroInstance hero(&callback);
	VisitorPackCapture gameEvents;
	// Arbitrary rewardable object instance; its type does not affect scouting.
	object.id = ObjectInstanceID(42);
	object.configuration.visitMode = Rewardable::VISIT_PLAYER_GLOBAL;
	object.configuration.info.emplace_back();
	object.configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.configuration.info.back().limiter.resources[GameResID::GOLD] = 1000;
	hero.id = ObjectInstanceID(7);
	hero.tempOwner = visitor;

	EXPECT_CALL(callback, getPlayerTeam(visitor)).WillRepeatedly(::testing::Return(&team));
	EXPECT_CALL(callback, getPlayerState(visitor, ::testing::_)).WillRepeatedly(::testing::Return(&player));

	object.onHeroVisit(gameEvents, &hero);

	ASSERT_FALSE(gameEvents.modes.empty());
	for(const auto mode : gameEvents.modes)
		EXPECT_EQ(mode, ChangeObjectVisitors::VISITOR_SCOUTED);
}
