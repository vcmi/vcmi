/*
 * CCampaignInfoScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CCampaignInfoScreen.h"

#include "../../CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../CPlayerInterface.h"

CCampaignInfoScreen::CCampaignInfoScreen()
{
	OBJECT_CONSTRUCTION;
	localSi = std::make_unique<StartInfo>(*GAME->interface()->cb->getStartInfo());
	localMi = std::make_unique<CMapInfo>();
	localMi->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*GAME->interface()->cb->getMapHeader()));

	screenType = ESelectionScreen::scenarioInfo;

	updateAfterStateChange();
}

CCampaignInfoScreen::~CCampaignInfoScreen() = default;

const CMapInfo * CCampaignInfoScreen::getMapInfo()
{
	return localMi.get();
}
const StartInfo * CCampaignInfoScreen::getStartInfo()
{
	return localSi.get();
}
