//
//  BitmapHandler.hpp
//  vcmieditor
//
//  Created by nordsoft on 29.08.2022.
//

#pragma once

namespace BitmapHandler
{
	//Load file from /DATA or /SPRITES
	QImage loadBitmap(std::string fname, bool setKey=true);
}

