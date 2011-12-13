#pragma once


#include "CBattleAnimation.h"
#include "../../lib/SHexField.h"

class CBattleInterface;

/*
 * CSpellEffectAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// This class manages a spell effect animation
class CSpellEffectAnimation : public CBattleAnimation
{
private:
	ui32 effect;
	SHexField destTile;
	std::string	customAnim;
	int	x, y, dx, dy;
	bool Vflip;
public:
	bool init();
	void nextFrame();
	void endAnim();

	CSpellEffectAnimation(CBattleInterface *_owner, ui32 _effect, SHexField _destTile, int _dx = 0, int _dy = 0, bool _Vflip = false);
	CSpellEffectAnimation(CBattleInterface *_owner, std::string _customAnim, int _x, int _y, int _dx = 0, int _dy = 0, bool _Vflip = false);
};