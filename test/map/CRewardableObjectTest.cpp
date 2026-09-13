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

class CartographerTeamVisitTest : public TinyMapGameTest
{
};

}

TEST(CRewardableObject, rewardValuesRespectScoutingVisibility)
{
	const PlayerColor visitor(0);
	IGameInfoCallbackMock objectCallback;
	TeamState team;
	CRewardableObject object(&objectCallback);
	GameInfoCallbackForTest callback(visitor);
	RewardableObjectInfo info;
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.info.emplace_back();
	object.configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.configuration.info.back().reward.resources[GameResID::GOLD] = 1000;
	object.configuration.info.back().reward.grantedArtifacts.push_back(ArtifactID::SPELLBOOK);
	object.configuration.showScoutedPreview = true;

	EXPECT_CALL(objectCallback, getPlayerTeam(visitor)).WillRepeatedly(::testing::Return(&team));

	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_FALSE(info.scouted);
	EXPECT_FALSE(info.rewardAvailable);
	EXPECT_FALSE(info.rewardValueKnown);
	EXPECT_EQ(info.guardStrength, 0);
	EXPECT_TRUE(info.rewards.empty());

	team.scoutedObjects.insert(object.id);
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_TRUE(info.scouted);
	EXPECT_TRUE(info.rewardAvailable);
	EXPECT_TRUE(info.rewardValueKnown);
	ASSERT_EQ(info.rewards.size(), 1);
	EXPECT_EQ(info.rewards.front().resources[GameResID::GOLD], 1000);
	EXPECT_EQ(info.rewards.front().grantedArtifacts, std::vector<ArtifactID>{ArtifactID::SPELLBOOK});

	object.configuration.info.back().reward.guards.emplace_back(CreatureID::NONE, 1);
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_FALSE(info.rewardValueKnown);
	ASSERT_EQ(info.rewards.size(), 1);
	EXPECT_EQ(info.rewards.front().resources[GameResID::GOLD], 1000);
	EXPECT_TRUE(info.rewards.front().grantedArtifacts.empty());
}

TEST(CRewardableObject, onceVisitStateIsKnownOnlyToScoutingTeam)
{
	const PlayerColor visitor(0);
	const PlayerColor ally(1);
	const PlayerColor enemy(2);

	IGameInfoCallbackMock callback;
	TeamState visitorsTeam;
	TeamState enemyTeam;
	CRewardableObject object(&callback);
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.visitMode = Rewardable::VISIT_ONCE;
	object.setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	visitorsTeam.scoutedObjects.insert(object.id);

	EXPECT_CALL(callback, getPlayerTeam(visitor)).WillOnce(::testing::Return(&visitorsTeam));
	EXPECT_CALL(callback, getPlayerTeam(ally)).WillOnce(::testing::Return(&visitorsTeam));
	EXPECT_CALL(callback, getPlayerTeam(enemy)).WillOnce(::testing::Return(&enemyTeam));

	EXPECT_TRUE(object.wasVisited(visitor));
	EXPECT_TRUE(object.wasVisited(ally));
	EXPECT_FALSE(object.wasVisited(enemy));
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
	RewardableObjectInfo info;
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
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info, &hero));
	EXPECT_TRUE(info.scouted);
	EXPECT_FALSE(info.rewardAvailable);
	EXPECT_TRUE(info.rewards.empty());

	object.subID = MapObjectSubID(2);
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info, &hero));
	EXPECT_FALSE(info.scouted);
	EXPECT_FALSE(info.rewardAvailable);
}

TEST_F(CartographerTeamVisitTest, cartographerVisitMarksObjectTypeVisitedForAllies)
{
	const PlayerColor visitor(0);
	const PlayerColor ally(1);
	const PlayerColor enemy(2);
	const MapObjectSubID cartographerType(1);
	startWithMap(TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false)
		.name("CartographerTeamVisit")
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
	auto cartographer = std::make_shared<CRewardableObject>(callback.get());
	cartographer->ID = Obj::CARTOGRAPHER;
	cartographer->subID = cartographerType;
	cartographer->appearance = findHeroByOwner(visitor)->appearance;
	cartographer->instanceName = "cartographerForTeamVisitTest";
	cartographer->setAnchorPos({9, 9, 0});
	map()->addNewObject(cartographer);

	ChangeObjectVisitors visit(ChangeObjectVisitors::VISITOR_ADD_HERO, cartographer->id, findHeroByOwner(visitor)->id);
	gameState()->apply(visit);

	EXPECT_EQ(visitorState.visitedObjectsGlobal.count({Obj::CARTOGRAPHER, cartographerType}), 1);
	EXPECT_EQ(allyState.visitedObjectsGlobal.count({Obj::CARTOGRAPHER, cartographerType}), 1);
	EXPECT_EQ(enemyState.visitedObjectsGlobal.count({Obj::CARTOGRAPHER, cartographerType}), 0);
	EXPECT_EQ(allyState.visitedObjectsGlobal.count({Obj::CARTOGRAPHER, MapObjectSubID(2)}), 0);
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
