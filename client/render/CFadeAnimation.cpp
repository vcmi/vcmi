/*
 * CAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAnimation.h"

#include "SDL_Extensions.h"
#include "ColorFilter.h"

#include "../CBitmapHandler.h"
#include "../Graphics.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/ISimpleResourceLoader.h"
#include "../../lib/JsonNode.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/vcmi_endian.h"

#include <SDL_surface.h>

float CFadeAnimation::initialCounter() const
{
	if (fadingMode == EMode::OUT)
		return 1.0f;
	return 0.0f;
}

void CFadeAnimation::update()
{
	if (!fading)
		return;

	if (fadingMode == EMode::OUT)
		fadingCounter -= delta;
	else
		fadingCounter += delta;

	if (isFinished())
	{
		fading = false;
		if (shouldFreeSurface)
		{
			SDL_FreeSurface(fadingSurface);
			fadingSurface = nullptr;
		}
	}
}

bool CFadeAnimation::isFinished() const
{
	if (fadingMode == EMode::OUT)
		return fadingCounter <= 0.0f;
	return fadingCounter >= 1.0f;
}

CFadeAnimation::CFadeAnimation()
	: delta(0),	fadingSurface(nullptr), fading(false), fadingCounter(0), shouldFreeSurface(false),
	  fadingMode(EMode::NONE)
{
}

CFadeAnimation::~CFadeAnimation()
{
	if (fadingSurface && shouldFreeSurface)
		SDL_FreeSurface(fadingSurface);
}

void CFadeAnimation::init(EMode mode, SDL_Surface * sourceSurface, bool freeSurfaceAtEnd, float animDelta)
{
	if (fading)
	{
		// in that case, immediately finish the previous fade
		// (alternatively, we could just return here to ignore the new fade request until this one finished (but we'd need to free the passed bitmap to avoid leaks))
		logGlobal->warn("Tried to init fading animation that is already running.");
		if (fadingSurface && shouldFreeSurface)
			SDL_FreeSurface(fadingSurface);
	}
	if (animDelta <= 0.0f)
	{
		logGlobal->warn("Fade anim: delta should be positive; %f given.", animDelta);
		animDelta = DEFAULT_DELTA;
	}

	if (sourceSurface)
		fadingSurface = sourceSurface;

	delta = animDelta;
	fadingMode = mode;
	fadingCounter = initialCounter();
	fading = true;
	shouldFreeSurface = freeSurfaceAtEnd;
}

void CFadeAnimation::draw(SDL_Surface * targetSurface, const Point &targetPoint)
{
	if (!fading || !fadingSurface || fadingMode == EMode::NONE)
	{
		fading = false;
		return;
	}

	CSDL_Ext::setAlpha(fadingSurface, (int)(fadingCounter * 255));
	CSDL_Ext::blitSurface(fadingSurface, targetSurface, targetPoint); //FIXME
	CSDL_Ext::setAlpha(fadingSurface, 255);
}
