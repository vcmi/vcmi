/*
 * IVideoPlayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/filesystem/ResourcePath.h"

struct SDL_Surface;

VCMI_LIB_NAMESPACE_BEGIN
class Point;
VCMI_LIB_NAMESPACE_END

class IVideoInstance
{
public:
	/// Returns current video timestamp
	virtual double timeStamp() = 0;

	/// Returns true if video playback is over
	virtual bool videoEnded() = 0;

	/// Returns dimensions of the video
	virtual Point size() = 0;

	/// Displays current frame at specified position
	virtual void show(const Point & position, SDL_Surface * to) = 0;

	/// Advances video playback by specified duration
	virtual void tick(uint32_t msPassed) = 0;

	/// activate or deactivate video
	virtual void activate() = 0;
	virtual void deactivate() = 0;

	virtual ~IVideoInstance() = default;
};

class IVideoPlayer : boost::noncopyable
{
public:
	/// Load video from specified path. Returns nullptr on failure
	virtual std::unique_ptr<IVideoInstance> open(const VideoPath & name, float scaleFactor) = 0;

	/// Extracts audio data from provided video in wav format
	virtual std::pair<std::unique_ptr<ui8[]>, si64> getAudio(const VideoPath & videoToOpen) = 0;

	virtual ~IVideoPlayer() = default;
};
