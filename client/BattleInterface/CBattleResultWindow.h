#pragma once

#include "../UIFramework/CIntObject.h"

struct SDL_Surface;
class AdventureMapButton;
class CBattleInterface;
struct SDL_Rect;
struct BattleResult;

/*
 * CBattleResultWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which is responsible for showing the battle result window
class CBattleResultWindow : public CIntObject
{
private:
	SDL_Surface *background;
	AdventureMapButton *exit;
	CBattleInterface *owner;
public:
	CBattleResultWindow(const BattleResult &br, const SDL_Rect &pos, CBattleInterface *_owner); //c-tor
	~CBattleResultWindow(); //d-tor

	void bExitf(); //exit button callback

	void activate();
	void deactivate();
	void show(SDL_Surface * to = 0);
};