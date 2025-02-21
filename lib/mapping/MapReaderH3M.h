/*
 * MapReaderH3M.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../ResourceSet.h"
#include "MapFeaturesH3M.h"
#include "MapIdentifiersH3M.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBinaryReader;
class CInputStream;
struct MapFormatFeaturesH3M;
class int3;
enum class EMapFormat : uint8_t;

class MapReaderH3M
{
public:
	explicit MapReaderH3M(CInputStream * stream);

	void setFormatLevel(const MapFormatFeaturesH3M & features);
	void setIdentifierRemapper(const MapIdentifiersH3M & remapper);

	ArtifactID readArtifact();
	ArtifactID readArtifact8();
	ArtifactID readArtifact32();
	CreatureID readCreature32();
	CreatureID readCreature();
	HeroTypeID readHero();
	HeroTypeID readHeroPortrait();
	TerrainId readTerrain();
	RoadId readRoad();
	RiverId readRiver();
	PrimarySkill readPrimary();
	SecondarySkill readSkill();
	SpellID readSpell();
	SpellID readSpell16();
	SpellID readSpell32();
	GameResID readGameResID();
	PlayerColor readPlayer();
	PlayerColor readPlayer32();

	void readBitmaskBuildings(std::set<BuildingID> & dest, std::optional<FactionID> faction);
	void readBitmaskFactions(std::set<FactionID> & dest, bool invert);
	void readBitmaskPlayers(std::set<PlayerColor> & dest, bool invert);
	void readBitmaskResources(std::set<GameResID> & dest, bool invert);
	void readBitmaskHeroClassesSized(std::set<HeroClassID> & dest, bool invert);
	void readBitmaskHeroes(std::set<HeroTypeID> & dest, bool invert);
	void readBitmaskHeroesSized(std::set<HeroTypeID> & dest, bool invert);
	void readBitmaskArtifacts(std::set<ArtifactID> & dest, bool invert);
	void readBitmaskArtifactsSized(std::set<ArtifactID> & dest, bool invert);
	void readBitmaskSpells(std::set<SpellID> & dest, bool invert);
	void readBitmaskSkills(std::set<SecondarySkill> & dest, bool invert);

	int3 readInt3();

	std::shared_ptr<ObjectTemplate> readObjectTemplate();
	void remapTemplate(ObjectTemplate & tmpl);

	void skipUnused(size_t amount);
	void skipZero(size_t amount);

	void readResources(TResources & resources);

	bool readBool();

	uint8_t readUInt8();
	int8_t readInt8();
	int8_t readInt8Checked(int8_t lowerLimit, int8_t upperLimit);

	int16_t readInt16();
	uint16_t readUInt16();

	uint32_t readUInt32();
	int32_t readInt32();

	std::string readBaseString();

private:
	template<class Identifier>
	Identifier remapIdentifier(const Identifier & identifier);

	template<class Identifier>
	void readBitmask(std::set<Identifier> & dest, int bytesToRead, int objectsToRead, bool invert);

	MapFormatFeaturesH3M features;
	MapIdentifiersH3M remapper;

	std::unique_ptr<CBinaryReader> reader;
};

VCMI_LIB_NAMESPACE_END
