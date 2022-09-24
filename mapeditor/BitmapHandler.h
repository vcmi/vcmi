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

#define read_le_u16(p) (* reinterpret_cast<const ui16 *>(p))
#define read_le_u32(p) (* reinterpret_cast<const ui32 *>(p))

#include <QImage>

namespace BitmapHandler
{
	//Load file from /DATA or /SPRITES
	QImage loadBitmap(const std::string & fname, bool setKey = true);
}

