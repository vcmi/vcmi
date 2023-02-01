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

#include "../../lib/filesystem/CInputStream.h"

#include <SDL_rwops.h>

static inline CInputStream* get_stream(SDL_RWops* context)
{
	return static_cast<CInputStream*>(context->hidden.unknown.data1);
}

static Sint64 impl_size(SDL_RWops* context)
{
    return get_stream(context)->getSize();
}

static Sint64 impl_seek(SDL_RWops* context, Sint64 offset, int whence)
{
	auto stream = get_stream(context);
	switch (whence)
	{
	case RW_SEEK_SET:
		return stream->seek(offset);
		break;
	case RW_SEEK_CUR:
		return stream->seek(stream->tell() + offset);
		break;
	case RW_SEEK_END:
		return stream->seek(stream->getSize() + offset);
		break;
	default:
		return -1;
	}

}

static std::size_t impl_read(SDL_RWops* context, void *ptr, size_t size, size_t maxnum)
{
    auto stream = get_stream(context);
    auto oldpos = stream->tell();

    auto count = stream->read(static_cast<ui8*>(ptr), size*maxnum);

	if (count != 0 && count != size*maxnum)
	{
		// if not a whole amount of objects of size has been read, we need to seek
		stream->seek(oldpos + size * (count / size));
	}

    return count / size;
}

static std::size_t impl_write(SDL_RWops* context, const void *ptr, size_t size, size_t num)
{
	// writing is not supported
    return 0;
}

static int impl_close(SDL_RWops* context)
{
	if (context == nullptr)
		return 0;

    delete get_stream(context);
    SDL_FreeRW(context);
    return 0;
}


SDL_RWops* MakeSDLRWops(std::unique_ptr<CInputStream> in)
{
	SDL_RWops* result = SDL_AllocRW();
	if (!result)
		return nullptr;

	result->size  = &impl_size;
	result->seek  = &impl_seek;
	result->read  = &impl_read;
	result->write = &impl_write;
	result->close = &impl_close;

	result->type = SDL_RWOPS_UNKNOWN;
	result->hidden.unknown.data1 = in.release();

	return result;
}
