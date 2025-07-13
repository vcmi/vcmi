/*
 * SDLImageScaler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../render/IImage.h"
#include "../../lib/Rect.h"

class SDLImageOptimizer : boost::noncopyable
{
	SDL_Surface * surf = nullptr;
	SDL_Surface * output = nullptr;
	Rect virtualDimensions = Rect(0,0,0,0);
public:
	SDLImageOptimizer(SDL_Surface * surf, const Rect & virtualDimensions);

	void optimizeSurface(SDL_Surface * formatSourceSurface);

	/// Aquires resulting surface and transfers surface ownership to the caller
	/// May return nullptr if input image was empty
	SDL_Surface * acquireResultSurface();

	/// Returns adjusted virtual dimensions of resulting surface
	const Rect & getResultDimensions() const;
};

/// Class that performs scaling of SDL surfaces
/// Object construction MUST be performed while holding UI lock
/// but task execution can be performed asynchronously if needed
class SDLImageScaler : boost::noncopyable
{
	SDL_Surface * intermediate = nullptr;
	SDL_Surface * ret = nullptr;
	Rect virtualDimensionsInput = Rect(0,0,0,0);
	Rect virtualDimensionsOutput = Rect(0,0,0,0);

public:
	SDLImageScaler(SDL_Surface * surf);
	SDLImageScaler(SDL_Surface * surf, const Rect & virtualDimensions, bool optimizeImage);
	~SDLImageScaler();

	/// Performs upscaling or downscaling to a requested dimensions
	/// Aspect ratio is NOT maintained.
	/// xbrz algorithm is not supported in this mode
	void scaleSurface(Point dimensions, EScalingAlgorithm algorithm);

	/// Performs upscaling by specified integral factor, potentially using xbrz algorithm if requested
	void scaleSurfaceIntegerFactor(int factor, EScalingAlgorithm algorithm);

	/// Aquires resulting surface and transfers surface ownership to the caller
	/// May return nullptr if input image was empty
	SDL_Surface * acquireResultSurface();

	/// Returns adjusted virtual dimensions of resulting surface
	const Rect & getResultDimensions() const;
};
