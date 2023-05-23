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
#include "CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

MapReaderH3M::MapReaderH3M(CInputStream * stream)
	: reader(std::make_unique<CBinaryReader>(stream))
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

	if(result == features.artifactIdentifierInvalid)
		return ArtifactID::NONE;

	if (result < features.artifactsCount)
		return result;

	logGlobal->warn("Map contains invalid artifact %d. Will be removed!", result.getNum());
	return ArtifactID::NONE;
}

ArtifactID MapReaderH3M::readArtifact32()
{
	ArtifactID result(reader->readInt32());

	if(result == ArtifactID::NONE)
		return ArtifactID::NONE;

	if (result < features.artifactsCount)
		return result;

	logGlobal->warn("Map contains invalid artifact %d. Will be removed!", result.getNum());
	return ArtifactID::NONE;
}

HeroTypeID MapReaderH3M::readHero()
{
	HeroTypeID result(reader->readUInt8());

	if(result.getNum() == features.heroIdentifierInvalid)
		return HeroTypeID(-1);

	assert(result.getNum() < features.heroesPortraitsCount);
	return result;
}

CreatureID MapReaderH3M::readCreature()
{
	CreatureID result;

	if(features.levelAB)
		result = CreatureID(reader->readUInt16());
	else
		result = CreatureID(reader->readUInt8());

	if(result == features.creatureIdentifierInvalid)
		return CreatureID::NONE;

	if(result < features.creaturesCount)
		return result;

	// this may be random creature in army/town, to be randomized later
	CreatureID randomIndex(result.getNum() - features.creatureIdentifierInvalid - 1);
	assert(randomIndex < CreatureID::NONE);

	if (randomIndex > -16)
		return randomIndex;

	logGlobal->warn("Map contains invalid creature %d. Will be removed!", result.getNum());
	return CreatureID::NONE;
}

TerrainId MapReaderH3M::readTerrain()
{
	TerrainId result(readUInt8());
	assert(result.getNum() < features.terrainsCount);
	return result;
}

RoadId MapReaderH3M::readRoad()
{
	RoadId result(readInt8());
	assert(result < Road::ORIGINAL_ROAD_COUNT);
	return result;
}

RiverId MapReaderH3M::readRiver()
{
	RiverId result(readInt8());
	assert(result < River::ORIGINAL_RIVER_COUNT);
	return result;
}

SecondarySkill MapReaderH3M::readSkill()
{
	SecondarySkill result(readUInt8());
	assert(result < features.skillsCount);
	return result;
}

SpellID MapReaderH3M::readSpell()
{
	SpellID result(readUInt8());
	if(result == features.spellIdentifierInvalid)
		return SpellID::NONE;
	if(result == features.spellIdentifierInvalid - 1)
		return SpellID::PRESET;

	assert(result < features.spellsCount);
	return result;
}

SpellID MapReaderH3M::readSpell32()
{
	SpellID result(readInt32());
	if(result == features.spellIdentifierInvalid)
		return SpellID::NONE;
	assert(result < features.spellsCount);
	return result;
}

PlayerColor MapReaderH3M::readPlayer()
{
	PlayerColor result(readUInt8());
	assert(result < PlayerColor::PLAYER_LIMIT || result == PlayerColor::NEUTRAL);
	return result;
}

PlayerColor MapReaderH3M::readPlayer32()
{
	PlayerColor result(readInt32());

	assert(result < PlayerColor::PLAYER_LIMIT || result == PlayerColor::NEUTRAL);
	return result;
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

void MapReaderH3M::readBitmask(std::vector<bool> & dest, const int bytesToRead, const int objectsToRead, bool invert)
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
				dest[index] = result;
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

void MapReaderH3M::skipUnused(size_t amount)
{
	reader->skip(amount);
}

void MapReaderH3M::skipZero(size_t amount)
{
#ifdef NDEBUG
	skipUnused(amount);
#else
	for(size_t i = 0; i < amount; ++i)
	{
		uint8_t value = reader->readUInt8();
		assert(value == 0);
	}
#endif
}

void MapReaderH3M::readResourses(TResources & resources)
{
	for(int x = 0; x < features.resourcesCount; ++x)
		resources[x] = reader->readInt32();
}

bool MapReaderH3M::readBool()
{
	uint8_t result = readUInt8();
	assert(result == 0 || result == 1);

	return result != 0;
}

ui8 MapReaderH3M::readUInt8()
{
	return reader->readUInt8();
}

si8 MapReaderH3M::readInt8()
{
	return reader->readInt8();
}

ui16 MapReaderH3M::readUInt16()
{
	return reader->readUInt16();
}

si16 MapReaderH3M::readInt16()
{
	return reader->readInt16();
}

ui32 MapReaderH3M::readUInt32()
{
	return reader->readUInt32();
}

si32 MapReaderH3M::readInt32()
{
	return reader->readInt32();
}

std::string MapReaderH3M::readBaseString()
{
	return reader->readBaseString();
}

CBinaryReader & MapReaderH3M::getInternalReader()
{
	return *reader;
}

VCMI_LIB_NAMESPACE_END
