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

enum class EVideoType : ui8
{
	INTRO = 0, // use entire window: stopOnKey = true, scale = true, overlay = false
	SPELLBOOK  // overlay video: stopOnKey = false, scale = false, overlay = true
};

class IVideoPlayer : boost::noncopyable
{
public:
	virtual bool open(const VideoPath & name, bool scale = false)=0; //true - succes
	virtual void close()=0;
	virtual bool nextFrame()=0;
	virtual void show(int x, int y, SDL_Surface *dst, bool update = true)=0;
	virtual void redraw(int x, int y, SDL_Surface *dst, bool update = true)=0; //reblits buffer
	virtual void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true, std::function<void()> restart = nullptr) = 0;
	virtual bool openAndPlayVideo(const VideoPath & name, int x, int y, EVideoType videoType) = 0;
	virtual std::pair<std::unique_ptr<ui8 []>, si64> getAudio(const VideoPath & videoToOpen) = 0;
	virtual Point size() = 0;

	virtual ~IVideoPlayer() = default;
};
