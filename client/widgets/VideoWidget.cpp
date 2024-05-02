/*
 * TextControls.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VideoWidget.h"

#include "../CGameInfo.h"
#include "../media/IVideoPlayer.h"
#include "../media/ISoundPlayer.h"
#include "../render/Canvas.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"

VideoWidget::VideoWidget(const Point & position, const VideoPath & looped)
	: VideoWidget(position, VideoPath(), looped)
{
}

VideoWidget::VideoWidget(const Point & position, const VideoPath & prologue, const VideoPath & looped)
	:current(prologue)
	,next(looped)
	,videoSoundHandle(-1)
{
}

VideoWidget::~VideoWidget() = default;

void VideoWidget::show(Canvas & to)
{
	CCS->videoh->update(pos.x + 107, pos.y + 70, to.getInternalSurface(), true, false);
}

void VideoWidget::activate()
{
	CCS->videoh->open(current);
	auto audioData = CCS->videoh->getAudio(current);
	videoSoundHandle = CCS->soundh->playSound(audioData, -1);

	if (videoSoundHandle != -1)
	{
		CCS->soundh->setCallback(videoSoundHandle, [this](){
			if (GH.windows().isTopWindow(this))
				this->videoSoundHandle = -1;
		});
	}
}

void VideoWidget::deactivate()
{
	CCS->videoh->close();
	CCS->soundh->stopSound(videoSoundHandle);
}

void VideoWidget::showAll(Canvas & to)
{
	CCS->videoh->update(pos.x + 107, pos.y + 70, to.getInternalSurface(), true, false);
}

void VideoWidget::tick(uint32_t msPassed)
{

}
