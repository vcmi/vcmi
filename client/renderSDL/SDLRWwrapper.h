/*
 * SDLRWwrapper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CInputStream;

VCMI_LIB_NAMESPACE_END

struct SDL_RWops;

SDL_RWops* MakeSDLRWops(std::unique_ptr<CInputStream> in);
