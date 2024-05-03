/*
 * CEmptyVideoPlayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IVideoPlayer.h"

class CEmptyVideoPlayer final : public IVideoPlayer
{
public:
	/// Plays video on top of the screen, returns only after playback is over
	virtual bool playIntroVideo(const VideoPath & name)
	{
		return false;
	};

	virtual void playSpellbookAnimation(const VideoPath & name, const Point & position)
	{
	}

	/// Load video from specified path
	virtual std::unique_ptr<IVideoInstance> open(const VideoPath & name, bool scaleToScreen)
	{
		return nullptr;
	};

	/// Extracts audio data from provided video in wav format
	virtual std::pair<std::unique_ptr<ui8 []>, si64> getAudio(const VideoPath & videoToOpen)
	{
		return { nullptr, 0};
	};
};
