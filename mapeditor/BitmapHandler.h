/*
 * BitmapHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QImage>

namespace BitmapHandler
{
	//Load file from /DATA or /SPRITES
	QImage loadBitmap(const std::string & fname, bool setKey = true);
}

