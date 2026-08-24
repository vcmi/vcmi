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

#include "../../lib/CPlayerState.h"
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
