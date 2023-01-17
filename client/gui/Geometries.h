/*
 * Geometries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct SDL_MouseMotionEvent;
struct SDL_Rect;

#include "../../lib/Point.h"
#include "../../lib/Rect.h"

VCMI_LIB_NAMESPACE_BEGIN
class int3;
VCMI_LIB_NAMESPACE_END

#ifdef VCMI_LIB_NAMESPACE
using VCMI_LIB_WRAP_NAMESPACE(Rect);
using VCMI_LIB_WRAP_NAMESPACE(Point);
#endif

enum class ETextAlignment {TOPLEFT, CENTER, BOTTOMRIGHT};

namespace Geometry
{

/// creates Point using provided event
Point fromSDL(const SDL_MouseMotionEvent & motion);

/// creates Rect using provided rect
Rect fromSDL(const SDL_Rect & rect);

/// creates SDL_Rect using provided rect
SDL_Rect toSDL(const Rect & rect);

}

