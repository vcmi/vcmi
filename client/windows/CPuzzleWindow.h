/*
 * CPuzzleWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"
#include "../../lib/int3.h"

class CLabel;
class CButton;
class CResDataBar;

/// Puzzle screen which gets uncovered when you visit obilisks
class CPuzzleWindow : public CWindowObject
{
private:
	int3 grailPos;
	std::shared_ptr<CPicture> logo;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CButton> quitb;
	std::shared_ptr<CResDataBar> resDataBar;

	std::vector<std::shared_ptr<CPicture>> piecesToRemove;
	std::vector<std::shared_ptr<CPicture>> visiblePieces;
	ui8 currentAlpha;

public:
	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;

	CPuzzleWindow(const int3 & grailPos, double discoveredRatio);
};
