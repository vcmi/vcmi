/*
 * VideoWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../lib/filesystem/ResourcePath.h"

class VideoWidget final : public CIntObject
{
	VideoPath current;
	VideoPath next;

	int videoSoundHandle;
public:
	VideoWidget(const Point & position, const VideoPath & prologue, const VideoPath & looped);
	VideoWidget(const Point & position, const VideoPath & looped);
	~VideoWidget();

	void activate() override;
	void deactivate() override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	void tick(uint32_t msPassed) override;
};
