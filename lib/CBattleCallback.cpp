#include "StdInc.h"
#include "CBattleCallback.h"
#include "BattleState.h"
#include "CGameState.h"
#include "NetPacks.h"
#include "spells/CSpellHandler.h"
#include "VCMI_Lib.h"
#include "CTownHandler.h"
#include "mapObjects/CGTownInstance.h"

/*
 * CBattleCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#define RETURN_IF_NOT_BATTLE(X) if(!duringBattle()) {logGlobal->errorStream() << __FUNCTION__ << " called when no battle!"; return X; }

namespace SiegeStuffThatShouldBeMovedToHandlers //  <=== TODO
{
	static void retreiveTurretDamageRange(const CGTownInstance * town, const CStack *turret, double &outMinDmg, double &outMaxDmg)
	{
		assert(turret->getCreature()->idNumber == CreatureID::ARROW_TOWERS);
		assert(town);
		assert(turret->position >= -4 && turret->position <= -2);

		float multiplier = (turret->position == -2) ? 1 : 0.5;

		int baseMin = 6;
		int baseMax = 10;

		outMinDmg = multiplier * (baseMin + town->getTownLevel() * 2);
		outMaxDmg = multiplier * (baseMax + town->getTownLevel() * 3);
	}

	static BattleHex lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
	{
		static const BattleHex lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

		return lineToHex[line];
	}

	static bool sameSideOfWall(BattleHex pos1, BattleHex pos2)
	{
		const int wallInStackLine = lineToWallHex(pos1.getY());
		const int wallInDestLine = lineToWallHex(pos2.getY());

		const bool stackLeft = pos1 < wallInStackLine;
		const bool destLeft = pos2 < wallInDestLine;

		return stackLeft != destLeft;
	}

	// parts of wall
	static const std::pair<int, EWallPart::EWallPart> wallParts[] =
	{
		std::make_pair(50,  EWallPart::KEEP),
		std::make_pair(183, EWallPart::BOTTOM_TOWER),
		std::make_pair(182, EWallPart::BOTTOM_WALL),
		std::make_pair(130, EWallPart::BELOW_GATE),
		std::make_pair(78,  EWallPart::OVER_GATE),
		std::make_pair(29,  EWallPart::UPPER_WALL),
		std::make_pair(12,  EWallPart::UPPER_TOWER),
		std::make_pair(95,  EWallPart::INDESTRUCTIBLE_PART_OF_GATE),
		std::make_pair(96,  EWallPart::GATE),
		std::make_pair(45,  EWallPart::INDESTRUCTIBLE_PART),
		std::make_pair(62,  EWallPart::INDESTRUCTIBLE_PART),
		std::make_pair(112, EWallPart::INDESTRUCTIBLE_PART),
		std::make_pair(147, EWallPart::INDESTRUCTIBLE_PART)
	};

	static EWallPart::EWallPart hexToWallPart(BattleHex hex)
	{
		for(auto & elem : wallParts)
		{
			if(elem.first == hex)
				return elem.second;
		}

		return EWallPart::INVALID; //not found!
	}

	static BattleHex WallPartToHex(EWallPart::EWallPart part)
	{
		for(auto & elem : wallParts)
		{
			if(elem.second == part)
				return elem.first;
		}

		return BattleHex::INVALID; //not found!
	}
}

using namespace SiegeStuffThatShouldBeMovedToHandlers;

boost::shared_mutex& CCallbackBase::getGsMutex()
{
	return *gs->mx;
}

bool CCallbackBase::duringBattle() const
{
	return getBattle() != nullptr;
}

void CCallbackBase::setBattle(const BattleInfo *B)
{
	battle = B;
}

boost::optional<PlayerColor> CCallbackBase::getPlayerID() const
{
	return player;
}

ETerrainType CBattleInfoEssentials::battleTerrainType() const
{
	RETURN_IF_NOT_BATTLE(ETerrainType::WRONG);
	return getBattle()->terrainType;
}

BFieldType CBattleInfoEssentials::battleGetBattlefieldType() const
{
	RETURN_IF_NOT_BATTLE(BFieldType::NONE);
	return getBattle()->battlefieldType;
}

std::vector<std::shared_ptr<const CObstacleInstance> > CBattleInfoEssentials::battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective /*= boost::none*/) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = battleGetMySide();
	}
	else
	{
		if(!!player && *perspective != battleGetMySide())
		{
			logGlobal->errorStream() << "Unauthorized access attempt!";
			assert(0); //I want to notice if that happens
			//perspective = battleGetMySide();
		}
	}

	for(auto oi : getBattle()->obstacles)
	{
		if(getBattle()->battleIsObstacleVisibleForSide(*oi, *perspective))
			ret.push_back(oi);
	}

	return ret;
}

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattlePerspective::BattlePerspective side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return side == BattlePerspective::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);

	for(const CStack *s : battleGetAllStacks())
	{
		if(s->attackerOwned == !side  &&  s->getCreature()->isItNativeTerrain(getBattle()->terrainType))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks(bool includeTurrets /*= false*/) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return !s->isGhost() && (includeTurrets || !s->isTurret());
	});
}

TStacks CBattleInfoEssentials::battleGetStacksIf(TStackFilter predicate) const
{
	TStacks ret;
	RETURN_IF_NOT_BATTLE(ret);

	vstd::copy_if(getBattle()->stacks, std::back_inserter(ret), predicate);

	return ret;
}

TStacks CBattleInfoEssentials::battleAliveStacks() const
{
	return battleGetStacksIf([](const CStack * s){
		return s->isValidTarget(false);
	});
}

TStacks CBattleInfoEssentials::battleAliveStacks(ui8 side) const
{
	return battleGetStacksIf([=](const CStack * s){
		return s->isValidTarget(false) && s->attackerOwned == !side;
	});
}

int CBattleInfoEssentials::battleGetMoatDmg() const
{
	RETURN_IF_NOT_BATTLE(0);

	auto town = getBattle()->town;
	if(!town)
		return 0;

	return town->town->moatDamage;
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	RETURN_IF_NOT_BATTLE(nullptr);


	if(!getBattle() || getBattle()->town == nullptr)
		return nullptr;

	return getBattle()->town;
}

BattlePerspective::BattlePerspective CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattlePerspective::INVALID);
	if(!player)
		return BattlePerspective::ALL_KNOWING;
	if(*player == getBattle()->sides[0].color)
		return BattlePerspective::LEFT_SIDE;
	if(*player == getBattle()->sides[1].color)
		return BattlePerspective::RIGHT_SIDE;

	logGlobal->errorStream() << "Cannot find player " << *player << " in battle!";
	return BattlePerspective::INVALID;
}

const CStack * CBattleInfoEssentials::battleActiveStack() const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	return battleGetStackByID(getBattle()->activeStack);
}

const CStack* CBattleInfoEssentials::battleGetStackByID(int ID, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	auto stacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->ID == ID && (!onlyAlive || s->alive());
	});

	if(stacks.empty())
		return nullptr;
	else
		return stacks[0];
}

bool CBattleInfoEssentials::battleDoWeKnowAbout(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto p = battleGetMySide();
	return p == BattlePerspective::ALL_KNOWING  ||  p == side;
}

si8 CBattleInfoEssentials::battleTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->tacticDistance;
}

si8 CBattleInfoEssentials::battleGetTacticsSide() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->tacticsSide;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " wrong argument!";
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " access check ";
		return nullptr;
	}

	return getBattle()->sides[side].hero;
}

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " wrong argument!";
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " access check ";
		return nullptr;
	}

	return getBattle()->sides[side].armyObject;
}

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo( ui8 side ) const
{
	auto hero = getBattle()->sides[side].hero;
	if(!hero)
	{
		logGlobal->warnStream() << __FUNCTION__ << ": side " << (int)side << " does not have hero!";
		return InfoAboutHero();
	}

	return InfoAboutHero(hero, battleDoWeKnowAbout(side));
}

int CBattleInfoEssentials::battleCastSpells(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->sides[side].castSpellsCount;
}

const IBonusBearer * CBattleInfoEssentials::getBattleNode() const
{
	return getBattle();
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastSpell(PlayerColor player, ECastingMode::ECastingMode mode) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	const ui8 side = playerToSide(player);
	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->warnStream() << "You can't check if enemy can cast given spell!";
		return ESpellCastProblem::INVALID;
	}

	switch (mode)
	{
	case ECastingMode::HERO_CASTING:
		{
			if(battleTacticDist())
				return ESpellCastProblem::ONGOING_TACTIC_PHASE;
			if(battleCastSpells(side) > 0)
				return ESpellCastProblem::ALREADY_CASTED_THIS_TURN;

			auto hero = battleGetFightingHero(side);

			if(!hero)
				return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
			if(!hero->getArt(ArtifactPosition::SPELLBOOK))
				return ESpellCastProblem::NO_SPELLBOOK;
			if(hero->hasBonusOfType(Bonus::BLOCK_ALL_MAGIC))
				return ESpellCastProblem::MAGIC_IS_BLOCKED;
		}
		break;
	default:
		break;
	}

	return ESpellCastProblem::OK;
}

bool CBattleInfoEssentials::battleCanFlee(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	ui8 mySide = playerToSide(player);
	const CGHeroInstance *myHero = battleGetFightingHero(mySide);

	//current player have no hero
	if(!myHero)
		return false;

	//eg. one of heroes is wearing shakles of war
	if(myHero->hasBonusOfType(Bonus::BATTLE_NO_FLEEING))
		return false;

	//we are besieged defender
	if(mySide == BattleSide::DEFENDER  &&  battleGetSiegeLevel())
	{
		auto town = battleGetDefendedTown();
		if(!town->hasBuilt(BuildingID::ESCAPE_TUNNEL, ETownType::STRONGHOLD))
			return false;
	}

	return true;
}

ui8 CBattleInfoEssentials::playerToSide(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(-1);
	int ret = vstd::find_pos_if(getBattle()->sides, [=](const SideInBattle &side){ return side.color == player; });
	if(ret < 0)
		logGlobal->warnStream() << "Cannot find side for player " << player;

	return ret;
}

ui8 CBattleInfoEssentials::battleGetSiegeLevel() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->town ? getBattle()->town->fortLevel() : CGTownInstance::NONE;
}

bool CBattleInfoEssentials::battleCanSurrender(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	//conditions like for fleeing + enemy must have a hero
	return battleCanFlee(player) && battleHasHero(!playerToSide(player));
}

bool CBattleInfoEssentials::battleHasHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	assert(side < 2);
	return getBattle()->sides[side].hero;
}

si8 CBattleInfoEssentials::battleGetWallState(int partOfWall) const
{
	RETURN_IF_NOT_BATTLE(0);
	if(getBattle()->town == nullptr || getBattle()->town->fortLevel() == CGTownInstance::NONE)
		return EWallState::NONE;

	assert(partOfWall >= 0 && partOfWall < EWallPart::PARTS_COUNT);
	return getBattle()->si.wallState[partOfWall];
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	RETURN_IF_NOT_BATTLE(EGateState::NONE);
	if(getBattle()->town == nullptr || getBattle()->town->fortLevel() == CGTownInstance::NONE)
		return EGateState::NONE;

	return getBattle()->si.gateState;
}

si8 CBattleInfoCallback::battleHasWallPenalty( const CStack * stack, BattleHex destHex ) const
{
	return battleHasWallPenalty(stack, stack->position, destHex);
}

si8 CBattleInfoCallback::battleHasWallPenalty(const IBonusBearer *bonusBearer, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);
	if (!battleGetSiegeLevel() || bonusBearer->hasBonusOfType(Bonus::NO_WALL_PENALTY))
		return false;

	const int wallInStackLine = lineToWallHex(shooterPosition.getY());
	const int wallInDestLine = lineToWallHex(destHex.getY());

	const bool stackLeft = shooterPosition < wallInStackLine;
	const bool destRight = destHex > wallInDestLine;

	if (stackLeft && destRight) //shooting from outside to inside
	{
		int row = (shooterPosition + destHex) / (2 * GameConstants::BFIELD_WIDTH);
		if (shooterPosition > destHex && ((destHex % GameConstants::BFIELD_WIDTH - shooterPosition % GameConstants::BFIELD_WIDTH) < 2)) //shooting up high
			row -= 2;
		const int wallPos = lineToWallHex(row);
		if (!isWallPartPotentiallyAttackable(battleHexToWallPart(wallPos))) return true;
	}

	return false;
}

si8 CBattleInfoCallback::battleCanTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const
{
	RETURN_IF_NOT_BATTLE(false);
	if (!getAccesibility(stack).accessible(destHex, stack))
		return false;

	if (battleGetSiegeLevel() && telportLevel < 2) //check for wall
		return sameSideOfWall(stack->position, destHex);

	return true;
}

std::set<BattleHex> CBattleInfoCallback::battleGetAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos  /*= BattleHex::INVALID*/) const
{
	std::set<BattleHex> attackedHexes;
	RETURN_IF_NOT_BATTLE(attackedHexes);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (BattleHex tile : at.hostileCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		if(battleGetStackByPos(tile, true)) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

SpellID CBattleInfoCallback::battleGetRandomStackSpell(const CStack * stack, ERandomSpell mode) const
{
	switch (mode)
	{
	case RANDOM_GENIE:
		return getRandomBeneficialSpell(stack); //target
		break;
	case RANDOM_AIMED:
		return getRandomCastedSpell(stack); //caster
		break;
	default:
		logGlobal->errorStream() << "Incorrect mode of battleGetRandomSpell (" << mode <<")";
		return SpellID::NONE;
	}
}

const CStack* CBattleInfoCallback::battleGetStackByPos(BattleHex pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	for(auto s : battleGetAllStacks(true))
		if(vstd::contains(s->getHexes(), pos) && (!onlyAlive || s->alive()))
				return s;

	return nullptr;
}

void CBattleInfoCallback::battleGetStackQueue(std::vector<const CStack *> &out, const int howMany, const int turn /*= 0*/, int lastMoved /*= -1*/) const
{
	RETURN_IF_NOT_BATTLE();

	//let's define a huge lambda
	auto takeStack = [&](std::vector<const CStack *> &st) -> const CStack*
	{
		const CStack *ret = nullptr;
		unsigned i, //fastest stack
			j=0; //fastest stack of the other side
		for(i = 0; i < st.size(); i++)
			if(st[i])
				break;

		//no stacks left
		if(i == st.size())
			return nullptr;

		const CStack *fastest = st[i], *other = nullptr;
		int bestSpeed = fastest->Speed(turn);

		//FIXME: comparison between bool and integer. Logic does not makes sense either
		if(fastest->attackerOwned != lastMoved)
		{
			ret = fastest;
		}
		else
		{
			for(j = i + 1; j < st.size(); j++)
			{
				if(!st[j]) continue;
				if(st[j]->attackerOwned != lastMoved || st[j]->Speed(turn) != bestSpeed)
					break;
			}

			if(j >= st.size())
			{
				ret = fastest;
			}
			else
			{
				other = st[j];
				if(other->Speed(turn) != bestSpeed)
					ret = fastest;
				else
					ret = other;
			}
		}

		assert(ret);
		if(ret == fastest)
			st[i] = nullptr;
		else
			st[j] = nullptr;

		lastMoved = ret->attackerOwned;
		return ret;
	};

	//We'll split creatures with remaining movement to 4 buckets
	// [0] - turrets/catapult,
	// [1] - normal (unmoved) creatures, other war machines,
	// [2] - waited cres that had morale,
	// [3] - rest of waited cres
	std::vector<const CStack *> phase[4];
	int toMove = 0; //how many stacks still has move
	const CStack *active = battleActiveStack();

	//active stack hasn't taken any action yet - must be placed at the beginning of queue, no matter what
	if(!turn && active && active->willMove() && !active->waited())
	{
		out.push_back(active);
		if(out.size() == howMany)
			return;
	}

	auto allStacks = battleGetAllStacks(true);
	if(!vstd::contains_if(allStacks, [](const CStack *stack) { return stack->willMove(100000); })) //little evil, but 100000 should be enough for all effects to disappear
	{
		//No stack will be able to move, battle is over.
		out.clear();
		return;
	}

	for(auto s : battleGetAllStacks(true))
	{
		if((turn <= 0 && !s->willMove()) //we are considering current round and stack won't move
			|| (turn > 0 && !s->canMove(turn)) //stack won't be able to move in later rounds
			|| (turn <= 0 && s == active && out.size() && s == out.front())) //it's active stack already added at the beginning of queue
		{
			continue;
		}

		int p = -1; //in which phase this tack will move?
		if(turn <= 0 && s->waited()) //consider waiting state only for ongoing round
		{
			if(vstd::contains(s->state, EBattleStackState::HAD_MORALE))
				p = 2;
			else
				p = 3;
		}
		else if(s->getCreature()->idNumber == CreatureID::CATAPULT  ||  s->getCreature()->idNumber == CreatureID::ARROW_TOWERS) //catapult and turrets are first
		{
			p = 0;
		}
		else
		{
			p = 1;
		}

		phase[p].push_back(s);
		toMove++;
	}

	for(int i = 0; i < 4; i++)
		boost::sort(phase[i], CMP_stack(i, turn > 0 ? turn : 0));

	for(size_t i = 0; i < phase[0].size() && i < howMany; i++)
		out.push_back(phase[0][i]);

	if(out.size() == howMany)
		return;

	if(lastMoved == -1)
	{
		if(active)
		{
			//FIXME: both branches contain same code!!!
			if(out.size() && out.front() == active)
				lastMoved = active->attackerOwned;
			else
				lastMoved = active->attackerOwned;
		}
		else
		{
			lastMoved = 0;
		}
	}

	int pi = 1;
	while(out.size() < howMany)
	{
		const CStack *hlp = takeStack(phase[pi]);
		if(!hlp)
		{
			pi++;
			if(pi > 3)
			{
				//if(turn != 2)
				battleGetStackQueue(out, howMany, turn + 1, lastMoved);
				return;
			}
		}
		else
		{
			out.push_back(hlp);
		}
	}
}

void CBattleInfoCallback::battleGetStackCountOutsideHexes(bool *ac) const
{
	RETURN_IF_NOT_BATTLE();
	auto accessibility = getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		ac[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);
}

std::vector<BattleHex> CBattleInfoCallback::battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<BattleHex> * attackable) const
{
	std::vector<BattleHex> ret;

	RETURN_IF_NOT_BATTLE(ret);
	if(!stack->position.isValid()) //turrets
		return ret;

	auto reachability = getReachability(stack);

	for (int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		// If obstacles or other stacks makes movement impossible, it can't be helped.
		if(!reachability.isReachable(i))
			continue;

		if(battleTacticDist() && battleGetTacticsSide() == !stack->attackerOwned)
		{
			//Stack has to perform tactic-phase movement -> can enter any reachable tile within given range
			if(!isInTacticRange(i))
				continue;
		}
		else
		{
			//Not tactics phase -> destination must be reachable and within stack range.
			if(reachability.distances[i] > stack->Speed(0, true))
				continue;
		}

		ret.push_back(i);

		if(addOccupiable && stack->doubleWide())
		{
			//If two-hex stack can stand on hex i then obviously it can occupy its second hex from that position
			ret.push_back(stack->occupiedHex(i));
		}
	}


	if(attackable)
	{
		auto meleeAttackable = [&](BattleHex hex) -> bool
		{
			// Return true if given hex has at least one available neighbour.
			// Available hexes are already present in ret vector.
			auto availableNeighbor = boost::find_if(ret, [=] (BattleHex availableHex)
				{  return BattleHex::mutualPosition(hex, availableHex) >= 0;  });

			return availableNeighbor != ret.end();
		};

		for(const CStack * otherSt : battleAliveStacks(stack->attackerOwned))
		{
			if(!otherSt->isValidTarget(false))
				continue;

			std::vector<BattleHex> occupied = otherSt->getHexes();

			if(battleCanShoot(stack, otherSt->position))
			{
				attackable->insert(attackable->end(), occupied.begin(), occupied.end());
				continue;
			}

			for(BattleHex he : occupied)
			{
				if(meleeAttackable(he))
					attackable->push_back(he);
			}
		}
	}

	//adding occupiable likely adds duplicates to ret -> clean it up
	boost::sort(ret);
	ret.erase(boost::unique(ret).end(), ret.end());
	return ret;
}

bool CBattleInfoCallback::battleCanAttack(const CStack * stack, const CStack * target, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist())
		return false;

	if (!stack || !target)
		return false;

	if (stack->owner == target->owner)
		return false;

	auto &id = stack->getCreature()->idNumber;
	if (id == CreatureID::FIRST_AID_TENT || id == CreatureID::CATAPULT)
		return false;

	if (!target->alive())
		return false;

	return true;
}

bool CBattleInfoCallback::battleCanShoot(const CStack * stack, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist()) //no shooting during tactics
		return false;

	const CStack *dst = battleGetStackByPos(dest);

	if(!stack || !dst)
		return false;

	if(stack->hasBonusOfType(Bonus::FORGETFULL)) //forgetfulness
		return false;

	if(stack->getCreature()->idNumber == CreatureID::CATAPULT && dst) //catapult cannot attack creatures
		return false;

	//const CGHeroInstance * stackHero = battleGetOwner(stack);
	if(stack->hasBonusOfType(Bonus::SHOOTER)//it's shooter
		&& stack->owner != dst->owner
		&& dst->alive()
		&& (!battleIsStackBlocked(stack)  ||  stack->hasBonusOfType(Bonus::FREE_SHOOTING))
		&& stack->shots
		)
		return true;
	return false;
}

TDmgRange CBattleInfoCallback::calculateDmgRange(const CStack* attacker, const CStack* defender, bool shooting,
												 ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg) const
{
	return calculateDmgRange(attacker, defender, attacker->count, shooting, charge, lucky, unlucky, deathBlow, ballistaDoubleDmg);
}

TDmgRange CBattleInfoCallback::calculateDmgRange(const BattleAttackInfo &info) const
{
	auto battleBonusValue = [&](const IBonusBearer * bearer, CSelector selector) -> int
	{
		auto noLimit = Selector::effectRange(Bonus::NO_LIMIT);
		auto limitMatches = info.shooting
				? Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT)
				: Selector::effectRange(Bonus::ONLY_MELEE_FIGHT);

		//any regular bonuses or just ones for melee/ranged
		return bearer->getBonuses(selector, noLimit.Or(limitMatches))->totalValue();
	};

	double additiveBonus = 1.0, multBonus = 1.0,
		minDmg = info.attackerBonuses->getMinDamage() * info.attackerCount,//TODO: ONLY_MELEE_FIGHT / ONLY_DISTANCE_FIGHT
		maxDmg = info.attackerBonuses->getMaxDamage() * info.attackerCount;

	const CCreature *attackerType = info.attacker->getCreature(),
		*defenderType = info.defender->getCreature();

	if(attackerType->idNumber == CreatureID::ARROW_TOWERS)
	{
		SiegeStuffThatShouldBeMovedToHandlers::retreiveTurretDamageRange(battleGetDefendedTown(), info.attacker, minDmg, maxDmg);
	}

	if(info.attackerBonuses->hasBonusOfType(Bonus::SIEGE_WEAPON) && attackerType->idNumber != CreatureID::ARROW_TOWERS) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		auto retreiveHeroPrimSkill = [&](int skill) -> int
		{
			const Bonus *b = info.attackerBonuses->getBonus(Selector::sourceTypeSel(Bonus::HERO_BASE_SKILL).And(Selector::typeSubtype(Bonus::PRIMARY_SKILL, skill)));
			return b ? b->val : 0; //if there is no hero or no info on his primary skill, return 0
		};


		minDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
	}

	int attackDefenceDifference = 0;

	double multAttackReduction = (100 - battleBonusValue (info.attackerBonuses, Selector::type(Bonus::GENERAL_ATTACK_REDUCTION))) / 100.0;
	attackDefenceDifference += battleBonusValue (info.attackerBonuses, Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK)) * multAttackReduction;

	double multDefenceReduction = (100 - battleBonusValue (info.attackerBonuses, Selector::type(Bonus::ENEMY_DEFENCE_REDUCTION))) / 100.0;
	attackDefenceDifference -= info.defenderBonuses->Defense() * multDefenceReduction;

	if(const Bonus *slayerEffect = info.attackerBonuses->getEffect(SpellID::SLAYER)) //slayer handling //TODO: apply only ONLY_MELEE_FIGHT / DISTANCE_FIGHT?
	{
		std::vector<int> affectedIds;
		int spLevel = slayerEffect->val;

		for(int g = 0; g < VLC->creh->creatures.size(); ++g)
		{
			for(const Bonus *b : VLC->creh->creatures[g]->getBonusList())
			{
				if ( (b->type == Bonus::KING3 && spLevel >= 3) || //expert
					(b->type == Bonus::KING2 && spLevel >= 2) || //adv +
					(b->type == Bonus::KING1 && spLevel >= 0) ) //none or basic +
				{
					affectedIds.push_back(g);
					break;
				}
			}
		}

		for(auto & affectedId : affectedIds)
		{
			if(defenderType->idNumber == affectedId)
			{
				attackDefenceDifference += SpellID(SpellID::SLAYER).toSpell()->getPower(spLevel);
				break;
			}
		}
	}

	//bonus from attack/defense skills
	if(attackDefenceDifference < 0) //decreasing dmg
	{
		const double dec = std::min(0.025 * (-attackDefenceDifference), 0.7);
		multBonus *= 1.0 - dec;
	}
	else //increasing dmg
	{
		const double inc = std::min(0.05 * attackDefenceDifference, 4.0);
		additiveBonus += inc;
	}


	//applying jousting bonus
	if( info.attackerBonuses->hasBonusOfType(Bonus::JOUSTING) && !info.defenderBonuses->hasBonusOfType(Bonus::CHARGE_IMMUNITY) )
		additiveBonus += info.chargedFields * 0.05;


	//handling secondary abilities and artifacts giving premies to them
	if(info.shooting)
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARCHERY) / 100.0;
	else
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::OFFENCE) / 100.0;

	if(info.defenderBonuses)
		multBonus *= (std::max(0, 100 - info.defenderBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARMORER))) / 100.0;

	//handling hate effect
	additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::HATE, defenderType->idNumber.toEnum()) / 100.;

	//luck bonus
	if (info.luckyHit)
	{
		additiveBonus += 1.0;
	}
	//unlucky hit, used only if negative luck is enabled
	if (info.unluckyHit)
	{
		additiveBonus -= 0.5; // FIXME: how bad (and luck in general) should work with following bonuses?
	}

	//ballista double dmg
	if(info.ballistaDoubleDamage)
	{
		additiveBonus += 1.0;
	}

	if (info.deathBlow) //Dread Knight and many WoGified creatures
	{
		additiveBonus += 1.0;
	}

	//handling spell effects
	if(!info.shooting) //eg. shield
	{
		multBonus *= (100 - info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) / 100.0;
	}
	else if(info.shooting) //eg. air shield
	{
		multBonus *= (100 - info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) / 100.0;
	}

	TBonusListPtr curseEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MINIMUM_DAMAGE));
	TBonusListPtr blessEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MAXIMUM_DAMAGE));
	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();
	double curseMultiplicativePenalty = curseEffects->size() ? (*std::max_element(curseEffects->begin(), curseEffects->end(), &Bonus::compareByAdditionalInfo))->additionalInfo : 0;

	if(curseMultiplicativePenalty) //curse handling (partial, the rest is below)
	{
		multBonus *= 1.0 - curseMultiplicativePenalty/100;
	}

	auto isAdvancedAirShield = [](const Bonus *bonus)
	{
		return bonus->source == Bonus::SPELL_EFFECT
			&& bonus->sid == SpellID::AIR_SHIELD
			&& bonus->val >= SecSkillLevel::ADVANCED;
	};

	//wall / distance penalty + advanced air shield
	const bool distPenalty = !info.attackerBonuses->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY) && battleHasDistancePenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);
	const bool obstaclePenalty = battleHasWallPenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);

	if(info.shooting)
	{
		if (distPenalty || info.defenderBonuses->hasBonus(isAdvancedAirShield))
		{
			multBonus *= 0.5;
		}
		if (obstaclePenalty)
		{
			multBonus *= 0.5; //cumulative
		}
	}
	if(!info.shooting && info.attackerBonuses->hasBonusOfType(Bonus::SHOOTER) && !info.attackerBonuses->hasBonusOfType(Bonus::NO_MELEE_PENALTY))
	{
		multBonus *= 0.5;
	}

	// psychic elementals versus mind immune units 50%
	if(attackerType->idNumber == CreatureID::PSYCHIC_ELEMENTAL
	   && info.defenderBonuses->hasBonusOfType(Bonus::MIND_IMMUNITY))
	{
		multBonus *= 0.5;
	}

	// TODO attack on petrified unit 50%
	// blinded unit retaliates

	minDmg *= additiveBonus * multBonus;
	maxDmg *= additiveBonus * multBonus;

	TDmgRange returnedVal;

	if(curseEffects->size()) //curse handling (rest)
	{
		minDmg += curseBlessAdditiveModifier;
		returnedVal = std::make_pair(int(minDmg), int(minDmg));
	}
	else if(blessEffects->size()) //bless handling
	{
		maxDmg += curseBlessAdditiveModifier;
		returnedVal =  std::make_pair(int(maxDmg), int(maxDmg));
	}
	else
	{
		returnedVal =  std::make_pair(int(minDmg), int(maxDmg));
	}

	//damage cannot be less than 1
	vstd::amax(returnedVal.first, 1);
	vstd::amax(returnedVal.second, 1);

	return returnedVal;
}

TDmgRange CBattleInfoCallback::calculateDmgRange( const CStack* attacker, const CStack* defender, TQuantity attackerCount,
	bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg ) const
{
	BattleAttackInfo bai(attacker, defender, shooting);
	bai.attackerCount = attackerCount;
	bai.chargedFields = charge;
	bai.luckyHit = lucky;
	bai.unluckyHit = unlucky;
	bai.deathBlow = deathBlow;
	bai.ballistaDoubleDamage = ballistaDoubleDmg;
	return calculateDmgRange(bai);
}

TDmgRange CBattleInfoCallback::battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));
	const bool shooting = battleCanShoot(attacker, defender->position);
	const BattleAttackInfo bai(attacker, defender, shooting);
	return battleEstimateDamage(bai, retaliationDmg);
}

std::pair<ui32, ui32> CBattleInfoCallback::battleEstimateDamage(const BattleAttackInfo &bai, std::pair<ui32, ui32> * retaliationDmg /*= nullptr*/) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));

	//const bool shooting = battleCanShoot(bai.attacker, bai.defenderPosition); //TODO handle bonus bearer
	//const ui8 mySide = !attacker->attackerOwned;

	TDmgRange ret = calculateDmgRange(bai);

	if(retaliationDmg)
	{
		if(bai.shooting)
		{
			retaliationDmg->first = retaliationDmg->second = 0;
		}
		else
		{
			ui32 TDmgRange::* pairElems[] = {&TDmgRange::first, &TDmgRange::second};
			for (int i=0; i<2; ++i)
			{
				BattleStackAttacked bsa;
				bsa.damageAmount = ret.*pairElems[i];
				bai.defender->prepareAttacked(bsa, gs->getRandomGenerator(), bai.defenderCount);

				auto retaliationAttack = bai.reverse();
				retaliationAttack.attackerCount = bsa.newAmount;
				retaliationDmg->*pairElems[!i] = calculateDmgRange(retaliationAttack).*pairElems[!i];
			}
		}
	}

	return ret;
}

std::shared_ptr<const CObstacleInstance> CBattleInfoCallback::battleGetObstacleOnPos(BattleHex tile, bool onlyBlocking /*= true*/) const
{
	RETURN_IF_NOT_BATTLE(std::shared_ptr<const CObstacleInstance>());

	for(auto &obs : battleGetAllObstacles())
	{
		if(vstd::contains(obs->getBlockedTiles(), tile)
			|| (!onlyBlocking  &&  vstd::contains(obs->getAffectedTiles(), tile)))
		{
			return obs;
		}
	}

	return std::shared_ptr<const CObstacleInstance>();
}

AccessibilityInfo CBattleInfoCallback::getAccesibility() const
{
	AccessibilityInfo ret;
	ret.fill(EAccessibility::ACCESSIBLE);

	//removing accessibility for side columns of hexes
	for(int y = 0; y < GameConstants::BFIELD_HEIGHT; y++)
	{
		ret[BattleHex(GameConstants::BFIELD_WIDTH - 1, y)] = EAccessibility::SIDE_COLUMN;
		ret[BattleHex(0, y)] = EAccessibility::SIDE_COLUMN;
	}

	//gate -> should be before stacks
	if(battleGetSiegeLevel() > 0)
	{
		EAccessibility::EAccessibility accessability = EAccessibility::ACCESSIBLE;
		switch(battleGetGateState())
		{
		case EGateState::CLOSED:
			accessability = EAccessibility::GATE;
			break;

		case EGateState::BLOCKED:
			accessability = EAccessibility::UNAVAILABLE;
			break;
		}
		ret[ESiegeHex::GATE_OUTER] = ret[ESiegeHex::GATE_INNER] = accessability;
	}

	//tiles occupied by standing stacks
	for(auto stack : battleAliveStacks())
	{
		for(auto hex : stack->getHexes())
			if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
				ret[hex] = EAccessibility::ALIVE_STACK;
	}

	//obstacles
	for(const auto &obst : battleGetAllObstacles())
	{
		for(auto hex : obst->getBlockedTiles())
			ret[hex] = EAccessibility::OBSTACLE;
	}

	//walls
	if(battleGetSiegeLevel() > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 62, 112, 147, 165};
		for(auto hex : permanentlyLocked)
			ret[hex] = EAccessibility::UNAVAILABLE;

		//TODO likely duplicated logic
		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] =
		{
			//which part of wall, which hex is blocked if this part of wall is not destroyed
			std::make_pair(2, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_4)),
			std::make_pair(3, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_3)),
			std::make_pair(4, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_2)),
			std::make_pair(5, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_1))
		};

		for(auto & elem : lockedIfNotDestroyed)
		{
			if(battleGetWallState(elem.first) != EWallState::DESTROYED)
				ret[elem.second] = EAccessibility::DESTRUCTIBLE_WALL;
		}
	}

	return ret;
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const CStack *stack) const
{
	return getAccesibility(stack->getHexes());
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const std::vector<BattleHex> &accessibleHexes) const
{
	auto ret = getAccesibility();
	for(auto hex : accessibleHexes)
		if(hex.isValid())
			ret[hex] = EAccessibility::ACCESSIBLE;

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const AccessibilityInfo &accessibility, const ReachabilityInfo::Parameters &params) const
{
	ReachabilityInfo ret;
	ret.accessibility = accessibility;
	ret.params = params;

	ret.predecessors.fill(BattleHex::INVALID);
	ret.distances.fill(ReachabilityInfo::INFINITE_DIST);

	if(!params.startPosition.isValid()) //if got call for arrow turrets
		return ret;

	const std::set<BattleHex> quicksands = getStoppers(params.perspective);
	//const bool twoHexCreature = params.doubleWide;


	std::queue<BattleHex> hexq; //bfs queue

	//first element
	hexq.push(params.startPosition);
	ret.distances[params.startPosition] = 0;

	while(!hexq.empty()) //bfs loop
	{
		const BattleHex curHex = hexq.front();
		hexq.pop();

		//walking stack can't step past the quicksands
		//TODO what if second hex of two-hex creature enters quicksand
		if(curHex != params.startPosition && vstd::contains(quicksands, curHex))
			continue;

		const int costToNeighbour = ret.distances[curHex] + 1;
		for(BattleHex neighbour : curHex.neighbouringTiles())
		{
			const bool accessible = accessibility.accessible(neighbour, params.doubleWide, params.attackerOwned);
			const int costFoundSoFar = ret.distances[neighbour];

			if(accessible  &&  costToNeighbour < costFoundSoFar)
			{
				hexq.push(neighbour);
				ret.distances[neighbour] = costToNeighbour;
				ret.predecessors[neighbour] = curHex;
			}
		}
	}

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const CStack *stack) const
{
	return makeBFS(getAccesibility(stack), ReachabilityInfo::Parameters(stack));
}

std::set<BattleHex> CBattleInfoCallback::getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const
{
	std::set<BattleHex> ret;
	RETURN_IF_NOT_BATTLE(ret);

	for(auto &oi : battleGetAllObstacles(whichSidePerspective))
	{
		if(battleIsObstacleVisibleForSide(*oi, whichSidePerspective))
		{
			range::copy(oi->getStoppingTile(), vstd::set_inserter(ret));
		}
	}

	return ret;
}

std::pair<const CStack *, BattleHex> CBattleInfoCallback::getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const
{
	auto reachability = getReachability(closest);
	auto avHexes = battleGetAvailableHexes(closest, false);

	// I hate std::pairs with their undescriptive member names first / second
	struct DistStack
	{
		int distanceToPred;
		BattleHex destination;
		const CStack *stack;
	};

	std::vector<DistStack> stackPairs;

	std::vector<const CStack *> possibleStacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->isValidTarget(false) && s != closest && (boost::logic::indeterminate(attackerOwned) || s->attackerOwned == attackerOwned);
	});

	for(const CStack * st : possibleStacks)
		for(BattleHex hex : avHexes)
			if(CStack::isMeleeAttackPossible(closest, st, hex))
			{
				DistStack hlp = {reachability.distances[st->position], hex, st};
				stackPairs.push_back(hlp);
			}

	if (stackPairs.size())
	{
		auto comparator = [](DistStack lhs, DistStack rhs) { return lhs.distanceToPred < rhs.distanceToPred; };
		auto minimal = boost::min_element(stackPairs, comparator);
		return std::make_pair(minimal->stack, minimal->destination);
	}
	else
		return std::make_pair<const CStack * , BattleHex>(nullptr, BattleHex::INVALID);
}

si8 CBattleInfoCallback::battleGetTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);

	//TODO get rid of this method
	if(battleDoWeKnowAbout(battleGetTacticsSide()))
		return battleTacticDist();

	return 0;
}

bool CBattleInfoCallback::isInTacticRange(BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto side = battleGetTacticsSide();
	auto dist = battleGetTacticDist();

	return ((!side && dest.getX() > 0 && dest.getX() <= dist)
		|| (side && dest.getX() < GameConstants::BFIELD_WIDTH - 1 && dest.getX() >= GameConstants::BFIELD_WIDTH - dist - 1));
}

ReachabilityInfo CBattleInfoCallback::getReachability(const CStack *stack) const
{
	ReachabilityInfo::Parameters params(stack);

	if(!battleDoWeKnowAbout(!stack->attackerOwned))
	{
		//Stack is held by enemy, we can't use his perspective to check for reachability.
		// Happens ie. when hovering enemy stack for its range. The arg could be set properly, but it's easier to fix it here.
		params.perspective = battleGetMySide();
	}

	return getReachability(params);
}

ReachabilityInfo CBattleInfoCallback::getReachability(const ReachabilityInfo::Parameters &params) const
{
	if(params.flying)
		return getFlyingReachability(params);
	else
		return makeBFS(getAccesibility(params.knownAccessible), params);
}

ReachabilityInfo CBattleInfoCallback::getFlyingReachability(const ReachabilityInfo::Parameters &params) const
{
	ReachabilityInfo ret;
	ret.accessibility = getAccesibility(params.knownAccessible);

	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		if(ret.accessibility.accessible(i, params.doubleWide, params.attackerOwned))
		{
			ret.predecessors[i] = params.startPosition;
			ret.distances[i] = BattleHex::getDistance(params.startPosition, i);
		}
	}

	return ret;
}

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes (const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	//does not return hex attacked directly
	//TODO: apply rotation to two-hex attackers
	bool isAttacker = attacker->attackerOwned;

	AttackableTiles at;
	RETURN_IF_NOT_BATTLE(at);

	const int WN = GameConstants::BFIELD_WIDTH;
	ui16 hex = (attackerPos != BattleHex::INVALID) ? attackerPos.hex : attacker->position.hex; //real or hypothetical (cursor) position

	//FIXME: dragons or cerbers can rotate before attack, making their base hex different (#1124)
	bool reverse = isToReverse (hex, destinationTile, isAttacker, attacker->doubleWide(), isAttacker);
	if (reverse)
	{
		hex = attacker->occupiedHex(hex); //the other hex stack stands on
	}
	if (attacker->hasBonusOfType(Bonus::ATTACKS_ALL_ADJACENT))
	{
		boost::copy (attacker->getSurroundingHexes (attackerPos), vstd::set_inserter (at.hostileCreaturePositions));
	}
	if (attacker->hasBonusOfType(Bonus::THREE_HEADED_ATTACK))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		for (BattleHex tile : hexes)
		{
			if ((BattleHex::mutualPosition(tile, destinationTile) > -1 && BattleHex::mutualPosition (tile, hex) > -1)) //adjacent both to attacker's head and attacked tile
			{
				const CStack * st = battleGetStackByPos(tile, true);
				if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
				{
					at.hostileCreaturePositions.insert(tile);
				}
			}
		}
	}
	if (attacker->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH) && BattleHex::mutualPosition (destinationTile.hex, hex) > -1) //only adjacent hexes are subject of dragon breath calculation
	{
		std::vector<BattleHex> hexes; //only one, in fact
		int pseudoVector = destinationTile.hex - hex;
		switch (pseudoVector)
		{
		case 1:
		case -1:
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector, hexes);
			break;
		case WN: //17 //left-down or right-down
		case -WN: //-17 //left-up or right-up
		case WN + 1: //18 //right-down
		case -WN + 1: //-16 //right-up
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector + (((hex/WN)%2) ? 1 : -1 ), hexes);
			break;
		case WN-1: //16 //left-down
		case -WN-1: //-18 //left-up
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector + (((hex/WN)%2) ? 1 : 0), hexes);
			break;
		}
		for (BattleHex tile : hexes)
		{
			//friendly stacks can also be damaged by Dragon Breath
			if (battleGetStackByPos (tile, true))
				at.friendlyCreaturePositions.insert (tile);
		}
	}

	return at;
}

std::set<const CStack*> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos /*= BattleHex::INVALID*/) const
{
	std::set<const CStack*> attackedCres;
	RETURN_IF_NOT_BATTLE(attackedCres);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);
	for (BattleHex tile : at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.insert(st);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.insert(st);
		}
	}
	return attackedCres;
}

//TODO: this should apply also to mechanics and cursor interface
bool CBattleInfoCallback::isToReverseHlp (BattleHex hexFrom, BattleHex hexTo, bool curDir) const
{
	int fromX = hexFrom.getX();
	int fromY = hexFrom.getY();
	int toX = hexTo.getX();
	int toY = hexTo.getY();

	if (curDir) // attacker, facing right
	{
		if (fromX < toX)
			return false;
		if (fromX > toX)
			return true;

		if (fromY % 2 == 0 && toY % 2 == 1)

			return true;
		return false;
	}
	else // defender, facing left
	{
		if(fromX < toX)
			return true;
		if(fromX > toX)
			return false;

		if (fromY % 2 == 1 && toY % 2 == 0)
			return true;
		return false;
	}
}

//TODO: this should apply also to mechanics and cursor interface
bool CBattleInfoCallback::isToReverse (BattleHex hexFrom, BattleHex hexTo, bool curDir, bool toDoubleWide, bool toDir) const
{
	if (hexTo < 0  ||  hexFrom < 0) //turret
		return false;

	if (toDoubleWide)
	{
		if (isToReverseHlp (hexFrom, hexTo, curDir))
		{
			if (toDir)
				return isToReverseHlp (hexFrom, hexTo-1, curDir);
			else
				return isToReverseHlp (hexFrom, hexTo+1, curDir);
		}
		return false;
	}
	else
	{
		return isToReverseHlp(hexFrom, hexTo, curDir);
	}
}

ReachabilityInfo::TDistances CBattleInfoCallback::battleGetDistances(const CStack * stack, BattleHex hex /*= BattleHex::INVALID*/, BattleHex * predecessors /*= nullptr*/) const
{
	ReachabilityInfo::TDistances ret;
	ret.fill(-1);
	RETURN_IF_NOT_BATTLE(ret);

	ReachabilityInfo::Parameters params(stack);
	params.perspective = battleGetMySide();
	params.startPosition = hex.isValid() ? hex : stack->position;
	auto reachability = getReachability(params);

	boost::copy(reachability.distances, ret.begin());

	if(predecessors)
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			predecessors[i] = reachability.predecessors[i];

	return ret;
}

si8 CBattleInfoCallback::battleHasDistancePenalty(const CStack * stack, BattleHex destHex) const
{
	return battleHasDistancePenalty(stack, stack->position, destHex);
}

si8 CBattleInfoCallback::battleHasDistancePenalty(const IBonusBearer *bonusBearer, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(bonusBearer->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY))
		return false;

	if(const CStack * dstStack = battleGetStackByPos(destHex, false))
	{
		//If any hex of target creature is within range, there is no penalty
		for(auto hex : dstStack->getHexes())
			if(BattleHex::getDistance(shooterPosition, hex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
				return false;

		//TODO what about two-hex shooters?
	}
	else
	{
		if (BattleHex::getDistance(shooterPosition, destHex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
			return false;
	}

	return true;
}

BattleHex CBattleInfoCallback::wallPartToBattleHex(EWallPart::EWallPart part) const
{
	RETURN_IF_NOT_BATTLE(BattleHex::INVALID);
	return WallPartToHex(part);
}

EWallPart::EWallPart CBattleInfoCallback::battleHexToWallPart(BattleHex hex) const
{
	RETURN_IF_NOT_BATTLE(EWallPart::INVALID);
	return hexToWallPart(hex);
}

bool CBattleInfoCallback::isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const
{
	RETURN_IF_NOT_BATTLE(false);
	return wallPart != EWallPart::INDESTRUCTIBLE_PART && wallPart != EWallPart::INDESTRUCTIBLE_PART_OF_GATE &&
			wallPart != EWallPart::INVALID;
}

std::vector<BattleHex> CBattleInfoCallback::getAttackableBattleHexes() const
{
	std::vector<BattleHex> attackableBattleHexes;
	RETURN_IF_NOT_BATTLE(attackableBattleHexes);

	for(auto & wallPartPair : wallParts)
	{
		if(isWallPartPotentiallyAttackable(wallPartPair.second))
		{
			auto wallState = static_cast<EWallState::EWallState>(battleGetWallState(static_cast<int>(wallPartPair.second)));
			if(wallState == EWallState::INTACT || wallState == EWallState::DAMAGED)
			{
				attackableBattleHexes.push_back(BattleHex(wallPartPair.first));
			}
		}
	}

	return attackableBattleHexes;
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastThisSpell(const ISpellCaster * caster, const CSpell * spell, ECastingMode::ECastingMode mode) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	if(caster == nullptr)
	{
		logGlobal->errorStream() << "CBattleInfoCallback::battleCanCastThisSpell: no spellcaster.";
		return ESpellCastProblem::INVALID;
	}
	const PlayerColor player = caster->getOwner();
	const ui8 side = playerToSide(player);
	if(!battleDoWeKnowAbout(side))
		return ESpellCastProblem::INVALID;

	ESpellCastProblem::ESpellCastProblem genProblem = battleCanCastSpell(player, mode);
	if(genProblem != ESpellCastProblem::OK)
		return genProblem;

	switch(mode)
	{
	case ECastingMode::HERO_CASTING:
		{
			const CGHeroInstance * castingHero = dynamic_cast<const CGHeroInstance *>(caster);//todo: unify hero|creature spell cost
			assert(castingHero);
			if(!castingHero->canCastThisSpell(spell))
				return ESpellCastProblem::HERO_DOESNT_KNOW_SPELL;
			if(castingHero->mana < battleGetSpellCost(spell, castingHero)) //not enough mana
				return ESpellCastProblem::NOT_ENOUGH_MANA;
		}
		break;
	}

	if(!spell->combatSpell)
		return ESpellCastProblem::ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL;

	const ESpellCastProblem::ESpellCastProblem specificProblem = spell->canBeCast(this, player);

	if(specificProblem != ESpellCastProblem::OK)
		return specificProblem;

	if(spell->isNegative() || spell->hasEffects())
	{
		bool allStacksImmune = true;
		//we are interested only in enemy stacks when casting offensive spells
		//TODO: should hero be able to cast non-smart negative spell if all enemy stacks are immune?
		auto stacks = spell->isNegative() ? battleAliveStacks(!side) : battleAliveStacks();
		for(auto stack : stacks)
		{
			if(ESpellCastProblem::OK == spell->isImmuneByStack(caster, stack))
			{
				allStacksImmune = false;
				break;
			}
		}

		if(allStacksImmune)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}

	if(battleMaxSpellLevel(side) < spell->level) //effect like Recanter's Cloak or Orb of Inhibition
		return ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED;

	//checking if there exists an appropriate target
	switch(spell->getTargetType())
	{
	case CSpell::CREATURE:
		if(mode == ECastingMode::HERO_CASTING)
		{
			const CSpell::TargetInfo ti(spell, caster->getSpellSchoolLevel(spell));
			bool targetExists = false;

			for(const CStack * stack : battleGetAllStacks()) //dead stacks will be immune anyway
			{
				bool immune =  ESpellCastProblem::OK != spell->isImmuneByStack(caster, stack);
				bool casterStack = stack->owner == caster->getOwner();

				if(!immune)
				{
					switch (spell->positiveness)
					{
					case CSpell::POSITIVE:
						if(casterStack || !ti.smart)
						{
							targetExists = true;
							break;
						}
						break;
					case CSpell::NEUTRAL:
							targetExists = true;
							break;
					case CSpell::NEGATIVE:
						if(!casterStack || !ti.smart)
						{
							targetExists = true;
							break;
						}
						break;
					}
				}
			}
			if(!targetExists)
			{
				return ESpellCastProblem::NO_APPROPRIATE_TARGET;
			}
		}
		break;
	case CSpell::OBSTACLE:
		break;
	}

	return ESpellCastProblem::OK;
}

std::vector<BattleHex> CBattleInfoCallback::battleGetPossibleTargets(PlayerColor player, const CSpell *spell) const
{
	std::vector<BattleHex> ret;
	RETURN_IF_NOT_BATTLE(ret);

	switch(spell->getTargetType())
	{
	case CSpell::CREATURE:
		{
			const CGHeroInstance * caster = battleGetFightingHero(playerToSide(player)); //TODO
			const CSpell::TargetInfo ti(spell, caster->getSpellSchoolLevel(spell));

			for(const CStack * stack : battleAliveStacks())
			{
				bool immune = ESpellCastProblem::OK != spell->isImmuneByStack(caster, stack);
				bool casterStack = stack->owner == caster->getOwner();

				if(!immune)
					switch (spell->positiveness)
					{
					case CSpell::POSITIVE:
						if(casterStack || ti.smart)
							ret.push_back(stack->position);
						break;

					case CSpell::NEUTRAL:
						ret.push_back(stack->position);
						break;

					case CSpell::NEGATIVE:
						if(!casterStack || ti.smart)
							ret.push_back(stack->position);
						break;
					}
			}
		}
		break;
	default:
		logGlobal->errorStream() << "FIXME " << __FUNCTION__ << " doesn't work with target type " << spell->getTargetType();
	}

	return ret;
}

ui32 CBattleInfoCallback::battleGetSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	RETURN_IF_NOT_BATTLE(-1);
	//TODO should be replaced using bonus system facilities (propagation onto battle node)

	ui32 ret = caster->getSpellCost(sp);

	//checking for friendly stacks reducing cost of the spell and
	//enemy stacks increasing it
	si32 manaReduction = 0;
	si32 manaIncrease = 0;


	for(auto stack : battleAliveStacks())
	{
		if(stack->owner == caster->tempOwner  &&  stack->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ALLY) )
		{
			vstd::amax(manaReduction, stack->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if( stack->owner != caster->tempOwner  &&  stack->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ENEMY) )
		{
			vstd::amax(manaIncrease, stack->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return ret - manaReduction + manaIncrease;
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastThisSpellHere(const ISpellCaster * caster, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	if(caster == nullptr)
	{
		logGlobal->errorStream() << "CBattleInfoCallback::battleCanCastThisSpellHere: no spellcaster.";
		return ESpellCastProblem::INVALID;
	}
	const PlayerColor player = caster->getOwner();
	ESpellCastProblem::ESpellCastProblem moreGeneralProblem = battleCanCastThisSpell(caster, spell, mode);
	if(moreGeneralProblem != ESpellCastProblem::OK)
		return moreGeneralProblem;

	if(spell->getTargetType() == CSpell::OBSTACLE)
	{
		if(spell->id == SpellID::REMOVE_OBSTACLE)
		{
			if(auto obstacle = battleGetObstacleOnPos(dest, false))
			{
				switch (obstacle->obstacleType)
				{
				case CObstacleInstance::ABSOLUTE_OBSTACLE: //cliff-like obstacles can't be removed
				case CObstacleInstance::MOAT:
					return ESpellCastProblem::NO_APPROPRIATE_TARGET;
				case CObstacleInstance::USUAL:
					return ESpellCastProblem::OK;

// 				//TODO FIRE_WALL only for ADVANCED level casters
// 				case CObstacleInstance::FIRE_WALL:
// 					return
// 				//TODO other magic obstacles for EXPERT
// 				case CObstacleInstance::QUICKSAND:
// 				case CObstacleInstance::LAND_MINE:
// 				case CObstacleInstance::FORCE_FIELD:
// 					return
				default:
//					assert(0);
					return ESpellCastProblem::OK;
				}
			}
		}
		//isObstacleOnTile(dest)
		//
		//
		//TODO
		//assert that it's remove obstacle
		//rules whether we can remove spell-created obstacle
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}


	//get dead stack if we cast resurrection or animate dead
	const CStack *deadStack = getStackIf([dest](const CStack *s) { return !s->alive() && s->coversPos(dest); });
	const CStack *aliveStack = getStackIf([dest](const CStack *s) { return s->alive() && s->coversPos(dest);});


	if(spell->isRisingSpell())
	{
		if(!deadStack && !aliveStack)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(deadStack && deadStack->owner != player) //you can resurrect only your own stacks //FIXME: it includes alive stacks as well
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}
	else if(spell->getTargetType() == CSpell::CREATURE)
	{
		if(!aliveStack)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isNegative() && aliveStack->owner == player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isPositive() && aliveStack->owner != player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}
	return spell->isImmuneAt(this, caster, mode, dest);
}

const CStack * CBattleInfoCallback::getStackIf(std::function<bool(const CStack*)> pred) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	auto stacks = battleGetAllStacks();
	auto stackItr = range::find_if(stacks, pred);
	return stackItr == stacks.end()
		? nullptr
		: *stackItr;
}

bool CBattleInfoCallback::battleIsStackBlocked(const CStack * stack) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	for(const CStack * s :  batteAdjacentCreatures(stack))
	{
		if (s->owner != stack->owner) //blocked by enemy stack
			return true;
	}
	return false;
}

std::set<const CStack*> CBattleInfoCallback:: batteAdjacentCreatures(const CStack * stack) const
{
	std::set<const CStack*> stacks;
	RETURN_IF_NOT_BATTLE(stacks);

	for (BattleHex hex : stack->getSurroundingHexes())
		if(const CStack *neighbour = battleGetStackByPos(hex, true))
			stacks.insert(neighbour);

	return stacks;
}

SpellID CBattleInfoCallback::getRandomBeneficialSpell(const CStack * subject) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);
	//This is complete list. No spells from mods.
	//todo: this should be Spellbook of caster Stack
	static const std::set<SpellID> allPossibleSpells =
	{
		SpellID::AIR_SHIELD,
		SpellID::ANTI_MAGIC,
		SpellID::BLESS,
		SpellID::BLOODLUST,
		SpellID::COUNTERSTRIKE,
		SpellID::CURE,
		SpellID::FIRE_SHIELD,
		SpellID::FORTUNE,
		SpellID::HASTE,
		SpellID::MAGIC_MIRROR,
		SpellID::MIRTH,
		SpellID::PRAYER,
		SpellID::PRECISION,
		SpellID::PROTECTION_FROM_AIR,
		SpellID::PROTECTION_FROM_EARTH,
		SpellID::PROTECTION_FROM_FIRE,
		SpellID::PROTECTION_FROM_WATER,
		SpellID::SHIELD,
		SpellID::SLAYER,
		SpellID::STONE_SKIN
	};
	std::vector<SpellID> beneficialSpells;

	auto getAliveEnemy = [=](const std::function<bool(const CStack * )> & pred)
	{
		return getStackIf([=](const CStack * stack)
		{
			return pred(stack) && stack->owner != subject->owner && stack->alive();
		});
	};

	for(const SpellID spellID : allPossibleSpells)
	{
		if (subject->hasBonusFrom(Bonus::SPELL_EFFECT, spellID)
			//TODO: this ability has special limitations
			|| battleCanCastThisSpellHere(subject, spellID.toSpell(), ECastingMode::CREATURE_ACTIVE_CASTING, subject->position) != ESpellCastProblem::OK)
			continue;

		switch (spellID)
		{
		case SpellID::SHIELD:
		case SpellID::FIRE_SHIELD: // not if all enemy units are shooters
			{
				auto walker = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
				{
					return !stack->shots;
				});

				if (!walker)
					continue;
			}
			break;
		case SpellID::AIR_SHIELD: //only against active shooters
			{
				auto shooter = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
				{
					return stack->hasBonusOfType(Bonus::SHOOTER) && stack->shots;
				});
				if (!shooter)
					continue;
			}
			break;
		case SpellID::ANTI_MAGIC:
		case SpellID::MAGIC_MIRROR:
		case SpellID::PROTECTION_FROM_AIR:
		case SpellID::PROTECTION_FROM_EARTH:
		case SpellID::PROTECTION_FROM_FIRE:
		case SpellID::PROTECTION_FROM_WATER:
			{
				const ui8 enemySide = (ui8)subject->attackerOwned;
				//todo: only if enemy has spellbook
				if (!battleHasHero(enemySide)) //only if there is enemy hero
					continue;
			}
			break;
		case SpellID::CURE: //only damaged units
			{
				//do not cast on affected by debuffs
				if (subject->firstHPleft >= subject->MaxHealth())
					continue;
			}
			break;
		case SpellID::BLOODLUST:
			{
				if (subject->shots) //if can shoot - only if enemy uits are adjacent
					continue;
			}
			break;
		case SpellID::PRECISION:
			{
				if (!(subject->hasBonusOfType(Bonus::SHOOTER) && subject->shots))
					continue;
			}
			break;
		case SpellID::SLAYER://only if monsters are present
			{
				auto kingMonster = getAliveEnemy([&](const CStack *stack) -> bool //look for enemy, non-shooting stack
				{
					const auto isKing = Selector::type(Bonus::KING1)
						.Or(Selector::type(Bonus::KING2))
						.Or(Selector::type(Bonus::KING3));

					return stack->hasBonus(isKing);
				});

				if (!kingMonster)
					continue;
			}
			break;
		}
		beneficialSpells.push_back(spellID);
	}

	if(!beneficialSpells.empty())
	{
		return *RandomGeneratorUtil::nextItem(beneficialSpells, gs->getRandomGenerator());
	}
	else
	{
		return SpellID::NONE;
	}
}

SpellID CBattleInfoCallback::getRandomCastedSpell(const CStack * caster) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);

	TBonusListPtr bl = caster->getBonuses(Selector::type(Bonus::SPELLCASTER));
	if (!bl->size())
		return SpellID::NONE;
	int totalWeight = 0;
	for(Bonus * b : *bl)
	{
		totalWeight += std::max(b->additionalInfo, 1); //minimal chance to cast is 1
	}
	int randomPos = gs->getRandomGenerator().nextInt(totalWeight - 1);
	for(Bonus * b : *bl)
	{
		randomPos -= std::max(b->additionalInfo, 1);
		if(randomPos < 0)
		{
			return SpellID(b->subtype);
		}
	}

	return SpellID::NONE;
}

int CBattleInfoCallback::battleGetSurrenderCost(PlayerColor Player) const
{
	RETURN_IF_NOT_BATTLE(-3);
	if(!battleCanSurrender(Player))
		return -1;

	int ret = 0;
	double discount = 0;
	for(const CStack *s : battleAliveStacks(playerToSide(Player)))
		if(s->base) //we pay for our stack that comes from our army slots - condition eliminates summoned cres and war machines
			ret += s->getCreature()->cost[Res::GOLD] * s->count;

	if(const CGHeroInstance *h = battleGetFightingHero(playerToSide(Player)))
		discount += h->valOfBonuses(Bonus::SURRENDER_DISCOUNT);

	ret *= (100.0 - discount) / 100.0;
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

si8 CBattleInfoCallback::battleMaxSpellLevel(ui8 side) const
{
	const IBonusBearer *node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBattleNode();

	if(!node)
		return GameConstants::SPELL_LEVELS;

	//We can't "just get value" - it'd be 0 if there are bonuses (and all would be blocked)
	auto b = node->getBonuses(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
	if(b->size())
		return b->totalValue();

	return GameConstants::SPELL_LEVELS;
}

boost::optional<int> CBattleInfoCallback::battleIsFinished() const
{
	auto stacks = battleGetAllStacks();

	//checking winning condition
	bool hasStack[2]; //hasStack[0] - true if attacker has a living stack; defender similarly
	hasStack[0] = hasStack[1] = false;

	for(auto & stack : stacks)
	{
		if(stack->alive() && !stack->hasBonusOfType(Bonus::SIEGE_WEAPON))
		{
			hasStack[1-stack->attackerOwned] = true;
		}
	}

	if(!hasStack[0] && !hasStack[1])
		return 2;
	if(!hasStack[1])
		return 0;
	if(!hasStack[0])
		return 1;
	return boost::none;
}

bool AccessibilityInfo::accessible(BattleHex tile, const CStack *stack) const
{
	return accessible(tile, stack->doubleWide(), stack->attackerOwned);
}

bool AccessibilityInfo::accessible(BattleHex tile, bool doubleWide, bool attackerOwned) const
{
	// All hexes that stack would cover if standing on tile have to be accessible.
	for(auto hex : CStack::getHexes(tile, doubleWide, attackerOwned))
	{
		// If the hex is out of range then the tile isn't accessible
		if(!hex.isValid())
			return false;
		// If we're no defender which step on gate and the hex isn't accessible, then the tile
		// isn't accessible
		else if(at(hex) != EAccessibility::ACCESSIBLE &&
			!(at(hex) == EAccessibility::GATE && !attackerOwned))
		{
			return false;
		}
	}
	return true;
}

bool AccessibilityInfo::occupiable(const CStack *stack, BattleHex tile) const
{
	//obviously, we can occupy tile by standing on it
	if(accessible(tile, stack))
		return true;

	if(stack->doubleWide())
	{
		//Check the tile next to -> if stack stands there, it'll also occupy considered hex
		const BattleHex anotherTile = tile + (stack->attackerOwned ? BattleHex::RIGHT : BattleHex::LEFT);
		if(accessible(anotherTile, stack))
			return true;
	}

	return false;
}

ReachabilityInfo::Parameters::Parameters()
{
	stack = nullptr;
	perspective = BattlePerspective::ALL_KNOWING;
	attackerOwned = doubleWide = flying = false;
}

ReachabilityInfo::Parameters::Parameters(const CStack *Stack)
{
	stack = Stack;
	perspective = (BattlePerspective::BattlePerspective)(!Stack->attackerOwned);
	startPosition = Stack->position;
	doubleWide = stack->doubleWide();
	attackerOwned = stack->attackerOwned;
	flying = stack->hasBonusOfType(Bonus::FLYING);
	knownAccessible = stack->getHexes();
}

ESpellCastProblem::ESpellCastProblem CPlayerBattleCallback::battleCanCastThisSpell(const CSpell * spell) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	ASSERT_IF_CALLED_WITH_PLAYER

	const ISpellCaster * hero = battleGetMyHero();
	if(hero == nullptr)
		return ESpellCastProblem::INVALID;
	else
		return CBattleInfoCallback::battleCanCastThisSpell(hero, spell, ECastingMode::HERO_CASTING);
}

bool CPlayerBattleCallback::battleCanFlee() const
{
	RETURN_IF_NOT_BATTLE(false);
	ASSERT_IF_CALLED_WITH_PLAYER
	return CBattleInfoEssentials::battleCanFlee(*player);
}

TStacks CPlayerBattleCallback::battleGetStacks(EStackOwnership whose /*= MINE_AND_ENEMY*/, bool onlyAlive /*= true*/) const
{
	if(whose != MINE_AND_ENEMY)
	{
		ASSERT_IF_CALLED_WITH_PLAYER
	}

	return battleGetStacksIf([=](const CStack * s){
		const bool ownerMatches = (whose == MINE_AND_ENEMY)
			|| (whose == ONLY_MINE && s->owner == player)
			|| (whose == ONLY_ENEMY && s->owner != player);

		return ownerMatches && s->isValidTarget(!onlyAlive);
	});
}

int CPlayerBattleCallback::battleGetSurrenderCost() const
{
	RETURN_IF_NOT_BATTLE(-3)
	ASSERT_IF_CALLED_WITH_PLAYER
	return CBattleInfoCallback::battleGetSurrenderCost(*player);
}

bool CPlayerBattleCallback::battleCanCastSpell(ESpellCastProblem::ESpellCastProblem *outProblem /*= nullptr*/) const
{
	RETURN_IF_NOT_BATTLE(false);
	ASSERT_IF_CALLED_WITH_PLAYER
	auto problem = CBattleInfoCallback::battleCanCastSpell(*player, ECastingMode::HERO_CASTING);
	if(outProblem)
		*outProblem = problem;

	return problem == ESpellCastProblem::OK;
}

const CGHeroInstance * CPlayerBattleCallback::battleGetMyHero() const
{
	return CBattleInfoEssentials::battleGetFightingHero(battleGetMySide());
}

InfoAboutHero CPlayerBattleCallback::battleGetEnemyHero() const
{
	return battleGetHeroInfo(!battleGetMySide());
}

BattleAttackInfo::BattleAttackInfo(const CStack *Attacker, const CStack *Defender, bool Shooting)
{
	attacker = Attacker;
	defender = Defender;

	attackerBonuses = Attacker;
	defenderBonuses = Defender;

	attackerPosition = Attacker->position;
	defenderPosition = Defender->position;

	attackerCount = Attacker->count;
	defenderCount = Defender->count;

	shooting = Shooting;
	chargedFields = 0;

	luckyHit = false;
	unluckyHit = false;
	deathBlow = false;
	ballistaDoubleDamage = false;
}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret = *this;
	std::swap(ret.attacker, ret.defender);
	std::swap(ret.attackerBonuses, ret.defenderBonuses);
	std::swap(ret.attackerPosition, ret.defenderPosition);
	std::swap(ret.attackerCount, ret.defenderCount);

	ret.shooting = false;
	ret.chargedFields = 0;
	ret.luckyHit = ret.ballistaDoubleDamage = ret.deathBlow = false;

	return ret;
}
