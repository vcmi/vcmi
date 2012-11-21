#include "StdInc.h"
#include "CBattleCallback.h"
#include "BattleState.h"
#include "CGameState.h"
#include "NetPacks.h"
#include "CSpellHandler.h"
#include "VCMI_Lib.h"

#define RETURN_IF_NOT_BATTLE(X) if(!duringBattle()) {tlog1 << __FUNCTION__ << " called when no battle!\n"; return X; }

namespace SiegeStuffThatShouldBeMovedToHandlers //  <=== TODO
{
	static void retreiveTurretDamageRange(const CStack *turret, double &outMinDmg, double &outMaxDmg)
	{
		assert(turret->getCreature()->idNumber == 149); //arrow turret

		switch(turret->position)
		{
		case -2: //keep
			outMinDmg = 15;
			outMaxDmg = 15;
			break;
		case -3: case -4: //turrets
			outMinDmg = 7.5;
			outMaxDmg = 7.5;
			break;
		default:
			tlog1 << "Unknown turret type!" << std::endl;
			outMaxDmg = outMinDmg = 1;
		}
	}

	static int lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
	{
		static const int lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

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

	static int getMoatDamage(int townType)
	{
		//TODO move to config file
		static const int dmgs[] = {70, 70, -1,
			90, 70, 90,
			70, 90, 70};

		if(townType >= 0 && townType < ARRAY_COUNT(dmgs))
			return dmgs[townType];

		tlog1 << "No moat info for town " << townType << std::endl;
		return 0;
	}
	static EWallParts::EWallParts hexToWallPart(BattleHex hex)
	{
		//potentially attackable parts of wall
		// -2 - indestructible walls
		static const std::pair<int, EWallParts::EWallParts> attackable[] =
		{
			std::make_pair(50,  EWallParts::KEEP),
			std::make_pair(183, EWallParts::BOTTOM_TOWER),
			std::make_pair(182, EWallParts::BOTTOM_WALL),
			std::make_pair(130, EWallParts::BELOW_GATE),
			std::make_pair(62,  EWallParts::OVER_GATE),
			std::make_pair(29,  EWallParts::UPPER_WAL),
			std::make_pair(12,  EWallParts::UPPER_TOWER),
			std::make_pair(95,  EWallParts::GATE),
			std::make_pair(96,  EWallParts::GATE),
			std::make_pair(45,  EWallParts::INDESTRUCTIBLE_PART),
			std::make_pair(78,  EWallParts::INDESTRUCTIBLE_PART),
			std::make_pair(112, EWallParts::INDESTRUCTIBLE_PART),
			std::make_pair(147, EWallParts::INDESTRUCTIBLE_PART)
		};

		for(int g = 0; g < ARRAY_COUNT(attackable); ++g)
		{
			if(attackable[g].first == hex)
				return attackable[g].second;
		}

		return EWallParts::INVALID; //not found!
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

int CCallbackBase::getPlayerID() const
{
	return player;
}

ui8 CBattleInfoEssentials::battleTerrainType() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->terrainType;
}

int CBattleInfoEssentials::battleGetBattlefieldType() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->battlefieldType;
}

std::vector<shared_ptr<const CObstacleInstance> > CBattleInfoEssentials::battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective /*= boost::none*/) const
{
	std::vector<shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = battleGetMySide();
	}
	else
	{
		if(player >= 0 && *perspective != battleGetMySide())
		{
			tlog1 << "Unauthorized access attempt!\n";
			assert(0); //I want to notice if that happens
			//perspective = battleGetMySide();
		}
	}

	BOOST_FOREACH(auto oi, getBattle()->obstacles)
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

	BOOST_FOREACH(const CStack *s, battleGetAllStacks())
	{
		if(s->attackerOwned == !side  &&  s->getCreature()->isItNativeTerrain(getBattle()->terrainType))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks() const /*returns all stacks, alive or dead or undead or mechanical :) */
{
	TStacks ret;
	RETURN_IF_NOT_BATTLE(ret);
	boost::copy(getBattle()->stacks, std::back_inserter(ret));
	return ret;
}

TStacks CBattleInfoEssentials::battleAliveStacks() const
{
	TStacks ret;
	vstd::copy_if(battleGetAllStacks(), std::back_inserter(ret), [](const CStack *s){ return s->alive(); });
	return ret;
}

TStacks CBattleInfoEssentials::battleAliveStacks(ui8 side) const
{
	TStacks ret;
	vstd::copy_if(battleGetAllStacks(), std::back_inserter(ret), [=](const CStack *s){ return s->alive()  &&  s->attackerOwned == !side; });
	return ret;
}

int CBattleInfoEssentials::battleGetMoatDmg() const
{
	RETURN_IF_NOT_BATTLE(0);

	auto town = getBattle()->town;
	if(!town)
		return 0;

	return getMoatDamage(town->subID);
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	RETURN_IF_NOT_BATTLE(nullptr);


	if(!getBattle() || getBattle()->town == NULL)
		return NULL;

	return getBattle()->town;
}

BattlePerspective::BattlePerspective CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattlePerspective::INVALID);
	if(player < 0)
		return BattlePerspective::ALL_KNOWING;
	if(player == getBattle()->sides[0])
		return BattlePerspective::LEFT_SIDE;
	if(player == getBattle()->sides[1])
		return BattlePerspective::RIGHT_SIDE;

	tlog1 << "Cannot find player " << player << " in battle!\n";
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

	BOOST_FOREACH(auto s, battleGetAllStacks())
		if(s->ID == ID  &&  (!onlyAlive || s->alive()))
			return s;

	return nullptr;
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
		tlog1 << "FIXME: " <<  __FUNCTION__ << " wrong argument!" << std::endl;
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		tlog1 << "FIXME: " <<  __FUNCTION__ << " access check " << std::endl;
		return nullptr;
	}

	return getBattle()->heroes[side];
}

int CBattleInfoEssentials::battleCastSpells(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->castSpells[side];
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastSpell(int player, ECastingMode::ECastingMode mode) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	const ui8 side = playerToSide(player);
	if(!battleDoWeKnowAbout(side))
	{
		tlog3 << "You can't check if enemy can cast given spell!\n";
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

bool CBattleInfoEssentials::battleCanFlee(int player) const
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
		if(!town->hasBuilt(EBuilding::ESCAPE_TUNNEL, ETownType::STRONGHOLD))
			return false;
	}

	return true;
}

ui8 CBattleInfoEssentials::playerToSide(int player) const
{
	RETURN_IF_NOT_BATTLE(-1);
	int ret = vstd::find_pos(getBattle()->sides, player);
	if(ret < 0)
		tlog3 << "Cannot find side for player " << player << std::endl;

	return ret;
}

ui8 CBattleInfoEssentials::battleGetSiegeLevel() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->siege;
}

bool CBattleInfoEssentials::battleCanSurrender(int player) const
{
	RETURN_IF_NOT_BATTLE(false);
	//conditions like for fleeing + enemy must have a hero
	return battleCanFlee(player) && battleHasHero(!playerToSide(player));
}

bool CBattleInfoEssentials::battleHasHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	assert(side < 2);
	return getBattle()->heroes[side];
}

ui8 CBattleInfoEssentials::battleGetWallState(int partOfWall) const
{
	RETURN_IF_NOT_BATTLE(0);
	if(getBattle()->siege == 0)
		return 0;

	assert(partOfWall >= 0 && partOfWall < EWallParts::PARTS_COUNT);
	return getBattle()->si.wallState[partOfWall];
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
		if (battleHexToWallPart(wallPos) != -1) //wall still exists or is indestructible
			return true;
	}

	return false;
}

si8 CBattleInfoCallback::battleCanTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(getAccesibility(stack).accessible(destHex, stack))
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

	BOOST_FOREACH (BattleHex tile, at.hostileCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	BOOST_FOREACH (BattleHex tile, at.friendlyCreaturePositions)
	{
		if(battleGetStackByPos(tile, true)) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

si32 CBattleInfoCallback::battleGetRandomStackSpell(const CStack * stack, ERandomSpell mode) const
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
		tlog1 << "Incorrect mode of battleGetRandomSpell (" << mode <<")\n";
		return -1;
	}
}

const CStack* CBattleInfoCallback::battleGetStackByPos(BattleHex pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	BOOST_FOREACH(auto s, battleGetAllStacks())
		if(vstd::contains(s->getHexes(), pos)  &&  (!onlyAlive || s->alive()))
			return s;

	return nullptr;
}

void CBattleInfoCallback::battleGetStackQueue(std::vector<const CStack *> &out, const int howMany, const int turn /*= 0*/, int lastMoved /*= -1*/) const
{
	RETURN_IF_NOT_BATTLE();

	//let's define a huge lambda
	auto takeStack = [&](std::vector<const CStack *> &st) -> const CStack*
	{
		const CStack *ret = NULL;
		unsigned i, //fastest stack
			j=0; //fastest stack of the other side
		for(i = 0; i < st.size(); i++)
			if(st[i])
				break;

		//no stacks left
		if(i == st.size())
			return nullptr;

		const CStack *fastest = st[i], *other = NULL;
		int bestSpeed = fastest->Speed(turn);

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
			st[i] = NULL;
		else
			st[j] = NULL;

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

	auto allStacks = battleGetAllStacks();
	if(!vstd::contains_if(allStacks, [](const CStack *stack) { return stack->willMove(100000); })) //little evil, but 100000 should be enough for all effects to disappear
	{
		//No stack will be able to move, battle is over.
		out.clear();
		return;
	}

	BOOST_FOREACH(auto s, battleGetAllStacks())
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
		else if(s->getCreature()->idNumber == 145  ||  s->getCreature()->idNumber == 149) //catapult and turrets are first
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

		BOOST_FOREACH(const CStack * otherSt, battleAliveStacks(stack->attackerOwned))
		{
			if(!otherSt->isValidTarget(false))
				continue;

			std::vector<BattleHex> occupied = otherSt->getHexes();

			if(battleCanShoot(stack, otherSt->position))
			{
				attackable->insert(attackable->end(), occupied.begin(), occupied.end());
				continue;
			}

			BOOST_FOREACH(BattleHex he, occupied)
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

	if(stack->getCreature()->idNumber == 145 && dst) //catapult cannot attack creatures
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
												 ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg) const
{
	return calculateDmgRange(attacker, defender, attacker->count, shooting, charge, lucky, deathBlow, ballistaDoubleDmg);
}

TDmgRange CBattleInfoCallback::calculateDmgRange(const BattleAttackInfo &info) const
{
	double additiveBonus = 1.0, multBonus = 1.0,
		minDmg = info.attackerBonuses->getMinDamage() * info.attackerCount,
		maxDmg = info.attackerBonuses->getMaxDamage() * info.attackerCount;

	const CCreature *attackerType = info.attacker->getCreature(), 
		*defenderType = info.defender->getCreature();

	if(attackerType->idNumber == 149) //arrow turret
	{
		SiegeStuffThatShouldBeMovedToHandlers::retreiveTurretDamageRange(info.attacker, minDmg, maxDmg);
	}

	if(info.attackerBonuses->hasBonusOfType(Bonus::SIEGE_WEAPON) && attackerType->idNumber != 149) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		auto retreiveHeroPrimSkill = [&](int skill) -> int
		{
			const Bonus *b = info.attackerBonuses->getBonus(Selector::sourceTypeSel(Bonus::HERO_BASE_SKILL) &&  Selector::typeSubtype(Bonus::PRIMARY_SKILL, skill));
			return b ? b->val : 0; //if there is no hero or no info on his primary skill, return 0
		};


		minDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
	}

	int attackDefenceDifference = 0;
	if(info.attackerBonuses->hasBonusOfType(Bonus::GENERAL_ATTACK_REDUCTION))
	{
		double multAttackReduction = info.attackerBonuses->valOfBonuses(Bonus::GENERAL_ATTACK_REDUCTION, -1024) / 100.0;
		attackDefenceDifference = info.attackerBonuses->Attack() * multAttackReduction;
	}
	else
	{
		attackDefenceDifference = info.attackerBonuses->Attack();
	}

	if(info.attackerBonuses->hasBonusOfType(Bonus::ENEMY_DEFENCE_REDUCTION))
	{
		double multDefenceReduction = (100 - info.attackerBonuses->valOfBonuses(Bonus::ENEMY_DEFENCE_REDUCTION, -1024)) / 100.0;
		attackDefenceDifference -= info.defenderBonuses->Defense() * multDefenceReduction;
	}
	else
	{
		attackDefenceDifference -= info.defenderBonuses->Defense();
	}

	//calculating total attack/defense skills modifier

	if(info.shooting) //precision handling (etc.)
		attackDefenceDifference += info.attackerBonuses->getBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT))->totalValue();
	else //bloodlust handling (etc.)
		attackDefenceDifference += info.attackerBonuses->getBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))->totalValue();


	if(const Bonus *slayerEffect = info.attackerBonuses->getEffect(Spells::SLAYER)) //slayer handling
	{
		std::vector<int> affectedIds;
		int spLevel = slayerEffect->val;

		for(int g = 0; g < VLC->creh->creatures.size(); ++g)
		{
			BOOST_FOREACH(const Bonus *b, VLC->creh->creatures[g]->getBonusList())
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

		for(ui32 g=0; g<affectedIds.size(); ++g)
		{
			if(defenderType->idNumber == affectedIds[g])
			{
				attackDefenceDifference += VLC->spellh->spells[Spells::SLAYER]->powers[spLevel];
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
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::ARCHERY) / 100.0;
	else
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::OFFENCE) / 100.0;

	if(info.defenderBonuses)
		multBonus *= (std::max(0, 100 - info.defenderBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::ARMORER))) / 100.0;

	//handling hate effect
	additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::HATE, defenderType->idNumber) / 100.;

	//luck bonus
	if (info.luckyHit)
	{
		additiveBonus += 1.0;
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
	if(!info.shooting && info.defenderBonuses->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) //eg. shield
	{
		multBonus *= info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 0) / 100.0;
	}
	else if(info.shooting && info.defenderBonuses->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) //eg. air shield
	{
		multBonus *= info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 1) / 100.0;
	}

	TBonusListPtr curseEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MINIMUM_DAMAGE)); //attacker->getEffect(42);
	TBonusListPtr blessEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MAXIMUM_DAMAGE)); //attacker->getEffect(43);
	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();
	double curseMultiplicativePenalty = curseEffects->size() ? (*std::max_element(curseEffects->begin(), curseEffects->end(), &Bonus::compareByAdditionalInfo))->additionalInfo : 0;

	if(curseMultiplicativePenalty) //curse handling (partial, the rest is below)
	{
		multBonus *= 1.0 - curseMultiplicativePenalty/100;
	}

	auto isAdvancedAirShield = [](const Bonus *bonus)
	{
		return bonus->source == Bonus::SPELL_EFFECT
			&& bonus->sid == Spells::AIR_SHIELD
			&& bonus->val >= SecSkillLevel::ADVANCED;
	};

	//wall / distance penalty + advanced air shield
	const bool distPenalty = !info.attackerBonuses->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY) && battleHasDistancePenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);
	const bool obstaclePenalty = battleHasWallPenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);

	if (info.shooting)
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
	bool shooting, ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg ) const
{
	BattleAttackInfo bai(attacker, defender, shooting);
	bai.attackerCount = attackerCount;
	bai.chargedFields = charge;
	bai.luckyHit = lucky;
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

std::pair<ui32, ui32> CBattleInfoCallback::battleEstimateDamage(const BattleAttackInfo &bai, std::pair<ui32, ui32> * retaliationDmg /*= NULL*/) const
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
				bai.defender->prepareAttacked(bsa);

				auto retaliationAttack = bai.reverse();
				retaliationAttack.attackerCount = bsa.newAmount;
				retaliationDmg->*pairElems[!i] = calculateDmgRange(retaliationAttack).*pairElems[!i];
			}
		}
	}

	return ret;
}

shared_ptr<const CObstacleInstance> CBattleInfoCallback::battleGetObstacleOnPos(BattleHex tile, bool onlyBlocking /*= true*/) const
{
	RETURN_IF_NOT_BATTLE(shared_ptr<const CObstacleInstance>());

	BOOST_FOREACH(auto &obs, battleGetAllObstacles())
	{
		if(vstd::contains(obs->getBlockedTiles(), tile)
			|| (!onlyBlocking  &&  vstd::contains(obs->getAffectedTiles(), tile)))
		{
			return obs;
		}
	}

	return shared_ptr<const CObstacleInstance>();
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
	if(battleGetSiegeLevel() > 0 && battleGetWallState(EWallParts::GATE) < 3) //gate is not destroyed
	{
		ret[95] = ret[96] = EAccessibility::GATE; //block gate's hexes
	}

	//tiles occupied by standing stacks
	BOOST_FOREACH(auto stack, battleAliveStacks())
	{
		BOOST_FOREACH(auto hex, stack->getHexes())
			if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
				ret[hex] = EAccessibility::ALIVE_STACK;
	}

	//obstacles
	BOOST_FOREACH(const auto &obst, battleGetAllObstacles())
	{
		BOOST_FOREACH(auto hex, obst->getBlockedTiles())
			ret[hex] = EAccessibility::OBSTACLE;
	}

	//walls
	if(battleGetSiegeLevel() > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 78, 112, 147, 165};
		BOOST_FOREACH(auto hex, permanentlyLocked)
			ret[hex] = EAccessibility::UNAVAILABLE;

		//TODO likely duplicated logic
		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] = //(which part of wall, which hex is blocked if this part of wall is not destroyed
			{std::make_pair(2, BattleHex(182)), std::make_pair(3, BattleHex(130)),
			std::make_pair(4, BattleHex(62)), std::make_pair(5, BattleHex(29))};

		for(int b=0; b<ARRAY_COUNT(lockedIfNotDestroyed); ++b)
		{
			if(battleGetWallState(lockedIfNotDestroyed[b].first) < 3)
				ret[lockedIfNotDestroyed[b].second] = EAccessibility::DESTRUCTIBLE_WALL;
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
	BOOST_FOREACH(auto hex, accessibleHexes)
		if(hex.isValid())
			ret[hex] = EAccessibility::ACCESSIBLE;

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const AccessibilityInfo &accessibility, const ReachabilityInfo::Parameters params) const
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
		BOOST_FOREACH(BattleHex neighbour, curHex.neighbouringTiles())
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

	BOOST_FOREACH(auto &oi, battleGetAllObstacles(whichSidePerspective))
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

	// I hate std::pairs with their undescriptive member names first / second
	struct DistStack
	{
		int distanceToPred;
		const CStack *stack;
	};

	std::vector<DistStack> stackPairs; //pairs <<distance, hex>, stack>
	for(int g=0; g<GameConstants::BFIELD_SIZE; ++g)
	{
		const CStack * atG = battleGetStackByPos(g);
		if(!atG || atG->ID == closest->ID) //if there is not stack or we are the closest one
			continue;

		if(boost::logic::indeterminate(attackerOwned) || atG->attackerOwned == attackerOwned)
		{
			if(reachability.isReachable(g))
				continue;

			DistStack hlp = {reachability.distances[reachability.predecessors[g]], atG};
			stackPairs.push_back(hlp);
		}
	}

	if(stackPairs.size() > 0)
	{
		auto comparator = [](DistStack lhs, DistStack rhs) { return lhs.distanceToPred < rhs.distanceToPred; };
		auto minimal = boost::min_element(stackPairs, comparator);
		return std::make_pair(minimal->stack, reachability.predecessors[minimal->stack->position]);
	}

	return std::make_pair<const CStack * , BattleHex>(NULL, BattleHex::INVALID);
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
		//tlog3 << "Falling back to our perspective for reachability lookup for " << stack->nodeName() << std::endl;
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

ReachabilityInfo CBattleInfoCallback::getFlyingReachability(const ReachabilityInfo::Parameters params) const
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

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	AttackableTiles at;
	RETURN_IF_NOT_BATTLE(at);

	const int WN = GameConstants::BFIELD_WIDTH;
	ui16 hex = (attackerPos != BattleHex::INVALID) ? attackerPos.hex : attacker->position.hex; //real or hypothetical (cursor) position
	//FIXME: dragons or cerbers can rotate before attack, making their base hex different (#1124)
	if (attacker->hasBonusOfType(Bonus::ATTACKS_ALL_ADJACENT))
	{
		boost::copy(attacker->getSurroundingHexes(attackerPos), vstd::set_inserter(at.hostileCreaturePositions));
// 		BOOST_FOREACH (BattleHex tile, attacker->getSurroundingHexes(attackerPos))
// 			at.hostileCreaturePositions.insert(tile);
	}
	if (attacker->hasBonusOfType(Bonus::THREE_HEADED_ATTACK))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		BOOST_FOREACH (BattleHex tile, hexes)
		{
			if ((BattleHex::mutualPosition(tile, destinationTile) > -1 && BattleHex::mutualPosition(tile, hex) > -1) //adjacent both to attacker's head and attacked tile
				|| tile == destinationTile) //or simply attacked directly
			{
				const CStack * st = battleGetStackByPos(tile, true);
				if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
				{
					at.hostileCreaturePositions.insert(tile);
				}
			}
		}
	}
	if (attacker->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH))
	{
		std::vector<BattleHex> hexes; //only one, in fact
		int pseudoVector = destinationTile.hex - hex;
		switch (pseudoVector)
		{
		case 1:
		case -1:
			BattleHex::checkAndPush(destinationTile.hex + pseudoVector, hexes);
			break;
		case WN: //17
		case WN + 1: //18
		case -WN: //-17
		case -WN + 1: //-16
			BattleHex::checkAndPush(destinationTile.hex + pseudoVector + ((hex/WN)%2 ? 1 : -1 ), hexes);
			break;
		case WN-1: //16
		case -WN-1: //-18
			BattleHex::checkAndPush(destinationTile.hex + pseudoVector + ((hex/WN)%2 ? 1 : 0), hexes);
			break;
		}
		BOOST_FOREACH (BattleHex tile, hexes)
		{
			//friendly stacks can also be damaged by Dragon Breath
			if(battleGetStackByPos(tile, true))
				at.friendlyCreaturePositions.insert(tile);
		}
	}

	return at;
}

std::set<const CStack*> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos /*= BattleHex::INVALID*/) const
{
	std::set<const CStack*> attackedCres;
	RETURN_IF_NOT_BATTLE(attackedCres);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);
	BOOST_FOREACH (BattleHex tile, at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.insert(st);
		}
	}
	BOOST_FOREACH (BattleHex tile, at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.insert(st);
		}
	}
	return attackedCres;
}

ReachabilityInfo::TDistances CBattleInfoCallback::battleGetDistances(const CStack * stack, BattleHex hex /*= BattleHex::INVALID*/, BattleHex * predecessors /*= NULL*/) const
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

	if(BattleHex::getDistance(shooterPosition, destHex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
		return false;

	if(const CStack * dstStack = battleGetStackByPos(destHex, false))
	{
		//If on dest hex stands stack that occupies a hex within our distance
		BOOST_FOREACH(auto hex, dstStack->getHexes())
			if(BattleHex::getDistance(shooterPosition, hex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
				return false;

		//TODO what about two-hex shooters?
	}

	return true;
}

EWallParts::EWallParts CBattleInfoCallback::battleHexToWallPart(BattleHex hex) const
{
	RETURN_IF_NOT_BATTLE(EWallParts::INVALID);
	return hexToWallPart(hex);
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleIsImmune(const CGHeroInstance * caster, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);


	// Get stack at destination hex -> subject of our spell.
	const CStack * subject = battleGetStackByPos(dest, !spell->isRisingSpell()); //only alive if not rising spell

	if(subject)
	{
		if (spell->isPositive() && subject->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
			return ESpellCastProblem::OK;

		switch (spell->id) //TODO: more general logic for new spells?
		{
		case Spells::DESTROY_UNDEAD:
			if (!subject->hasBonusOfType(Bonus::UNDEAD))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
			break;
		case Spells::DEATH_RIPPLE:
			if (subject->hasBonusOfType(Bonus::SIEGE_WEAPON))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //don't break here - undeads and war machines are immune, non-living are not
		case Spells::BLESS:
		case Spells::CURSE: //undeads are immune to bless & curse
			if (subject->hasBonusOfType(Bonus::UNDEAD))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
			break;
		case Spells::HASTE:
		case Spells::SLOW:
		case Spells::TELEPORT:
		case Spells::CLONE:
			if (subject->hasBonusOfType(Bonus::SIEGE_WEAPON))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //war machines are immune to some spells than involve movement
			if (spell->id == Spells::CLONE && caster) //TODO: how about stacks casting Clone?
			{
				if (vstd::contains(subject->state, EBattleStackState::CLONED))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //can't clone already cloned creature
				int maxLevel = (std::max(caster->getSpellSchoolLevel(spell), (ui8)1) + 4);
				int creLevel = subject->getCreature()->level;
				if (maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, 1-7 for expert
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
			}
			break;
		case Spells::FORGETFULNESS:
			if (!subject->hasBonusOfType(Bonus::SHOOTER))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
			break;
		case Spells::DISPEL_HELPFUL_SPELLS:
			{
				TBonusListPtr spellBon = subject->getSpellBonuses();
				bool hasPositiveSpell = false;
				BOOST_FOREACH(const Bonus * b, *spellBon)
				{
					if(VLC->spellh->spells[b->sid]->isPositive())
					{
						hasPositiveSpell = true;
						break;
					}
				}
				if(!hasPositiveSpell)
				{
					return ESpellCastProblem::NO_SPELLS_TO_DISPEL;
				}
			}
			break;
		}

		const bool damageSpell = spell->isDamageSpell();
		auto battleTestElementalImmunity = [&](Bonus::BonusType element) -> bool //helper for battleisImmune
		{
			if (!spell->isPositive()) //negative or indifferent
			{
				if ((damageSpell && subject->hasBonusOfType(element, 2)) || subject->hasBonusOfType(element, 1))
					return true;
			}
			else if (spell->isPositive()) //positive
			{
				if (subject->hasBonusOfType(element, 0)) //must be immune to all spells
					return true;
			}
			return false;
		};


		if (damageSpell && subject->hasBonusOfType(Bonus::DIRECT_DAMAGE_IMMUNITY))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;

		if (spell->fire)
		{
			if (battleTestElementalImmunity(Bonus::FIRE_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->water)
		{
			if (battleTestElementalImmunity(Bonus::WATER_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->earth)
		{
			if (battleTestElementalImmunity(Bonus::EARTH_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->air)
		{
			if (battleTestElementalImmunity(Bonus::AIR_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (vstd::contains(VLC->spellh->mindSpells, spell->id))
		{
			if (subject->hasBonusOfType(Bonus::MIND_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}

		if (spell->isRisingSpell())
		{
			if (subject->count >= subject->baseAmount) //TODO: calculate potential hp raised
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}

		TBonusListPtr immunities = subject->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));
		if(subject->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES))
		{
			immunities->remove_if([](const Bonus* b){  return b->source == Bonus::CREATURE_ABILITY;  });
		}

		if(subject->hasBonusOfType(Bonus::SPELL_IMMUNITY, spell->id)
			|| ( immunities->size() > 0  &&  immunities->totalValue() >= spell->level  &&  spell->level))
		{
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
	}
	else //no target stack on this tile
	{
		if(spell->getTargetType() == CSpell::CREATURE
			|| (spell->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE
				&& mode == ECastingMode::HERO_CASTING //TODO why???
				&& caster
				&& caster->getSpellSchoolLevel(spell) < SecSkillLevel::EXPERT))
		{
			return ESpellCastProblem::WRONG_SPELL_TARGET;
		}
	}

	return ESpellCastProblem::OK;
}

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastThisSpell( int player, const CSpell * spell, ECastingMode::ECastingMode mode ) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	const ui8 side = playerToSide(player);
	if(!battleDoWeKnowAbout(side))
		return ESpellCastProblem::INVALID;

	ESpellCastProblem::ESpellCastProblem genProblem = battleCanCastSpell(player, mode);
	if(genProblem != ESpellCastProblem::OK)
		return genProblem;

	//Casting hero, set only if he is an actual caster.
	const CGHeroInstance *castingHero = mode == ECastingMode::HERO_CASTING
										? battleGetFightingHero(side)
										: nullptr;


	switch(mode)
	{
	case ECastingMode::HERO_CASTING:
		{
			assert(castingHero);
			if(!castingHero->canCastThisSpell(spell))
				return ESpellCastProblem::HERO_DOESNT_KNOW_SPELL;
			if(castingHero->mana < battleGetSpellCost(spell, castingHero)) //not enough mana
				return ESpellCastProblem::NOT_ENOUGH_MANA;
		}
		break;
	}


	if(spell->id < 10) //it's adventure spell (not combat))
		return ESpellCastProblem::ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL;

	//TODO?
	//if(NBonus::hasOfType(heroes[1-cside], Bonus::SPELL_IMMUNITY, spell->id)) //non - casting hero provides immunity for this spell
	//	return ESpellCastProblem::SECOND_HEROS_SPELL_IMMUNITY;
	if(spell->isNegative())
	{
		bool allEnemiesImmune = true;
		BOOST_FOREACH(auto enemyStack, battleAliveStacks(!side))
		{
			if(!enemyStack->hasBonusOfType(Bonus::SPELL_IMMUNITY, spell->id))
			{
				allEnemiesImmune = false;
				break;
			}
		}

		if(allEnemiesImmune)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}


	if(battleMaxSpellLevel() < spell->level) //effect like Recanter's Cloak or Orb of Inhibition
		return ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED;

	//IDs of summon elemental spells (fire, earth, water, air)
	int spellIDs[] = {	Spells::SUMMON_FIRE_ELEMENTAL, Spells::SUMMON_EARTH_ELEMENTAL,
						Spells::SUMMON_WATER_ELEMENTAL, Spells::SUMMON_AIR_ELEMENTAL };
	//(fire, earth, water, air) elementals
	int creIDs[] = {114, 113, 115, 112};

	int arpos = vstd::find_pos(spellIDs, spell->id);
	if(arpos < ARRAY_COUNT(spellIDs))
	{
		//check if there are summoned elementals of other type
		BOOST_FOREACH( const CStack * st, battleAliveStacks())
			if(vstd::contains(st->state, EBattleStackState::SUMMONED) && st->getCreature()->idNumber == creIDs[arpos])
				return ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED;
	}

	//checking if there exists an appropriate target
	switch(spell->getTargetType())
	{
	case CSpell::CREATURE:
	case CSpell::CREATURE_EXPERT_MASSIVE:  
		if(mode == ECastingMode::HERO_CASTING)
		{
			const CGHeroInstance * caster = battleGetFightingHero(side);
			bool targetExists = false;
			BOOST_FOREACH(const CStack * stack, battleAliveStacks())
			{
				switch (spell->positiveness)
				{
				case CSpell::POSITIVE:
					if(stack->owner == caster->getOwner())
					{
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
						{
							targetExists = true;
							break;
						}
					}
					break;
				case CSpell::NEUTRAL:
					if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
					{
						targetExists = true;
						break;
					}
					break;
				case CSpell::NEGATIVE:
					if(stack->owner != caster->getOwner())
					{
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
						{
							targetExists = true;
							break;
						}
					}
					break;
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

std::vector<BattleHex> CBattleInfoCallback::battleGetPossibleTargets(int player, const CSpell *spell) const
{
	std::vector<BattleHex> ret;
	RETURN_IF_NOT_BATTLE(ret);

	auto mode = ECastingMode::HERO_CASTING; //TODO get rid of this!

	switch(spell->getTargetType())
	{
	case CSpell::CREATURE:
	case CSpell::CREATURE_EXPERT_MASSIVE:
		{
			const CGHeroInstance * caster = battleGetFightingHero(playerToSide(player)); //TODO

			BOOST_FOREACH(const CStack * stack, battleAliveStacks())
			{
				switch (spell->positiveness)
				{
				case CSpell::POSITIVE:
					if(stack->owner == caster->getOwner())
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
							ret.push_back(stack->position);
					break;

				case CSpell::NEUTRAL:
					if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
						ret.push_back(stack->position);
					break;

				case CSpell::NEGATIVE:
					if(stack->owner != caster->getOwner())
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
							ret.push_back(stack->position);
					break;
				}
			}
		}
		break;
	default:
		tlog1 << "FIXME " << __FUNCTION__ << " doesn't work with target type " << spell->getTargetType() << std::endl;
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


	BOOST_FOREACH(auto stack, battleAliveStacks())
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

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastThisSpellHere( int player, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest ) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	ESpellCastProblem::ESpellCastProblem moreGeneralProblem = battleCanCastThisSpell(player, spell, mode);
	if(moreGeneralProblem != ESpellCastProblem::OK)
		return moreGeneralProblem;

	if(spell->getTargetType() == CSpell::OBSTACLE)
	{
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
		if(spell->id == Spells::ANIMATE_DEAD  &&  deadStack  &&  !deadStack->hasBonusOfType(Bonus::UNDEAD))
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(deadStack && deadStack->owner != player) //you can resurrect only your own stacks //FIXME: it includes alive stacks as well
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}
	else if(spell->getTargetType() == CSpell::CREATURE  ||  spell->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(!aliveStack)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isNegative() && aliveStack->owner == player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isPositive() && aliveStack->owner != player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}


	if (mode == ECastingMode::HERO_CASTING)
		return battleIsImmune(battleGetFightingHero(playerToSide(player)), spell, mode, dest);
	else
		return battleIsImmune(NULL, spell, mode, dest);
}

ui32 CBattleInfoCallback::calculateSpellBonus(ui32 baseDamage, const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature) const
{
	ui32 ret = baseDamage;
	//applying sorcery secondary skill
	if(caster)
	{
		ret *= (100.0 + caster->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::SORCERY)) / 100.0;
		ret *= (100.0 + caster->valOfBonuses(Bonus::SPELL_DAMAGE) + caster->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, sp->id)) / 100.0;

		if(sp->air)
			ret *= (100.0 + caster->valOfBonuses(Bonus::AIR_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->fire) //only one type of bonus for Magic Arrow
			ret *= (100.0 + caster->valOfBonuses(Bonus::FIRE_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->water)
			ret *= (100.0 + caster->valOfBonuses(Bonus::WATER_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->earth)
			ret *= (100.0 + caster->valOfBonuses(Bonus::EARTH_SPELL_DMG_PREMY)) / 100.0;

		if (affectedCreature && affectedCreature->getCreature()->level) //Hero specials like Solmyr, Deemer
			ret *= (100. + ((caster->valOfBonuses(Bonus::SPECIAL_SPELL_LEV, sp->id) * caster->level) / affectedCreature->getCreature()->level)) / 100.0;
	}
	return ret;
}

ui32 CBattleInfoCallback::calculateSpellDmg( const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower ) const
{
	ui32 ret = 0; //value to return

	//check if spell really does damage - if not, return 0
	if(VLC->spellh->damageSpells.find(sp->id) == VLC->spellh->damageSpells.end())
		return 0;

	ret = usedSpellPower * sp->power;
	ret += sp->powers[spellSchoolLevel];

	//affected creature-specific part
	if(affectedCreature)
	{
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 0)) //air spell & protection from air
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 0);
			ret /= 100;
		}
		else if(sp->fire && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 1)) //fire spell & protection from fire
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 1);
			ret /= 100;
		}
		else if(sp->water && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 2)) //water spell & protection from water
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 2);
			ret /= 100;
		}
		else if (sp->earth && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 3)) //earth spell & protection from earth
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 3);
			ret /= 100;
		}
		//general spell dmg reduction
		//FIXME?
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, -1))
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, -1);
			ret /= 100;
		}
		//dmg increasing
		if( affectedCreature->hasBonusOfType(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id) )
		{
			ret *= 100 + affectedCreature->valOfBonuses(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id);
			ret /= 100;
		}
	}
	ret = calculateSpellBonus(ret, sp, caster, affectedCreature);
	return ret;
}

std::set<const CStack*> CBattleInfoCallback::getAffectedCreatures(const CSpell * spell, int skillLevel, ui8 attackerOwner, BattleHex destinationTile)
{
	std::set<const CStack*> attackedCres; /*std::set to exclude multiple occurrences of two hex creatures*/

	const ui8 attackerSide = playerToSide(attackerOwner) == 1;
	const auto attackedHexes = spell->rangeInHexes(destinationTile, skillLevel, attackerSide);
	const bool onlyAlive = spell->id != Spells::RESURRECTION && spell->id != Spells::ANIMATE_DEAD; //when casting resurrection or animate dead we should be allow to select dead stack
	//fixme: what about other rising spells (Sacrifice) ?
	if(spell->id == Spells::DEATH_RIPPLE || spell->id == Spells::DESTROY_UNDEAD || spell->id == Spells::ARMAGEDDON)
	{
		BOOST_FOREACH(const CStack *stack, battleGetAllStacks())
		{
			if((spell->id == Spells::DEATH_RIPPLE && !stack->getCreature()->isUndead()) //death ripple
				|| (spell->id == Spells::DESTROY_UNDEAD && stack->getCreature()->isUndead()) //destroy undead
				|| (spell->id == Spells::ARMAGEDDON) //Armageddon
				)
			{
				if(stack->isValidTarget())
					attackedCres.insert(stack);
			}
		}
	}
	else if (spell->id == Spells::CHAIN_LIGHTNING)
	{
		std::set<BattleHex> possibleHexes;
		BOOST_FOREACH (auto stack, battleGetAllStacks())
		{
			if (stack->isValidTarget())
			{
				BOOST_FOREACH (auto hex, stack->getHexes())
				{
					possibleHexes.insert (hex);
				}
			}
		}
		BattleHex lightningHex =  destinationTile;
		for (int i = 0; i < 5; ++i) //TODO: depends on spell school level
		{
			auto stack = battleGetStackByPos (lightningHex, true);
			if (!stack)
				break;
			attackedCres.insert (stack);
			BOOST_FOREACH (auto hex, stack->getHexes())
			{
				possibleHexes.erase (hex); //can't hit same place twice
			}
			if (!possibleHexes.size()) //not enough targets
				break;
			lightningHex = BattleHex::getClosestTile (attackerOwner, destinationTile, possibleHexes);
		}
	}
	else if (spell->range[skillLevel].size() > 1) //custom many-hex range
	{
		BOOST_FOREACH(BattleHex hex, attackedHexes)
		{
			if(const CStack * st = battleGetStackByPos(hex, onlyAlive))
			{
				if (spell->id == 76) //Death Cloud //TODO: fireball and fire immunity
				{
					if (st->isLiving() || st->coversPos(destinationTile)) //directly hit or alive
					{
						attackedCres.insert(st);
					}
				}
				else
					attackedCres.insert(st);
			}
		}
	}
	else if(spell->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(skillLevel < 3)  /*not expert */
		{
			const CStack * st = battleGetStackByPos(destinationTile, onlyAlive);
			if(st)
				attackedCres.insert(st);
		}
		else
		{
			BOOST_FOREACH (auto stack, battleGetAllStacks())
			{
				/*if it's non negative spell and our unit or non positive spell and hostile unit */
				if((!spell->isNegative() && stack->owner == attackerOwner)
					||(!spell->isPositive() && stack->owner != attackerOwner )
					)
				{
					if(stack->isValidTarget(!onlyAlive))
						attackedCres.insert(stack);
				}
			}
		} //if(caster->getSpellSchoolLevel(s) < 3)
	}
	else if(spell->getTargetType() == CSpell::CREATURE)
	{
		if(const CStack * st = battleGetStackByPos(destinationTile, onlyAlive))
			attackedCres.insert(st);
	}
	else //custom range from attackedHexes
	{
		BOOST_FOREACH(BattleHex hex, attackedHexes)
		{
			if(const CStack * st = battleGetStackByPos(hex, onlyAlive))
				attackedCres.insert(st);
		}
	}
	return attackedCres;
}

const CStack * CBattleInfoCallback::getStackIf(boost::function<bool(const CStack*)> pred) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	auto stacks = battleGetAllStacks();
	auto stackItr = range::find_if(stacks, pred);
	return stackItr == stacks.end()
		? NULL
		: *stackItr;
}

bool CBattleInfoCallback::battleIsStackBlocked(const CStack * stack) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	BOOST_FOREACH(const CStack * s,  batteAdjacentCreatures(stack))
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

	BOOST_FOREACH (BattleHex hex, stack->getSurroundingHexes())
		if(const CStack *neighbour = battleGetStackByPos(hex, true))
			stacks.insert(neighbour);

	return stacks;
}

TSpell CBattleInfoCallback::getRandomBeneficialSpell(const CStack * subject) const
{
	RETURN_IF_NOT_BATTLE(-1);
	std::vector<TSpell> possibleSpells;

	BOOST_FOREACH(const CSpell *spell, VLC->spellh->spells)
	{
		if (spell->isPositive()) //only positive
		{
			if (subject->hasBonusFrom(Bonus::SPELL_EFFECT, spell->id)
				|| battleCanCastThisSpellHere(subject->owner, spell, ECastingMode::CREATURE_ACTIVE_CASTING, subject->position) != ESpellCastProblem::OK)
				continue;

			switch (spell->id)
			{
			case Spells::SHIELD:
			case Spells::FIRE_SHIELD: // not if all enemy units are shooters
				{
					auto walker = getStackIf([&](const CStack *stack) //look for enemy, non-shooting stack
					{
						return stack->owner != subject->owner && !stack->shots;
					});

					if (!walker)
						continue;
				}
				break;
			case Spells::AIR_SHIELD: //only against active shooters
				{

					auto shooter = getStackIf([&](const CStack *stack) //look for enemy, non-shooting stack
					{
						return stack->owner != subject->owner && stack->hasBonusOfType(Bonus::SHOOTER) && stack->shots;
					});
					if (!shooter)
						continue;
				}
				break;
			case Spells::ANTI_MAGIC:
			case Spells::MAGIC_MIRROR:
				{
					if (!battleHasHero(subject->attackerOwned)) //only if there is enemy hero
						continue;
				}
				break;
			case Spells::CURE: //only damaged units - what about affected by curse?
				{
					if (subject->firstHPleft >= subject->MaxHealth())
						continue;
				}
				break;
			case Spells::BLOODLUST:
				{
					if (subject->shots) //if can shoot - only if enemy uits are adjacent
						continue;
				}
				break;
			case Spells::PRECISION:
				{
					if (!(subject->hasBonusOfType(Bonus::SHOOTER) && subject->shots))
						continue;
				}
				break;
			case Spells::SLAYER://only if monsters are present
				{
					auto kingMonster = getStackIf([&](const CStack *stack) //look for enemy, non-shooting stack
					{
						return stack->owner != subject->owner
							&& (stack->hasBonus(Selector::type(Bonus::KING1) || Selector::type(Bonus::KING2) || Selector::type(Bonus::KING3)));
					});

					if (!kingMonster)
						continue;
				}
				break;
			case Spells::CLONE: //not allowed
				continue;
				break;
			}
			possibleSpells.push_back(spell->id);
		}
	}

	if (possibleSpells.size())
		return possibleSpells[rand() % possibleSpells.size()];
	else
		return -1;
}

TSpell CBattleInfoCallback::getRandomCastedSpell(const CStack * caster) const
{
	RETURN_IF_NOT_BATTLE(-1);

	TBonusListPtr bl = caster->getBonuses(Selector::type(Bonus::SPELLCASTER));
	if (!bl->size())
		return -1;
	int totalWeight = 0;
	BOOST_FOREACH(Bonus * b, *bl)
	{
		totalWeight += std::max(b->additionalInfo, 1); //minimal chance to cast is 1
	}
	int randomPos = rand() % totalWeight;
	BOOST_FOREACH(Bonus * b, *bl)
	{
		randomPos -= std::max(b->additionalInfo, 1);
		if(randomPos < 0)
		{
			return b->subtype;
		}
	}

	return -1;
}

int CBattleInfoCallback::battleGetSurrenderCost(int Player) const
{
	RETURN_IF_NOT_BATTLE(-3);
	if(!battleCanSurrender(Player))
		return -1;

	int ret = 0;
	double discount = 0;
	BOOST_FOREACH(const CStack *s, battleAliveStacks(playerToSide(Player)))
		if(s->base) //we pay for our stack that comes from our army slots - condition eliminates summoned cres and war machines
			ret += s->getCreature()->cost[Res::GOLD] * s->count;

	if(const CGHeroInstance *h = battleGetFightingHero(playerToSide(Player)))
		discount += h->valOfBonuses(Bonus::SURRENDER_DISCOUNT);

	ret *= (100.0 - discount) / 100.0;
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

si8 CBattleInfoCallback::battleMaxSpellLevel() const
{
	const CBonusSystemNode *node = NULL;
	if(const CGHeroInstance *h =  battleGetFightingHero(battleGetMySide()))
		node = h;
	//TODO else use battle node
	if(!node)
		return GameConstants::SPELL_LEVELS;

	//We can't "just get value" - it'd be 0 if there are bonuses (and all would be blocked)
	auto b = node->getBonuses(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
	if(b->size())
		return b->totalValue();

	return GameConstants::SPELL_LEVELS;
}

bool AccessibilityInfo::accessible(BattleHex tile, const CStack *stack) const
{
	return accessible(tile, stack->doubleWide(), stack->attackerOwned);
}

bool AccessibilityInfo::accessible(BattleHex tile, bool doubleWide, bool attackerOwned) const
{
	// All hexes that stack would cover if standing on tile have to be accessible.
	BOOST_FOREACH(auto hex, CStack::getHexes(tile, doubleWide, attackerOwned))
	{
        // If the hex is out of range then the tile isn't accessible
        if(!hex.isValid())
            return false;
        // If we're no defender which step on gate and the hex isn't accessible, then the tile
        // isn't accessible
        else if(at(hex) != EAccessibility::ACCESSIBLE &&
                !(at(hex) == EAccessibility::GATE && !attackerOwned))
            return false;
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
	return CBattleInfoCallback::battleCanCastThisSpell(player, spell, ECastingMode::HERO_CASTING);
}

ESpellCastProblem::ESpellCastProblem CPlayerBattleCallback::battleCanCastThisSpell(const CSpell * spell, BattleHex destination) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	return battleCanCastThisSpellHere(player, spell, ECastingMode::HERO_CASTING, destination);
}

ESpellCastProblem::ESpellCastProblem CPlayerBattleCallback::battleCanCreatureCastThisSpell(const CSpell * spell, BattleHex destination) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	return battleCanCastThisSpellHere(player, spell, ECastingMode::CREATURE_ACTIVE_CASTING, destination);
}

bool CPlayerBattleCallback::battleCanFlee() const
{
	RETURN_IF_NOT_BATTLE(false);
	return CBattleInfoEssentials::battleCanFlee(player);
}

TStacks CPlayerBattleCallback::battleGetStacks(EStackOwnership whose /*= MINE_AND_ENEMY*/, bool onlyAlive /*= true*/) const
{
	TStacks ret;
	RETURN_IF_NOT_BATTLE(ret);
	vstd::copy_if(battleGetAllStacks(), std::back_inserter(ret), [=](const CStack *s) -> bool
	{
		const bool ownerMatches = (whose == MINE_AND_ENEMY)
			|| (whose == ONLY_MINE && s->owner == player)
			|| (whose == ONLY_ENEMY && s->owner != player);
		const bool alivenessMatches = s->alive()  ||  !onlyAlive;
		return ownerMatches && alivenessMatches;
	});

	return ret;
}

int CPlayerBattleCallback::battleGetSurrenderCost() const
{
	RETURN_IF_NOT_BATTLE(-3)
	return CBattleInfoCallback::battleGetSurrenderCost(player);
}

bool CPlayerBattleCallback::battleCanCastSpell(ESpellCastProblem::ESpellCastProblem *outProblem /*= nullptr*/) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto problem = CBattleInfoCallback::battleCanCastSpell(player, ECastingMode::HERO_CASTING);
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
	InfoAboutHero ret; 
	assert(0);
	///TODO implement and replace usages of battleGetFightingHero obtaining enemy hero
	return ret;
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
	shooting = Shooting;
	chargedFields = 0;

	luckyHit = false;
	deathBlow = false;
	ballistaDoubleDamage = false;
}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret = *this;
	std::swap(ret.attacker, ret.defender);
	std::swap(ret.attackerBonuses, ret.defenderBonuses);
	std::swap(ret.attackerPosition, ret.defenderPosition);

	ret.attackerCount = ret.attacker->count;
	ret.shooting = false;
	ret.chargedFields = 0;
	ret.luckyHit = ret.ballistaDoubleDamage = ret.deathBlow = false;

	return ret;
}
