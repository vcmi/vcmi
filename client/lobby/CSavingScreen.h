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
struct LobbyScenarioInfo;

class CSelectionBase;

class CSavingScreen : public CSelectionBase
{
	std::shared_ptr<ElementInfo> selectedElement;
	/// Texts of the selected save, from the map info received for it
	std::optional<TranslatorOverlay> localTextOverlay;

public:
	std::shared_ptr<CMapInfo> localMi;

	CSavingScreen();

	void changeSelection(const std::shared_ptr<ElementInfo> & element);
	/// Shows map info of the selected save; replies for a save that is no longer selected are ignored
	void onScenarioInfoReceived(const LobbyScenarioInfo & pack);
	void saveGame();

	const CMapInfo * getMapInfo() override;
	const StartInfo * getStartInfo() override;
	
protected:
	void close() override;
};
