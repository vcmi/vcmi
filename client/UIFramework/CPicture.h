#pragma once

#include "CIntObject.h"

struct SDL_Surface;
struct SRect;

/*
 * CPicture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Image class
class CPicture : public CIntObject
{
public: 
	SDL_Surface * bg;
	SRect * srcRect; //if NULL then whole surface will be used
	bool freeSurf; //whether surface will be freed upon CPicture destruction
	bool needRefresh;//Surface needs to be displayed each frame

	operator SDL_Surface*()
	{
		return bg;
	}

	CPicture(const SRect & r, const SDL_Color & color, bool screenFormat = false); //rect filled with given color
	CPicture(const SRect & r, ui32 color, bool screenFormat = false); //rect filled with given color
	CPicture(SDL_Surface * BG, int x = 0, int y=0, bool Free = true); //wrap existing SDL_Surface
	CPicture(const std::string &bmpname, int x=0, int y=0);
	CPicture(SDL_Surface *BG, const SRect &SrcRext, int x = 0, int y = 0, bool free = false); //wrap subrect of given surface
	~CPicture();
	void init();

	void createSimpleRect(const SRect &r, bool screenFormat, ui32 color);
	void show(SDL_Surface * to);
	void showAll(SDL_Surface * to);
	void convertToScreenBPP();
	void colorizeAndConvert(int player);
	void colorize(int player);
};