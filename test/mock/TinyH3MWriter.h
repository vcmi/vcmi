/*
 * TinyH3MWriter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/mapping/MapFeaturesH3M.h"

class int3;

namespace TinyH3M
{

/// Low-level byte writer; one-to-one mirror of MapReaderH3M.
/// Holds the same MapFormatFeaturesH3M so per-format field sizes come from a
/// single source of truth shared with the loader.
class TinyH3MWriter
{
public:
	TinyH3MWriter();

	void setFormatLevel(const MapFormatFeaturesH3M & features);

	const std::vector<uint8_t> & buffer() const { return data; }
	std::vector<uint8_t>         take() { return std::move(data); }

	// ---- primitives -----------------------------------------------------

	void writeUInt8(uint8_t v);
	void writeInt8(int8_t v);
	void writeUInt16(uint16_t v);
	void writeUInt32(uint32_t v);
	void writeInt32(int32_t v);
	void writeBool(bool v);

	void writeBaseString(const std::string & s);

	// Emits zero bytes — symmetric to MapReaderH3M::skipZero.
	void skipZero(size_t amount);

	// ---- identifiers ----------------------------------------------------

	void writeInt3(const int3 & p);
	void writeHero(HeroTypeID v);
	void writeArtifact(ArtifactID v);
	void writeCreature(CreatureID v);
	void writePlayer(PlayerColor v);
	void writeSpell32(SpellID v);   // 4 bytes; used by spell scrolls
	void writeGameResID(GameResID v); // 1 signed byte

	// Writes `bytes` bytes whose value is 0xff (all bits set).
	// Helper for default "all-allowed" tables where building a std::set with every id is wasteful.
	void writeAllOnes(size_t bytes);

private:
	std::vector<uint8_t>     data;
	MapFormatFeaturesH3M     features;
};

} // namespace TinyH3M
