/*
 * PacksForClientBattle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"
#include "BattleChanges.h"
#include "PacksForClient.h"
#include "../battle/BattleAction.h"
#include "../battle/BattleInfo.h"
#include "../battle/BattleHexArray.h"
#include "../battle/BattleUnitTurnReason.h"
#include "../texts/MetaString.h"

class CClient;

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CArmedInstance;
class IBattleState;
class BattleInfo;

struct DLL_LINKAGE BattleStart : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	std::unique_ptr<BattleInfo> info;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & info;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleNextRound : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleSetActiveStack : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	uint32_t stack = 0;
	BattleUnitTurnReason reason;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stack;
		h & reason;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleCancelled: public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleResultAccepted : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	struct HeroBattleResults
	{
		ObjectInstanceID heroID;
		ObjectInstanceID armyID;
		TExpType exp = 0;

		template <typename Handler> void serialize(Handler & h)
		{
			h & heroID;
			h & armyID;
			h & exp;
		}
	};

	BattleID battleID = BattleID::NONE;
	BattleSideArray<HeroBattleResults> heroResult;
	BattleSide winnerSide;
	std::vector<BulkMoveArtifacts> artifacts;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & heroResult;
		h & winnerSide;
		h & artifacts;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleResult : public Query
{
	void applyFirstCl(CClient * cl);

	BattleID battleID = BattleID::NONE;
	EBattleResult result = EBattleResult::NORMAL;
	BattleSide winner = BattleSide::NONE; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	BattleSideArray<std::map<CreatureID, si32>> casualties; //first => casualties of attackers - map crid => number
	BattleSideArray<TExpType> exp{0,0}; //exp for attacker and defender

	void visitTyped(ICPackVisitor & visitor) override;
	void applyGs(CGameState *gs) override {}

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & queryID;
		h & result;
		h & winner;
		h & casualties;
		h & exp;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleLogMessage : public CPackForClient
{
	BattleID battleID = BattleID::NONE;
	std::vector<MetaString> lines;

	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & lines;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleStackMoved : public CPackForClient
{
	BattleID battleID = BattleID::NONE;
	ui32 stack = 0;
	BattleHexArray tilesToMove;
	int distance = 0;
	bool teleporting = false;
	
	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stack;
		h & tilesToMove;
		h & distance;
		h & teleporting;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleUnitsChanged : public CPackForClient
{
	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	BattleID battleID = BattleID::NONE;
	std::vector<UnitChanges> changedStacks;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & changedStacks;
		assert(battleID != BattleID::NONE);
	}
};

struct BattleStackAttacked
{
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);

	BattleID battleID = BattleID::NONE;
	ui32 stackAttacked = 0, attackerID = 0;
	ui32 killedAmount = 0;
	int64_t damageAmount = 0;
	UnitChanges newState;
	enum EFlags { KILLED = 1, SECONDARY = 2, REBIRTH = 4, CLONE_KILLED = 8, SPELL_EFFECT = 16, FIRE_SHIELD = 32, };
	ui32 flags = 0; //uses EFlags (above)
	SpellID spellID = SpellID::NONE; //only if flag SPELL_EFFECT is set

	bool killed() const//if target stack was killed
	{
		return flags & KILLED || flags & CLONE_KILLED;
	}
	bool cloneKilled() const
	{
		return flags & CLONE_KILLED;
	}
	bool isSecondary() const//if stack was not a primary target (receives no spell effects)
	{
		return flags & SECONDARY;
	}
	///Attacked with spell (SPELL_LIKE_ATTACK)
	bool isSpell() const
	{
		return flags & SPELL_EFFECT;
	}
	bool willRebirth() const//resurrection, e.g. Phoenix
	{
		return flags & REBIRTH;
	}
	bool fireShield() const
	{
		return flags & FIRE_SHIELD;
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stackAttacked;
		h & attackerID;
		h & newState;
		h & flags;
		h & killedAmount;
		h & damageAmount;
		h & spellID;
		assert(battleID != BattleID::NONE);
	}
	bool operator<(const BattleStackAttacked & b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct DLL_LINKAGE BattleAttack : public CPackForClient
{
	void applyGs(CGameState * gs) override;
	BattleUnitsChanged attackerChanges;

	BattleID battleID = BattleID::NONE;
	std::vector<BattleStackAttacked> bsa;
	ui32 stackAttacking = 0;
	ui32 flags = 0; //uses Eflags (below)
	enum EFlags { SHOT = 1, COUNTER = 2, LUCKY = 4, UNLUCKY = 8, BALLISTA_DOUBLE_DMG = 16, DEATH_BLOW = 32, SPELL_LIKE = 64, LIFE_DRAIN = 128 };

	BattleHex tile;
	SpellID spellID = SpellID::NONE; //for SPELL_LIKE

	bool shot() const//distance attack - decrease number of shots
	{
		return flags & SHOT;
	}
	bool counter() const//is it counterattack?
	{
		return flags & COUNTER;
	}
	bool lucky() const
	{
		return flags & LUCKY;
	}
	bool unlucky() const
	{
		return flags & UNLUCKY;
	}
	bool ballistaDoubleDmg() const //if it's ballista attack and does double dmg
	{
		return flags & BALLISTA_DOUBLE_DMG;
	}
	bool deathBlow() const
	{
		return flags & DEATH_BLOW;
	}
	bool spellLike() const
	{
		return flags & SPELL_LIKE;
	}
	bool lifeDrain() const
	{
		return flags & LIFE_DRAIN;
	}

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & bsa;
		h & stackAttacking;
		h & flags;
		h & tile;
		h & spellID;
		h & attackerChanges;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE StartAction : public CPackForClient
{
	StartAction() = default;
	explicit StartAction(BattleAction act)
		: ba(std::move(act))
	{
	}
	void applyFirstCl(CClient * cl);
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	BattleAction ba;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & ba;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE EndAction : public CPackForClient
{
	void visitTyped(ICPackVisitor & visitor) override;
	void applyGs(CGameState *gs) override {}

	BattleID battleID = BattleID::NONE;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
	}
};

struct DLL_LINKAGE BattleSpellCast : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	bool activeCast = true;
	BattleSide side = BattleSide::NONE; //which hero did cast spell
	SpellID spellID; //id of spell
	ui8 manaGained = 0; //mana channeling ability
	BattleHex tile; //destination tile (may not be set in some global/mass spells
	std::set<ui32> affectedCres; //ids of creatures affected by this spell, generally used if spell does not set any effect (like dispel or cure)
	std::set<ui32> resistedCres; // creatures that resisted the spell (e.g. Dwarves)
	std::set<ui32> reflectedCres; // creatures that reflected the spell (e.g. Magic Mirror spell)
	si32 casterStack = -1; // -1 if not cated by creature, >=0 caster stack ID
	bool castByHero = true; //if true - spell has been cast by hero, otherwise by a creature

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & side;
		h & spellID;
		h & manaGained;
		h & tile;
		h & affectedCres;
		h & resistedCres;
		h & reflectedCres;
		h & casterStack;
		h & castByHero;
		h & activeCast;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE StacksInjured : public CPackForClient
{
	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	BattleID battleID = BattleID::NONE;
	std::vector<BattleStackAttacked> stacks;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stacks;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleResultsApplied : public CPackForClient
{
	BattleID battleID = BattleID::NONE;
	PlayerColor victor;
	PlayerColor loser;
	ChangeSpells learnedSpells;
	std::vector<BulkMoveArtifacts> artifacts;
	CStackBasicDescriptor raisedStack;
	void visitTyped(ICPackVisitor & visitor) override;
	void applyGs(CGameState *gs) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & victor;
		h & loser;
		h & learnedSpells;
		h & artifacts;
		h & raisedStack;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleObstaclesChanged : public CPackForClient
{
	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	BattleID battleID = BattleID::NONE;
	std::vector<ObstacleChanges> changes;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & changes;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE CatapultAttack : public CPackForClient
{
	struct AttackInfo
	{
		si16 destinationTile;
		EWallPart attackedPart;
		ui8 damageDealt;

		template <typename Handler> void serialize(Handler & h)
		{
			h & destinationTile;
			h & attackedPart;
			h & damageDealt;
		}
	};

	CatapultAttack();
	~CatapultAttack() override;

	void applyGs(CGameState * gs) override;
	void applyBattle(IBattleState * battleState);

	BattleID battleID = BattleID::NONE;
	std::vector< AttackInfo > attackedParts;
	int attacker = -1; //if -1, then a spell caused this

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & attackedParts;
		h & attacker;
		assert(battleID != BattleID::NONE);
	}
};

struct DLL_LINKAGE BattleSetStackProperty : public CPackForClient
{
	enum BattleStackProperty { CASTS, ENCHANTER_COUNTER, UNBIND, CLONED, HAS_CLONE };

	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	int stackID = 0;
	BattleStackProperty which = CASTS;
	int val = 0;
	int absolute = 0;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stackID;
		h & which;
		h & val;
		h & absolute;
		assert(battleID != BattleID::NONE);
	}

protected:
	void visitTyped(ICPackVisitor & visitor) override;
};

///activated at the beginning of turn
struct DLL_LINKAGE BattleTriggerEffect : public CPackForClient
{
	void applyGs(CGameState * gs) override; //effect

	BattleID battleID = BattleID::NONE;
	int stackID = 0;
	int effect = 0; //use corresponding Bonus type
	int val = 0;
	int additionalInfo = 0;

	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & stackID;
		h & effect;
		h & val;
		h & additionalInfo;
		assert(battleID != BattleID::NONE);
	}

protected:
	void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE BattleUpdateGateState : public CPackForClient
{
	void applyGs(CGameState * gs) override;

	BattleID battleID = BattleID::NONE;
	EGateState state = EGateState::NONE;
	template <typename Handler> void serialize(Handler & h)
	{
		h & battleID;
		h & state;
		assert(battleID != BattleID::NONE);
	}

protected:
	void visitTyped(ICPackVisitor & visitor) override;
};

VCMI_LIB_NAMESPACE_END
