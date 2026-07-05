#include "BAI/v15/graph/nodes/hex.h"

#include "AI/MMAI/common.h"
#include "lib/spells/CSpellHandler.h"
#include "vcmi/spells/Service.h"
#include "vcmi/spells/Spell.h"

namespace MMAI::BAI::V15::Graph::Nodes
{

int Hex::CalcId(const BattleHex & bh)
{
	ASSERT(bh.isAvailable(), "Hex unavailable: " + std::to_string(bh.toInt()));
	return bh.getX() - 1 + (bh.getY() * 15);
}

std::pair<int, int> Hex::CalcXY(const BattleHex & bh)
{
	return {bh.getX() - 1, bh.getY()};
}

Hex::Hex(const Args & args) : bhex(args.bhex), id(CalcId(args.bhex))
{
	auto [x, y] = CalcXY(bhex);

	guardflags.set(); // See note for attr()/setattr() in Hex.h

	for(const auto & obstacle : args.obstacles)
		setMoatFlags(obstacle.get(), args.isGateOpen, args.side);

	setattr(A::Y_COORD, y);
	setattr(A::X_COORD, x);
	setattr(A::WALL_HEALTH, EU(args.wallHP));

	if(!args.isSiege)
	{
		setattr(A::IS_SIEGE_GATE, 0);
		setattr(A::IS_SIEGE_BRIDGE, 0);
	}
	else if(bhex == BattleHex::GATE_INNER || bhex == BattleHex::GATE_OUTER)
		setattr(A::IS_SIEGE_GATE, 1);
	else if(bhex == BattleHex::GATE_BRIDGE)
		setattr(A::IS_SIEGE_BRIDGE, 1);

	switch(args.accessibility)
	{
		case EAccessibility::ACCESSIBLE:
			setattr(A::IS_PASSABLE, 1);
			break;
		case EAccessibility::OBSTACLE:
		case EAccessibility::UNAVAILABLE:
			setattr(A::IS_OBSTACLE, 1);
			break;
		case EAccessibility::GATE:
			setattr(A::IS_PASSABLE, args.side == BattleSide::DEFENDER);
			break;
		case EAccessibility::ALIVE_STACK:
		case EAccessibility::DESTRUCTIBLE_WALL:
			break; // nothing to set
		default:
			THROW_FORMAT("Unexpected hex accessibility for bhex %d: %d", bhex.toInt() % EU(args.accessibility));
	}
}

std::string Hex::name() const
{
	std::stringstream ss;
	ss << detail::Hex_Base::name() << "(y=" << attr(A::Y_COORD) << ",x=" << attr(A::X_COORD) << ")";
	return ss.str();
}

void Hex::setMoatFlags(const CObstacleInstance * obstacle, bool isGateOpen, BattleSide side)
{
	switch(obstacle->obstacleType)
	{
		case CObstacleInstance::MOAT:
			if(!(bhex == BattleHex::GATE_BRIDGE && isGateOpen))
			{
				setattr(A::IS_STOPPING, 1);
				setattr(A::IS_DAMAGING_L, 1);
				setattr(A::IS_DAMAGING_R, 1);
			}
			break;

		case CObstacleInstance::SPELL_CREATED:
		{
			const auto * spell = SpellID(obstacle->ID).toSpell();

			// XXX: can't compare SpellID because there is no constant for tower moat
			// => compare string identifiers
			//  landMine - regular land mine spell
			//  towerMoat - tower land mine
			//  quicksand - no need to check this (checking stopsMovement() is preferred)
			// Ideally, a "damaging" property of the spell or obstacle would be
			// enough and we wouldn't need to check exactly what obstacle this is,
			// but there seems to be no easy way to obtain such information
			if(spell->identifier == "landMine" || spell->identifier == "towerMoat")
			{
				const auto * so = dynamic_cast<const SpellCreatedObstacle *>(obstacle);
				if(side == so->casterSide)
					setattr(side == BattleSide::DEFENDER ? A::IS_DAMAGING_L : A::IS_DAMAGING_R, 1);
				else
					setattr(side == BattleSide::DEFENDER ? A::IS_DAMAGING_R : A::IS_DAMAGING_L, 1);
			}

			if(obstacle->stopsMovement())
				setattr(A::IS_STOPPING, 1);

			break;
		}
		default:
			break;
	}
}

int Hex::attr(Attribute a) const
{
	return attrs.at(EU(a));
}

void Hex::setattr(Attribute a, int value)
{
	attrs.at(EU(a)) = value;
}

}
