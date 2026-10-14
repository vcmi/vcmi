/*
 * BattleTestFixture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "mock/TinyMapGameTest.h"

#include "../../../server/IGameServer.h"
// every scenario reads the units it placed and the battle they are in, so the fixture brings both
#include "../../../lib/CStack.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

/// Everything one spell cast produced, as the clients see it.
struct RecordedCast
{
	BattleSpellCast announcement;
	int64_t damage = 0;
	uint32_t killed = 0;
	std::vector<std::string> logLines;
};

/// Applies everything the game handler emits straight to the game state, which is the whole of
/// "being a server" that a battle needs, and keeps the packs that describe what happened.
class RecordingGameServer : public IGameServer
{
public:
	std::shared_ptr<CGameState> gameState;
	std::vector<RecordedCast> casts;

	void setState(EServerState value) override { state = value; }
	EServerState getState() const override { return state; }
	bool isPlayerHost(const PlayerColor &) const override { return true; }
	bool hasPlayerAt(PlayerColor, GameConnectionID) const override { return true; }
	bool hasBothPlayersAtSameConnection(PlayerColor, PlayerColor) const override { return true; }
	void sendPack(CPackForClient &, GameConnectionID) override {}
	void applyPack(CPackForClient & pack) override;

	/// Casts of one spell, in the order they happened.
	std::vector<RecordedCast> castsOf(const SpellID & spell) const;

private:
	EServerState state = EServerState::GAMEPLAY;
	bool recording = false;

	void record(CPackForClient & pack);
};

/// A real game with two heroes and a battle between them, which is what tests of server-side
/// battle abilities need before they can do anything at all. Everything that could scale damage
/// on its own is stripped from both heroes, so a scenario measures only what it sets up.
class BattleTestFixture : public TinyMapGameTest
{
public:
	/// Free hexes in the middle of the field, away from the armies placed by the layout.
	static constexpr int leftHex = 5 * GameConstants::BFIELD_WIDTH + 7;
	static constexpr int rightHex = leftHex + 1;

	/// Fixes every roll of the battle, so that a failure can be reproduced.
	static constexpr int seed = 1337;

	void TearDown() override;

	/// Two heroes with a token army each, so that a battle between them is valid.
	void startGame();
	void startBattle();
	/// Ends the tactics phase, which fires the battle-start triggers and activates the first
	/// stack. Call once every unit a scenario needs is on the field.
	void beginCombat();

	BattleInfo * battle() const;

	CStack * addStack(BattleSide side, const CreatureID & creature, const BattleHex & position, int32_t count);
	void giveArtifact(const CGHeroInstance * hero, ArtifactID artifact, ArtifactPosition position);

	/// Casts a hero spell at a unit, reporting whether the game allowed it at all.
	bool castOn(const CGHeroInstance * hero, SpellID spellID, const CStack * target) const;

	/// Melee attack of the given stack against whatever stands on `targetHex`.
	bool attack(const CStack * attacker, const BattleHex & targetHex);
	/// Waits out the current round with every unit defending, leaving the battle in the next one.
	void endRound();

	/// Removes the stack's ability to retaliate, which would otherwise injure the attacker and
	/// shrink the attacking stack that most of these abilities scale with.
	static void blockRetaliation(CStack * stack);
	/// Collapses the stack's damage range onto its maximum, the way Bless does. Granted directly
	/// rather than cast, because some of the creatures that need it are undead and refuse the spell.
	static void forceMaximumDamage(CStack * stack);

	/// Creature declared by a mod, by its full identifier - "vcmi-test:testSoulStealer".
	static CreatureID creatureByName(const std::string & name);

	/// Shared rather than unique so that tests need not see the definition of the game handler
	/// only in order to destroy one.
	std::shared_ptr<CGameHandler> gameHandler;
	RecordingGameServer server;

	/// Hero of the side that starts on the left of the battlefield, and its opponent.
	CGHeroInstance * attackerSideHero = nullptr;
	CGHeroInstance * defenderSideHero = nullptr;

protected:
	/// A battle runs real game logic, which the mocked services cannot answer.
	Services * gameServices() override;
	void configurePlayer(PlayerSettings & settings) const override;

private:
	static void makeNeutral(CGHeroInstance * hero);
};
