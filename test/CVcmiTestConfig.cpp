
/*
 * CVcmiTestConfig.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CVcmiTestConfig.h"

#include "../lib/CConsoleHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/logging/CLogger.h"
#include "../lib/CConfigHandler.h"

CVcmiTestConfig::CVcmiTestConfig()
{
	console = new CConsoleHandler;
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() / "VCMI_Test_log.txt", console);
	logConfig.configureDefault();
	preinitDLL(console);
	settings.init();
	logConfig.configure();
	loadDLLClasses();
	logGlobal->infoStream() << "Initialized global test setup.";
}

CVcmiTestConfig::~CVcmiTestConfig()
{
	std::cout << "Ending global test tear-down." << std::endl;
}
