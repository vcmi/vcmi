/*
 * Unit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Unit.h"

#include "../GameLibrary.h"
#include "../texts/CGeneralTextHandler.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

#include <vcmi/Faction.h>
#include <vcmi/FactionService.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{

///Unit
Unit::~Unit() = default;

bool Unit::isDead() const
{
	return !alive() && !isGhost();
}

bool Unit::isTurret() const
{
	return creatureIndex() == CreatureID::ARROW_TOWERS;
}

std::string Unit::getDescription() const
{
	boost::format fmt("Unit %d of side %d");
	fmt % unitId() % static_cast<int>(unitSide());
	return fmt.str();
}

//TODO: deduplicate these functions
const IBonusBearer* Unit::getBonusBearer() const
{
	return this;
}

const BattleHexArray & Unit::getSurroundingHexes(const BattleHex & assumedPosition) const
{
	BattleHex hex = (assumedPosition.toInt() != BattleHex::INVALID) ? assumedPosition : getPosition(); //use hypothetical position

	return getSurroundingHexes(hex, doubleWide(), unitSide());
}

const BattleHexArray & Unit::getSurroundingHexes(const BattleHex & position, bool twoHex, BattleSide side)
{
	if(!twoHex)
		return position.getNeighbouringTiles();

	return position.getNeighbouringTilesDoubleWide(side);
}

BattleHexArray Unit::getAttackableHexes(const Unit * attacker) const
{
	if (!attacker->doubleWide())
	{
		return getSurroundingHexes();
	}
	else
	{
		BattleHexArray result;

		for (const auto & attackOrigin : getSurroundingHexes())
		{
			if (!coversPos(attacker->occupiedHex(attackOrigin)) && attackOrigin.isAvailable())
				result.insert(attackOrigin);

			bool isAttacker = attacker->unitSide() == BattleSide::ATTACKER;
			BattleHex::EDir headDirection = isAttacker ? BattleHex::RIGHT : BattleHex::LEFT;
			BattleHex headHex = attackOrigin.cloneInDirection(headDirection);

			if (!coversPos(headHex) && headHex.isAvailable())
				result.insert(headHex);
		}
		return result;
	}
}

bool Unit::coversPos(const BattleHex & pos) const
{
	return getPosition() == pos || (doubleWide() && (occupiedHex() == pos));
}

const BattleHexArray & Unit::getHexes() const
{
	return getHexes(getPosition(), doubleWide(), unitSide());
}

const BattleHexArray & Unit::getHexes(const BattleHex & assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), unitSide());
}

BattleHexArray::ArrayOfBattleHexArrays Unit::precomputeUnitHexes(BattleSide side, bool twoHex)
{
	BattleHexArray::ArrayOfBattleHexArrays result;

	for (BattleHex assumedPos = 0; assumedPos < GameConstants::BFIELD_SIZE; ++assumedPos)
	{
		BattleHexArray hexes;
		hexes.insert(assumedPos);

		if(twoHex)
			hexes.insert(occupiedHex(assumedPos, twoHex, side));

		result[assumedPos.toInt()] = std::move(hexes);
	}

	return result;
}

const BattleHexArray & Unit::getHexes(const BattleHex & assumedPos, bool twoHex, BattleSide side)
{
	static const std::array<BattleHexArray::ArrayOfBattleHexArrays, 4> precomputed = {
		precomputeUnitHexes(BattleSide::ATTACKER, false),
		precomputeUnitHexes(BattleSide::ATTACKER, true),
		precomputeUnitHexes(BattleSide::DEFENDER, false),
		precomputeUnitHexes(BattleSide::DEFENDER, true),
	};

	static const std::array<BattleHexArray, 5> invalidHexes = {
		BattleHexArray({BattleHex( 0)}),
		BattleHexArray({BattleHex(-1)}),
		BattleHexArray({BattleHex(-2)}),
		BattleHexArray({BattleHex(-3)}),
		BattleHexArray({BattleHex(-4)})
	};

	if (assumedPos.isValid())
	{
		int index = side == BattleSide::ATTACKER ? 0 : 2;
		return precomputed[index + twoHex][assumedPos.toInt()];
	}
	else
	{
		// Towers and such
		return invalidHexes.at(-assumedPos.toInt());
	}
}

BattleHex Unit::occupiedHex() const
{
	return occupiedHex(getPosition(), doubleWide(), unitSide());
}

BattleHex Unit::occupiedHex(const BattleHex & assumedPos) const
{
	return occupiedHex(assumedPos, doubleWide(), unitSide());
}

BattleHex Unit::occupiedHex(const BattleHex & assumedPos, bool twoHex, BattleSide side)
{
	if(twoHex)
	{
		if(side == BattleSide::ATTACKER)
			return assumedPos.toInt() - 1;
		else
			return assumedPos.toInt() + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

void Unit::addText(MetaString & text, EMetaText type, int32_t serial, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		serial = LIBRARY->generaltexth->pluralText(serial, getCount());
	else if(plural)
		serial = LIBRARY->generaltexth->pluralText(serial, 2);
	else
		serial = LIBRARY->generaltexth->pluralText(serial, 1);

	text.appendLocalString(type, serial);
}

void Unit::addNameReplacement(MetaString & text, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		text.replaceName(creatureId(), getCount());
	else if(plural)
		text.replaceNamePlural(creatureIndex());
	else
		text.replaceNameSingular(creatureIndex());
}

std::string Unit::formatGeneralMessage(const int32_t baseTextId) const
{
	const int32_t textId = LIBRARY->generaltexth->pluralText(baseTextId, getCount());

	MetaString text;
	text.appendLocalString(EMetaText::GENERAL_TXT, textId);
	text.replaceName(creatureId(), getCount());

	return text.toString();
}

int Unit::getRawSurrenderCost() const
{
	//we pay for our stack that comes from our army slots - condition eliminates summoned cres and war machines
	if(unitSlot().validSlot())
		return creatureCost() * getCount();
	else
		return 0;
}

///UnitInfo
void UnitInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("count", count);
	handler.serializeId("type", type, CreatureID(CreatureID::NONE));
	handler.serializeInt("side", side);
	si16 positionValue = position.toInt();
	handler.serializeInt("position", positionValue);
	position = positionValue;
	handler.serializeBool("summoned", summoned);
}

void UnitInfo::save(JsonNode & data)
{
	data.clear();
	JsonSerializer ser(nullptr, data);
	ser.serializeStruct("newUnitInfo", *this);
}

void UnitInfo::load(uint32_t id_, const JsonNode & data)
{
	id = id_;
    JsonDeserializer deser(nullptr, data);
    deser.serializeStruct("newUnitInfo", *this);
}

}

VCMI_LIB_NAMESPACE_END
