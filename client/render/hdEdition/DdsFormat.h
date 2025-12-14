/*
 * DdsFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/Rect.h"

struct SDL_Surface;

VCMI_LIB_NAMESPACE_BEGIN
class CInputStream;
VCMI_LIB_NAMESPACE_END

constexpr std::uint32_t FOURCC(char a, char b, char c, char d) noexcept
{
	return  (std::uint32_t(static_cast<std::uint8_t>(a))      ) |
			(std::uint32_t(static_cast<std::uint8_t>(b)) <<  8) |
			(std::uint32_t(static_cast<std::uint8_t>(c)) << 16) |
			(std::uint32_t(static_cast<std::uint8_t>(d)) << 24);
}

class DdsFormat
{
#pragma pack(push,1)
	struct DDSPixelFormat
	{
		uint32_t size;
		uint32_t flags;
		uint32_t fourCC;
		uint32_t rgbBits;
		uint32_t rMask;
		uint32_t gMask;
		uint32_t bMask;
		uint32_t aMask;
	};

	struct DDSHeader
	{
		uint32_t size;
		uint32_t flags;
		uint32_t height;
		uint32_t width;
		uint32_t pitchOrLinearSize;
		uint32_t depth;
		uint32_t mipMapCount;
		std::array<uint32_t, 11> reserved1;
		DDSPixelFormat pixel_format;
		uint32_t caps;
		uint32_t caps2;
		uint32_t caps3;
		uint32_t caps4;
		uint32_t reserved2;
	};
#pragma pack(pop)

	struct CachedDDS
	{
		uint32_t w;
		uint32_t h;
		size_t memSize;
		std::shared_ptr<std::vector<uint8_t>> rgba;
	};

	const size_t DDS_CACHE_MEMORY_CAP = 256 * 1024 * 1024; // MB
	size_t currentDDSMemory = 0;

	std::list<std::string> lruList; // front = most recent
	std::mutex cacheMutex;

	struct CacheEntry
	{
		CachedDDS data;
		std::list<std::string>::iterator lruIt;
	};

	std::unordered_map<std::string, CacheEntry> ddsCache;

	void touchLRU(CacheEntry & e, const std::string & key);
	void evictIfNeeded();
	void insertIntoCache(const std::string & key, const CachedDDS & cd);

public:
	SDL_Surface * load(CInputStream * stream, const std::string & cacheName, const Rect * rect = nullptr);
};
