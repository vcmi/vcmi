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
	ArtifactID readArtifact32();
	CreatureID readCreature();
	HeroTypeID readHero();
	TerrainId readTerrain();
	RoadId readRoad();
	RiverId readRiver();
	SecondarySkill readSkill();
	SpellID readSpell();
	SpellID readSpell32();
	PlayerColor readPlayer();
	PlayerColor readPlayer32();

	void readBitmaskBuildings(std::set<BuildingID> & dest, std::optional<FactionID> faction);
	void readBitmaskFactions(std::set<FactionID> & dest, bool invert);
	void readBitmaskResources(std::set<GameResID> & dest, bool invert);
	void readBitmaskHeroClassesSized(std::set<HeroClassID> & dest, bool invert);
	void readBitmaskHeroes(std::vector<bool> & dest, bool invert);
	void readBitmaskHeroesSized(std::vector<bool> & dest, bool invert);
	void readBitmaskArtifacts(std::vector<bool> & dest, bool invert);
	void readBitmaskArtifactsSized(std::vector<bool> & dest, bool invert);
	void readBitmaskSpells(std::vector<bool> & dest, bool invert);
	void readBitmaskSpells(std::set<SpellID> & dest, bool invert);
	void readBitmaskSkills(std::vector<bool> & dest, bool invert);
	void readBitmaskSkills(std::set<SecondarySkill> & dest, bool invert);

	int3 readInt3();

	void skipUnused(size_t amount);
	void skipZero(size_t amount);

	void readResourses(TResources & resources);

	bool readBool();

	ui8 readUInt8();
	si8 readInt8();
	ui16 readUInt16();
	si16 readInt16();
	ui32 readUInt32();
	si32 readInt32();

	std::string readBaseString();

	CBinaryReader & getInternalReader();

private:
	template<class Identifier>
	Identifier remapIdentifier(const Identifier & identifier);

	template<class Identifier>
	void readBitmask(std::set<Identifier> & dest, int bytesToRead, int objectsToRead, bool invert);

	template<class Identifier>
	void readBitmask(std::vector<bool> & dest, int bytesToRead, int objectsToRead, bool invert);

	MapFormatFeaturesH3M features;
	MapIdentifiersH3M remapper;

	std::unique_ptr<CBinaryReader> reader;
};

VCMI_LIB_NAMESPACE_END
