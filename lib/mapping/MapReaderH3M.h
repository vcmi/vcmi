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

	void setFormatLevel(EMapFormat format, uint8_t hotaVersion);

	ArtifactID readArtifact();
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

	template<class Identifier>
	void readBitmask(std::set<Identifier> & dest, int bytesToRead, int objectsToRead, bool invert)
	{
		std::vector<bool> bitmap;
		bitmap.resize(objectsToRead, false);
		readBitmask(bitmap, bytesToRead, objectsToRead, invert);

		for(int i = 0; i < bitmap.size(); i++)
			if(bitmap[i])
				dest.insert(static_cast<Identifier>(i));
	}

	/** Reads bitmask to boolean vector
	* @param dest destination vector, shall be filed with "true" values
	* @param byteCount size in bytes of bimask
	* @param limit max count of vector elements to alter
	* @param negate if true then set bit in mask means clear flag in vertor
	*/
	void readBitmask(std::vector<bool> & dest, int bytesToRead, int objectsToRead, bool invert);

	/**
	* Helper to read map position
	*/
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
	MapFormatFeaturesH3M features;

	std::unique_ptr<CBinaryReader> reader;
};

VCMI_LIB_NAMESPACE_END
