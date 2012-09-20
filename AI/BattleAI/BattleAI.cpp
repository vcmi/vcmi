#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "BattleAI.h"
#include "../../lib/BattleState.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/VCMI_Lib.h"

using boost::optional;
CBattleCallback * cbc;

//#define LOGL(text) tlog6 << (text) << std::endl
//#define LOGFL(text, formattingEl) tlog6 << boost::str(boost::format(text) % formattingEl) << std::endl
#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

class StackWithBonuses : public IBonusBearer
{
public:
	const CStack *stack;
	mutable std::vector<Bonus> bonusesToAdd;

	virtual const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL, const std::string &cachingStr = "") const OVERRIDE
	{
		TBonusListPtr ret = make_shared<BonusList>();
		const TBonusListPtr originalList = stack->getAllBonuses(selector, limit, root, cachingStr);
		boost::copy(*originalList, std::back_inserter(*ret));
		BOOST_FOREACH(auto &bonus, bonusesToAdd)
		{
			if(selector(&bonus)  &&  (!limit || !limit(&bonus)))
				ret->push_back(&bonus);
		}

		//TODO limiters?

		return ret;
	}
};

struct Skirmish
{
	const CStack *attacker, *defender;
	int retaliationDamage, dealtDamage;

	Skirmish(const CStack *Attacker, const CStack *Defender)
		:attacker(Attacker), defender(Defender)
	{
		TDmgRange retal, dmg = cbc->battleEstimateDamage(attacker, defender, &retal);
		dealtDamage = (dmg.first + dmg.second) / 2;
		retaliationDamage = (retal.first + retal.second) / 2;

		if(attacker->hasBonusOfType(Bonus::ADDITIONAL_ATTACK))
			dealtDamage *= 2;
		if(attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || defender->hasBonusOfType(Bonus::NO_RETALIATION))
			retaliationDamage = 0;

	}
};

CBattleAI::CBattleAI(void)
	: side(-1), cb(NULL)
{
	print("created");
}


CBattleAI::~CBattleAI(void)
{
	print("destroyed");
}

void CBattleAI::init( CBattleCallback * CB )
{
	print("init called, saving ptr to IBattleCallback");
	cbc = cb = CB;
	playerID = CB->getPlayerID();; //TODO should be sth in callback
	CB->waitTillRealize = true;
	CB->unlockGsWhenWaiting = false;
}

void CBattleAI::actionFinished( const BattleAction *action )
{
	print("actionFinished called");
}

void CBattleAI::actionStarted( const BattleAction *action )
{
	print("actionStarted called");
}

struct EnemyInfo
{
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s)
	{}
	void calcDmg(const CStack * ourStack)
	{
		TDmgRange retal, dmg = cbc->battleEstimateDamage(ourStack, s, &retal);
		adi = (dmg.first + dmg.second) / 2;
		adr = (retal.first + retal.second) / 2;
	}

	bool operator==(const EnemyInfo& ei) const
	{
		return s == ei.s;
	}
};

bool isMoreProfitable(const EnemyInfo &ei1, const EnemyInfo& ei2)
{
	return (ei1.adi-ei1.adr) < (ei2.adi - ei2.adr);
}

int distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances& dists, BattleHex *chosenHex = NULL)
{
	int ret = 1000000;
	BOOST_FOREACH(BattleHex n, hex.neighbouringTiles())
	{
		if(dists[n] >= 0 && dists[n] < ret)
		{
			ret = dists[n];
			if(chosenHex)
				*chosenHex = n;
		}
	}

	return ret;
}

bool isCloser(const EnemyInfo & ei1, const EnemyInfo & ei2, const ReachabilityInfo::TDistances & dists)
{
	return distToNearestNeighbour(ei1.s->position, dists) < distToNearestNeighbour(ei2.s->position, dists);
}

static bool willSecondHexBlockMoreEnemyShooters(const BattleHex &h1, const BattleHex &h2)
{
	int shooters[2] = {0}; //count of shooters on hexes

	for(int i = 0; i < 2; i++)
		BOOST_FOREACH(BattleHex neighbour, (i ? h2 : h1).neighbouringTiles())
			if(const CStack *s = cbc->battleGetStackByPos(neighbour))
				if(s->getCreature()->isShooting())
						shooters[i]++;

	return shooters[0] < shooters[1];
}

template <typename Container, typename Pred>
auto sum(const Container & c, Pred p) -> decltype(p(*boost::begin(c)))
{
	double ret = 0;
	BOOST_FOREACH(const auto &element, c)
	{
		ret += p(element);
	}

	return ret;
}

struct ThreatMap
{	
	std::array<std::vector<BattleAttackInfo>, GameConstants::BFIELD_SIZE> threatMap; // [hexNr] -> enemies able to strike
	
	const CStack *endangered; 
	std::array<int, GameConstants::BFIELD_SIZE> sufferedDamage; 

	struct ThreatMap(const CStack *Endangered)
		: endangered(Endangered)
	{
		sufferedDamage.fill(0);

		BOOST_FOREACH(const CStack *enemy, cbc->battleGetStacks())
		{
			//Consider only stacks of different owner
			if(enemy->attackerOwned == endangered->attackerOwned)
				continue;

			//Look-up which tiles can be melee-attacked
			std::array<bool, GameConstants::BFIELD_SIZE> meleeAttackable;
			meleeAttackable.fill(false);
			auto enemyReachability = cbc->getReachability(enemy);
			for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			{
				if(enemyReachability.isReachable(i))
				{
					meleeAttackable[i] = true;
					BOOST_FOREACH(auto n, BattleHex(i).neighbouringTiles())
						meleeAttackable[n] = true;
				}
			}

			//Gather possible assaults
			for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			{
				if(cbc->battleCanShoot(enemy, i))
					threatMap[i].push_back(BattleAttackInfo(enemy, endangered, true));
				else if(meleeAttackable[i])
				{
					BattleAttackInfo bai(enemy, endangered, false);
					bai.chargedFields = std::max(BattleHex::getDistance(enemy->position, i) - 1, 0); //TODO check real distance (BFS), not just metric
					threatMap[i].push_back(BattleAttackInfo(bai));
				}
			}
		}

		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			sufferedDamage[i] = sum(threatMap[i], [](const BattleAttackInfo &bai) -> int
			{
				auto dmg = cbc->calculateDmgRange(bai);
				return (dmg.first + dmg.second)/2;
			});
		}
	}
};

struct AttackPossibility
{
	const CStack *enemy; //redundant (to attack.defender) but looks nice
	BattleHex tile; //tile from which we attack
	BattleAttackInfo attack;

	int damageDealt;
	int damageReceived; //usually by counter-attack
	int damageDiff() const
	{
		return damageDealt - damageReceived;
	}

	int attackValue() const
	{
		//TODO consider tactical advantage
		return damageDiff();
	}
};

struct PotentialTargets
{
	std::vector<AttackPossibility> possibleAttacks;
	std::vector<const CStack *> unreachableEnemies;

	std::function<AttackPossibility(bool,BattleHex)>  GenerateAttackInfo; //args: shooting, destHex

	PotentialTargets(const CStack *attacker, optional<IBonusBearer*> attackerBonuses = boost::none)
	{
		auto dists = cbc->battleGetDistances(attacker);
		std::vector<BattleHex> avHexes = cbc->battleGetAvailableHexes(attacker, false);

		BOOST_FOREACH(const CStack *enemy, cbc->battleGetStacks())
		{
			//Consider only stacks of different owner
			if(enemy->attackerOwned == attacker->attackerOwned)
				continue;
			
			GenerateAttackInfo = [&](bool shooting, BattleHex hex) -> AttackPossibility
			{
				auto bai = BattleAttackInfo(attacker, enemy, shooting);
				if(attackerBonuses)
					bai.attackerBonuses = *attackerBonuses;

				AttackPossibility ap = {enemy, hex, bai, 0, 0};
				if(hex.isValid())
				{
					assert(dists[hex] <= attacker->Speed());
					ap.attack.chargedFields = dists[hex];
				}

				std::pair<ui32, ui32> retaliation;
				auto attackDmg = cbc->battleEstimateDamage(ap.attack, &retaliation);
				ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
				ap.damageReceived = (retaliation.first + retaliation.second) / 2;
				//TODO other damage related to attack (eg. fire shield and other abilities)
				//TODO limit max damage by total stacks health (dealing 100000 dmg to single Pikineer is not that effective)

				return ap;
			};

			if(cbc->battleCanShoot(attacker, enemy->position))
			{
				possibleAttacks.push_back(GenerateAttackInfo(true, BattleHex::INVALID));
			}
			else
			{
				BOOST_FOREACH(BattleHex hex, avHexes)
					if(CStack::isMeleeAttackPossible(attacker, enemy, hex))
						possibleAttacks.push_back(GenerateAttackInfo(false, hex));

				if(!vstd::contains_if(possibleAttacks, [=](const AttackPossibility &pa) { return pa.enemy == enemy; }))
					unreachableEnemies.push_back(enemy);
			}
		}
	}

	AttackPossibility bestAction() const
	{
		if(possibleAttacks.empty())
			throw std::runtime_error("No best action, since we don't have any actions");

		return *vstd::maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.damageDiff(); } );
	}
};

BattleAction CBattleAI::activeStack( const CStack * stack )
{
	try
	{
		print("activeStack called for " + stack->nodeName());
		if(stack->type->idNumber == 145) //catapult
			return useCatapult(stack);

		if(cb->battleCanCastSpell())
			attemptCastingSpell();

		ThreatMap threatsToUs(stack);
		PotentialTargets targets(stack);

		if(targets.possibleAttacks.size())
		{
			auto hlp = targets.bestAction();
			if(hlp.attack.shooting)
				return BattleAction::makeShotAttack(stack, hlp.enemy);
			else
				return BattleAction::makeMeleeAttack(stack, hlp.enemy, hlp.tile);
		}
		else
		{
			if(stack->waited())
			{
				auto dists = cbc->battleGetDistances(stack);
				const EnemyInfo &ei= *range::min_element(targets.unreachableEnemies, boost::bind(isCloser, _1, _2, boost::ref(dists)));
				if(distToNearestNeighbour(ei.s->position, dists) < GameConstants::BFIELD_SIZE)
				{
					return goTowards(stack, ei.s->position);
				}
			}
			else
			{
				return BattleAction::makeWait(stack);
			}
		}
	}
	catch(std::exception &e)
	{
		tlog1 << "Exception occurred in " << __FUNCTION__ << " " << e.what() << std::endl;
	}

	return BattleAction::makeDefend(stack);
}

void CBattleAI::battleAttack(const BattleAttack *ba)
{
	print("battleAttack called");
}

void CBattleAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	print("battleStacksAttacked called");
}

void CBattleAI::battleEnd(const BattleResult *br)
{
	print("battleEnd called");
}

void CBattleAI::battleNewRoundFirst(int round)
{
	print("battleNewRoundFirst called");
}

void CBattleAI::battleNewRound(int round)
{
	print("battleNewRound called");
}

void CBattleAI::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance)
{
	print("battleStackMoved called");;
}

void CBattleAI::battleSpellCast(const BattleSpellCast *sc)
{
	print("battleSpellCast called");
}

void CBattleAI::battleStacksEffectsSet(const SetStackEffect & sse)
{
	print("battleStacksEffectsSet called");
}

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side)
{
	print("battleStart called");
	side = Side;
}

void CBattleAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom)
{
	print("battleStacksHealedRes called");
}

void CBattleAI::battleNewStackAppeared(const CStack * stack)
{
	print("battleNewStackAppeared called");
}

void CBattleAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	print("battleObstaclesRemoved called");
}

void CBattleAI::battleCatapultAttacked(const CatapultAttack & ca)
{
	print("battleCatapultAttacked called");
}

void CBattleAI::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	print("battleStacksRemoved called");
}

void CBattleAI::print(const std::string &text) const
{
	tlog6 << "CBattleAI [" << this <<"]: " << text << std::endl;
}

BattleAction CBattleAI::goTowards(const CStack * stack, BattleHex destination)
{
	assert(destination.isValid());
	auto avHexes = cb->battleGetAvailableHexes(stack, false);
	auto reachability = cb->getReachability(stack);

	if(vstd::contains(avHexes, destination))
		return BattleAction::makeMove(stack, destination);

	auto destNeighbours = destination.neighbouringTiles();
	if(vstd::contains_if(destNeighbours, [&](BattleHex n) { return stack->coversPos(destination); }))
	{
		tlog3 << "Warning: already standing on neighbouring tile!" << std::endl;
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}

	vstd::erase_if(destNeighbours, [&](BattleHex hex){ return !reachability.accessibility.accessible(hex, stack); });

	if(!avHexes.size() || !destNeighbours.size()) //we are blocked or dest is blocked
	{
		print("goTowards: Stack cannot move! That's " + stack->nodeName());
		return BattleAction::makeDefend(stack);
	}

	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto distToDestNeighbour = [&](BattleHex hex) -> int
		{
			auto nearestNeighbourToHex = vstd::minElementByFun(destNeighbours, [&](BattleHex a)
			{
				return BattleHex::getDistance(a, hex);
			});

			return BattleHex::getDistance(*nearestNeighbourToHex, hex);
		};

		auto nearestAvailableHex = vstd::minElementByFun(avHexes, distToDestNeighbour);
		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex bestNeighbor = destination;
		if(distToNearestNeighbour(destination, reachability.distances, &bestNeighbor) > GameConstants::BFIELD_SIZE)
		{
			print("goTowards: Cannot reach");
			return BattleAction::makeDefend(stack);
		}

		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			assert(currentDest.isValid());
			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);

			currentDest = reachability.predecessors[currentDest];
		}
	}
}

BattleAction CBattleAI::useCatapult(const CStack * stack)
{
	throw std::exception("The method or operation is not implemented.");
}

bool isSupportedSpell(const CSpell *spell)
{
	switch(spell->id)
	{
		// permanent effects
	case Spells::SHIELD:
	case Spells::AIR_SHIELD:
	case Spells::FIRE_SHIELD:
	case Spells::PROTECTION_FROM_AIR:
	case Spells::PROTECTION_FROM_FIRE:
	case Spells::PROTECTION_FROM_WATER:
	case Spells::PROTECTION_FROM_EARTH:
	case Spells::ANTI_MAGIC:
	case Spells::MAGIC_MIRROR:
	case Spells::BLESS:
	case Spells::CURSE:
	case Spells::BLOODLUST:
	case Spells::PRECISION:
	case Spells::WEAKNESS:
	case Spells::STONE_SKIN:
	case Spells::DISRUPTING_RAY:
	case Spells::PRAYER:
	case Spells::MIRTH:
	case Spells::SORROW:
	case Spells::FORTUNE:
	case Spells::MISFORTUNE:
	case Spells::HASTE:
	case Spells::SLOW:
	case Spells::SLAYER:
	case Spells::FRENZY:
	case Spells::COUNTERSTRIKE:
	case Spells::BERSERK:
	case Spells::HYPNOTIZE:
	case Spells::FORGETFULNESS:
	case Spells::BLIND:
	case Spells::STONE_GAZE:
	case Spells::POISON:
	case Spells::BIND:
	case Spells::DISEASE:
	case Spells::PARALYZE:
	case Spells::AGE:
	case Spells::ACID_BREATH_DEFENSE:
		return true;

	default:
		return false;
	}
};

struct PossibleSpellcast
{
	const CSpell *spell;
	BattleHex dest;
};

void CBattleAI::attemptCastingSpell()
{
	LOGL("Casting spells sounds like fun. Let's see...");

	auto known = cb->battleGetFightingHero(side);

	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;
	vstd::copy_if(VLC->spellh->spells, std::back_inserter(possibleSpells), [this] (const CSpell *s) -> bool
	{ 
		auto problem = cbc->battleCanCastThisSpell(s); 
		return problem == ESpellCastProblem::OK; 
	});
	LOGFL("I can cast %d spells.", possibleSpells.size());

	vstd::erase_if(possibleSpells, [](const CSpell *s) 
	{return !isSupportedSpell(s); });
	LOGFL("I know about workings of %d of them.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	BOOST_FOREACH(auto spell, possibleSpells)
	{
		BOOST_FOREACH(auto hex, cbc->battleGetPossibleTargets(playerID, spell))
		{
			PossibleSpellcast ps = {spell, hex};
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;

	std::map<const CStack*, int> valueOfStack;
	BOOST_FOREACH(auto stack, cb->battleGetStacks())
	{
		PotentialTargets pt(stack);
		valueOfStack[stack] = pt.bestAction().attackValue();
	}

	auto evaluateSpellcast = [&] (const PossibleSpellcast &ps) -> int
	{
		int skillLevel = 0;

		StackWithBonuses swb;
		swb.stack = cb->battleGetStackByPos(ps.dest);
		if(!swb.stack)
			return -1;

		Bonus pseudoBonus;
		pseudoBonus.sid = ps.spell->id;
		pseudoBonus.val = skillLevel;
		pseudoBonus.turnsRemain = 1; //TODO
		CStack::stackEffectToFeature(swb.bonusesToAdd, pseudoBonus);

		PotentialTargets pt(swb.stack, &swb);
		auto newValue = pt.bestAction().attackValue();
		auto oldValue = valueOfStack[swb.stack];
		auto gain = newValue - oldValue;
		if(swb.stack->owner != playerID) //enemy
			gain = -gain;

		LOGFL("Casting %s on %s would improve the stack by %d points (from %d to %d)",
			ps.spell->name % swb.stack->nodeName() % gain % (oldValue) % (newValue));

		return gain;
	};

	auto castToPerform = *vstd::maxElementByFun(possibleCasts, evaluateSpellcast);
	BattleAction spellcast;
	spellcast.actionType = BattleAction::HERO_SPELL;
	spellcast.additionalInfo = castToPerform.spell->id;
	spellcast.destinationTile = castToPerform.dest;
	spellcast.side = side;
	spellcast.stackNumber = (!side) ? -1 : -2;

	cb->battleMakeAction(&spellcast);
}