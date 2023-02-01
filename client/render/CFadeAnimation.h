/*
 * CFadeAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class Rect;
class Point;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;
class CDefFile;
class ColorFilter;

const float DEFAULT_DELTA = 0.05f;

class CFadeAnimation
{
public:
	enum class EMode
	{
		NONE, IN, OUT
	};
private:
	float delta;
	SDL_Surface * fadingSurface;
	bool fading;
	float fadingCounter;
	bool shouldFreeSurface;

	float initialCounter() const;
	bool isFinished() const;
public:
	EMode fadingMode;

	CFadeAnimation();
	~CFadeAnimation();
	void init(EMode mode, SDL_Surface * sourceSurface, bool freeSurfaceAtEnd = false, float animDelta = DEFAULT_DELTA);
	void update();
	void draw(SDL_Surface * targetSurface, const Point & targetPoint);
	bool isFading() const { return fading; }
};
