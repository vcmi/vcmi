//
//  BitmapHandler.hpp
//  vcmieditor
//
//  Created by nordsoft on 29.08.2022.
//

#pragma once

#define read_le_u16(p) (* reinterpret_cast<const ui16 *>(p))
#define read_le_u32(p) (* reinterpret_cast<const ui32 *>(p))

namespace BitmapHandler
{
	//Load file from /DATA or /SPRITES
	QImage loadBitmap(std::string fname, bool setKey=true);
}

