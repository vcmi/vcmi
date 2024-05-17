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

class Canvas;

VCMI_LIB_NAMESPACE_BEGIN
class Point;
VCMI_LIB_NAMESPACE_END

class IVideoInstance
{
public:
	/// Returns true if video playback is over
	virtual bool videoEnded() = 0;

	/// Returns dimensions of the video
	virtual Point size() = 0;

	/// Displays current frame at specified position
	virtual void show(const Point & position, Canvas & canvas) = 0;

	/// Advances video playback by specified duration
	virtual void tick(uint32_t msPassed) = 0;

	virtual ~IVideoInstance() = default;
};

class IVideoPlayer : boost::noncopyable
{
public:
	/// Plays video on top of the screen, returns only after playback is over, aborts on input event
	virtual bool playIntroVideo(const VideoPath & name) = 0;

	/// Plays video on top of the screen, returns only after playback is over
	virtual void playSpellbookAnimation(const VideoPath & name, const Point & position) = 0;

	/// Load video from specified path. Returns nullptr on failure
	virtual std::unique_ptr<IVideoInstance> open(const VideoPath & name, bool scaleToScreen) = 0;

	/// Extracts audio data from provided video in wav format
	virtual std::pair<std::unique_ptr<ui8[]>, si64> getAudio(const VideoPath & videoToOpen) = 0;

	virtual ~IVideoPlayer() = default;
};
