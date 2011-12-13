#pragma once

#include "../GUIBase.h"

class CBattleInterface;
class CDefHandler;
class CGHeroInstance;
struct SDL_Surface;

/*
 * CBattleHero.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Hero battle animation
class CBattleHero : public CIntObject
{
public:
	bool flip; //false if it's attacking hero, true otherwise
	CDefHandler *dh, *flag; //animation and flag
	const CGHeroInstance *myHero; //this animation's hero instance
	const CBattleInterface *myOwner; //battle interface to which this animation is assigned
	int phase; //stage of animation
	int nextPhase; //stage of animation to be set after current phase is fully displayed
	int image; //frame of animation
	ui8 flagAnim, flagAnimCount; //for flag animation
	void show(SDL_Surface *to); //prints next frame of animation to to
	void activate();
	void deactivate();
	void setPhase(int newPhase); //sets phase of hero animation
	void clickLeft(tribool down, bool previousState); //call-in
	CBattleHero(const std::string &defName, int phaseG, int imageG, bool filpG, ui8 player, const CGHeroInstance *hero, const CBattleInterface *owner); //c-tor
	~CBattleHero(); //d-tor
};