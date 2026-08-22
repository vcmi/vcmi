/*
 * SDLRWwrapper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SDLRWwrapper.h"

#include "lib/filesystem/CInputStream.h"

#include <SDL3/SDL_iostream.h>

static inline CInputStream* get_stream(void* userdata)
{
	return static_cast<CInputStream*>(userdata);
}

static Sint64 SDLCALL impl_size(void* userdata)
{
	return get_stream(userdata)->getSize();
}

static Sint64 SDLCALL impl_seek(void* userdata, Sint64 offset, SDL_IOWhence whence)
{
	auto stream = get_stream(userdata);
	switch (whence)
	{
	case SDL_IO_SEEK_SET:
		return stream->seek(offset);
	case SDL_IO_SEEK_CUR:
		return stream->seek(stream->tell() + offset);
	case SDL_IO_SEEK_END:
		return stream->seek(stream->getSize() + offset);
	default:
		return -1;
	}
}

static std::size_t SDLCALL impl_read(void* userdata, void *ptr, size_t size, SDL_IOStatus *status)
{
	// unlike SDL2, SDL3 asks for a plain byte count and reports a short read as EOF
	auto count = get_stream(userdata)->read(static_cast<ui8*>(ptr), size);

	if (count == 0)
		*status = SDL_IO_STATUS_EOF;

	return count;
}

static std::size_t SDLCALL impl_write(void* userdata, const void *ptr, size_t size, SDL_IOStatus *status)
{
	// writing is not supported
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

static bool SDLCALL impl_flush(void* userdata, SDL_IOStatus *status)
{
	return true;
}

static bool SDLCALL impl_close(void* userdata)
{
	delete get_stream(userdata);
	return true;
}

SDL_IOStream* MakeSDLIOStream(std::unique_ptr<CInputStream> in)
{
	SDL_IOStreamInterface iface;
	SDL_INIT_INTERFACE(&iface);

	iface.size  = &impl_size;
	iface.seek  = &impl_seek;
	iface.read  = &impl_read;
	iface.write = &impl_write;
	iface.flush = &impl_flush;
	iface.close = &impl_close;

	SDL_IOStream * result = SDL_OpenIO(&iface, in.get());
	if (!result)
		return nullptr;

	in.release();
	return result;
}
