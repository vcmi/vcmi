#pragma once

#include "../GUIBase.h"

struct SDL_Surface;
class CStack;
class CBattleInterface;

/*
 * CStackQueue.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Shows the stack queue
class CStackQueue : public CIntObject
{
	class StackBox : public CIntObject
	{
	public:
		const CStack *my;
		SDL_Surface *bg;

		void hover (bool on);
		void showAll(SDL_Surface *to);
		void setStack(const CStack *nStack);
		StackBox(SDL_Surface *BG);
		~StackBox();
	};

public:
	static const int QUEUE_SIZE = 10;
	const bool embedded;
	std::vector<const CStack *> stacksSorted;
	std::vector<StackBox *> stackBoxes;

	SDL_Surface *box;
	SDL_Surface *bg;
	CBattleInterface * owner;

	void showAll(SDL_Surface *to);
	CStackQueue(bool Embedded, CBattleInterface * _owner);
	~CStackQueue();
	void update();
	void blitBg( SDL_Surface * to );
	//void showAll(SDL_Surface *to);
};