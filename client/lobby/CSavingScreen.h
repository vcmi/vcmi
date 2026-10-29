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
#include "../Translator.h"

struct StartInfo;
class CMapInfo;
class ElementInfo;

class CSelectionBase;

class CSavingScreen : public CSelectionBase
{
	std::shared_ptr<ElementInfo> selectedElement;
	/// Texts of the selected save, read from the local file since the server does not send map info during the game
	std::optional<TranslatorOverlay> localTextOverlay;

public:
	std::shared_ptr<CMapInfo> localMi;

	CSavingScreen();

	void changeSelection(const std::shared_ptr<ElementInfo> & element);
	void saveGame();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
	
protected:
	void close() override;
};
