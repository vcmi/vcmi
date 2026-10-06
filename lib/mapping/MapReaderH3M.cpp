/*
 * MapReaderH3M.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapReaderH3M.h"

#include "../filesystem/CBinaryReader.h"
#include "../int3.h"
#include "../mapObjects/ObjectTemplate.h"

template<>
BuildingID MapReaderH3M::remapIdentifier(const BuildingID & identifier)
{
	return identifier;
}

template<>
GameResID MapReaderH3M::remapIdentifier(const GameResID & identifier)
{
	return identifier;
}

template<>
SpellID MapReaderH3M::remapIdentifier(const SpellID & identifier)
{
	return identifier;
}

template<>
PlayerColor MapReaderH3M::remapIdentifier(const PlayerColor & identifier)
{
	return identifier;
}

template<class Identifier>
Identifier MapReaderH3M::remapIdentifier(const Identifier & identifier)
{
	return remapper.remap(identifier);
}

template<class Identifier>
Identifier MapReaderH3M::validateIdentifier(const Identifier & identifier, int32_t count, const std::string & typeName)
{
	if(identifier == Identifier::NONE || (identifier.getNum() >= 0 && identifier.getNum() < count))
		return identifier;

	logGlobal->warn("Map '%s': Map contains invalid %s %d!", mapName, typeName, identifier.getNum());
	return Identifier::NONE;
}

MapReaderH3M::MapReaderH3M(CInputStream * stream, const std::string & mapName)
	: reader(std::make_unique<CBinaryReader>(stream))
	, mapName(mapName)
{
}

void MapReaderH3M::setFormatLevel(const MapFormatFeaturesH3M & newFeatures)
{
	features = newFeatures;
}

void MapReaderH3M::setIdentifierRemapper(const MapIdentifiersH3M & newRemapper)
{
	remapper = newRemapper;
}

ArtifactID MapReaderH3M::readArtifact()
{
	ArtifactID result;

	if(features.levelAB)
		result = ArtifactID(reader->readUInt16());
	else
		result = ArtifactID(reader->readUInt8());

	if(result.getNum() == features.artifactIdentifierInvalid)
		return ArtifactID::NONE;

	if (result.getNum() < features.artifactsCount)
		return remapIdentifier(result);

	logGlobal->warn("Map '%s': Map contains invalid artifact %d. Will be removed!", mapName, result.getNum());
	return ArtifactID::NONE;
}

ArtifactID MapReaderH3M::readArtifact8()
{
	ArtifactID result(reader->readUInt8());

	if(result.getNum() == 0xff)
		return ArtifactID::NONE;

	if (result.getNum() < features.artifactsCount)
		return remapIdentifier(result);

	logGlobal->warn("Map '%s': Map contains invalid artifact %d. Will be removed!", mapName, result.getNum());
	return ArtifactID::NONE;
}

ArtifactID MapReaderH3M::readArtifact32()
{
	return toArtifact(reader->readInt32());
}

ArtifactID MapReaderH3M::toArtifact(int32_t raw)
{
	return remapIdentifier(validateIdentifier(ArtifactID(raw), features.artifactsCount, "artifact"));
}

HeroTypeID MapReaderH3M::readHero()
{
	HeroTypeID result(reader->readUInt8());

	if(result.getNum() == features.heroIdentifierInvalid)
		return HeroTypeID::NONE;

	return remapIdentifier(validateIdentifier(result, features.heroesCount, "hero"));
}

HeroTypeID MapReaderH3M::readHero32()
{
	HeroTypeID result(reader->readInt32());
	return remapIdentifier(validateIdentifier(result, features.heroesCount, "hero"));
}

HeroTypeID MapReaderH3M::readHeroPortrait()
{
	HeroTypeID result(reader->readUInt8());

	if(result.getNum() == features.heroIdentifierInvalid)
		return HeroTypeID::NONE;

	if (result.getNum() >= features.heroesPortraitsCount)
	{
		logGlobal->warn("Map '%s': Map contains invalid hero portrait ID %d. Will be reset!", mapName, result.getNum() );
		return HeroTypeID::NONE;
	}

	return remapper.remapPortrait(result);
}

CreatureID MapReaderH3M::readCreature32(const std::string & context)
{
	CreatureID result(reader->readUInt32());

	if(result.getNum() == features.creatureIdentifierInvalid)
		return CreatureID::NONE;

	if(result.getNum() < features.creaturesCount)
		return remapIdentifier(result);

	// this may be random creature in army/town, to be randomized later
	CreatureID randomIndex(result.getNum() - features.creatureIdentifierInvalid - 1);
	assert(randomIndex < CreatureID::NONE);

	if (randomIndex.getNum() > -16)
		return randomIndex;

	if(context.empty())
		logGlobal->warn("Map '%s': Map contains invalid creature %d. Will be removed!", mapName, result.getNum());
	else
		logGlobal->warn("Map '%s': Map contains invalid creature %d in %s. Will be removed!", mapName, result.getNum(), context);
	return CreatureID::NONE;
}

CreatureID MapReaderH3M::readCreature(const std::string & context)
{
	CreatureID result;

	if(features.levelAB)
		result = CreatureID(reader->readUInt16());
	else
		result = CreatureID(reader->readUInt8());

	if(result.getNum() == features.creatureIdentifierInvalid)
		return CreatureID::NONE;

	if(result.getNum() < features.creaturesCount)
		return remapIdentifier(result);

	// this may be random creature in army/town, to be randomized later
	CreatureID randomIndex(result.getNum() - features.creatureIdentifierInvalid - 1);
	assert(randomIndex < CreatureID::NONE);

	if (randomIndex.getNum() > -16)
		return randomIndex;

	if(context.empty())
		logGlobal->warn("Map '%s': Map contains invalid creature %d. Will be removed!", mapName, result.getNum());
	else
		logGlobal->warn("Map '%s': Map contains invalid creature %d in %s. Will be removed!", mapName, result.getNum(), context);
	return CreatureID::NONE;
}

FactionID MapReaderH3M::readFaction32()
{
	FactionID result(readInt32());
	return remapIdentifier(validateIdentifier(result, features.factionsCount, "faction"));
}

TerrainId MapReaderH3M::readTerrain()
{
	uint8_t raw = readUInt8();

	if(raw >= features.terrainsCount)
	{
		// Map uses terrain index we don't support (e.g. extended terrain from other editor/mod)
		// Fallback: clamp to last known terrain
		logGlobal->warn(
			"Map '%s': Map uses terrain %u but only %u types are supported. Clamping to %u.",
			mapName,
			raw, features.terrainsCount, features.terrainsCount ? features.terrainsCount - 1 : 0);

		raw = features.terrainsCount ? features.terrainsCount - 1 : 0;
	}

	TerrainId result(raw);
	return remapIdentifier(result);
}

RoadId MapReaderH3M::readRoad()
{
	const uint8_t raw = readInt8();
	// Keep low 3 bits as road type; discard high-bit flags set by some editors (HotA?)
	uint8_t type = raw & 0x07;

	if(type > features.roadsCount)
	{
		// Map uses extended road type not supported by current config.
		// Fallback: use the last supported road type (usually cobblestone).
		logGlobal->warn(
			"Map '%s': Map uses road type %u but only %u types are supported. Clamping to %u.",
			mapName,
			type, features.roadsCount, features.roadsCount);

		type = features.roadsCount;
	}

	return RoadId(type);
}

RiverId MapReaderH3M::readRiver()
{
	const uint8_t raw = readInt8();
	// Keep low 3 bits as river type (0..7); discard high-bit flags set by some editors (HotA?)
	uint8_t type = raw & 0x07;

	if(type > features.riversCount)
	{
		// Map uses extended river type not supported by our config
		// Fallback: clamp to last supported river type
		const uint8_t fallback = features.riversCount ? features.riversCount : 0;

		logGlobal->warn(
			"Map '%s': Map uses river type %u but only %u types are supported. Clamping to %u.",
			mapName,
			type, features.riversCount, fallback);

		type = fallback;
	}

	return RiverId(type);
}

PrimarySkill MapReaderH3M::readPrimary()
{
	PrimarySkill result(readUInt8());
	return validateIdentifier(result, GameConstants::PRIMARY_SKILLS, "primary skill");
}

PrimarySkill MapReaderH3M::readPrimary32()
{
	PrimarySkill result(readInt32());
	return validateIdentifier(result, GameConstants::PRIMARY_SKILLS, "primary skill");
}

SecondarySkill MapReaderH3M::readSkill()
{
	SecondarySkill result(readUInt8());
	return remapIdentifier(validateIdentifier(result, features.skillsCount, "secondary skill"));
}

SecondarySkill MapReaderH3M::readSkill32()
{
	SecondarySkill result(readInt32());
	return remapIdentifier(validateIdentifier(result, features.skillsCount, "secondary skill"));
}

SpellID MapReaderH3M::readSpell()
{
	SpellID result(readUInt8());
	if(result.getNum() == features.spellIdentifierInvalid)
		return SpellID::NONE;
	if(result.getNum() == features.spellIdentifierInvalid - 1)
		return SpellID::PRESET;

	return remapIdentifier(validateIdentifier(result, features.spellsCount, "spell"));
}

SpellID MapReaderH3M::readSpell16()
{
	return toSpell(readInt16());
}

SpellID MapReaderH3M::readSpell32()
{
	return toSpell(readInt32());
}

SpellID MapReaderH3M::toSpell(int32_t raw)
{
	if(raw == features.spellIdentifierInvalid)
		return SpellID::NONE;
	return validateIdentifier(SpellID(raw), features.spellsCount, "spell");
}

GameResID MapReaderH3M::readGameResID()
{
	return toGameResID(readInt8());
}

GameResID MapReaderH3M::readGameResID32()
{
	return toGameResID(readInt32());
}

GameResID MapReaderH3M::toGameResID(int32_t raw)
{
	return validateIdentifier(GameResID(raw), features.resourcesCount, "resource");
}

PlayerColor MapReaderH3M::readPlayer()
{
	uint8_t value = readUInt8();

	if (value == 255)
		return PlayerColor::NEUTRAL;

	if (value >= PlayerColor::PLAYER_LIMIT_I)
	{
		logGlobal->warn("Map '%s': Map contains invalid player ID %d. Will be reset!", mapName, value);
		return PlayerColor::NEUTRAL;
	}

	return PlayerColor(value);
}

PlayerColor MapReaderH3M::readPlayer32()
{
	uint32_t value = readUInt32();

	if (value == 255)
		return PlayerColor::NEUTRAL;

	// HotA scripts use it as "current hero" marker
	if (value == 254)
		return PlayerColor::UNFLAGGABLE;

	if (value >= PlayerColor::PLAYER_LIMIT_I)
	{
		logGlobal->warn("Map '%s': Map contains invalid player ID %d. Will be reset!", mapName, value);
		return PlayerColor::NEUTRAL;
	}

	return PlayerColor(value);
}

BuildingID MapReaderH3M::readBuilding32(std::optional<FactionID> faction)
{
	uint32_t value = readUInt32();
	return remapper.remapBuilding(faction, value);
}

void MapReaderH3M::readBitmaskBuildings(std::set<BuildingID> & dest, std::optional<FactionID> faction)
{
	std::set<BuildingID> h3m;
	readBitmask(h3m, features.buildingsBytes, features.buildingsCount, false);

	for (auto const & h3mEntry : h3m)
	{
		BuildingID mapped = remapper.remapBuilding(faction, h3mEntry);

		if (mapped != BuildingID::NONE) // artifact merchant may be set in random town, but not present in actual town
			dest.insert(mapped);
	}
}

void MapReaderH3M::readBitmaskFactions(std::set<FactionID> & dest, bool invert)
{
	readBitmask(dest, features.factionsBytes, features.factionsCount, invert);
}

void MapReaderH3M::readBitmaskPlayers(std::set<PlayerColor> & dest, bool invert)
{
	readBitmask(dest, 1, 8, invert);
}

void MapReaderH3M::readBitmaskResources(std::set<GameResID> & dest, bool invert)
{
	readBitmask(dest, features.resourcesBytes, features.resourcesCount, invert);
}

void MapReaderH3M::readBitmaskHeroClassesSized(std::set<HeroClassID> & dest, bool invert)
{
	uint32_t classesCount = reader->readUInt32();
	uint32_t classesBytes = (classesCount + 7) / 8;

	readBitmask(dest, classesBytes, classesCount, invert);
}

void MapReaderH3M::readBitmaskHeroes(std::set<HeroTypeID> & dest, bool invert)
{
	readBitmask<HeroTypeID>(dest, features.heroesBytes, features.heroesCount, invert);
}

void MapReaderH3M::readBitmaskHeroesSized(std::set<HeroTypeID> & dest, bool invert)
{
	uint32_t heroesCount = readUInt32();
	uint32_t heroesBytes = (heroesCount + 7) / 8;
	if(heroesCount > features.heroesCount)
		logGlobal->warn("Map '%s': Map contains %d heroes, but only %d are supported. Extra heroes will be ignored!", mapName, heroesCount, features.heroesCount);

	readBitmask<HeroTypeID>(dest, heroesBytes, std::min<int>(heroesCount, features.heroesCount), invert);
}

void MapReaderH3M::readBitmaskArtifacts(std::set<ArtifactID> &dest, bool invert)
{
	readBitmask<ArtifactID>(dest, features.artifactsBytes, features.artifactsCount, invert);
}

void MapReaderH3M::readBitmaskArtifactsSized(std::set<ArtifactID> &dest, bool invert)
{
	uint32_t artifactsCount = reader->readUInt32();
	uint32_t artifactsBytes = (artifactsCount + 7) / 8;
	if(artifactsCount > features.artifactsCount)
		logGlobal->warn("Map '%s': Map contains %d artifacts, but only %d are supported. Extra artifacts will be ignored!", mapName, artifactsCount, features.artifactsCount);

	readBitmask<ArtifactID>(dest, artifactsBytes, std::min<int>(artifactsCount, features.artifactsCount), invert);
}

void MapReaderH3M::readBitmaskSpells(std::set<SpellID> & dest, bool invert)
{
	readBitmask(dest, features.spellsBytes, features.spellsCount, invert);
}

void MapReaderH3M::readBitmaskSkills(std::set<SecondarySkill> & dest, bool invert)
{
	readBitmask(dest, features.skillsBytes, features.skillsCount, invert);
}

template<class Identifier>
void MapReaderH3M::readBitmask(std::set<Identifier> & dest, int bytesToRead, int objectsToRead, bool invert)
{
	for(int byte = 0; byte < bytesToRead; ++byte)
	{
		const ui8 mask = reader->readUInt8();
		for(int bit = 0; bit < 8; ++bit)
		{
			if(byte * 8 + bit < objectsToRead)
			{
				const size_t index = byte * 8 + bit;
				const bool flag = mask & (1 << bit);
				const bool result = (flag != invert);

				Identifier h3mID(index);
				Identifier vcmiID = remapIdentifier(h3mID);

				if (result)
					dest.insert(vcmiID);
				else
					dest.erase(vcmiID);
			}
		}
	}
}

int3 MapReaderH3M::readInt3()
{
	int3 p;
	p.x = reader->readUInt8();
	p.y = reader->readUInt8();
	p.z = reader->readUInt8();
	return p;
}

std::shared_ptr<ObjectTemplate> MapReaderH3M::readObjectTemplate()
{
	auto tmpl = std::make_shared<ObjectTemplate>();
	tmpl->readMap(*reader);
	return tmpl;
}

void MapReaderH3M::remapTemplate(ObjectTemplate & tmpl)
{
	remapper.remapTemplate(tmpl, mapName);
}

void MapReaderH3M::skipUnused(size_t amount)
{
	reader->skip(amount);
}

void MapReaderH3M::skipZero(size_t amount)
{
	for(size_t i = 0; i < amount; ++i)
	{
		uint8_t value = reader->readUInt8();
		if(value != 0)
			logGlobal->warn("Map '%s': Expected zero byte, but %d found!", mapName, static_cast<int>(value));
		assert(value == 0);
	}
}

void MapReaderH3M::readResources(TResources & resources)
{
	for(int x = 0; x < features.resourcesCount; ++x)
		resources[x] = reader->readInt32();
}

bool MapReaderH3M::readBool()
{
	const uint8_t raw = readUInt8();

	if(raw != 0 && raw != 1)
	{
		logGlobal->warn(
			"Map '%s': Invalid bool value %u, using LSB (%u) as value",
			mapName,
			static_cast<unsigned>(raw),
			static_cast<unsigned>(raw & 1));
	}

	return (raw & 1) != 0;
}

int32_t MapReaderH3M::readInt32Checked(int32_t lowerLimit, int32_t upperLimit)
{
	int32_t result = readInt32();
	int32_t resultClamped = std::clamp(result, lowerLimit, upperLimit);
	if (result != resultClamped)
		logGlobal->warn("Map '%s': Map contains out of range value %d! Expected %d-%d", mapName, result, lowerLimit, upperLimit);

	return resultClamped;
}

int8_t MapReaderH3M::readInt8Checked(int8_t lowerLimit, int8_t upperLimit)
{
	int8_t result = readInt8();
	int8_t resultClamped = std::clamp(result, lowerLimit, upperLimit);
	if (result != resultClamped)
		logGlobal->warn("Map '%s': Map contains out of range value %d! Expected %d-%d", mapName, static_cast<int>(result), static_cast<int>(lowerLimit), static_cast<int>(upperLimit));

	return resultClamped;
}

int8_t MapReaderH3M::readInt8Strict(int8_t lowerLimit, int8_t upperLimit)
{
	int8_t result = readInt8();
	if (result < lowerLimit || result > upperLimit)
		throw std::runtime_error("Map '" + mapName + "': Map contains out of range value " + std::to_string(result) + ", expected " + std::to_string(lowerLimit) + "-" + std::to_string(upperLimit));

	return result;
}

uint8_t MapReaderH3M::readUInt8()
{
	return reader->readUInt8();
}

int8_t MapReaderH3M::readInt8()
{
	return reader->readInt8();
}

uint16_t MapReaderH3M::readUInt16()
{
	return reader->readUInt16();
}

int16_t MapReaderH3M::readInt16()
{
	return reader->readInt16();
}

uint32_t MapReaderH3M::readUInt32()
{
	return reader->readUInt32();
}

int32_t MapReaderH3M::readInt32()
{
	return reader->readInt32();
}

std::string MapReaderH3M::readBaseString()
{
	return reader->readBaseString();
}
