/*
 * QuestTest.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mock/TinyMapGameTest.h"
#include "../mock/mock_IGameEventCallback.h"

class CGObjectInstance;
class CGHeroInstance;
class SeerHut;
class QuestGuard;

/// Test fixture for scenarios involving quest objects. Loads a TinyH3MBuilder
/// scenario into a live CGameState and exposes helpers for the everyday
/// quest-flow steps: locate a placed object, walk a hero onto it, answer
/// dialogs, hand out resources, advance the calendar.
class QuestTest : public TinyMapGameTest
{
public:
	QuestTest()
		: gameEventCallback(std::make_shared<GameEventCallbackMock>(this))
	{
	}

	bool rollCombatAbility(const IBattleInfoCallback &, const battle::Unit &, int) override { return false; }
	// ---- public test API ------------------------------------------------

	/// Pretend the player has just defeated `target` — short-circuits the
	/// adventure-map battle that a kill-quest would normally require.
	void markObjectDestroyed(PlayerColor player, ObjectInstanceID target);

	/// Walk `hero` onto `obj` and trigger its visit handler.
	void visit(CGHeroInstance * hero, CGObjectInstance * obj);

	/// Answer the most recent BlockingDialog. Fails if none is pending.
	void answerDialog(CGHeroInstance * hero, int32_t answer);

	/// Advance the in-game calendar by `days`.
	void advanceDays(int days);

protected:
	void onMapStarted() override;
	GameEventCallbackMock & gameEvents() const { return *gameEventCallback; }

private:
	std::shared_ptr<GameEventCallbackMock> gameEventCallback;
};
