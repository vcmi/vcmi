/*
 * CSavingScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSelectionBase.h"

class CSelectionBase;
struct StartInfo;
class CMapInfo;

class CSavingScreen : public CSelectionBase
{
public:
	const StartInfo * localSi;
	CMapInfo * localMi;

	CSavingScreen();
	~CSavingScreen();

	void changeSelection(std::shared_ptr<CMapInfo> to);
	void saveGame();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
};
