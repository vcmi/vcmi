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

#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../MetaString.h"
#include "../NetPacksBase.h"

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
	fmt % unitId() % unitSide();
	return fmt.str();
}

//TODO: deduplicate these functions
const IBonusBearer* Unit::getBonusBearer() const
{
	return this;
}

std::vector<BattleHex> Unit::getSurroundingHexes(BattleHex assumedPosition) const
{
	BattleHex hex = (assumedPosition != BattleHex::INVALID) ? assumedPosition : getPosition(); //use hypothetical position

	return getSurroundingHexes(hex, doubleWide(), unitSide());
}

std::vector<BattleHex> Unit::getSurroundingHexes(BattleHex position, bool twoHex, ui8 side)
{
	std::vector<BattleHex> hexes;
	if(twoHex)
	{
		const BattleHex otherHex = occupiedHex(position, twoHex, side);

		if(side == BattleSide::ATTACKER)
		{
			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				BattleHex::checkAndPush(position.cloneInDirection(dir, false), hexes);

			BattleHex::checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), hexes);
			BattleHex::checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::LEFT, false), hexes);
			BattleHex::checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::TOP_LEFT, false), hexes);
		}
		else
		{
			BattleHex::checkAndPush(position.cloneInDirection(BattleHex::EDir::TOP_LEFT, false), hexes);

			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				BattleHex::checkAndPush(otherHex.cloneInDirection(dir, false), hexes);

			BattleHex::checkAndPush(position.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), hexes);
			BattleHex::checkAndPush(position.cloneInDirection(BattleHex::EDir::LEFT, false), hexes);
		}
		return hexes;
	}
	else
	{
		return position.neighbouringTiles();
	}
}

std::vector<BattleHex> Unit::getAttackableHexes(const Unit * attacker) const
{
	auto defenderHexes = battle::Unit::getHexes(
		getPosition(),
		doubleWide(),
		unitSide());
	
	std::vector<BattleHex> targetableHexes;

	for(auto defenderHex : defenderHexes)
	{
		auto hexes = battle::Unit::getHexes(
			defenderHex,
			attacker->doubleWide(),
			unitSide());

		if(hexes.size() == 2 && BattleHex::getDistance(hexes.front(), hexes.back()) != 1)
			hexes.pop_back();

		for(auto hex : hexes)
			vstd::concatenate(targetableHexes, hex.neighbouringTiles());
	}

	vstd::removeDuplicates(targetableHexes);

	return targetableHexes;
}

bool Unit::coversPos(BattleHex pos) const
{
	return getPosition() == pos || (doubleWide() && (occupiedHex() == pos));
}

std::vector<BattleHex> Unit::getHexes() const
{
	return getHexes(getPosition(), doubleWide(), unitSide());
}

std::vector<BattleHex> Unit::getHexes(BattleHex assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), unitSide());
}

std::vector<BattleHex> Unit::getHexes(BattleHex assumedPos, bool twoHex, ui8 side)
{
	std::vector<BattleHex> hexes;
	hexes.push_back(assumedPos);

	if(twoHex)
		hexes.push_back(occupiedHex(assumedPos, twoHex, side));

	return hexes;
}

BattleHex Unit::occupiedHex() const
{
	return occupiedHex(getPosition(), doubleWide(), unitSide());
}

BattleHex Unit::occupiedHex(BattleHex assumedPos) const
{
	return occupiedHex(assumedPos, doubleWide(), unitSide());
}

BattleHex Unit::occupiedHex(BattleHex assumedPos, bool twoHex, ui8 side)
{
	if(twoHex)
	{
		if(side == BattleSide::ATTACKER)
			return assumedPos - 1;
		else
			return assumedPos + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

void Unit::addText(MetaString & text, EMetaText type, int32_t serial, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		serial = VLC->generaltexth->pluralText(serial, getCount());
	else if(plural)
		serial = VLC->generaltexth->pluralText(serial, 2);
	else
		serial = VLC->generaltexth->pluralText(serial, 1);

	text.appendLocalString(type, serial);
}

void Unit::addNameReplacement(MetaString & text, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		text.replaceCreatureName(creatureId(), getCount());
	else if(plural)
		text.replaceLocalString(EMetaText::CRE_PL_NAMES, creatureIndex());
	else
		text.replaceLocalString(EMetaText::CRE_SING_NAMES, creatureIndex());
}

std::string Unit::formatGeneralMessage(const int32_t baseTextId) const
{
	const int32_t textId = VLC->generaltexth->pluralText(baseTextId, getCount());

	MetaString text;
	text.appendLocalString(EMetaText::GENERAL_TXT, textId);
	text.replaceCreatureName(creatureId(), getCount());

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
	handler.serializeId("type", type, CreatureID::NONE);
	handler.serializeInt("side", side);
	handler.serializeInt("position", position);
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
