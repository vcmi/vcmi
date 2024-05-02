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
#include "../lib/Point.h"

class CEmptyVideoPlayer final : public IVideoPlayer
{
public:
	void redraw( int x, int y, SDL_Surface *dst, bool update) override {};
	void show( int x, int y, SDL_Surface *dst, bool update) override {};
	bool nextFrame() override {return false;};
	void close() override {};
	bool open(const VideoPath & name, bool scale) override {return false;};
	void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update, std::function<void()> restart) override {}
	bool openAndPlayVideo(const VideoPath & name, int x, int y, EVideoType videoType) override { return false; }
	std::pair<std::unique_ptr<ui8 []>, si64> getAudio(const VideoPath & videoToOpen) override { return std::make_pair(nullptr, 0); };
	Point size() override { return Point(0, 0); };
};
