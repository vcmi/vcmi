/*
 * DdsFormat.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "DdsFormat.h"

#include <SDL_surface.h>
#include <squish.h>

#include "../../../lib/filesystem/CInputStream.h"

void DdsFormat::touchLRU(CacheEntry & e, const std::string & key)
{
	lruList.erase(e.lruIt);
	lruList.push_front(key);
	e.lruIt = lruList.begin();
}

void DdsFormat::evictIfNeeded()
{
	while (currentDDSMemory > DDS_CACHE_MEMORY_CAP && !lruList.empty())
	{
		const std::string &oldKey = lruList.back();
		auto it = ddsCache.find(oldKey);
		if (it != ddsCache.end())
		{
			currentDDSMemory -= it->second.data.memSize;
			lruList.pop_back();
			ddsCache.erase(it);
		}
		else
		{
			lruList.pop_back();
		}
	}
}

void DdsFormat::insertIntoCache(const std::string & key, const CachedDDS & cd)
{
	auto it = ddsCache.find(key);
	if (it != ddsCache.end())
	{
		currentDDSMemory -= it->second.data.memSize;
		lruList.erase(it->second.lruIt);
		ddsCache.erase(it);
	}

	lruList.push_front(key);

	CacheEntry entry;
	entry.data = cd;
	entry.lruIt = lruList.begin();
	ddsCache.emplace(key, entry);

	currentDDSMemory += cd.memSize;
	evictIfNeeded();
}

SDL_Surface * DdsFormat::load(CInputStream * stream, const std::string & cacheName, const Rect * rect)
{
	const std::string key = cacheName;

	std::shared_ptr<std::vector<uint8_t>> rgba;
	uint32_t w = 0;
	uint32_t h = 0;

	// ---------- Check Cache First ----------
	{
		std::lock_guard lock(cacheMutex);
		auto it = ddsCache.find(key);

		if (it != ddsCache.end())
		{
			touchLRU(it->second, key);

			w = it->second.data.w;
			h = it->second.data.h;
			rgba = it->second.data.rgba;

			// Continue to rectangle extraction
		}
	}

	// Only decode DDS if cache miss
	if (!rgba)
	{
		uint32_t magic = 0;
		stream->read(reinterpret_cast<ui8*>(&magic), 4);
		if (magic != FOURCC('D','D','S',' '))
			return nullptr;

		DDSHeader hdr{};
		stream->read(reinterpret_cast<ui8*>(&hdr), sizeof(hdr));

		w = hdr.width;
		h = hdr.height;

		uint32_t fourcc = hdr.pixel_format.fourCC;
		int squishFlags = 0;

		if (fourcc == FOURCC('D','X','T','1'))
			squishFlags = squish::kDxt1;
		else if (fourcc == FOURCC('D','X','T','5'))
			squishFlags = squish::kDxt5;
		else
			return nullptr;

		int blockBytes = (fourcc == FOURCC('D','X','T','1')) ? 8 : 16;
		int blocks = ((w + 3) / 4) * ((h + 3) / 4);
		int compressedSize = blocks * blockBytes;

		std::vector<uint8_t> comp(compressedSize);
		stream->read(comp.data(), compressedSize);

		rgba = std::make_shared<std::vector<uint8_t>>(w * h * 4);
		squish::DecompressImage(rgba->data(), w, h, comp.data(), squishFlags);

		// Insert decoded DDS into cache
		{
			std::lock_guard<std::mutex> lock(cacheMutex);
			CachedDDS cd;
			cd.w = w;
			cd.h = h;
			cd.rgba = rgba;
			cd.memSize = w * h * 4;
			insertIntoCache(key, cd);
		}
	}

	// ---------- Rectangle extraction ----------
	int rx = 0;
	int ry = 0;
	int rw = static_cast<int>(w);
	int rh = static_cast<int>(h);

	if (rect)
	{
		rx = std::max(0, rect->x);
		ry = std::max(0, rect->y);
		rw = std::min(rect->w, static_cast<int>(w) - rx);
		rh = std::min(rect->h, static_cast<int>(h) - ry);

		if (rw <= 0 || rh <= 0)
			return nullptr;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, rw, rh, 32, SDL_PIXELFORMAT_RGBA32);

	uint8_t* dst = static_cast<uint8_t*>(surf->pixels);

	for (int y = 0; y < rh; ++y)
	{
		const uint8_t* srcLine = rgba->data() + ((ry + y) * w + rx) * 4;
		std::copy_n(srcLine, rw * 4, dst + y * surf->pitch);
	}

	return surf;
}
