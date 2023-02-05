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
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../gui/CGuiHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

CCampaignInfoScreen::CCampaignInfoScreen() 
	: localSi(new StartInfo(*LOCPLINT->cb->getStartInfo())), localMi(new CMapInfo())
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	localMi->mapHeader = std::make_unique<CMapHeader>(*LOCPLINT->cb->getMapHeader());

	screenType = ESelectionScreen::scenarioInfo;

	updateAfterStateChange();
}

CCampaignInfoScreen::~CCampaignInfoScreen()
{
	vstd::clear_pointer(localSi);
	vstd::clear_pointer(localMi);
}

const CMapInfo * CCampaignInfoScreen::getMapInfo()
{
	return localMi;
}
const StartInfo * CCampaignInfoScreen::getStartInfo()
{
	return localSi;
}
