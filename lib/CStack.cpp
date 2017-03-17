/*
 * CStack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStack.h"
#include "BattleInfo.h"
#include "spells/CSpellHandler.h"
#include "CRandomGenerator.h"
#include "NetPacks.h"


CStack::CStack(const CStackInstance *Base, PlayerColor O, int I, bool AO, SlotID S)
	: base(Base), ID(I), owner(O), slot(S), attackerOwned(AO),
	counterAttacksPerformed(0),counterAttacksTotalCache(0), cloneID(-1),
	firstHPleft(-1), position(), shots(0), casts(0), resurrected(0)
{
	assert(base);
	type = base->type;
	count = baseAmount = base->count;
	setNodeType(STACK_BATTLE);
}
CStack::CStack()
{
	init();
	setNodeType(STACK_BATTLE);
}
CStack::CStack(const CStackBasicDescriptor *stack, PlayerColor O, int I, bool AO, SlotID S)
	: base(nullptr), ID(I), owner(O), slot(S), attackerOwned(AO),
	counterAttacksPerformed(0), counterAttacksTotalCache(0), cloneID(-1),
	firstHPleft(-1), position(), shots(0), casts(0), resurrected(0)
{
	type = stack->type;
	count = baseAmount = stack->count;
	setNodeType(STACK_BATTLE);
}

void CStack::init()
{
	base = nullptr;
	type = nullptr;
	ID = -1;
	count = baseAmount = -1;
	firstHPleft = -1;
	owner = PlayerColor::NEUTRAL;
	slot = SlotID(255);
	attackerOwned = false;
	position = BattleHex();
	counterAttacksPerformed = 0;
	counterAttacksTotalCache = 0;
	cloneID = -1;

	shots = 0;
	casts = 0;
	resurrected = 0;
}

void CStack::postInit()
{
	assert(type);
	assert(getParentNodes().size());

	firstHPleft = MaxHealth();
	shots = getCreature()->valOfBonuses(Bonus::SHOTS);
	counterAttacksPerformed = 0;
	counterAttacksTotalCache = 0;
	casts = valOfBonuses(Bonus::CASTS);
	resurrected = 0;
	cloneID = -1;
}

ui32 CStack::level() const
{
	if (base)
		return base->getLevel(); //creatture or commander
	else
		return std::max(1, (int)getCreature()->level); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	si32 magicResistance;
	if (base) //TODO: make war machines receive aura of magic resistance
	{
		magicResistance = base->magicResistance();
		int auraBonus = 0;
		for (const CStack * stack : base->armyObj->battle-> batteAdjacentCreatures(this))
	{
		if (stack->owner == owner)
		{
			vstd::amax(auraBonus, stack->valOfBonuses(Bonus::SPELL_RESISTANCE_AURA)); //max value
		}
	}
		magicResistance += auraBonus;
		vstd::amin (magicResistance, 100);
	}
	else
		magicResistance = type->magicResistance();
	return magicResistance;
}

void CStack::stackEffectToFeature(std::vector<Bonus> & sf, const Bonus & sse)
{
	const CSpell * sp = SpellID(sse.sid).toSpell();

	std::vector<Bonus> tmp;
	sp->getEffects(tmp, sse.val);

	for(Bonus& b : tmp)
	{
		if(b.turnsRemain == 0)
			b.turnsRemain = sse.turnsRemain;
		sf.push_back(b);
	}
}

bool CStack::willMove(int turn /*= 0*/) const
{
	return ( turn ? true : !vstd::contains(state, EBattleStackState::DEFENDING) )
		&& !moved(turn)
		&& canMove(turn);
}

bool CStack::canMove( int turn /*= 0*/ ) const
{
	return alive()
		&& !hasBonus(Selector::type(Bonus::NOT_ACTIVE).And(Selector::turns(turn))); //eg. Ammo Cart or blinded creature
}

bool CStack::moved( int turn /*= 0*/ ) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::MOVED);
	else
		return false;
}

bool CStack::waited(int turn /*= 0*/) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::WAITING);
	else
		return false;
}

bool CStack::doubleWide() const
{
	return getCreature()->doubleWide;
}

BattleHex CStack::occupiedHex() const
{
	return occupiedHex(position);
}

BattleHex CStack::occupiedHex(BattleHex assumedPos) const
{
	if (doubleWide())
	{
		if (attackerOwned)
			return assumedPos - 1;
		else
			return assumedPos + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

std::vector<BattleHex> CStack::getHexes() const
{
	return getHexes(position);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), attackerOwned);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos, bool twoHex, bool AttackerOwned)
{
	std::vector<BattleHex> hexes;
	hexes.push_back(assumedPos);

	if (twoHex)
	{
		if (AttackerOwned)
			hexes.push_back(assumedPos - 1);
		else
			hexes.push_back(assumedPos + 1);
	}

	return hexes;
}

bool CStack::coversPos(BattleHex pos) const
{
	return vstd::contains(getHexes(), pos);
}

std::vector<BattleHex> CStack::getSurroundingHexes(BattleHex attackerPos) const
{
	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos : position; //use hypothetical position
	std::vector<BattleHex> hexes;
	if (doubleWide())
	{
		const int WN = GameConstants::BFIELD_WIDTH;
		if(attackerOwned)
		{ //position is equal to front hex
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+2 : WN+1 ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), hexes);
			BattleHex::checkAndPush(hex - 2, hexes);
			BattleHex::checkAndPush(hex + 1, hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-2 : WN-1 ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), hexes);
		}
		else
		{
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN-1 : WN-2 ), hexes);
			BattleHex::checkAndPush(hex + 2, hexes);
			BattleHex::checkAndPush(hex - 1, hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN+1 : WN+2 ), hexes);
		}
		return hexes;
	}
	else
	{
		return hex.neighbouringTiles();
	}
}

std::vector<si32> CStack::activeSpells() const
{
	std::vector<si32> ret;

	std::stringstream cachingStr;
	cachingStr << "!type_" << Bonus::NONE << "source_" << Bonus::SPELL_EFFECT;
	CSelector selector = Selector::sourceType(Bonus::SPELL_EFFECT)
		.And(CSelector([](const Bonus *b)->bool
		{
			return b->type != Bonus::NONE;
		}));

	TBonusListPtr spellEffects = getBonuses(selector, Selector::all, cachingStr.str());
	for(const std::shared_ptr<Bonus> it : *spellEffects)
	{
		if (!vstd::contains(ret, it->sid)) //do not duplicate spells with multiple effects
			ret.push_back(it->sid);
	}

	return ret;
}

CStack::~CStack()
{
	detachFromAll();
}

const CGHeroInstance * CStack::getMyHero() const
{
	if(base)
		return dynamic_cast<const CGHeroInstance *>(base->armyObj);
	else //we are attached directly?
		for(const CBonusSystemNode *n : getParentNodes())
			if(n->getNodeType() == HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return nullptr;
}

ui32 CStack::totalHealth() const
{
	return ((count > 0) ? MaxHealth() * (count-1) : 0) + firstHPleft;//do not hide possible invalid firstHPleft for dead stack
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << "Battle stack [" << ID << "]: " << count << " creatures of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

std::pair<int,int> CStack::countKilledByAttack(int damageReceived) const
{
	int newRemainingHP = 0;
	int killedCount = damageReceived / MaxHealth();
	unsigned damageFirst = damageReceived % MaxHealth();

	if (damageReceived && vstd::contains(state, EBattleStackState::CLONED)) // block ability should not kill clone (0 damage)
	{
		killedCount = count;
	}
	else
	{
		if( firstHPleft <= damageFirst )
		{
			killedCount++;
			newRemainingHP = firstHPleft + MaxHealth() - damageFirst;
		}
		else
		{
			newRemainingHP = firstHPleft - damageFirst;
		}
	}

	if(killedCount == count)
		newRemainingHP = 0;

	return std::make_pair(killedCount, newRemainingHP);
}

void CStack::prepareAttacked(BattleStackAttacked &bsa, CRandomGenerator & rand, boost::optional<int> customCount /*= boost::none*/) const
{
	auto afterAttack = countKilledByAttack(bsa.damageAmount);

	bsa.killedAmount = afterAttack.first;
	bsa.newHP = afterAttack.second;


	if(bsa.damageAmount && vstd::contains(state, EBattleStackState::CLONED)) // block ability should not kill clone (0 damage)
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
		return; // no rebirth I believe
	}

	const int countToUse = customCount ? *customCount : count;

	if(countToUse <= bsa.killedAmount) //stack killed
	{
		bsa.newAmount = 0;
		bsa.flags |= BattleStackAttacked::KILLED;
		bsa.killedAmount = countToUse; //we cannot kill more creatures than we have

		int resurrectFactor = valOfBonuses(Bonus::REBIRTH);
		if(resurrectFactor > 0 && casts) //there must be casts left
		{
			int resurrectedStackCount = base->count * resurrectFactor / 100;

			// last stack has proportional chance to rebirth
			auto diff = base->count * resurrectFactor / 100.0 - resurrectedStackCount;
			if (diff > rand.nextDouble(0, 0.99))
			{
				resurrectedStackCount += 1;
			}

			if(hasBonusOfType(Bonus::REBIRTH, 1))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedStackCount, 1);
			}

			if(resurrectedStackCount > 0)
			{
				bsa.flags |= BattleStackAttacked::REBIRTH;
				bsa.newAmount = resurrectedStackCount; //risky?
				bsa.newHP = MaxHealth(); //resore full health
			}
		}
	}
	else
	{
		bsa.newAmount = countToUse - bsa.killedAmount;
	}
}

bool CStack::isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos /*= BattleHex::INVALID*/, BattleHex defenderPos /*= BattleHex::INVALID*/)
{
	if (!attackerPos.isValid())
	{
		attackerPos = attacker->position;
	}
	if (!defenderPos.isValid())
	{
		defenderPos = defender->position;
	}

	return
		(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0)						//front <=> front
		|| (attacker->doubleWide()									//back <=> front
		&& BattleHex::mutualPosition(attackerPos + (attacker->attackerOwned ? -1 : 1), defenderPos) >= 0)
		|| (defender->doubleWide()									//front <=> back
		&& BattleHex::mutualPosition(attackerPos, defenderPos + (defender->attackerOwned ? -1 : 1)) >= 0)
		|| (defender->doubleWide() && attacker->doubleWide()//back <=> back
		&& BattleHex::mutualPosition(attackerPos + (attacker->attackerOwned ? -1 : 1), defenderPos + (defender->attackerOwned ? -1 : 1)) >= 0);

}

bool CStack::ableToRetaliate() const //FIXME: crash after clone is killed
{
	return alive()
		&& (counterAttacksPerformed < counterAttacksTotal() || hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS))
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON)
		&& !hasBonusOfType(Bonus::HYPNOTIZED)
		&& !hasBonusOfType(Bonus::NO_RETALIATION);
}

ui8 CStack::counterAttacksTotal() const
{
	//after dispell bonus should remain during current round
	ui8 val = 1 + valOfBonuses(Bonus::ADDITIONAL_RETALIATION);
	vstd::amax(counterAttacksTotalCache, val);
	return counterAttacksTotalCache;
}

si8 CStack::counterAttacksRemaining() const
{
	return counterAttacksTotal() - counterAttacksPerformed;
}

std::string CStack::getName() const
{
	return (count > 1) ? type->namePl : type->nameSing; //War machines can't use base
}

bool CStack::isValidTarget(bool allowDead/* = false*/) const
{
	return (alive() || (allowDead && isDead())) && position.isValid() && !isTurret();
}

bool CStack::isDead() const
{
	return !alive() && !isGhost();
}

bool CStack::isGhost() const
{
	return vstd::contains(state,EBattleStackState::GHOST);
}

bool CStack::isTurret() const
{
	return type->idNumber == CreatureID::ARROW_TOWERS;
}

bool CStack::canBeHealed() const
{
	return firstHPleft < MaxHealth()
		&& isValidTarget()
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON);
}

void CStack::makeGhost()
{
	state.erase(EBattleStackState::ALIVE);
	state.insert(EBattleStackState::GHOST_PENDING);
}

bool CStack::alive() const //determines if stack is alive
{
	return vstd::contains(state,EBattleStackState::ALIVE);
}

ui32 CStack::calculateHealedHealthPoints(ui32 toHeal, const bool resurrect) const
{
	if(!resurrect && !alive())
	{
		logGlobal->warnStream() <<"Attempt to heal corpse detected.";
		return 0;
	}

	return std::min<ui32>(toHeal, MaxHealth() - firstHPleft + (resurrect ? (baseAmount - count) * MaxHealth() : 0));
}

ui8 CStack::getSpellSchoolLevel(const CSpell * spell, int * outSelectedSchool) const
{
	int skill = valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, spell->id));

	vstd::abetween(skill, 0, 3);

	return skill;
}

ui32 CStack::getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const
{
	//stacks does not have sorcery-like bonuses (yet?)
	return base;
}

int CStack::getEffectLevel(const CSpell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int CStack::getEffectPower(const CSpell * spell) const
{
	return valOfBonuses(Bonus::CREATURE_SPELL_POWER) * count / 100;
}

int CStack::getEnchantPower(const CSpell * spell) const
{
	int res = valOfBonuses(Bonus::CREATURE_ENCHANT_POWER);
	if(res<=0)
		res = 3;//default for creatures
	return res;
}

int CStack::getEffectValue(const CSpell * spell) const
{
	return valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, spell->id.toEnum()) * count;
}

const PlayerColor CStack::getOwner() const
{
	return owner;
}

void CStack::getCasterName(MetaString & text) const
{
	//always plural name in case of spell cast.
	text.addReplacement(MetaString::CRE_PL_NAMES, type->idNumber.num);
}

void CStack::getCastDescription(const CSpell * spell, const std::vector<const CStack*> & attacked, MetaString & text) const
{
	text.addTxt(MetaString::GENERAL_TXT, 565);//The %s casts %s
	//todo: use text 566 for single creature
	getCasterName(text);
	text.addReplacement(MetaString::SPELL_NAME, spell->id.toEnum());
}
